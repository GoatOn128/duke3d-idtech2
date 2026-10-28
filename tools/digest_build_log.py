#!/usr/bin/env python3
"""Turn a raw CMake/Ninja/clang-cl build log into a readable markdown digest.

Used by the Windows CI workflow. The digest and the (optionally truncated)
log are committed back to the repo root so they can be read without opening
the Actions UI. Only the Python standard library is required.
"""

import argparse
import datetime
import re
import sys

# clang-cl:  path\file.cpp(12,34): error: message
# MSVC:      path\file.cpp(12): error C2065: message
# GCC-style: path/file.cpp:12:34: error: message
DIAG_RE = re.compile(
    r"^(?P<file>.+?)"
    r"(?:\((?P<line>\d+)(?:,(?P<col>\d+))?\)|:(?P<gline>\d+):(?P<gcol>\d+):)\s*:?\s*"
    r"(?P<kind>fatal error|error|warning|note)\s*:?\s*"
    r"(?:(?P<code>C\d{4}):)?\s*"
    r"(?P<msg>.*)$"
)

# Tool-level failures without a source location, e.g.
#   lld-link: error: undefined symbol: ...
#   clang: error: unable to execute command: ...
TOOL_ERROR_RE = re.compile(r"^(?P<tool>[A-Za-z][\w.-]*): error: (?P<msg>.*)$")

SOURCE_EXT_RE = re.compile(r"\.(c|cc|cpp|cxx|h|hh|hpp|rc|inc|inl|def)$", re.IGNORECASE)

MAX_ERRORS_SHOWN = 60
MAX_WARNINGS_PER_KIND = 12
TAIL_LINES = 30


def truncate_log_in_place(path, max_bytes):
    """Cap the log file at its last max_bytes bytes so repo commits stay small."""
    with open(path, "rb") as handle:
        data = handle.read()
    if len(data) <= max_bytes:
        return False
    kept = data[-max_bytes:]
    banner = (
        b"[log truncated: showing the last %d of %d bytes]\n" % (max_bytes, len(data))
    )
    with open(path, "wb") as handle:
        handle.write(banner + kept)
    return True


def parse_log(text):
    """Collect diagnostics plus tool-level errors (e.g. lld-link) with context."""
    diags = []
    tool_errors = []
    lines = text.splitlines()
    for index, line in enumerate(lines):
        stripped = line.strip()
        tool_match = TOOL_ERROR_RE.match(stripped)
        if tool_match and not DIAG_RE.match(stripped):
            context = []
            for follow in lines[index + 1 : index + 4]:
                if not follow.strip():
                    break
                context.append(follow.rstrip())
            tool_errors.append(
                {
                    "tool": tool_match.group("tool"),
                    "msg": tool_match.group("msg").strip(),
                    "context": context,
                }
            )
            continue
        match = DIAG_RE.match(stripped)
        if not match:
            continue
        file_name = match.group("file").strip()
        # Only treat plausible source paths as diagnostics.
        if not SOURCE_EXT_RE.search(file_name):
            continue
        kind = match.group("kind")
        if kind == "note":
            continue
        context = []
        back = index - 1
        while back >= 0 and len(context) < 2:
            previous = lines[back].rstrip()
            if not previous.strip():
                break
            if DIAG_RE.match(previous.strip()):
                break
            # Skip clang snippet/caret decoration.
            if re.match(r"^\s*(\^|~|\|)", previous):
                break
            context.insert(0, previous)
            back -= 1
        diags.append(
            {
                "file": file_name,
                "line": match.group("line") or match.group("gline") or "0",
                "col": match.group("col") or match.group("gcol") or "",
                "kind": kind,
                "code": match.group("code") or "",
                "msg": match.group("msg").strip(),
                "context": context,
            }
        )
    return diags, tool_errors, lines


def warning_kind(diag):
    """Group key for a warning: the MSVC code when present, else the leading words."""
    if diag["code"]:
        return "MSVC " + diag["code"]
    words = diag["msg"].split()
    return " ".join(words[:2]).lower() if words else "(empty message)"


def unique_locations(diags):
    seen = []
    for diag in diags:
        location = "%s(%s)" % (diag["file"], diag["line"])
        if location not in seen:
            seen.append(location)
    return seen


def render_header(args, error_count, warning_count):
    out = ["# Build digest", ""]
    out.append("- Commit: `%s`" % args.commit)
    out.append("- Branch: `%s`" % args.branch)
    out.append("- Result: %s" % (args.result or "unknown (see the Actions run)"))
    if args.run_url:
        out.append("- Actions run: %s" % args.run_url)
    out.append(
        "- Generated: %s UTC" % datetime.datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S")
    )
    out.append("")
    out.append(
        "Errors: **%d**, warnings: **%d**." % (error_count, warning_count)
    )
    out.append("")
    return out


def render_tool_errors(tool_errors):
    out = ["## Tool errors", ""]
    if not tool_errors:
        out.append("No tool-level errors (lld-link, ninja, ...) in the log.")
        out.append("")
        return out
    for entry in tool_errors[:MAX_ERRORS_SHOWN]:
        out.append("- `%s: error: %s`" % (entry["tool"], entry["msg"]))
        if entry["context"]:
            out.append("")
            out.append("  ```")
            for context_line in entry["context"]:
                out.append("  " + context_line.strip())
            out.append("  ```")
            out.append("")
    if len(tool_errors) > MAX_ERRORS_SHOWN:
        out.append("(list capped at %d)" % MAX_ERRORS_SHOWN)
        out.append("")
    return out


def render_errors(errors):
    out = ["## Errors", ""]
    if not errors:
        out.append("No errors in the log.")
        out.append("")
        return out
    out.append(
        "Showing the first %d of %d error(s)." % (min(MAX_ERRORS_SHOWN, len(errors)), len(errors))
    )
    out.append("")
    for diag in errors[:MAX_ERRORS_SHOWN]:
        location = "%s(%s)" % (diag["file"], diag["line"])
        if diag["col"]:
            location = "%s,%s" % (location, diag["col"])
        out.append("- `%s`: %s" % (location, diag["msg"]))
        if diag["context"]:
            out.append("")
            out.append("  ```")
            for context_line in diag["context"]:
                out.append("  " + context_line.strip())
            out.append("  ```")
            out.append("")
    out.append("")
    return out


def render_warnings(warnings):
    out = ["## Warnings", ""]
    if not warnings:
        out.append("No warnings in the log.")
        out.append("")
        return out
    kinds = {}
    for diag in warnings:
        kinds.setdefault(warning_kind(diag), []).append(diag)
    out.append("%d warning(s) in %d kind(s):" % (len(warnings), len(kinds)))
    out.append("")
    out.append("| Kind | Count | Locations |")
    out.append("| --- | --- | --- |")
    for kind in sorted(kinds, key=lambda name: -len(kinds[name])):
        entries = kinds[kind]
        locations = unique_locations(entries)
        sample = " ".join(locations[:3])
        if len(locations) > 3:
            sample += " (+%d more)" % (len(locations) - 3)
        out.append("| %s | %d | %s |" % (kind, len(entries), sample))
    out.append("")
    out.append("### Warning messages by kind")
    out.append("")
    for kind in sorted(kinds, key=lambda name: -len(kinds[name])):
        out.append("- **%s**" % kind)
        seen_locs = []
        for diag in kinds[kind]:
            location = "%s(%s)" % (diag["file"], diag["line"])
            if location in seen_locs:
                continue
            seen_locs.append(location)
            out.append(
                "  - `%s`: %s" % (location, diag["msg"])
            )
            if len(seen_locs) >= MAX_WARNINGS_PER_KIND:
                break
        if len(seen_locs) >= MAX_WARNINGS_PER_KIND:
            out.append("  - (list capped)")
    out.append("")
    return out


def render_cmake_errors(lines):
    """Configure-time failures do not match the compiler regex; list them raw."""
    hits = [line.strip() for line in lines if line.strip().startswith("CMake Error")]
    out = ["## CMake errors", ""]
    if not hits:
        out.append("No CMake errors in the log.")
        out.append("")
        return out
    out.append("```")
    for line in hits[:20]:
        out.append(line)
    out.append("```")
    out.append("")
    return out


def render_tail(lines):
    out = ["## Log tail", ""]
    out.append("```")
    for line in lines[-TAIL_LINES:]:
        out.append(line)
    out.append("```")
    out.append("")
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", default="build-windows.log", help="raw build log")
    parser.add_argument("--out", default="build-digest.md", help="markdown digest")
    parser.add_argument("--commit", default="", help="commit the log was built from")
    parser.add_argument("--branch", default="", help="branch the log was built from")
    parser.add_argument("--run-url", default="", help="Actions run URL")
    parser.add_argument("--result", default="", help="short result line, e.g. exit codes")
    parser.add_argument(
        "--truncate-log-bytes",
        type=int,
        default=0,
        help="keep only the last N bytes of the log file",
    )
    args = parser.parse_args()

    if args.truncate_log_bytes > 0:
        truncate_log_in_place(args.log, args.truncate_log_bytes)

    with open(args.log, "r", encoding="utf-8", errors="replace") as handle:
        text = handle.read()

    diags, tool_errors, lines = parse_log(text)
    errors = [diag for diag in diags if diag["kind"] in ("error", "fatal error")]
    warnings = [diag for diag in diags if diag["kind"] == "warning"]
    error_total = len(errors) + len(tool_errors)

    out = render_header(args, error_total, len(warnings))
    out += render_cmake_errors(lines)
    out += render_tool_errors(tool_errors)
    out += render_errors(errors)
    out += render_warnings(warnings)
    out += render_tail(lines)

    with open(args.out, "w", encoding="utf-8") as handle:
        handle.write("\n".join(out) + "\n")

    print(
        "digest written to %s: %d error(s), %d warning(s)"
        % (args.out, error_total, len(warnings))
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
