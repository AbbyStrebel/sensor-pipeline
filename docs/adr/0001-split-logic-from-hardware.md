# ADR-0001: Split pure logic from hardware code

- **Status:** Accepted
- **Date:** 2026-10-17

## Context

This is an ESP32 (ESP-IDF / FreeRTOS) project whose goals include real
automated testing: unit tests, memory checking (valgrind), sanitizers, and
coverage — ideally running in CI on every change, without a physical board
attached.

Two hard facts shape the design:

1. **Tools like valgrind, AddressSanitizer, and gcov cannot run on the ESP32.**
   The ESP32 is a bare-metal Xtensa target; these tools require a host OS
   (x86/Linux). They can only analyze code compiled for and run on the host.
2. **Hardware-dependent code (FreeRTOS tasks, GPIO, peripheral drivers) can
   only run on the chip** (or a heavyweight simulator), and is slow to
   iterate on.

If logic and hardware access are tangled together, none of the logic can be
tested with host tools — a common failure mode in embedded codebases.

## Decision

Structure the code so that **pure logic contains no hardware/IDF dependencies**
and lives in its own component (`processor`), while hardware-facing code
(`sampler`, `reporter`, `app_core`) stays thin and delegates real work to the
pure logic.

- `processor` includes only standard C headers. It compiles unchanged for both
  the ESP32 (cross-compiled) and the host (native gcc).
- Hardware components own FreeRTOS primitives and peripheral access only.

## Consequences

**Easier:**
- Pure logic is unit-tested on the host in milliseconds, under valgrind/ASan,
  with coverage — all in CI, no board required.
- The same tested source runs on the device: no duplicated logic.
- Porting to another ESP32 variant mostly affects the thin hardware layer; the
  logic is portable as-is.

**Harder / trade-offs accepted:**
- Requires discipline: it is tempting to put real work directly inside a
  FreeRTOS task. The standard is to keep tasks thin and push logic into
  host-testable functions.
- Some indirection (interfaces between layers) that a tiny project could
  technically skip.

**Verified by the whole project since:** `processor` reaches 100% line/branch
coverage in host tests, valgrind reports zero leaks, and both the esp32
firmware and the host binary build from one source tree.
