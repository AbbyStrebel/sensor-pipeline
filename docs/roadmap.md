# Roadmap & deferred work

A living snapshot of what's done, what's paused and why, and the exact steps to
resume. The phase checklist lives in the [README](../README.md); this file
tracks the *blocked* and *planned* items in enough detail to pick up cold.

## Status at a glance

| Phase | State |
|-------|-------|
| 0 Skeleton | ✅ done |
| 1 Host tests in CI | ✅ done |
| 2 RTOS wiring | ✅ done (verified on real board) |
| 3 Lint gate | ✅ done |
| 3.5 CD / releases | ✅ done (v0.1.0 published; Pages live) |
| 4 On-target CI | 🟡 code + docs done; **blocked on runner hardware** |
| 4.5 RTOS deepening | 📋 planned (no hardware blocker) |
| 5 Security capstone | ⛔ **deferred, blocked on sacrificial ESP32** |

## Deferred — hardware-blocked

### Phase 4 completion — self-hosted runner

**What's done:** the `on-target` CI job, the `test_target/` pytest-embedded
tests, and [`self-hosted-runner.md`](self-hosted-runner.md). The job is guarded
(push-to-main of this repo only) and build-free (flashes the prebuilt bundle),
so it just sits idle until a runner labeled `esp32` registers.

**Why blocked:** needs a sacrificial, network-isolated box with an ESP32
attached. The main home server was ruled out because this is a *public* repo and
a self-hosted runner there is an unacceptable exposure (a malicious fork PR could
run code on it). VLANs need sorting first.

**Options when revisiting (the server dream isn't dead):**
- Cheap x86 mini-PC or Pi 4/5 as a dedicated sacrificial runner (recommended).
- A **VLAN-isolated VM/container on the existing server** with the ESP32 passed
  through — "my server, sort of" without risking the whole host.
- Making the **repo private** largely removes the fork-PR risk, which could make
  even the main server defensible (trade-off: loses public-portfolio value).

**Unblock checklist:**
- [ ] Acquire runner box (x86 mini-PC / Pi 4/5; Pi Zero 2 W only if desperate).
- [ ] Put it on an isolated VLAN; create a dedicated non-privileged user.
- [ ] Attach an ESP32; note its serial port (`ls -l /dev/ttyUSB* /dev/ttyACM*`).
- [ ] GitHub → Settings → Actions → require approval for outside collaborators.
- [ ] Register the runner with the **`esp32`** label (see self-hosted-runner.md).
- [ ] Add repo variable `ESP32_PORT` if not `/dev/ttyUSB0`.
- [ ] Push to `main`; confirm the `on-target` job runs and passes.
- [ ] Revisit the dev-host pytest-embedded serial-capture quirk if it recurs on
      the runner (flash worked; monitor read 0 bytes — may be host-specific).

### Phase 5 — security capstone

**Why blocked:** eFuse burning (Secure Boot v2 / flash encryption) is
**irreversible** and can brick a board. It must be done on a board you can
afford to lock. One of the three on-hand ESP32s can be the sacrificial unit, but
this is deferred until there's bandwidth to do it carefully.

**Plan:** fully written in [`phase-5-plan.md`](phase-5-plan.md) so it can be
executed later without re-deriving the design. **Do not start without reading
the hazards section there.**

## Planned — not hardware-blocked

### Phase 4.5 — RTOS deepening (optional, dev-board only)

Runs on an ESP32 you flash manually (no self-hosted runner needed). Deepens the
FreeRTOS learning that is the point of the project. Candidate exercises, roughly
increasing in difficulty:

- **Second processing stage:** split filtering into its own task, so the flow is
  sampler → queue1 → processor task → queue2 → reporter. Exercises a two-queue
  pipeline and more task coordination.
- **Task notifications:** replace a queue hop with a lightweight FreeRTOS task
  notification where appropriate; compare the two mechanisms.
- **Runtime config under contention:** add a console command (or a second task)
  that changes `reporter_config_t.window` at runtime, so the mutex is actually
  contended — the first time the Phase 2 mutex does real work.
- **Stack/heap introspection:** log task high-water marks; right-size the task
  stacks from data instead of the current 2048-word guess.

Each remains host-testable where logic is involved (keep it in `processor` per
[ADR-0001](adr/0001-split-logic-from-hardware.md)) and verifiable by manual
flash.

## Open follow-ups (done, just confirm periodically)

- Pages site: https://abbystrebel.github.io/sensor-pipeline/ (live).
- Release automation: `v0.1.0` published successfully via the tag-triggered job.
