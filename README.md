# sensor-pipeline

A small, focused ESP32 (ESP-IDF) project for learning **FreeRTOS** and **good
engineering practices** (CI/CD, unit testing, static analysis, memory tooling,
and device security). Built in independent phases — each phase is useful on its
own and builds on the last.

## Why it's structured this way

The core idea is a **hard split between pure logic and hardware code**:

| Layer | Components | Depends on hardware? | Tested how? |
|-------|-----------|----------------------|-------------|
| Pure logic | `processor` | No | Host (x86) Unity tests + valgrind + ASan |
| Hardware / RTOS | `sampler`, `reporter`, `app_core` | Yes (FreeRTOS, drivers) | On-target (pytest-embedded) |
| Entrypoint | `main` | Yes | On-target |

This split is deliberate: **valgrind and AddressSanitizer cannot run on the
ESP32** (Xtensa bare-metal). They only work on the host. By keeping all the real
logic in `processor` — with zero IDF/FreeRTOS includes — that logic compiles for
the `linux` target and runs under those tools in CI. The hardware components stay
thin and are verified on a real board.

```
sensor-pipeline/
├── main/                     # app_main — esp32 entrypoint, delegates to app_core
├── components/
│   ├── processor/            # PURE LOGIC: moving-average filter. No IDF deps.
│   ├── sampler/              # FreeRTOS task: samples source -> queue (Phase 2)
│   ├── reporter/             # FreeRTOS task: queue -> UART output (Phase 2)
│   └── app_core/             # composition root: creates queues/tasks
├── test_host/                # linux-target test project (Unity) for processor
├── sdkconfig.defaults        # tracked, reproducible config
└── .github/workflows/        # CI (Phase 1+)
```

## Building

### Firmware (esp32 target)

```sh
. $IDF_PATH/export.sh
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### Host tests (native gcc, not the IDF linux target)

The pure-logic `processor` component has no IDF dependencies, so host unit tests
build as a plain native binary via a small Makefile. This is deliberate:

- builds in under a second (no ESP-IDF component tree to compile),
- produces working `gcov` coverage — the ESP-IDF *linux target* runtime
  intercepts filesystem calls and prevents `.gcda` files from being written,
  so coverage does not work there,
- runs cleanly under valgrind.

```sh
cd test_host
make test        # build + run the unit tests
make coverage    # build with gcov, run, print coverage summary
make valgrind    # run under valgrind (fails on any leak/error)
make clean
```

Unity is sourced from `$IDF_PATH` (not vendored). Override `UNITY_DIR` or
`IDF_PATH` if yours lives elsewhere.

## Phase plan

- [x] **Phase 0 — Skeleton.** Dual-target component layout; esp32 + linux builds
      both green; clang-format, gitignore, README.
- [x] **Phase 1 — Host tests in CI.** Unity tests for `processor` (native gcc
      build), valgrind, and gcov coverage; GitHub Actions runs them on every
      push/PR alongside the esp32 firmware build. Pinned to ESP-IDF
      `release-v5.5`.
- [ ] **Phase 2 — RTOS wiring.** Real sampler/reporter tasks, a queue, a
      mutex-protected shared state, UART output. Runs on a board.
- [ ] **Phase 3 — Lint gate.** clang-tidy + cppcheck + `-Werror` in CI.
- [ ] **Phase 4 — On-target CI.** Self-hosted GitHub runner with an ESP32
      attached; pytest-embedded flashes + tests real hardware.
- [ ] **Phase 5 — Security capstone.** Secure Boot v2 + flash encryption in
      **development mode** on a sacrificial board; firmware signed in CI with a
      key stored as a CI secret. Done = board rejects an unsigned image.

## Toolchain notes

- **Local ESP-IDF:** `v6.1-dev` (developer machine).
- **CI ESP-IDF:** pinned to **`release-v5.5`** (stable) for reproducible builds.
  Intentional mismatch — CI catches "works on my machine" drift.
- Plain **ESP32** (Xtensa LX6, dual-core). Secure Boot v2 uses **RSA-3072**.

## Security / secrets (Phase 5, summary)

- **On-device secrets** live in **encrypted NVS**, protected by flash encryption.
- **Signing keys** (Secure Boot, code signing) live in **CI secrets**, never in
  the repo. `.gitignore` blocks `*.pem` / `*.key` as a backstop.
- eFuse burning is **irreversible** — only done on the sacrificial board, in
  development mode (keeps JTAG + re-flash so you can iterate).
