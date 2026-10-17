"""Read-only PR scope planning; selected SDK suites must succeed, skips are NOT_RUN.

Normal PR: changed paths (including rename sources) select a union of SDK suites.
Manual workflow_dispatch profile=full, or a ci/full-matrix label applied by a
current write/maintain/admin collaborator, selects all suites. No implicit full
dispatch fallback. These routes require separately authorized runner budget.
Workflow-only changes reuse historical builds only when protected job definitions
match the exact PR base. Independent review/branch-rule acceptance is still needed.
"""
import argparse
import base64
import json
import os
from pathlib import Path
import re
import sys
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

SUITES = {"dns": "linux-dns", "callback": "linux-callback", "compile": "compile"}
WORKFLOWS = (".github/workflows/esp-idf-compile.yml",
             ".github/workflows/validate-contracts.yml")
LOCAL_CI_FILES = {"tools/local_ci.py", "tools/local-ci.config.json", "tools/local-ci.sh",
                  "tools/bootstrap-local-ci.sh", "tools/bootstrap-local-ci.ps1"}
HOST_REGRESSIONS = {
    "Run offline CI scope tests": "python -B -m unittest discover -s tests/ci_scope -p 'test_*.py'",
    "Run offline Local CI tests": "python -B -m unittest discover -s tests/local_ci -p 'test_*.py'",
}


class ScopeError(ValueError):
    pass


def valid_path(path):
    if (not isinstance(path, str) or not path or path.startswith("/") or
            "\\" in path or any(p in ("", ".", "..") for p in path.split("/")) or
            any(ord(c) < 32 or ord(c) == 127 for c in path)):
        raise ScopeError("missing/unsafe changed-file path")
    return path


def changed_paths(files):
    paths = set()
    for item in files:
        if not isinstance(item, dict) or item.get("status") not in {
                "added", "modified", "removed", "renamed", "copied", "changed", "unchanged"}:
            raise ScopeError("unsupported changed-file record")
        paths.add(valid_path(item.get("filename")))
        if item["status"] == "renamed":
            paths.add(valid_path(item.get("previous_filename")))
    return paths


def classify(paths):
    selected = {key: False for key in SUITES}
    for raw in paths:
        path = valid_path(raw)
        if path.startswith("tests/espidf_dns_probe/"):
            selected["dns"] = True
        elif path.startswith("tests/espidf_linux_probe/"):
            selected["callback"] = True
        elif path.startswith(("components/", "firmware/", "tests/espidf_probe/")):
            selected["compile"] = True
        elif (path in LOCAL_CI_FILES or path in (*WORKFLOWS, "tools/ci_scope.py", "tools/validate_contracts.py",
                      "TASKS.md", "BACKLOG.md", "AGENTS.md", "README.md", "LICENSE", ".gitignore") or
              path.startswith(("docs/", "evidence/", "specs/", "tests/host/",
                               "tests/host_cpp/", "tests/ci_scope/", "tests/local_ci/"))):
            continue
        else:
            # Unknown implementation/security/build input must never become docs-only.
            return dict.fromkeys(SUITES, True)
    return selected


def job_block(text, job, require_guard=False):
    match = re.search(r"^  " + re.escape(job) + r":\n(.*?)(?=^  [\w-]+:\n|\Z)",
                      text.replace("\r\n", "\n"), re.M | re.S)
    if not match:
        raise ScopeError("protected SDK job missing")
    key = next(key for key, value in SUITES.items() if value == job)
    needs = re.findall(r"^    needs:([^\n]*)\n", match[1], flags=re.M)
    conditions = re.findall(r"^    if:([^\n]*)\n", match[1], flags=re.M)
    if require_guard or needs or conditions:
        expected = "${{ needs.scope.outputs." + key + " == 'true' }}"
        if needs != [" scope"] or conditions != [" " + expected]:
            raise ScopeError("unreviewed SDK needs/if routing")
    # Only the two validated single-line routing keys may differ from legacy base.
    return re.sub(r"^    (?:needs|if):[^\n]*\n", "", match[1], flags=re.M).rstrip()


def host_regressions(text, required=False):
    text = text.replace("\r\n", "\n")
    for name, command in HOST_REGRESSIONS.items():
        marker = "      - name: " + name + "\n"
        block = marker + "        run: " + command + "\n"
        if marker in text:
            if text.count(marker) != 1 or block not in text:
                raise ScopeError("Host offline regression command drift")
        elif required:
            raise ScopeError("mandatory Host offline regression missing")


def protected_workflow(text, path, require_sdk_guards=False):
    if path == WORKFLOWS[0]:
        return tuple(job_block(text, job, require_sdk_guards) for job in SUITES.values())
    marker = "      - name: Check Python syntax\n"
    text = text.replace("\r\n", "\n")
    host_regressions(text)
    if text.count(marker) != 1 or "    runs-on: ubuntu-latest\n" not in text:
        raise ScopeError("protected Host suite missing")
    return text[text.index(marker):].rstrip()


def fetch_json(route):
    # Fixed platform host; never follow a filename/PR-provided URL or print tokens.
    request = Request("https://api.github.com/" + route, headers={
        "Authorization": "Bearer " + os.environ["GITHUB_TOKEN"],
        "Accept": "application/vnd.github+json", "X-GitHub-Api-Version": "2022-11-28"})
    try:
        with urlopen(request, timeout=20) as response:
            return json.load(response)
    except HTTPError as error:
        raise ScopeError(f"GitHub read failed HTTP {error.code}") from None
    except (URLError, TimeoutError, json.JSONDecodeError):
        raise ScopeError("GitHub read failed; no SDK jobs admitted") from None


def collect_files(get, root, pr):
    total = pr.get("changed_files")
    if type(total) is not int or not 0 <= total < 3000:
        raise ScopeError("PR changed-file coverage unavailable/API limit reached")
    files = []
    for page in range(1, (total + 99) // 100 + 1):
        batch = get(f"{root}/files?per_page=100&page={page}")
        if not isinstance(batch, list) or len(batch) != min(100, total - len(files)):
            raise ScopeError("incomplete PR file pagination")
        files.extend(batch)
    changed_paths(files)  # Also checks shape before counting filenames.
    if len({f["filename"] for f in files}) != total:
        raise ScopeError("duplicate/incomplete PR file coverage")
    return files


def reviewer_full(get, repo, pr):
    if not any(label.get("name") == "ci/full-matrix" for label in pr.get("labels", [])):
        return False
    # Labels persist across synchronize events. Resolve their actual most recent issuer.
    actor = None
    for page in range(1, 31):
        events = get(f"repos/{repo}/issues/{pr['number']}/events?per_page=100&page={page}")
        if not isinstance(events, list):
            raise ScopeError("full-validation label provenance unavailable")
        for item in events:
            if item.get("label", {}).get("name") == "ci/full-matrix":
                if item.get("event") == "labeled":
                    actor = item.get("actor", {}).get("login")
                elif item.get("event") == "unlabeled":
                    actor = None
        if len(events) < 100:
            break
    else:
        raise ScopeError("full-validation label provenance truncated")
    if not actor or not re.fullmatch(r"[A-Za-z0-9-]+", actor):
        raise ScopeError("full-validation label issuer missing")
    permission = get(f"repos/{repo}/collaborators/{actor}/permission").get("permission")
    if permission not in ("admin", "maintain", "write"):
        raise ScopeError("full-validation label lacks current reviewer write authority")
    return True


def plan(event_name, event, repo, get=fetch_json):
    if event_name == "workflow_dispatch":
        if event.get("inputs", {}).get("profile") != "full":
            raise ScopeError("dispatch requires explicit profile=full")
        return dict.fromkeys(SUITES, True)
    if event_name != "pull_request" or not re.fullmatch(r"[\w.-]+/[\w.-]+", repo):
        raise ScopeError("unsupported event/repository identity")
    host_regressions(Path(WORKFLOWS[1]).read_text(), required=True)
    expected = event["pull_request"]
    number = expected["number"]
    if type(number) is not int or number <= 0:
        raise ScopeError("invalid PR identity")
    root = f"repos/{repo}/pulls/{number}"
    pr = get(root)

    def stable(snapshot):
        for side in ("base", "head"):
            sha = snapshot[side]["sha"]
            if not re.fullmatch(r"[0-9a-f]{40}", sha) or sha != expected[side]["sha"]:
                raise ScopeError("PR revision drift; wait for current event")
    stable(pr)
    files = collect_files(get, root, pr)
    paths = changed_paths(files)
    full = reviewer_full(get, repo, pr)
    if not full:
        # Routing-only workflows may skip SDK ONLY if protected commands/config match base.
        for path in set(WORKFLOWS) & paths:
            data = get(f"repos/{repo}/contents/{path}?ref={pr['base']['sha']}")
            if data.get("encoding") != "base64":
                raise ScopeError("base workflow content unavailable")
            before = base64.b64decode(data["content"], validate=False).decode("utf-8")
            if protected_workflow(before, path) != protected_workflow(
                    Path(path).read_text(), path, require_sdk_guards=True):
                raise ScopeError("workflow build/test semantics changed; explicit full approval required")
    stable(get(root))
    return dict.fromkeys(SUITES, True) if full else classify(paths)


def summarize(needs):
    scope = needs.get("scope", {})
    good = scope.get("result") == "success"
    lines = ["## SDK scope result", "Skipped suites are NOT_RUN, never test PASS."]
    for key, job in SUITES.items():
        value = scope.get("outputs", {}).get(key)
        result = needs.get(job, {}).get("result", "absent")
        selected = value == "true"
        good = good and value in ("true", "false")
        good = good and (result == "success" if selected else result == "skipped")
        lines.append(f"- {job}: selected={value}, result={result}; " +
                     ("PASS" if selected and result == "success" else
                      "NOT_RUN" if not selected and result == "skipped" else "FAIL"))
    return good, "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("plan", "summary"))
    mode = parser.parse_args().mode
    try:
        if mode == "plan":
            result = plan(os.environ["GITHUB_EVENT_NAME"],
                          json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text()),
                          os.environ["GITHUB_REPOSITORY"])
            text = "## SDK scope plan\n" + "\n".join(
                f"- {SUITES[key]}: {'RUN' if value else 'SKIPPED / NOT_RUN'}"
                for key, value in result.items()) + "\n"
            with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
                for key, value in result.items():
                    output.write(f"{key}={str(value).lower()}\n")
            good = True
        else:
            good, text = summarize(json.loads(os.environ["CI_NEEDS"]))
        print(text, end="")
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a", encoding="utf-8") as summary:
            summary.write(text)
        return 0 if good else 1
    except (ScopeError, KeyError, TypeError, OSError, ValueError) as error:
        print(f"CI scope FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
