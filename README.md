# sensor-pipeline

A small, focused ESP32 (ESP-IDF) project for learning **FreeRTOS** and **good
engineering practices** (CI/CD, unit testing, static analysis, memory tooling,
and device security). Built in independent phases — each phase is useful on its
own and builds on the last.

> **New here?** See [`CONTRIBUTING.md`](CONTRIBUTING.md) for the branch/PR/CI
> workflow, [`docs/adr/`](docs/adr/) for why key decisions were made,
> [`docs/roadmap.md`](docs/roadmap.md) for current status + deferred work, and
> the published **[API docs](https://abbystrebel.github.io/sensor-pipeline/)**
> (or run `make docs` to build them locally).

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
│   ├── contracts/            # shared message types (sample_t); no logic
│   ├── sampler/              # FreeRTOS task: samples source -> queue (Phase 2)
│   ├── reporter/             # FreeRTOS task: queue -> UART output (Phase 2)
│   └── app_core/             # composition root: creates queues/tasks
├── test_host/                # linux-target test project (Unity) for processor
├── sdkconfig.defaults        # tracked, reproducible config
└── .github/workflows/        # CI (Phase 1+)
```

## Building

### Firmware (esp32 or esp32c6)

Pick a target with `set-target`; everything else is identical. The target list
is defined once in [`tools/targets.sh`](tools/targets.sh).

```sh
. $IDF_PATH/export.sh
idf.py set-target esp32      # or: idf.py set-target esp32c6
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

First build for a RISC-V target (esp32c6) needs the RISC-V toolchain:
`$IDF_PATH/install.sh esp32,esp32c6` installs both alongside each other.

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
- [x] **Phase 2 — RTOS wiring.** Real sampler (producer, pri 5) and reporter
      (consumer, pri 4) FreeRTOS tasks, a queue between them, and mutex-protected
      shared config. Builds with `-Werror`. On-target behavior to be verified in
      Phase 4 (or by manual flash).
- [x] **Phase 3 — Lint gate.** Curated clang-tidy + cppcheck + clang-format
      check in CI, plus pre-commit hooks for local use. (`-Werror` landed earlier
      as part of hardening.)
- [x] **Phase 3.5 — CD / release packaging.** `tools/package.sh` builds a full
      flashable bundle (bootloader + partition table + app + flash_args + ELF +
      map), with SHA-256 checksums, a `manifest.json`, and a `flash.sh`. CI
      uploads it on every push/PR and publishes a GitHub Release on `v*` tags
      (release job scoped to `contents: write`, tag-triggered only).
- [~] **Phase 4 — On-target CI.** pytest-embedded tests
      ([`test_target/`](test_target/)) and a guarded, Pi/SBC-friendly
      self-hosted-runner job (consumes the prebuilt bundle, flashes, tests) are
      **done**. Remaining: acquire + register the runner hardware (see below).
      Setup + security: [`docs/self-hosted-runner.md`](docs/self-hosted-runner.md).
- [ ] **Phase 5 — Security capstone.** Secure Boot v2 + flash encryption in
      **development mode** on a sacrificial board; firmware signed in CI with a
      key stored as a CI secret. Done = board rejects an unsigned image.

## Hardware to acquire (unblocks later phases)

Some phases need physical hardware that isn't on hand yet:

- **Self-hosted runner box (Phase 4).** A sacrificial, network-isolated machine
  with an ESP32 permanently attached, to run the `on-target` CI job. A cheap x86
  mini-PC or a Raspberry Pi 4/5 is ideal; a Pi Zero 2 W is marginal (flash-and-
  test only). The job is build-free specifically so a modest SBC suffices. Must
  be VLAN-isolated and run a non-privileged runner user — see
  [`docs/self-hosted-runner.md`](docs/self-hosted-runner.md). *(Blocked on
  acquiring the box + network setup.)*
- **Sacrificial ESP32 for Phase 5.** eFuse burning (Secure Boot / flash
  encryption) is irreversible, so it is done on a board you can afford to lock,
  in development mode. One of the on-hand Xtensa ESP32s is the intended
  sacrifice; the ESP32-C6 is kept un-fused for now so both targets stay usable
  for day-to-day build/flash and the single- vs dual-core experiments.

## Toolchain notes

- **Local ESP-IDF:** `v6.1-dev` (developer machine).
- **CI ESP-IDF:** pinned to **`release-v5.5`** (stable) for reproducible builds.
  Intentional mismatch — CI catches "works on my machine" drift.
- **Supported targets** (build for all of them; CI matrices over the list):
  - **ESP32** — Xtensa LX6, **dual-core**. Secure Boot v2 uses **RSA-3072**.
  - **ESP32-C6** — RISC-V, **single-core** (+ LP core), Wi-Fi 6 / BLE /
    802.15.4. Secure Boot v2 uses **ECDSA** (not RSA).
  The authoritative list lives in [`tools/targets.sh`](tools/targets.sh);
  adding a variant is a one-line edit there plus the CI matrices. The
  same source runs on both — the pure-logic/hardware split (ADR-0001) keeps
  `processor` architecture-agnostic, so a new variant touches only config/CI.
  The single- vs dual-core contrast is intentional: it makes concurrency
  behavior (e.g. cross-core visibility of shared state) observable on real,
  differing hardware.

## Security / secrets (Phase 5, summary)

- **On-device secrets** live in **encrypted NVS**, protected by flash encryption.
- **Signing keys** (Secure Boot, code signing) live in **CI secrets**, never in
  the repo. `.gitignore` blocks `*.pem` / `*.key` as a backstop.
- **Signing scheme differs by target** (decided at Phase 5 time, per board):
  on **ESP32** (Xtensa) Secure Boot v2 uses an **RSA-3072** key; on **ESP32-C6**
  (RISC-V) it uses an **ECDSA** key (P-256/P-384). The signing *process* and
  key-handling are the same; only the key type/algorithm changes. No eFuses are
  burned yet — Phase 5 is deferred, and this note just keeps the plan accurate
  for whichever board it runs on.
- eFuse burning is **irreversible** — only done on the sacrificial board, in
  development mode (keeps JTAG + re-flash so you can iterate).
