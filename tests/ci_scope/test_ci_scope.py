"""Deterministic offline CI routing, coverage and summary tests; no SDK execution."""
import base64
import copy
import importlib.util
import json
from pathlib import Path
import re
import unittest
from unittest.mock import patch
from urllib.error import HTTPError, URLError

spec = importlib.util.spec_from_file_location("ci_scope", Path(__file__).parents[2] / "tools/ci_scope.py")
scope = importlib.util.module_from_spec(spec)
spec.loader.exec_module(scope)
BASE, HEAD = "a" * 40, "b" * 40
REPO, ROOT = "owner/repo", "repos/owner/repo/pulls/12"


class SelectorTests(unittest.TestCase):
    def test_path_scope_matrix(self):
        cases = {
            "dns-only": (["tests/espidf_dns_probe/main/x.cpp"], (1, 0, 0)),
            "a6-only": (["tests/espidf_linux_probe/sdkconfig.defaults"], (0, 1, 0)),
            "component": (["components/a/x.cpp"], (0, 0, 1)),
            "firmware": (["firmware/a/sdkconfig.defaults"], (0, 0, 1)),
            "a5": (["tests/espidf_probe/main/x.cpp"], (0, 0, 1)),
            "mixed": (["tests/espidf_dns_probe/a", "tests/espidf_linux_probe/b", "components/c"], (1, 1, 1)),
            "docs": (["docs/security.md", "README.md"], (0, 0, 0)),
            "coordination": (["TASKS.md", "BACKLOG.md", "evidence/inbox/a.md"], (0, 0, 0)),
            "host": (["tests/host/test_a.py", "tests/host_cpp/a.cpp", "tools/validate_contracts.py", "specs/a.json"], (0, 0, 0)),
            "routing": ([*scope.WORKFLOWS, "tools/ci_scope.py", "tests/ci_scope/test_ci_scope.py"], (0, 0, 0)),
            "unknown": (["new_runtime/a.cpp"], (1, 1, 1)),
            "security": ([".github/actions/security/action.yml"], (1, 1, 1)),
            "toolchain": (["CMakeLists.txt"], (1, 1, 1)),
            "union": (["README.md", "tests/espidf_dns_probe/a"], (1, 0, 0)),
        }
        for label, (paths, expected) in cases.items():
            with self.subTest(label=label):
                self.assertEqual(tuple(scope.classify(paths).values()), expected)

    def test_rename_old_path_and_deletion(self):
        for old, new in [("components/a.cpp", "docs/a.md"), ("docs/a.md", "components/a.cpp")]:
            paths = scope.changed_paths([dict(status="renamed", filename=new, previous_filename=old)])
            self.assertTrue(scope.classify(paths)["compile"])
        paths = scope.changed_paths([dict(status="removed", filename="tests/espidf_dns_probe/a")])
        self.assertTrue(scope.classify(paths)["dns"])

    def test_invalid_paths_and_records(self):
        for path in [None, "", "/abs", "a/../b", "a\\b", "a\nb", "a//b"]:
            with self.subTest(path=path), self.assertRaises(scope.ScopeError):
                scope.classify([path])
        for record in [{}, {"status": "other", "filename": "a"}, {"status": "renamed", "filename": "a"}]:
            with self.subTest(record=record), self.assertRaises(scope.ScopeError):
                scope.changed_paths([record])

    def test_pagination_complete_with_rename_on_second_page(self):
        first = [dict(filename=f"docs/{i}.md", status="modified") for i in range(100)]
        last = [dict(filename="docs/new.md", status="renamed", previous_filename="firmware/old.cpp")]
        calls = []
        def get(route):
            calls.append(route)
            return first if route.endswith("page=1") else last
        files = scope.collect_files(get, ROOT, {"changed_files": 101})
        self.assertEqual(len(calls), 2)
        self.assertTrue(scope.classify(scope.changed_paths(files))["compile"])

    def test_pagination_fail_closed(self):
        for total in [None, -1, True, 3000]:
            with self.subTest(total=total), self.assertRaises(scope.ScopeError):
                scope.collect_files(lambda _: [], ROOT, {"changed_files": total})
        for data in [[], {}, [dict(filename="a", status="modified")] * 2]:
            with self.subTest(data=data), self.assertRaises(scope.ScopeError):
                scope.collect_files(lambda _: data, ROOT, {"changed_files": 2})

    def test_api_errors(self):
        for error in [HTTPError("https://api.github.com", 403, "denied", {}, None), URLError("offline"),
                      TimeoutError(), json.JSONDecodeError("bad", "", 0)]:
            with self.subTest(error=type(error).__name__), patch.dict(scope.os.environ, GITHUB_TOKEN="test-only"), \
                    patch.object(scope, "urlopen", side_effect=error), self.assertRaises(scope.ScopeError):
                scope.fetch_json("repos/owner/repo/pulls/12")

    def test_protected_sdk_jobs_allow_only_gating_difference(self):
        before = "jobs:\n" + "".join(f"  {job}:\n    runs-on: ubuntu-latest\n    steps:\n      - run: build\n\n"
                                        for job in scope.SUITES.values())
        after = before
        for key, job in scope.SUITES.items():
            after = after.replace(f"  {job}:\n", f"  {job}:\n    needs: scope\n    if: " + "${{ needs.scope.outputs." + key + " == 'true' }}\n")
        self.assertEqual(scope.protected_workflow(before, scope.WORKFLOWS[0]), scope.protected_workflow(after, scope.WORKFLOWS[0]))
        self.assertNotEqual(scope.protected_workflow(before, scope.WORKFLOWS[0]),
                            scope.protected_workflow(after.replace("run: build", "run: skip"), scope.WORKFLOWS[0]))
        with self.assertRaises(scope.ScopeError):
            scope.protected_workflow("jobs: {}", scope.WORKFLOWS[0])

    def test_unreviewed_routing_guards_rejected(self):
        original = Path(scope.WORKFLOWS[0]).read_text()
        for altered in (original.replace("needs: scope", "needs: [scope]", 1),
                        original.replace("needs: scope", "needs: other", 1),
                        original.replace("needs.scope.outputs.dns == 'true'", "always()", 1),
                        original.replace("    needs: scope\n", "", 1)):
            with self.assertRaises(scope.ScopeError):
                scope.protected_workflow(altered, scope.WORKFLOWS[0], require_sdk_guards=True)

    def test_each_candidate_sdk_job_requires_exact_complete_guards(self):
        original = Path(scope.WORKFLOWS[0]).read_text().replace("\r\n", "\n")
        self.assertEqual(
            tuple(scope.classify(["tools/ci_scope.py", "tests/ci_scope/test_ci_scope.py"]).values()),
            (False, False, False),
        )
        scope.protected_workflow(original, scope.WORKFLOWS[0], require_sdk_guards=True)

        for key, job in scope.SUITES.items():
            expected_if = "    if: ${{ needs.scope.outputs." + key + " == 'true' }}\n"
            start = original.index("  " + job + ":\n")
            next_job = re.search(r"^  [\w-]+:\n", original[start + 1:], re.M)
            end = start + 1 + next_job.start() if next_job else len(original)
            block = original[start:end]

            mutations = {
                "both_missing": block.replace("    needs: scope\n", "", 1).replace(expected_if, "", 1),
                "needs_missing": block.replace("    needs: scope\n", "", 1),
                "if_missing": block.replace(expected_if, "", 1),
                "wrong_needs": block.replace("    needs: scope\n", "    needs: other\n", 1),
                "wrong_if": block.replace(expected_if, expected_if.replace("outputs." + key, "outputs.other"), 1),
                "duplicate_needs": block.replace("    needs: scope\n", "    needs: scope\n    needs: scope\n", 1),
                "duplicate_if": block.replace(expected_if, expected_if + expected_if, 1),
                "unsupported_if": block.replace(expected_if, "    if: always()\n", 1),
            }
            for label, altered_block in mutations.items():
                candidate = original[:start] + altered_block + original[end:]
                with self.subTest(job=job, mutation=label), self.assertRaises(scope.ScopeError):
                    scope.protected_workflow(candidate, scope.WORKFLOWS[0], require_sdk_guards=True)

    def test_local_ci_paths_and_rename_union(self):
        paths = [*scope.LOCAL_CI_FILES, "tests/local_ci/test_local_ci.py"]
        self.assertFalse(any(scope.classify(paths).values()))
        for path in (".env", "credentials.pem", "CMakeLists.txt", "tools/local_ci.py.bak"):
            self.assertTrue(all(scope.classify(paths + [path]).values()))
        changed = scope.changed_paths([dict(status="renamed", filename="tools/local_ci.py",
                                            previous_filename="firmware/build.cmake")])
        self.assertTrue(scope.classify(changed)["compile"])

    def test_mandatory_host_regression_commands(self):
        original = Path(scope.WORKFLOWS[1]).read_text()
        scope.host_regressions(original, required=True)
        for name, command in scope.HOST_REGRESSIONS.items():
            for altered in (original.replace(command, "echo skipped"),
                            original.replace("      - name: " + name + "\n        run: " + command + "\n", "")):
                with self.assertRaises(scope.ScopeError):
                    scope.host_regressions(altered, required=True)

    def test_protected_host_commands(self):
        before = host_workflow_fixture()
        self.assertEqual(scope.protected_workflow(before, scope.WORKFLOWS[1]),
                         scope.protected_workflow("# routing\n" + before, scope.WORKFLOWS[1]))
        self.assertNotEqual(scope.protected_workflow(before, scope.WORKFLOWS[1]),
                            scope.protected_workflow(before.replace("run: python tools/validate_contracts.py",
                                                                    "run: echo skipped"), scope.WORKFLOWS[1]))


def host_workflow_fixture():
    return """jobs:
  validate-contracts:
    runs-on: ubuntu-latest
    steps:
      - name: Run offline CI scope tests
        run: python -B -m unittest discover -s tests/ci_scope -p 'test_*.py'
      - name: Run offline Local CI tests
        run: python -B -m unittest discover -s tests/local_ci -p 'test_*.py'
      - name: Check Python syntax
        run: python -m py_compile tools/validate_contracts.py
      - name: Validate contract
        run: python tools/validate_contracts.py
      - name: Run host tests
        run: python -m unittest tests.host.test_validate_contracts
"""


class PortableHeaderWorkflowTests(unittest.TestCase):
    def setUp(self):
        self.before = host_workflow_fixture()
        self.step = scope.PORTABLE_HEADER_STEP
        self.anchor = scope.HOST_TEST_STEP_MARKER
        self.candidate = self.before.replace(self.anchor, self.step + self.anchor)

    def test_exact_portable_header_step_is_the_only_accepted_host_change(self):
        scope.require_protected_workflow_match(self.before, self.candidate, scope.WORKFLOWS[1])

    def test_header_step_command_tamper_is_rejected(self):
        altered = self.candidate.replace(
            "python -B tests/host_cpp/check_portable_headers.py", "python other-check.py")
        with self.assertRaises(scope.ScopeError):
            scope.require_protected_workflow_match(self.before, altered, scope.WORKFLOWS[1])

    def test_header_step_omission_is_rejected(self):
        with self.assertRaises(scope.ScopeError):
            scope.require_protected_workflow_match(self.before, self.before, scope.WORKFLOWS[1])

    def test_header_step_duplication_is_rejected(self):
        altered = self.before.replace(self.anchor, self.step + self.step + self.anchor)
        with self.assertRaises(scope.ScopeError):
            scope.require_protected_workflow_match(self.before, altered, scope.WORKFLOWS[1])

    def test_moved_header_step_is_rejected(self):
        altered = self.candidate.replace(self.step, "", 1)
        syntax_marker = "      - name: Check Python syntax\n"
        altered = altered.replace(syntax_marker, self.step + syntax_marker)
        with self.assertRaises(scope.ScopeError):
            scope.require_protected_workflow_match(self.before, altered, scope.WORKFLOWS[1])

    def test_original_host_regression_change_is_rejected(self):
        altered = self.candidate.replace(
            "python -B -m unittest discover -s tests/ci_scope -p 'test_*.py'",
            "echo skipped")
        with self.assertRaises(scope.ScopeError):
            scope.require_protected_workflow_match(self.before, altered, scope.WORKFLOWS[1])

    def test_unrelated_host_step_is_rejected(self):
        unrelated = "      - name: Unexpected step\n        run: echo unexpected\n"
        altered = self.candidate.replace(self.anchor, unrelated + self.anchor)
        with self.assertRaises(scope.ScopeError):
            scope.require_protected_workflow_match(self.before, altered, scope.WORKFLOWS[1])

    def test_sdk_guard_change_is_rejected(self):
        sdk = Path(scope.WORKFLOWS[0]).read_text(encoding="utf-8")
        altered = sdk.replace("needs: scope", "needs: other", 1)
        with self.assertRaises(scope.ScopeError):
            scope.require_protected_workflow_match(sdk, altered, scope.WORKFLOWS[0])


class PortableBuildLinkWorkflowTests(unittest.TestCase):
    def setUp(self):
        self.header = scope.PORTABLE_HEADER_STEP
        self.step = scope.PORTABLE_BUILD_LINK_STEP
        self.anchor = scope.HOST_TEST_STEP_MARKER
        self.before = host_workflow_fixture().replace(self.anchor, self.header + self.anchor)
        self.candidate = self.before.replace(self.anchor, self.step + self.anchor)

    def assert_candidate_matches(self, candidate):
        scope.require_protected_workflow_match(
            self.before, candidate, scope.WORKFLOWS[1], required_host_steps=("header", "build"))

    def test_exact_portable_build_link_step_is_the_only_accepted_host_change(self):
        self.assert_candidate_matches(self.candidate)

    def test_build_link_command_tamper_is_rejected(self):
        altered = self.candidate.replace(
            "python -B tests/host_cpp/check_portable_build_link.py", "python other-build.py")
        with self.assertRaises(scope.ScopeError):
            self.assert_candidate_matches(altered)

    def test_build_link_step_omission_is_rejected(self):
        with self.assertRaises(scope.ScopeError):
            self.assert_candidate_matches(self.before)

    def test_build_link_step_duplication_is_rejected(self):
        altered = self.before.replace(self.anchor, self.step + self.step + self.anchor)
        with self.assertRaises(scope.ScopeError):
            self.assert_candidate_matches(altered)

    def test_build_link_step_moved_from_approved_position_is_rejected(self):
        altered = self.candidate.replace(self.step, "", 1)
        syntax_marker = "      - name: Check Python syntax\n"
        altered = altered.replace(syntax_marker, self.step + syntax_marker)
        with self.assertRaises(scope.ScopeError):
            self.assert_candidate_matches(altered)

    def test_header_step_removal_is_rejected(self):
        altered = self.candidate.replace(self.header, "", 1)
        with self.assertRaises(scope.ScopeError):
            self.assert_candidate_matches(altered)

    def test_original_host_regression_change_is_rejected(self):
        altered = self.candidate.replace(
            "python -B -m unittest discover -s tests/ci_scope -p 'test_*.py'", "echo skipped")
        with self.assertRaises(scope.ScopeError):
            self.assert_candidate_matches(altered)

    def test_unrelated_host_step_is_rejected(self):
        unrelated = "      - name: Unexpected step\n        run: echo unexpected\n"
        altered = self.candidate.replace(self.anchor, unrelated + self.anchor)
        with self.assertRaises(scope.ScopeError):
            self.assert_candidate_matches(altered)

    def test_sdk_guard_change_is_rejected(self):
        sdk = Path(scope.WORKFLOWS[0]).read_text(encoding="utf-8")
        altered = sdk.replace("needs: scope", "needs: other", 1)
        with self.assertRaises(scope.ScopeError):
            scope.require_protected_workflow_match(sdk, altered, scope.WORKFLOWS[0])


class EventTests(unittest.TestCase):
    def fixtures(self, paths, labels=()):
        pr = dict(number=12, head={"sha": HEAD}, base={"sha": BASE}, changed_files=len(paths),
                  labels=[{"name": name} for name in labels])
        event, calls = {"pull_request": copy.deepcopy(pr)}, []
        def get(route):
            calls.append(route)
            if route == ROOT:
                return copy.deepcopy(pr)
            if "/files?" in route:
                return [dict(filename=p, status="modified") for p in paths]
            if "/contents/" in route:
                path = route.split("/contents/")[1].split("?ref=")[0]
                return {"encoding": "base64", "content": base64.b64encode(Path(path).read_bytes()).decode()}
            if "/events?" in route:
                return [dict(event="labeled", label={"name": "ci/full-matrix"}, actor={"login": "reviewer"})]
            if "/permission" in route:
                return {"permission": "write"}
            self.fail("unexpected offline route " + route)
        return event, pr, calls, get

    def test_normal_dns_pr(self):
        event, _, calls, get = self.fixtures(["tests/espidf_dns_probe/a"])
        self.assertEqual(scope.plan("pull_request", event, REPO, get), dict(dns=True, callback=False, compile=False))
        self.assertEqual(calls.count(ROOT), 2)

    def test_actual_routing_only_pr_no_sdk(self):
        event, _, _, get = self.fixtures([*scope.WORKFLOWS, "tools/ci_scope.py", "tests/ci_scope/test_ci_scope.py"])
        self.assertFalse(any(scope.plan("pull_request", event, REPO, get).values()))

    def test_material_workflow_difference_stops(self):
        event, _, _, get = self.fixtures([scope.WORKFLOWS[0]])
        def altered(route):
            if "/contents/" in route:
                text = Path(scope.WORKFLOWS[0]).read_text().replace("espressif/idf:v6.0.2", "other-sdk")
                return {"encoding": "base64", "content": base64.b64encode(text.encode()).decode()}
            return get(route)
        with self.assertRaises(scope.ScopeError):
            scope.plan("pull_request", event, REPO, altered)

    def test_head_or_base_drift_stops(self):
        for side in ("head", "base"):
            event, pr, _, get = self.fixtures(["README.md"])
            pr[side]["sha"] = "c" * 40
            with self.subTest(side=side), self.assertRaises(scope.ScopeError):
                scope.plan("pull_request", event, REPO, get)

    def test_reviewer_full_label_and_unauthorized_or_missing_issuer(self):
        event, _, _, get = self.fixtures(["README.md"], ["ci/full-matrix"])
        self.assertTrue(all(scope.plan("pull_request", event, REPO, get).values()))
        for bad in ("read", None):
            def denied(route):
                return {"permission": bad} if "/permission" in route else get(route)
            with self.subTest(permission=bad), self.assertRaises(scope.ScopeError):
                scope.plan("pull_request", event, REPO, denied)
        with self.assertRaises(scope.ScopeError):
            scope.plan("pull_request", event, REPO, lambda route: [] if "/events?" in route else get(route))

    def test_manual_full_requires_explicit_profile_and_supported_event(self):
        self.assertTrue(all(scope.plan("workflow_dispatch", {"inputs": {"profile": "full"}}, REPO).values()))
        for event_name, event in [("workflow_dispatch", {}), ("workflow_dispatch", {"inputs": {"profile": "typo"}}), ("push", {})]:
            with self.subTest(event=event), self.assertRaises(scope.ScopeError):
                scope.plan(event_name, event, REPO)

    def test_end_of_pagination_revision_drift_stops(self):
        event, pr, _, get = self.fixtures(["README.md"])
        reads = 0
        def changed(route):
            nonlocal reads
            if route == ROOT:
                reads += 1
                if reads == 2:
                    pr["head"]["sha"] = "c" * 40
            return get(route)
        with self.assertRaises(scope.ScopeError):
            scope.plan("pull_request", event, REPO, changed)

    def test_label_history_truncation_and_permission_api_failure_stop(self):
        event, _, _, get = self.fixtures(["README.md"], ["ci/full-matrix"])
        def truncated(route):
            return [{}] * 100 if "/events?" in route else get(route)
        with self.assertRaises(scope.ScopeError):
            scope.plan("pull_request", event, REPO, truncated)
        def unavailable(route):
            if "/permission" in route:
                raise scope.ScopeError("reviewer permission unavailable")
            return get(route)
        with self.assertRaises(scope.ScopeError):
            scope.plan("pull_request", event, REPO, unavailable)


class SummaryTests(unittest.TestCase):
    def needs(self, selected):
        needs = {"scope": {"result": "success", "outputs": dict.fromkeys(scope.SUITES, str(selected).lower())}}
        needs.update({job: {"result": "success" if selected else "skipped"} for job in scope.SUITES.values()})
        return needs

    def test_selected_success_and_unselected_not_run(self):
        self.assertTrue(scope.summarize(self.needs(True))[0])
        good, report = scope.summarize(self.needs(False))
        self.assertTrue(good)
        self.assertEqual(report.count("; NOT_RUN"), 3)
        self.assertNotIn("; PASS", report)

    def test_selected_failure_cancel_skip_absent_fail_closed(self):
        for result in ["failure", "cancelled", "skipped", "absent"]:
            needs = self.needs(True)
            needs["compile"]["result"] = result
            with self.subTest(result=result):
                self.assertFalse(scope.summarize(needs)[0])

    def test_scope_failure_missing_outputs_and_unexpected_execution_fail(self):
        for mutate in [lambda n: n["scope"].update(result="failure"), lambda n: n["scope"]["outputs"].pop("dns"),
                       lambda n: n["compile"].update(result="success")]:
            needs = self.needs(False)
            mutate(needs)
            self.assertFalse(scope.summarize(needs)[0])

    def test_mixed_selection_and_missing_selected_job(self):
        needs = self.needs(False)
        needs["scope"]["outputs"]["dns"] = "true"
        needs["linux-dns"]["result"] = "success"
        self.assertTrue(scope.summarize(needs)[0])
        needs.pop("linux-dns")
        self.assertFalse(scope.summarize(needs)[0])


if __name__ == "__main__":
    unittest.main()
