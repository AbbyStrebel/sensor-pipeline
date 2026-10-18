#!/usr/bin/env bash
# Run cppcheck on first-party sources. Shared by pre-commit and CI so there is
# a single source of truth for the flags.
#
# Usage: tools/cppcheck.sh [extra files...]
#   With no args, checks all first-party sources.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

INCLUDES=(
  -I components/processor/include
  -I components/contracts/include
  -I components/sampler/include
  -I components/reporter/include
  -I components/app_core/include
)

FILES=("$@")
if [ ${#FILES[@]} -eq 0 ]; then
  FILES=(
    components/processor/processor.c
    components/sampler/sampler.c
    components/reporter/reporter.c
    components/app_core/app_core.c
    main/main.c
  )
fi

exec cppcheck \
  --enable=warning,style,performance,portability \
  --std=c11 \
  --inline-suppr \
  --quiet \
  --error-exitcode=2 \
  --suppress=missingInclude \
  --suppress=missingIncludeSystem \
  "${INCLUDES[@]}" \
  "${FILES[@]}"
