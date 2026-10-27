# Project conventions

Rules for working in this ESP32 / ESP-IDF repo. The *why* lives in
`docs/adr/`; this file is the short, always-on checklist.

## Supported build targets

- Two chips are first-class: **esp32** (Xtensa, dual-core) and **esp32c6**
  (RISC-V, single-core). Build/CI must keep both working.
- The authoritative target list is `tools/targets.sh` (`SUPPORTED_TARGETS`).
  Adding a variant = one edit there, plus the CI matrices in `ci.yml`. Do not
  hardcode a single chip in build scripts or `sdkconfig.defaults`.
- The target is chosen by `idf.py set-target <chip>`, NOT pinned in
  `sdkconfig.defaults`. Any target-specific config goes in
  `sdkconfig.defaults.<target>`.
- Single- vs dual-core is an intentional contrast to explore, not a bug to
  hide. Code that touches cross-task shared state should be correct on the
  dual-core esp32 (prefer passing data via task args / queues over relying on
  incidental single-core serialization).

## The logic/hardware split is a hard rule (ADR-0001)

- `components/processor/` is PURE logic. It must include ONLY standard C
  headers (`<stddef.h>`, `<stdint.h>`, `<math.h>`, ...). Never add
  FreeRTOS, ESP-IDF, `esp_log.h`, or any hardware include to it.
- Why it matters: that zero-dependency property is what lets `processor`
  compile natively and run under valgrind / AddressSanitizer / gcov on the
  host. Adding one IDF include silently breaks the entire test strategy.
- Hardware components (`sampler`, `reporter`, `app_core`, `main`) own all
  FreeRTOS primitives and peripheral access.

## Keep tasks thin — logic goes in `processor`

- New signal/processing logic belongs in `processor`, with host unit tests,
  NOT inside a FreeRTOS task. Tasks are plumbing: sample, queue, log.
- If you're tempted to do real math inside `sampler_task`/`reporter_task`,
  put it in `processor` and call into it instead.

## Two build paths — do not confuse them

- Firmware (esp32): `idf.py set-target esp32 && idf.py build`.
- Host unit tests: plain native gcc via `test_host/Makefile`, NOT the IDF
  linux target. Use `make test` / `make coverage` / `make valgrind` /
  `make asan` from `test_host/`. (The IDF linux-target runtime blocks gcov;
  see ADR-0002.)
- On-target tests: pytest-embedded in `test_target/` (needs real hardware).

## Quality gates (all code must pass)

- Builds are `-Werror`. Do not suppress warnings to get a build green.
- Run static analysis through the shared wrappers so flags stay consistent:
  `tools/cppcheck.sh` and `tools/clang-tidy.sh` (clang-tidy needs a prior
  `idf.py build` for the compile DB).
- Format with the repo `.clang-format`. Pre-commit hooks mirror the CI lint
  gate; `pre-commit run --all-files` before pushing.
- Prefer fixed-capacity / no dynamic allocation in code that runs on-device.
