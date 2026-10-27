# Self-hosted runner (on-target testing)

The `on-target` CI job flashes a real ESP32 and runs the pytest-embedded tests
against it (see [`test_target/`](../test_target/)). This requires a
**self-hosted GitHub Actions runner** on a machine with an ESP32 attached,
because GitHub's cloud runners have no USB-connected hardware.

> **Status:** the workflow job exists and is guarded, but there is no runner
> registered yet. The `on-target` job is a matrix over the supported targets
> (`esp32`, `esp32c6`); each leg runs on a runner whose label matches its
> chip. A leg only runs once a runner carrying that label is online, so until
> you register one, the legs simply stay queued/do not run. Nothing else in CI
> is affected. You can bring up one board at a time — register an `esp32c6`
> runner and the C6 leg runs; the `esp32` leg waits for an `esp32` runner.

## ⚠️ Security: read this first

A self-hosted runner executes repository code **on your own machine**. On a
**public** repo this is dangerous if misconfigured — a malicious pull request
could run arbitrary code on the runner. This project mitigates that:

1. **The job only runs on `push` to `main` of this exact repo** — never on
   `pull_request`, never on forks:
   ```yaml
   if: github.event_name == 'push' &&
       github.ref == 'refs/heads/main' &&
       github.repository == 'AbbyStrebel/sensor-pipeline'
   ```
   Only people with write access (you) can push to `main`, so only your code
   runs on the runner.
2. **In repo settings**, set **Settings → Actions → General → Fork pull request
   workflows** to require approval for all outside collaborators (or disable
   fork PRs from using Actions entirely).
3. **Treat the runner box as sacrificial / low-trust:**
   - Run the runner as a **dedicated non-privileged user** (not root, not your
     main user). It only needs access to the serial device (group `uucp`/
     `dialout`) and its own home.
   - Put the box on an **isolated network segment / VLAN** that cannot reach
     your other machines or secrets.
   - Store no SSH keys, cloud credentials, or personal data on it.
   - Assume worst case = that one box is compromised; keep the blast radius there.

Residual risk remains (a compromised GitHub account could push to `main`; a
malicious dependency pulled during the job runs on the box). The isolation above
is what keeps that contained.

## Hardware: what box to use

The `on-target` job is deliberately **build-free** — it downloads the firmware
bundle already built in the cloud `package` job, flashes it, and runs the tests.
So the runner does **not** need to compile ESP-IDF; it only needs Python +
esptool + pytest-embedded + a USB port. That keeps the requirements modest.

| Box | Verdict |
|-----|---------|
| Raspberry Pi Zero (v1) | Too weak — single core, 512 MB; the Python/pytest-embedded stack will struggle. Avoid. |
| Raspberry Pi Zero 2 W | Marginal but workable for flash-and-test-only (no build). Tight on RAM. |
| Raspberry Pi 4 / 5 (2–4 GB) | Comfortable. Recommended if buying a Pi. |
| Old x86 mini-PC / laptop | Easiest overall — ESP-IDF tooling + Docker are first-class on x86. |

A cheap sacrificial x86 box or a Pi 4 is the sweet spot. VLAN-isolate it.

## Registering the runner

On the runner box (as the dedicated non-privileged user):

1. In GitHub: **Settings → Actions → Runners → New self-hosted runner**. Follow
   the download/configure commands it shows. When prompted for labels, **add the
   label for the board attached to this runner** — `esp32` for a Xtensa ESP32,
   `esp32c6` for the RISC-V C6 (the job targets `runs-on: [self-hosted,
   <target>]`). A runner with a given chip attached should carry exactly that
   chip's label. One box can host one board, or several boards on one box can
   each get their own runner + label.
2. Give the runner user serial access:
   ```sh
   sudo usermod -aG uucp "$USER"   # or 'dialout' on Debian/Ubuntu/Raspberry Pi OS
   # log out/in for the group to take effect
   ```
3. Install the test prerequisites (the job also pip-installs these, but esptool
   and Python must exist):
   ```sh
   python3 -m pip install --user esptool pytest-embedded \
     pytest-embedded-serial-esp pytest-embedded-idf
   ```
4. Find the board's serial port and set it as a repo variable. The port
   variable is **per target**: `ESP32_PORT` for the esp32 runner, `ESP32C6_PORT`
   for the esp32c6 runner (both default to `/dev/ttyUSB0` if unset):
   ```sh
   ls -l /dev/ttyUSB* /dev/ttyACM*   # identify the board (CP210x/CH340/native)
   ```
   Then in GitHub: **Settings → Secrets and variables → Actions → Variables →
   New variable**, name `ESP32_PORT` or `ESP32C6_PORT`, value e.g.
   `/dev/ttyUSB0`.
5. Start the runner (`./run.sh`, or install it as a service with
   `./svc.sh install && ./svc.sh start` under the dedicated user).

Once it is online and labeled with its chip (`esp32` or `esp32c6`), the next
push to `main` runs that target's on-target leg automatically.

## Known local-testing note

When developing the pytest-embedded tests on the dev machine, the correct
invocation is (from `test_target/`), substituting your chip for `<TARGET>`
(`esp32` or `esp32c6`):

```sh
. "$IDF_PATH/export.sh"
idf.py -C .. set-target <TARGET>
idf.py -C .. build
pytest --target <TARGET> --port <PORT> --app-path ..
```

## Known issue: pytest-embedded serial monitor captures nothing (flaky)

**Symptom.** `pytest` flashes the board successfully, then every `dut.expect(...)`
times out with an *empty* capture buffer (`Bytes in current buffer:` is blank),
even though the board boots and prints normally.

**What has been verified (2026-10):**

- The firmware is fine — a plain `pyserial` read of the same port shows the full
  boot banner and steady `seq=... raw=... smoothed=...` output on both the
  esp32c6 (`/dev/ttyACM1`, native USB-Serial/JTAG) and the Xtensa esp32.
- It is **not** local-environment-specific: it reproduces inside the pinned CI
  container (`espressif/idf:release-v5.5`, pytest-embedded 2.9.3) with the board
  passed through via `--device`. So the eventual self-hosted `on-target` job is
  expected to hit it too.
- It is **session-dependent and flaky**, not a clean config switch. On an
  identical setup (esp32c6, `--shm-size=1g`, pinned container):
  - a single test, run in isolation, passed (observed 4/4 at one point),
  - two tests passed once,
  - the full suite failed repeatedly with all per-test `dut.log` files empty —
    including the very test that had passed when run alone.

**Hypotheses tested and DISPROVEN (don't re-chase these):**

- *Local environment (Python 3.14 / IDF v6.1-dev).* No — reproduces in the
  pinned `espressif/idf:release-v5.5` container (Python 3.12, IDF v5.5.6).
- *Docker `/dev/shm` too small for the multiprocessing queue.* No — raising
  `--shm-size` to 1g did not fix it.
- *pytest output capture.* No — fails with and without `-s`.
- *Native-USB (C6) re-enumeration on reset.* No — also fails on the Xtensa
  esp32, which uses an external UART bridge that does not re-enumerate.
- *Per-test serial port close/reopen across function-scoped `dut` fixtures.*
  No — the test was refactored to a **single** test function (one flash, one
  monitor session) and it still fails 3/3.
- *Flash/reset race.* No — running with `--skip-autoflash y` (monitor an
  already-running board, exactly what the working manual `pyserial` read does)
  **still** captures nothing.

**Where that leaves the root cause (still OPEN).** With flashing, reset,
re-enumeration, shm, capture mode, and fixture scope all eliminated, the
remaining suspect is pytest-embedded's own serial *reader* (its background
multiprocessing reader → `msg_queue` → `pexpect`) not delivering bytes in this
environment: a plain `pyserial` read of the same port in the same container
gets data, but pytest-embedded's reader gets an empty buffer. This is inside
pytest-embedded, not in our test code or firmware. The one observed pass was
almost certainly a timing fluke, not a reproducible good config.

**Status / impact.** This does NOT block the multi-target work: both targets
build clean and run correctly on real hardware, verified directly by `pyserial`
reads (full boot banner + steady, in-order, correctly-smoothed `seq=` stream on
both the esp32 and esp32c6). It DOES block trusting the automated `on-target`
CI job. Recommended next steps (not yet attempted, outside this session's
scope):
  - try a newer/older `pytest-embedded` pin, or the versions ESP-IDF's own CI
    uses, to see if the reader behaves;
  - try without the Docker container (run the reader on a bare host) on the
    eventual self-hosted runner — the container's process/IPC model is a prime
    suspect and the real runner won't necessarily use Docker;
  - open an upstream issue against `espressif/pytest-embedded` with the
    "plain pyserial reads, pytest-embedded reader does not" reproducer;
  - as an interim gate, validate on-target with a plain `pyserial` smoke script
    instead of pytest-embedded.

For now, final on-target validation can be done by a plain `pyserial` read; the
pytest-embedded capture failure is tracked here with the evidence above.
