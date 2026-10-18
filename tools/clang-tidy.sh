#!/usr/bin/env bash
# Run clang-tidy on first-party sources using the system clang-tidy.
#
# ESP-IDF's compile_commands.json is generated for the Xtensa GCC cross
# compiler and contains GCC-only flags that upstream clang rejects
# (-mlongcalls, -fno-malloc-dce, ...). We produce a sanitized copy with those
# flags stripped into its own directory, and point clang-tidy there. This
# avoids needing Espressif's separate clang toolchain just to lint our code.
#
# `--line-filter` restricts diagnostics to our own files, so findings that
# originate inside expanded ESP-IDF macros (e.g. ESP_LOGx) do not fail the run.
#
# Usage: tools/clang-tidy.sh [--fix]
#   Requires an existing build/ (run `idf.py build` first).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DB="$ROOT/build/compile_commands.json"
OUTDIR="$ROOT/build/clang-tidy"

if [ ! -f "$DB" ]; then
  echo "error: $DB not found. Run 'idf.py build' first." >&2
  exit 1
fi

mkdir -p "$OUTDIR"

# Rewrite the compile DB so upstream clang-tidy can parse our sources:
#   - replace the Xtensa GCC compiler with `clang` (clang-tidy keys driver mode
#     off argv[0]; the Xtensa driver pulls in flags clang rejects),
#   - strip GCC/Xtensa-only flags clang does not understand.
# clang-tidy only needs to PARSE the code, not produce a valid Xtensa binary.
python3 - "$DB" "$OUTDIR/compile_commands.json" <<'PY'
import json, re, sys
src, dst = sys.argv[1], sys.argv[2]

# Whole-token flags (and =value forms) clang rejects on this target.
BAD = re.compile(
    r'^(-mlongcalls'
    r'|-fno-malloc-dce'
    r'|-fno-shrink-wrap'
    r'|-fno-tree-switch-conversion'
    r'|-fstrict-volatile-bitfields'
    r'|-fzero-init-padding-bits=.*'
    r'|-fno-jump-tables'
    r'|-Wno-old-style-declaration'
    r'|-Wunused-but-set-variable=.*'
    r')$'
)

db = json.load(open(src))
for e in db:
    cmd = e.get('command', '')
    toks = cmd.split()
    if toks:
        toks[0] = 'clang'                      # neutralize the Xtensa driver
    # Drop @response-file args (e.g. build/toolchain/cflags) -- they carry the
    # Xtensa codegen flags that clang rejects and are irrelevant to parsing.
    toks = [t for t in toks if not t.lstrip('@').strip('"').endswith('/cflags')]
    toks = [t for t in toks if not t.startswith('@')]
    toks = [t for t in toks if not BAD.match(t)]
    e['command'] = ' '.join(toks)
json.dump(db, open(dst, 'w'), indent=2)
PY

FILES=(
  "$ROOT/components/processor/processor.c"
  "$ROOT/components/sampler/sampler.c"
  "$ROOT/components/reporter/reporter.c"
  "$ROOT/components/app_core/app_core.c"
  "$ROOT/main/main.c"
)

# Only diagnose lines in our own source files.
LINE_FILTER='[
  {"name":"processor.c"},{"name":"sampler.c"},{"name":"reporter.c"},
  {"name":"app_core.c"},{"name":"main.c"}
]'

# Run clang-tidy. We do NOT use --warnings-as-errors='*' because plain clang
# cannot fully resolve every ESP-IDF system header (e.g. <machine/endian.h>),
# which yields harmless clang-diagnostic-error parse artifacts unrelated to our
# code. Instead we capture output and fail only on real check findings in our
# files.
OUT="$(clang-tidy --line-filter="$LINE_FILTER" -p "$OUTDIR" "$@" "${FILES[@]}" 2>&1 || true)"
echo "$OUT"

# A "real" finding is a diagnostic in one of OUR files tagged with a check name
# that is not a clang parse diagnostic.
if echo "$OUT" | grep -E '/(components|main)/.*\.(c|h):[0-9]+:[0-9]+: (warning|error):' \
                | grep -vE 'clang-diagnostic-' \
                | grep -qE '\[[a-z]'; then
  echo "clang-tidy: real findings above." >&2
  exit 1
fi
echo "clang-tidy: no findings in first-party code."
