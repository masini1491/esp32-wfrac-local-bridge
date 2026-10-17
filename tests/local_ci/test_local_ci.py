"""Offline runner tests: no Docker daemon, sudo, login or network is executed."""
import importlib.util
import contextlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("local_ci", ROOT / "tools/local_ci.py")
ci = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ci)


class LocalCiTests(unittest.TestCase):
    def test_interactive_commands_rejected_before_spawn(self):
        cases = (["sudo", "docker", "info"], ["su"], ["login"], ["runas.exe"],
                 ["sudo", "-n", "bash"], ["sudo", "-n", "docker", "login"],
                 ["docker", "login"], ["gh", "auth", "login"],
                 ["bash", "-lc", "sudo docker info"], ["sh", "-c", "su root"])
        for argv in cases:
            with self.subTest(argv=argv), mock.patch.object(ci.subprocess, "Popen") as spawn:
                with self.assertRaises(ci.InteractiveAuthRequired):
                    ci.command(argv)
                spawn.assert_not_called()

    def test_explicit_noninteractive_docker_only(self):
        ci.noninteractive(["sudo", "-n", "docker", "image", "inspect", "example"])
        self.assertEqual(ci.docker_prefix(False), ["docker"])
        self.assertEqual(ci.docker_prefix(True), ["sudo", "-n", "docker"])

    def test_child_has_no_interactive_input(self):
        with mock.patch.object(ci.subprocess, "Popen", wraps=subprocess.Popen) as spawn:
            code, output = ci.command([sys.executable, "-c", "import sys; print(repr(sys.stdin.read()))"])
        self.assertEqual(code, 0)
        self.assertEqual(output.strip(), "''")
        self.assertIs(spawn.call_args.kwargs["stdin"], subprocess.DEVNULL)

    def test_password_required_classification(self):
        import io
        process = mock.Mock(stdout=io.BytesIO(b"sudo: a password is required\n"))
        process.wait.return_value = 1
        with mock.patch.object(ci.subprocess, "Popen", return_value=process) as spawn:
            with self.assertRaisesRegex(ci.InteractiveAuthRequired, "INTERACTIVE_AUTH_REQUIRED"):
                ci.command(["sudo", "-n", "docker", "info"])
        self.assertEqual(spawn.call_count, 1)

    def test_plain_docker_denial_never_auto_elevates(self):
        with mock.patch.object(ci, "command", return_value=(1, "permission denied")) as command:
            with self.assertRaisesRegex(ci.Stop, "DOCKER_ACCESS_REQUIRED"):
                ci.checked(["docker", "info"])
        command.assert_called_once_with(["docker", "info"])

    def test_missing_executable_no_install(self):
        with mock.patch.object(ci.subprocess, "Popen", side_effect=FileNotFoundError):
            with self.assertRaises(FileNotFoundError):
                ci.command(["docker", "info"])

    def test_log_is_bounded_and_redacted(self):
        script = "print('password=synthetic'); print('-----BEGIN PRIVATE KEY-----'); print('hidden'); print('-----END PRIVATE KEY-----'); print('x'*9000); print('ok')"
        code, output = ci.command([sys.executable, "-c", script], limit=64)
        self.assertEqual(code, 0)
        self.assertLessEqual(len(output.encode()), 64)
        self.assertNotIn("synthetic", output)
        self.assertNotIn("hidden", output)
        # A sensitive marker suppresses the entire remaining stream, not just its line.
        self.assertNotIn("ok", output)

    def test_multiline_credentials_fail_closed(self):
        cases = ('Authorization:\nBearer synthetic-auth\n\ntrailing-secret\n',
                 '"password":\n"synthetic-password"\n}\ntrailing-secret\n',
                 "credentials = {\n'user': 'synthetic-account',\n'key': 'synthetic-key'\n}\n",
                 "Operator-ID:\nsynthetic-operator\n", "private_key:\nsynthetic-key\n",
                 "-----BEGIN PRIVATE KEY-----\nsynthetic-key\n-----END PRIVATE KEY-----\ntrailing-secret\n")
        for payload in cases:
            with self.subTest(marker=payload.splitlines()[0]):
                script = "import sys; sys.stderr.write(" + repr(payload) + ")"
                code, output = ci.command([sys.executable, "-c", script])
                self.assertEqual(code, 0)
                self.assertIn("REDACTED", output)
                self.assertNotIn("synthetic-", output)
                self.assertNotIn("trailing-secret", output)
                self.assertNotIn("synthetic-", ci.redact(payload))

    def test_oversized_line_cannot_hide_credential_marker(self):
        script = "print('x'*9000 + 'password:'); print('synthetic-continuation')"
        code, output = ci.command([sys.executable, "-c", script])
        self.assertEqual(code, 0)
        self.assertIn("REDACTED UNCLASSIFIABLE", output)
        self.assertNotIn("synthetic-continuation", output)

    def test_split_chunk_sensitive_marker(self):
        script = "import sys; sys.stdout.write(' '*4089 + 'Authorization:\\nsynthetic-value\\n')"
        code, output = ci.command([sys.executable, "-c", script])
        self.assertEqual(code, 0)
        self.assertNotIn("synthetic-value", output)

    def test_clean_diagnostics_are_retained(self):
        code, output = ci.command([sys.executable, "-c", "print('build step PASS'); print('count: 42')"])
        self.assertEqual(code, 0)
        self.assertIn("build step PASS", output)
        self.assertIn("count: 42", output)

    def test_command_timeout_is_not_pass(self):
        with self.assertRaisesRegex(ci.Stop, "timeout"):
            ci.command([sys.executable, "-c", "import time; time.sleep(10)"], timeout=0.1)

    def test_current_workflow_parity(self):
        _, jobs = ci.contract(ROOT)
        self.assertEqual(len(jobs), 9)
        self.assertEqual(len(ci.select(jobs, "targeted")), 5)
        self.assertEqual(len(ci.select(jobs, "full")), 9)
        cells = [v for v in jobs.values() if v["env"].get("IDF_TARGET")]
        self.assertEqual(len(cells), 6)
        for cell in cells:
            self.assertTrue(any("A5" in s["name"] for s in cell["steps"]))
        d1 = next(s for s in jobs["host"]["steps"] if "Run D1" in s["name"])
        self.assertIn('--sdk "$IDF_PATH"', d1["command"])
        self.assertNotIn("--fetch-pinned-parser", d1["command"])
        self.assertEqual(jobs["host"]["timeout"], 1800)  # Explicit local cap, not a CI default claim.
        self.assertIsNone(jobs["host"]["workflow_timeout_seconds"])
        self.assertEqual(jobs["a6b"]["timeout"], 900)
        self.assertEqual(jobs["a7b"]["timeout"], 900)
        self.assertTrue(all(cell["timeout"] == 1200 for cell in cells))
        for name in ("Run offline CI scope tests", "Run offline Local CI tests"):
            self.assertIn(name, [step["name"] for step in jobs["host"]["steps"]])

    def test_only_reviewed_remote_sdk_conditions_are_supported(self):
        for key in ("dns", "callback", "compile"):
            approved = {"needs": "scope", "if": "${{ needs.scope.outputs." + key + " == 'true' }}"}
            ci.sdk_routing(approved, key)
            for bad in ({}, {**approved, "needs": ["scope"]}, {**approved, "needs": "other"},
                        {**approved, "if": "${{ always() }}"}, {**approved, "if": "false"},
                        {**approved, "if": approved["if"] + " || true"}):
                with self.assertRaises(ci.Stop):
                    ci.sdk_routing(bad, key)

    def test_hash_repin_cannot_bypass_condition_validation(self):
        original = (ROOT / ".github/workflows/esp-idf-compile.yml").read_text()
        config = json.loads((ROOT / "tools/local-ci.config.json").read_text())
        for before, replacement in (("needs: scope", "needs: [scope]"), ("needs: scope", "needs: other"),
                                    ("needs.scope.outputs.dns == 'true'", "always()"),
                                    ("needs: scope", "needs: scope\n    needs: scope")):
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                (root / "tools").mkdir()
                for path in config["workflow_blobs"]:
                    output = root / path
                    output.parent.mkdir(parents=True, exist_ok=True)
                    raw = ((original.replace(before, replacement, 1)).encode()
                           if path.endswith("esp-idf-compile.yml") else (ROOT / path).read_bytes())
                    output.write_bytes(raw)
                    config["workflow_blobs"][path] = ci.blob(raw)
                (root / "tools/local-ci.config.json").write_text(json.dumps(config))
                with self.assertRaisesRegex(ci.Stop, "needs/if|duplicate"):
                    ci.contract(root)

    def test_host_main_push_and_pr_checks_sdk_not_push(self):
        import yaml
        host = yaml.load((ROOT / ".github/workflows/validate-contracts.yml").read_text(), Loader=yaml.BaseLoader)
        sdk = yaml.load((ROOT / ".github/workflows/esp-idf-compile.yml").read_text(), Loader=yaml.BaseLoader)
        self.assertEqual(host["on"]["push"]["branches"], ["main"])
        self.assertIn("pull_request", host["on"])
        self.assertNotIn("push", sdk["on"])
        self.assertIn("pull_request", sdk["on"])
        self.assertIn("workflow_dispatch", sdk["on"])
        self.assertNotIn("needs", host["jobs"]["validate-contracts"])
        self.assertNotIn("if", host["jobs"]["validate-contracts"])

    def test_effective_job_timeout_is_workflow_capped(self):
        self.assertEqual(ci.job_budget({"timeout-minutes": "2"}, 600)["timeout"], 120)
        self.assertEqual(ci.job_budget({"timeout-minutes": "20"}, 60)["timeout"], 60)
        for job, cap in (({}, None), ({"timeout-minutes": "0"}, None), ({}, -1)):
            with self.assertRaises(ci.Stop):
                ci.job_budget(job, cap)

    def test_workflow_drift_fail_closed(self):
        config = json.loads((ROOT / "tools/local-ci.config.json").read_text())
        for mutation in (b"\n# command drift", b"\n# condition drift", b"\n# matrix drift"):
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                (root / "tools").mkdir()
                (root / "tools/local-ci.config.json").write_text(json.dumps(config))
                for path in config["workflow_blobs"]:
                    dest = root / path
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    dest.write_bytes((ROOT / path).read_bytes() + mutation)
                with self.assertRaisesRegex(ci.Stop, "workflow drift"):
                    ci.contract(root)

    def test_unknown_scope_rejected(self):
        for profile, suite in ((None, None), ("unknown", None), (None, "missing")):
            with self.assertRaises(ci.Stop):
                ci.select({"host": {}}, profile, suite)

    def test_partial_profile_never_full_pass(self):
        jobs = {"host": {}, "a6b": {}}
        self.assertTrue(ci.accounting({"host": {"status": "PASS"}, "a6b": {"status": "NOT_RUN"}}, ["host"], jobs))
        for status in ("FAIL", "NOT_RUN"):
            self.assertFalse(ci.accounting({"host": {"status": status}, "a6b": {"status": "PASS"}}, list(jobs), jobs))
        self.assertFalse(ci.accounting({"host": {"status": "PASS"}}, ["host"], jobs))

    def test_docker_isolation(self):
        with tempfile.TemporaryDirectory() as directory:
            root, output = Path(directory) / "src", Path(directory) / "out"
            with mock.patch.object(ci.os, "getuid", return_value=1000, create=True), mock.patch.object(ci.os, "getgid", return_value=1000, create=True):
                argv = ci.docker_args(["docker"], {"id": "sha256:example"}, root, output, "exact", "host", "main", "abc")
        for flag in ("--network=none", "--read-only", "--pull=never", "--cap-drop=ALL", "--security-opt=no-new-privileges"):
            self.assertIn(flag, argv)
        self.assertNotIn("--privileged", argv)
        self.assertFalse(any("docker.sock" in arg for arg in argv))
        self.assertIn("1000:1000", argv)
        self.assertTrue(any("dst=/src,readonly" in arg for arg in argv))
        self.assertIn("/tmp:rw,nosuid,nodev,noexec", argv)
        self.assertIn("TMPDIR=/out/tmp", argv)
        self.assertLess(argv.index("TMPDIR=/out/tmp"), argv.index("--entrypoint"))

    def test_suite_temp_is_private_and_independent(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            root = base / "source"
            root.mkdir()
            paths = []
            for suite in ("host", "a6b", "a7b"):
                output = base / suite
                output.mkdir()
                paths.append(ci.prepare_suite_temp(root, output))
                self.assertEqual(paths[-1], output / "tmp")
                self.assertTrue(paths[-1].is_dir())
                self.assertEqual(paths[-1].stat().st_mode & 0o777, 0o700)
            self.assertEqual(len(set(p.resolve() for p in paths)), 3)

    def test_suite_temp_rejects_source_reuse_and_symlink_escape(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            root, output, outside = base / "source", base / "out", base / "outside"
            root.mkdir()
            output.mkdir()
            outside.mkdir()
            with self.assertRaises(ci.Stop):
                ci.prepare_suite_temp(root, root)
            self.assertFalse((root / "tmp").exists())
            (output / "tmp").symlink_to(outside, target_is_directory=True)
            with self.assertRaises(ci.Stop):
                ci.prepare_suite_temp(root, output)
            self.assertEqual(list(outside.iterdir()), [])
            link = base / "linked-output"
            link.symlink_to(outside, target_is_directory=True)
            with self.assertRaises(ci.Stop):
                ci.prepare_suite_temp(root, link)
            fresh = base / "fresh"
            fresh.mkdir()
            ci.prepare_suite_temp(root, fresh)
            with self.assertRaises(ci.Stop):
                ci.prepare_suite_temp(root, fresh)

    def test_child_tempfile_uses_suite_tmpdir_and_can_execute(self):
        script = """
import os
from pathlib import Path
import subprocess
import tempfile
expected = Path(os.environ['TMPDIR']).resolve()
assert Path(tempfile.gettempdir()).resolve() == expected
with tempfile.TemporaryDirectory() as directory:
    assert Path(directory).resolve().parent == expected
    program = Path(directory) / 'exec-probe'
    program.write_text('#!/bin/sh\\nexit 0\\n')
    program.chmod(0o700)
    subprocess.run([str(program)], stdin=subprocess.DEVNULL, check=True, timeout=5)
print('SUITE_TMPDIR_EXEC_PASS')
"""
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            root, output = base / "source", base / "out"
            root.mkdir()
            output.mkdir()
            temporary = ci.prepare_suite_temp(root, output)
            # Fresh child imports tempfile only after its environment is set.
            with mock.patch.dict(ci.os.environ, {"TMPDIR": str(temporary),
                                                 "TEMP": "/invalid-fallback", "TMP": "/invalid-fallback"}):
                code, log = ci.command([sys.executable, "-B", "-c", script])
            self.assertEqual(code, 0, log)
            self.assertIn("SUITE_TMPDIR_EXEC_PASS", log)
            self.assertEqual(list(temporary.iterdir()), [])

    def test_source_output_rejected(self):
        with self.assertRaisesRegex(ci.Stop, "outside"):
            ci.docker_args([], {}, ROOT, ROOT / "build", "x", "host", "main", "abc")

    def test_remote_docker_rejected_before_daemon_access(self):
        context = [{"Endpoints": {"docker": {"Host": "tcp://example.invalid:2375"}}}]
        with mock.patch.dict(ci.os.environ, {}, clear=True), mock.patch.object(ci, "checked", return_value=json.dumps(context)) as check:
            with self.assertRaisesRegex(ci.Stop, "non-local"):
                ci.image_info(["docker"], {"image": "example"}, None)
        self.assertEqual(check.call_count, 1)

    def test_new_preflight_does_not_cache_auth(self):
        config = {"image": "example", "platform": "linux/amd64"}
        context = json.dumps([{"Endpoints": {"docker": {"Host": "unix:///var/run/docker.sock"}}}])
        digest = "sha256:" + "a" * 64
        info = json.dumps([{"Id": "sha256:" + "b" * 64, "Os": "linux", "Architecture": "amd64", "RepoDigests": ["espressif/idf@" + digest]}])
        with mock.patch.dict(ci.os.environ, {}, clear=True), mock.patch.object(ci, "checked", side_effect=[context, ci.InteractiveAuthRequired(ci.AUTH_HELP), context, info]) as check:
            with self.assertRaises(ci.InteractiveAuthRequired):
                ci.image_info(["sudo", "-n", "docker"], config, digest)
            self.assertEqual(ci.image_info(["sudo", "-n", "docker"], config, digest)["digest"], digest)
        self.assertEqual(check.call_count, 4)

    def test_digest_and_platform_are_verified(self):
        context = json.dumps([{"Endpoints": {"docker": {"Host": "unix:///var/run/docker.sock"}}}])
        for platform, digest in (("arm64", "sha256:" + "a" * 64), ("amd64", None), ("amd64", "sha256:" + "c" * 64)):
            info = json.dumps([{"Id": "sha256:" + "b" * 64, "Os": "linux", "Architecture": platform, "RepoDigests": []}])
            with mock.patch.dict(ci.os.environ, {}, clear=True), mock.patch.object(ci, "checked", side_effect=[context, info]):
                with self.assertRaises(ci.Stop):
                    ci.image_info(["docker"], {"image": "example", "platform": "linux/amd64"}, digest)

    def test_docs_do_not_offer_credential_workarounds(self):
        text = (ROOT / "docs/LOCAL_CI.md").read_text()
        for required in ("INTERACTIVE_AUTH_REQUIRED", "NOT_RUN", "sudo -n"):
            self.assertIn(required, text)
        script = (ROOT / "tools/bootstrap-local-ci.ps1").read_text()
        self.assertNotIn("Read-Host", script)
        self.assertNotIn("-Verb RunAs", script)

    def test_dirty_tree_is_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            sentinel = root / "unknown.txt"
            sentinel.write_bytes(b"preserve me")
            answers = [str(root), "https://github.com/masini1491/esp32-wfrac-local-bridge.git",
                       "abc", "main", "?? unknown.txt"]
            with mock.patch.object(ci, "git", side_effect=answers):
                with self.assertRaisesRegex(ci.Stop, "dirty tree"):
                    ci.identity(root, "main", "abc")
            self.assertEqual(sentinel.read_bytes(), b"preserve me")

    def test_wrong_repository_branch_and_head(self):
        for origin, branch, sha in (("https://example.invalid/other", "main", "abc"),
                                    ("https://github.com/masini1491/esp32-wfrac-local-bridge.git", "other", "abc"),
                                    ("https://github.com/masini1491/esp32-wfrac-local-bridge.git", "main", "drift")):
            with mock.patch.object(ci, "git", side_effect=[str(ROOT), origin, sha, branch]):
                with self.assertRaisesRegex(ci.Stop, "mismatch"):
                    ci.identity(ROOT, "main", "abc")

    def test_unfinished_git_operation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            (root / "MERGE_HEAD").write_text("synthetic")
            answers = [str(root), "https://github.com/masini1491/esp32-wfrac-local-bridge.git",
                       "abc", "main", "", "MERGE_HEAD"]
            with mock.patch.object(ci, "git", side_effect=answers):
                with self.assertRaisesRegex(ci.Stop, "unfinished"):
                    ci.identity(root, "main", "abc")

    def test_sdk_version_mismatch_stops_before_tests(self):
        with mock.patch.object(ci.Path, "exists", return_value=True), mock.patch.dict(ci.os.environ, {"WFRAC_OFFLINE": "1", "IDF_PATH": "/synthetic"}), mock.patch.object(ci.Path, "iterdir", return_value=iter([Path("lo")])), mock.patch.object(ci, "checked", return_value="ESP-IDF wrong"):
            with self.assertRaisesRegex(ci.Stop, "IDF version mismatch"):
                ci.inside_check({"idf_version": "ESP-IDF v6.0.2"})

    def test_offline_guard_rejects_network_interface(self):
        with mock.patch.object(ci.Path, "exists", return_value=True), mock.patch.dict(ci.os.environ, {"WFRAC_OFFLINE": "1"}), mock.patch.object(ci.Path, "iterdir", return_value=iter([Path("lo"), Path("eth0")])), mock.patch.object(ci, "checked") as execute:
            with self.assertRaisesRegex(ci.Stop, "non-loopback"):
                ci.inside_check({})
        execute.assert_not_called()

    def test_auth_gate_does_not_retry_cleanup_or_cache_success(self):
        _, jobs = ci.contract(ROOT)
        with tempfile.TemporaryDirectory() as directory:
            with mock.patch.object(sys, "argv", ["local_ci.py", "--sudo-docker"]):
                args = ci.arguments()
            with mock.patch.object(ci, "identity", return_value={"sha": "abc", "tree": "def"}), mock.patch.object(ci, "platform") as platform, mock.patch.object(ci, "image_info", return_value={"id": "synthetic"}), mock.patch.object(ci.tempfile, "mkdtemp", return_value=directory), mock.patch.object(ci, "docker_args", return_value=["sudo", "-n", "docker", "run"]), mock.patch.object(ci, "command", side_effect=ci.InteractiveAuthRequired(ci.AUTH_HELP)) as execute, mock.patch.object(ci, "checked") as cleanup, contextlib.redirect_stdout(io.StringIO()):
                platform.system.return_value = "Linux"
                platform.machine.return_value = "x86_64"
                self.assertEqual(ci.main(args), 1)
            execute.assert_called_once()
            cleanup.assert_not_called()
            report = json.loads((Path(directory) / "result.json").read_text())
            self.assertIn("INTERACTIVE_AUTH_REQUIRED", report["error"])
            self.assertEqual(set(report["jobs"]), set(jobs))
            self.assertTrue(all(r["status"] == "NOT_RUN" for r in report["jobs"].values()))

    def run_failure(self, *, preflight=None, primary=None, cleanup=None):
        """Fault-inject only; no Docker process, credential helper or sudo is launched."""
        with tempfile.TemporaryDirectory() as directory:
            with mock.patch.object(sys, "argv", ["local_ci.py", "--run", "--profile", "full",
                                                 "--expected-sha", "abc", "--sudo-docker"]):
                args = ci.arguments()
            capture = io.StringIO()
            with mock.patch.object(ci, "identity", return_value={"sha": "abc", "tree": "def"}), mock.patch.object(ci, "platform") as platform, mock.patch.object(ci, "image_info", side_effect=preflight, return_value={"id": "synthetic"}) as image, mock.patch.object(ci.tempfile, "mkdtemp", return_value=directory), mock.patch.object(ci, "docker_args", return_value=["sudo", "-n", "docker", "run"]) as docker, mock.patch.object(ci, "command", side_effect=primary) as command, mock.patch.object(ci, "checked", side_effect=cleanup) as remove, contextlib.redirect_stdout(capture):
                platform.system.return_value = "Linux"
                platform.machine.return_value = "x86_64"
                self.assertEqual(ci.main(args), 1)
            report = json.loads((Path(directory) / "result.json").read_text())
            self.assertEqual(report["status"], "FAIL")
            return report, capture.getvalue(), image.call_args_list, docker.call_count, command.call_count, remove.call_args_list

    def test_preflight_auth_failure_is_persisted_before_container(self):
        report, _, _, docker_calls, command_calls, cleanup_calls = self.run_failure(
            preflight=ci.InteractiveAuthRequired(ci.AUTH_HELP))
        self.assertEqual(report["error_code"], "INTERACTIVE_AUTH_REQUIRED")
        self.assertEqual(report["preflight"], {"status": "FAIL", "phase": "image"})
        self.assertIsNone(report["image"])
        self.assertEqual(report["full_profile"], "NOT_RUN")
        self.assertEqual(len(report["jobs"]), 9)
        self.assertTrue(all(row["status"] == "NOT_RUN" for row in report["jobs"].values()))
        self.assertEqual((docker_calls, command_calls, cleanup_calls), (0, 0, []))
        self.assertEqual(report["image_preflight"]["status"], "FAIL")
        self.assertTrue(all(row["status"] == "NOT_RUN" for row in report["sdk_integrity"].values()))

    def test_preflight_permission_missing_image_and_executable_failures_persist(self):
        for error in (ci.Stop("DOCKER_ACCESS_REQUIRED"), ci.Stop("image unavailable"),
                      FileNotFoundError("docker executable unavailable")):
            with self.subTest(error=type(error).__name__):
                report, _, _, docker_calls, command_calls, cleanup_calls = self.run_failure(preflight=error)
                self.assertEqual(report["preflight"]["status"], "FAIL")
                self.assertTrue(all(row["status"] == "NOT_RUN" for row in report["jobs"].values()))
                self.assertEqual(report["full_profile"], "NOT_RUN")
                self.assertEqual((docker_calls, command_calls, cleanup_calls), (0, 0, []))

    def test_timeout_preserves_primary_when_cleanup_fails(self):
        for secondary in (ci.Stop("cleanup denied"), FileNotFoundError("cleanup executable missing"),
                          ci.InteractiveAuthRequired(ci.AUTH_HELP)):
            with self.subTest(secondary=type(secondary).__name__):
                report, _, _, _, command_calls, cleanup_calls = self.run_failure(
                    primary=ci.OperationTimeout("original operation timeout"), cleanup=secondary)
                self.assertEqual(report["error"], "original operation timeout")
                self.assertEqual(report["error_code"], "OPERATION_TIMEOUT")
                self.assertEqual(report["jobs"]["host"]["status"], "FAIL")
                self.assertEqual(report["cleanup"]["host"]["status"], "FAIL")
                if isinstance(secondary, ci.InteractiveAuthRequired):
                    self.assertEqual(report["cleanup"]["host"]["error_code"], "INTERACTIVE_AUTH_REQUIRED")
                self.assertEqual(report["full_profile"], "FAIL")
                self.assertTrue(all(r["status"] == "NOT_RUN" for n, r in report["jobs"].items() if n != "host"))
                self.assertEqual(command_calls, 1)
                self.assertEqual(len(cleanup_calls), 1)
                self.assertEqual(cleanup_calls[0].args[0][:5], ["sudo", "-n", "docker", "rm", "-f"])
                self.assertTrue(cleanup_calls[0].args[0][-1].startswith("wfrac-ci-"))

    def test_successful_cleanup_cannot_turn_timeout_into_pass(self):
        report, _, _, _, _, cleanup_calls = self.run_failure(primary=ci.OperationTimeout("original timeout"))
        self.assertEqual(report["error"], "original timeout")
        self.assertEqual(report["cleanup"]["host"]["status"], "PASS")
        self.assertEqual(report["jobs"]["host"]["status"], "FAIL")
        self.assertEqual(report["full_profile"], "FAIL")
        self.assertEqual(len(cleanup_calls), 1)

    def test_sensitive_multiline_error_not_in_result_or_stdout(self):
        report, stdout, *_ = self.run_failure(preflight=ci.Stop('Authorization:\nsynthetic-auth\ntrailing-secret'))
        for output in (json.dumps(report), stdout):
            self.assertIn("REDACTED", output)
            self.assertNotIn("synthetic-auth", output)
            self.assertNotIn("trailing-secret", output)

    def evidence_run(self, steps, provenance="valid", code=0, full=True):
        config, jobs = ci.contract(ROOT)
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            cli = ["local_ci.py", "--run", "--expected-sha", "abc"]
            cli += ["--profile", "full"] if full else ["--suite", "host"]
            with mock.patch.object(sys, "argv", cli):
                args = ci.arguments()
            def fake_container(*argv, **kwargs):
                target = output / "host"
                self.assertTrue((target / "tmp").is_dir())  # Prepared before container launch.
                if provenance == "valid":
                    data = {"sdk": config["idf_sha"], "lwip": config["lwip_sha"],
                            "tools": {tool: "synthetic-version" for tool in ("python", "gcc", "g++", "cmake", "ninja")}}
                    (target / "provenance.json").write_text(json.dumps(data))
                elif provenance is not None:
                    (target / "provenance.json").write_text(provenance)
                if steps == "valid":
                    rows = [{**s, "status": "PASS", "exit_code": 0} for s in jobs["host"]["steps"]]
                    (target / "steps.json").write_text(json.dumps(rows))
                elif steps is not None:
                    (target / "steps.json").write_text(steps)
                return code, "synthetic container output"
            with mock.patch.object(ci, "identity", return_value={"sha": "abc", "tree": "def"}), mock.patch.object(ci, "platform") as platform, mock.patch.object(ci, "image_info", return_value={"id": "synthetic"}), mock.patch.object(ci.tempfile, "mkdtemp", return_value=directory), mock.patch.object(ci, "docker_args", return_value=["docker", "run"]), mock.patch.object(ci, "command", side_effect=fake_container) as execute, contextlib.redirect_stdout(io.StringIO()):
                platform.system.return_value = "Linux"
                platform.machine.return_value = "x86_64"
                result = ci.main(args)
            report = json.loads((output / "result.json").read_text())
            return result, report, execute.call_count

    def test_missing_steps_after_attempt_is_fail_not_not_run(self):
        code, report, calls = self.evidence_run(None)
        self.assertEqual((code, calls), (1, 1))
        self.assertEqual(report["status"], "FAIL")
        self.assertEqual(report["full_profile"], "FAIL")
        self.assertEqual(report["jobs"]["host"]["status"], "FAIL")
        self.assertEqual(report["sdk_integrity"]["host"]["status"], "PASS")
        self.assertIn("steps.json", report["error"])
        self.assertTrue(all(r["status"] == "NOT_RUN" for n, r in report["jobs"].items() if n != "host"))

    def test_invalid_step_evidence_is_fail(self):
        for payload in ("{", "{}", "[]", '[{"name":"unexpected","status":"PASS","exit_code":0}]'):
            with self.subTest(payload=payload):
                code, report, calls = self.evidence_run(payload)
                self.assertEqual((code, calls), (1, 1))
                self.assertEqual(report["jobs"]["host"]["status"], "FAIL")
                self.assertEqual(report["full_profile"], "FAIL")

    def test_image_pass_does_not_imply_sdk_integrity_pass(self):
        for provenance in (None, "{", "[]", '{}', '{"sdk":"wrong","lwip":"wrong","tools":{}}'):
            with self.subTest(provenance=provenance):
                code, report, _ = self.evidence_run(None, provenance=provenance)
                self.assertEqual(code, 1)
                self.assertEqual(report["image_preflight"]["status"], "PASS")
                self.assertEqual(report["sdk_integrity"]["host"]["status"], "FAIL")
                self.assertEqual(report["jobs"]["host"]["status"], "FAIL")
                self.assertEqual(report["full_profile"], "FAIL")

    def test_sdk_pass_does_not_imply_suite_pass(self):
        code, report, _ = self.evidence_run(None, code=1)
        self.assertEqual(code, 1)
        self.assertEqual(report["image_preflight"]["status"], "PASS")
        self.assertEqual(report["sdk_integrity"]["host"]["status"], "PASS")
        self.assertEqual(report["jobs"]["host"]["status"], "FAIL")

    def test_complete_evidence_allows_only_selected_suite_pass(self):
        code, report, calls = self.evidence_run("valid", full=False)
        self.assertEqual((code, calls), (0, 1))
        self.assertEqual(report["status"], "PASS")
        self.assertEqual(report["jobs"]["host"]["status"], "PASS")
        self.assertEqual(report["full_profile"], "NOT_RUN")
        self.assertTrue(all(r["status"] == "NOT_RUN" for n, r in report["sdk_integrity"].items() if n != "host"))

    def test_steps_receive_remaining_shared_budget(self):
        clock = [100.0]
        steps = [{"name": "one", "command": "synthetic one"}, {"name": "two", "command": "synthetic two"}]
        budgets = []
        def fake_step(*args, timeout, **kwargs):
            budgets.append(timeout)
            clock[0] += 4.0
            return 0, "synthetic output"
        with tempfile.TemporaryDirectory() as directory, mock.patch.object(ci.time, "monotonic", side_effect=lambda: clock[0]), mock.patch.object(ci, "command", side_effect=fake_step):
            ci.run_steps(Path(directory), Path(directory), steps, 110.0, 1024)
        self.assertEqual(budgets, [10.0, 6.0])

    def test_exhausted_budget_prevents_next_step(self):
        clock = [100.0]
        steps = [{"name": "one", "command": "synthetic one"}, {"name": "two", "command": "synthetic two"}]
        def fake_step(*args, **kwargs):
            clock[0] = 110.0
            return 0, "late zero exit"
        with tempfile.TemporaryDirectory() as directory, mock.patch.object(ci.time, "monotonic", side_effect=lambda: clock[0]), mock.patch.object(ci, "command", side_effect=fake_step) as execute:
            with self.assertRaises(ci.OperationTimeout):
                ci.run_steps(Path(directory), Path(directory), steps, 110.0, 1024)
            execute.assert_called_once()

    def test_expired_budget_does_not_launch_a_step(self):
        with tempfile.TemporaryDirectory() as directory, mock.patch.object(ci.time, "monotonic", return_value=110.0), mock.patch.object(ci, "command") as execute:
            with self.assertRaises(ci.OperationTimeout):
                ci.run_steps(Path(directory), Path(directory), [{"name": "one", "command": "never"}], 110.0, 1024)
            execute.assert_not_called()

    def test_sdk_integrity_time_counts_against_job_budget(self):
        clock = [100.0]
        def fake_check(config):
            clock[0] += 10.0
        with mock.patch.object(ci.time, "monotonic", side_effect=lambda: clock[0]), mock.patch.object(ci, "inside_check", side_effect=fake_check), mock.patch.object(ci, "run_steps") as steps:
            with self.assertRaises(ci.OperationTimeout):
                ci.inside_job(ROOT, {}, {"host": {"timeout": 10}}, "host", {}, "main", "abc")
            steps.assert_not_called()

    @contextlib.contextmanager
    def colliding_package(self):
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            site = temp / "site-packages"
            (site / "tests").mkdir(parents=True)
            (site / "tests/__init__.py").write_text('ORIGIN = "third-party"\n')
            with mock.patch.dict(ci.os.environ, {"PYTHONPATH": str(site),
                                                 "PYTHONPYCACHEPREFIX": str(temp / "pycache")}):
                yield temp, site

    def test_third_party_regular_tests_package_shadows_original_namespace(self):
        self.assertFalse((ROOT / "tests/__init__.py").exists())
        self.assertFalse((ROOT / "tests/host/__init__.py").exists())
        with self.colliding_package() as (temp, site):
            work = temp / "legacy"
            work.mkdir()
            (work / "tests").symlink_to(ROOT / "tests", target_is_directory=True)
            code, log = ci.command([sys.executable, "-B", "-c",
                "import tests; print(tests.__file__, flush=True); import tests.host.test_validate_contracts"], cwd=work)
            self.assertNotEqual(code, 0)
            self.assertIn(str(site / "tests/__init__.py"), log)
            self.assertIn("No module named 'tests.host'", log)

    def test_output_only_package_runs_exact_host_command_despite_collision(self):
        module = ROOT / "tests/host/test_validate_contracts.py"
        before = module.read_bytes()
        _, jobs = ci.contract(ROOT)
        step = next(s for s in jobs["host"]["steps"] if s["name"] == "Run host tests")
        self.assertEqual(step["command"].strip(), "python -m unittest tests.host.test_validate_contracts")
        with self.colliding_package() as (temp, site):
            work = temp / "work"
            ci.workspace_facade(ROOT, work)
            binary = temp / "bin"
            binary.mkdir()
            (binary / "python").symlink_to(sys.executable)
            with mock.patch.dict(ci.os.environ, {"PATH": str(binary) + ci.os.pathsep + ci.os.environ["PATH"],
                                                 "PYTHONDONTWRITEBYTECODE": "1"}):
                code, log = ci.command(["bash", "-eo", "pipefail", "-c", step["command"]], cwd=work)
            self.assertEqual(code, 0, log)
            self.assertIn("Ran 76 tests", log)
            self.assertIn("OK", log)
            self.assertFalse((work / "tests").is_symlink())
            self.assertFalse((work / "tests/host").is_symlink())
            self.assertTrue((work / "tests/host/test_validate_contracts.py").is_symlink())
            self.assertEqual((work / "tests/host/test_validate_contracts.py").resolve(), module.resolve())
            self.assertFalse((work / ".git").exists())
            self.assertTrue((work / "tests/host_cpp").is_symlink())
        self.assertEqual(module.read_bytes(), before)
        self.assertFalse((ROOT / "tests/__init__.py").exists())
        self.assertFalse((ROOT / "tests/host/__init__.py").exists())

    def test_facade_preserves_existing_source_initializers(self):
        with tempfile.TemporaryDirectory() as directory:
            root, work = Path(directory) / "source", Path(directory) / "work"
            (root / "tests/host").mkdir(parents=True)
            for path in (root / "tests/__init__.py", root / "tests/host/__init__.py"):
                path.write_text("# existing source initializer\n")
            ci.workspace_facade(root, work)
            for relative in ("tests/__init__.py", "tests/host/__init__.py"):
                self.assertTrue((work / relative).is_symlink())
                self.assertEqual((work / relative).resolve(), root / relative)

    def test_facade_refuses_to_write_inside_source(self):
        with self.assertRaisesRegex(ci.Stop, "outside source"):
            ci.workspace_facade(ROOT, ROOT / "forbidden-facade")
        self.assertFalse((ROOT / "forbidden-facade").exists())


if __name__ == "__main__":
    unittest.main()
