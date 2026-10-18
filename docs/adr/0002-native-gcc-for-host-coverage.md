# ADR-0002: Use native gcc (not the ESP-IDF linux target) for host tests and coverage

- **Status:** Accepted
- **Date:** 2026-10-18

## Context

We need to run the `processor` unit tests on the host (see
[ADR-0001](0001-split-logic-from-hardware.md)). ESP-IDF offers a "linux target"
(`idf.py --preview set-target linux`) that compiles an IDF project for the host,
which seemed like the natural way to reuse the IDF build system for host tests.

In practice it had two serious problems:

1. **Coverage did not work.** With the linux target, gcov failed at runtime with
   `profiling:/home:Cannot create directory` and `... .gcda:Skip`, so no `.gcda`
   coverage data was ever written. The IDF linux-target runtime intercepts
   filesystem calls, which prevents gcov from creating its output files. Several
   workarounds were tried (explicit `__gcov_dump()`, a weak symbol, a macro
   gate, `GCOV_PREFIX`/`GCOV_PREFIX_STRIP`); none produced `.gcda` files.
2. **Builds were very slow.** The linux target pulls in a large portion of the
   IDF component tree (freertos, lwip, mbedtls, drivers, …), making a "host unit
   test" take many minutes to build the first time.

The `processor` component has **no ESP-IDF dependencies at all** — it is plain
C. That means it does not actually need the IDF build system to be tested.

## Decision

Build the host unit tests as a **plain native gcc binary** via a small Makefile
(`test_host/Makefile`), compiling `processor.c` + the test file + Unity
directly. Do **not** use the ESP-IDF linux target for host tests.

Unity is still sourced from `$IDF_PATH` (not vendored), so the only dependency
is an ESP-IDF checkout, which contributors already have.

## Consequences

**Easier:**
- gcov coverage works (currently 100% line/branch on `processor.c`).
- valgrind and AddressSanitizer/UBSan run cleanly on the native binary.
- Builds in well under a second instead of minutes.
- The test binary is a normal process that exits cleanly — no simulated
  scheduler that never returns.

**Harder / trade-offs accepted:**
- This approach only works for components with no IDF dependencies. Code that
  genuinely needs IDF/FreeRTOS APIs must be tested on-target instead (planned:
  pytest-embedded on a self-hosted runner). This reinforces ADR-0001: keep
  logic IDF-free so it stays host-testable.
- Two build systems coexist (CMake/IDF for firmware, Make for host tests). This
  is acceptable because they serve genuinely different purposes.
