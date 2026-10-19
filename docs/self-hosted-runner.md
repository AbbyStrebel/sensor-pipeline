# Self-hosted runner (on-target testing)

The `on-target` CI job flashes a real ESP32 and runs the pytest-embedded tests
against it (see [`test_target/`](../test_target/)). This requires a
**self-hosted GitHub Actions runner** on a machine with an ESP32 attached,
because GitHub's cloud runners have no USB-connected hardware.

> **Status:** the workflow job exists and is guarded, but there is no runner
> registered yet. Until a runner labeled `esp32` comes online, the `on-target`
> job simply stays queued/does not run. Nothing else in CI is affected.

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
   label `esp32`** (the job targets `runs-on: [self-hosted, esp32]`).
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
4. Find the ESP32's serial port and set it as a repo variable:
   ```sh
   ls -l /dev/ttyUSB* /dev/ttyACM*   # identify the ESP32 (CP210x/CH340/native)
   ```
   Then in GitHub: **Settings → Secrets and variables → Actions → Variables →
   New variable**, name `ESP32_PORT`, value e.g. `/dev/ttyUSB0`. (The job
   defaults to `/dev/ttyUSB0` if unset.)
5. Start the runner (`./run.sh`, or install it as a service with
   `./svc.sh install && ./svc.sh start` under the dedicated user).

Once it is online and labeled `esp32`, the next push to `main` runs the
on-target job automatically.

## Known local-testing note

When developing the pytest-embedded tests on the dev machine, the correct
invocation is (from `test_target/`):

```sh
. "$IDF_PATH/export.sh"
idf.py -C .. build
pytest --target esp32 --port <PORT> --app-path ..
```

On some host setups pytest-embedded's post-flash serial monitor may not capture
output even though flashing succeeds and the board prints normally (verifiable
with a plain `pyserial` read). This is environment-specific (serial driver /
esptool / pytest-embedded version interaction). The firmware and test logic are
independent of it; final on-target validation happens on the runner.
