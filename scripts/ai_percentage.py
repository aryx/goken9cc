#!/usr/bin/env python3
"""Estimate how much of goken9cc's C/assembly was last written by Claude Code.

Unlike principia-softwarica's docs/scripts/ai_percentage.py, this codebase
has no blank-line-delimited "%claude: paragraph" convention to lean on --
source files just get an inline `/* claude: ... */` or `// claude: ...`
comment dropped next to whatever Claude touched, with no marked end. That
makes a text-based scan unreliable: it can find the *start* of a
Claude-written region but not where it stops.

Instead this script uses `git blame` as the ground truth. Every commit made
through Claude Code in this repo carries a "Co-Authored-By: Claude ..."
trailer (see CLAUDE.md's commit instructions), so for each line currently in
a tracked .c/.h/.s file we look up which commit last touched it and check
whether that commit has the trailer.

IMPORTANT: this repo does a lot of "port this file from src/libmach" /
"import from principia" style commits, where a commit adds a file at a new
path that is substantially (sometimes byte-for-byte) copied from a file that
already existed elsewhere in the tree, untouched by that same commit. Plain
`git blame` has no way to see that -- a file that's new *at that path* gets
100% of its lines credited to whichever commit added it there, even if every
line was copied verbatim from decades-old human code sitting one directory
over. `-C -C` tells git blame to also search the *rest of the parent tree*
(not just files touched in the same commit) for the true origin of each
line, which is exactly this case. It's ~30x slower than plain blame, which
is why this script shells out to `git blame` in parallel across all CPU
cores rather than running it serially.

Caveats:
  - A line "last touched" by a human commit that only reflowed code Claude
    originally wrote will be attributed to the human -- same "blame follows
    the last edit" limitation any git-blame-based metric has.
  - `-C -C` only looks one commit's parent tree deep. A copy of a copy (or a
    copy from a file that was itself later deleted/renamed away) can still
    dead-end at the wrong commit. `-C -C -C` searches full history and is
    noticeably more accurate (see the sym.c spot-check in this script's
    commit message / conversation), but is minutes slower per large file;
    not used here by default.
  - Lines with uncommitted local changes (blame hash all-zero) are reported
    separately and excluded from the percentage.
"""
import os
import re
import subprocess
import sys
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor, as_completed

EXTENSIONS = (".c", ".h", ".s")
ZERO_HASH = "0" * 40
BLAME_FLAGS = ["-C", "-C"]


def run(cmd):
    return subprocess.run(
        cmd, capture_output=True, text=True, check=True
    ).stdout


def claude_commits():
    out = run(
        [
            "git", "log", "--format=%H",
            "--grep=Co-authored-by:.*Claude", "-i", "-E",
        ]
    )
    return set(out.split())


def tracked_source_files():
    out = run(["git", "ls-files"])
    return [
        f for f in out.splitlines()
        if f.endswith(EXTENSIONS) and os.path.isfile(f)
    ]


def blame_hashes(path):
    """Return the blame commit hash for each non-blank line of path."""
    out = subprocess.run(
        ["git", "blame", *BLAME_FLAGS, "--line-porcelain", "--", path],
        capture_output=True, text=True,
    ).stdout
    hashes = []
    current_hash = None
    for line in out.splitlines():
        if line.startswith("\t"):
            content = line[1:]
            if content.strip() != "":
                hashes.append(current_hash)
            current_hash = None
        elif re.match(r"^[0-9a-f]{40} \d+ \d+", line):
            current_hash = line.split(" ", 1)[0]
    return hashes


def blame_counts(path, claude_set):
    """Return (total, ai, human, uncommitted) non-blank line counts for path."""
    total = ai = human = uncommitted = 0
    for h in blame_hashes(path):
        total += 1
        if h == ZERO_HASH:
            uncommitted += 1
        elif h in claude_set:
            ai += 1
        else:
            human += 1
    return total, ai, human, uncommitted


def top_dir(path):
    parts = path.split("/", 1)
    return parts[0] if len(parts) > 1 else "."


def main():
    root = run(["git", "rev-parse", "--show-toplevel"]).strip()
    os.chdir(root)

    claude_set = claude_commits()
    if not claude_set:
        print("No Claude-authored commits found (no "
              "'Co-authored-by: Claude' trailers in git log).",
              file=sys.stderr)
        sys.exit(1)

    files = tracked_source_files()

    stats = defaultdict(lambda: [0, 0, 0, 0])  # total, ai, human, uncommitted
    with ProcessPoolExecutor() as pool:
        futures = {
            pool.submit(blame_counts, f, claude_set): f for f in files
        }
        for i, fut in enumerate(as_completed(futures), 1):
            f = futures[fut]
            t, a, h, u = fut.result()
            d = stats[top_dir(f)]
            d[0] += t; d[1] += a; d[2] += h; d[3] += u
            if i % 100 == 0:
                print(f"  ...blamed {i}/{len(files)} files", file=sys.stderr)

    print(f"{'Dir':<16} {'LOC':>8} {'AI':>8} {'%AI':>6} {'Uncommitted':>12}")
    print("-" * 54)

    grand_total = grand_ai = grand_human = grand_uncommitted = 0
    active_total = active_ai = 0
    for d in sorted(stats, key=lambda k: -stats[k][0]):
        t, a, h, u = stats[d]
        pct = 100.0 * a / (t - u) if (t - u) else 0.0
        print(f"{d:<16} {t:>8} {a:>8} {pct:>5.1f}% {u:>12}")
        grand_total += t; grand_ai += a; grand_human += h; grand_uncommitted += u
        if d != "GO":
            active_total += t - u
            active_ai += a

    print("-" * 54)
    denom = grand_total - grand_uncommitted
    tpct = 100.0 * grand_ai / denom if denom else 0.0
    print(f"{'TOTAL':<16} {grand_total:>8} {grand_ai:>8} {tpct:>5.1f}% {grand_uncommitted:>12}")

    apct = 100.0 * active_ai / active_total if active_total else 0.0
    print(f"{'TOTAL excl GO/':<16} {active_total:>8} {active_ai:>8} {apct:>5.1f}%")
    print(f"\n({len(claude_set)} Claude-authored commits, {len(files)} source files)")


if __name__ == "__main__":
    main()
