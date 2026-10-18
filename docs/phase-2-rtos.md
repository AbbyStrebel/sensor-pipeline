# Phase 2 — RTOS wiring (learning note)

> This doubles as the pull-request description for `feat/phase2-rtos` and as a
> durable explanation of what Phase 2 did and why. Kept in the repo so it
> survives even if the branch is deleted.

## The goal

Phases 0 and 1 built the scaffolding — component layout, a host-tested filter,
and CI. None of it was actually *FreeRTOS* yet. Phase 2 is the first real RTOS
program: multiple tasks running concurrently, communicating safely, and sharing
state.

## What we built

A two-task pipeline:

```
sampler task (priority 5)        QUEUE (8 slots)       reporter task (priority 4)
─────────────────────────        ─────────────        ──────────────────────────
every 100ms:                                           forever:
  generate a sample          ──► xQueueSend  ──►         xQueueReceive  (BLOCKS)
  (seq number + value)                                   read config (MUTEX)
  sleep until next tick                                  run the filter
                                                         log raw + smoothed
                               shared config {window} ◄── guarded by a MUTEX
```

Files:
- `components/contracts/include/sample.h` — the message type (`sample_t`: a
  sequence number + a value) that travels through the queue.
- `components/sampler/sampler.c` — the **producer** task. Wakes every 100ms,
  makes a (faked) reading, sends it to the queue.
- `components/reporter/reporter.c` — the **consumer** task. Blocks waiting for
  samples, smooths each with the filter, logs the result.
- `components/app_core/app_core.c` — the **composition root**. Creates the queue
  and the config mutex, then starts both tasks.

## Why each decision

**Two separate tasks instead of one loop.** The core RTOS lesson. A single
`while(1)` would force us to interleave "read" and "process/print" by hand, and
any slow step would stall everything. With two tasks, the scheduler handles
interleaving: the sampler sleeps while the reporter works, and vice versa. Each
task does one job.

**A queue to connect them.** Two tasks cannot safely share a plain variable —
one could read it mid-write. A queue is the RTOS-blessed hand-off: thread-safe
by construction, and it *copies* the data so there is no shared-memory hazard.
It also lets the consumer **block** — sleep until data arrives — which is the
most important RTOS habit: *don't poll, block*. A blocked task uses zero CPU.

**A mutex for the config.** The queue handles flowing data; the `window` setting
is *shared mutable state* that a future feature (runtime window changes) could
touch. Shared state needs a lock so no task sees a half-updated value. We used a
FreeRTOS mutex specifically for its **priority inheritance**, which prevents
priority inversion. It is slightly over-built for today's code (nothing changes
the window yet) — it is there to learn the pattern and to make runtime changes
safe later.

**Different priorities (sampler 5, reporter 4).** The scheduler always runs the
highest-priority ready task. The sampler is time-critical (must tick every
100ms), so it is higher; the reporter does its work in the gaps while the
sampler sleeps. Priority design in miniature: the deadline-sensitive task wins.

**`vTaskDelayUntil` instead of `vTaskDelay`.** `vTaskDelay(100ms)` sleeps 100ms
*after* the work finishes, so the period drifts by however long the work took.
`vTaskDelayUntil` targets fixed time boundaries for a steady cadence. The
hardware output confirmed it: log timestamps landed ~100ms apart.

**The filter stays in `processor`, not in the task.** The discipline from
[ADR-0001](adr/0001-split-logic-from-hardware.md): tasks are thin RTOS plumbing;
the math lives in `processor`, which has no hardware dependencies and is
host-tested at 100% coverage. The exact same filter code we unit-tested runs on
the chip.

**The new `contracts` component.** `sample.h` is shared by both the sampler and
the reporter. Putting it inside either task's component (or `app_core`) would
create a dependency cycle, which the build rejects. A tiny dependency-free
component just for shared types is the standard fix: shared contracts live in a
neutral leaf everyone can depend on.

## Verified

- **Firmware builds** with `-Werror` (86% app partition free).
- **Host tests** still 5/5, `processor` coverage still 100% (unchanged).
- **On real hardware** (ESP32, CP210x @ `/dev/ttyUSB1`): tasks start in order;
  `seq` increments with no gaps and no dropped-sample warnings; log timestamps
  are a steady ~100ms apart; `smoothed` visibly lags/dampens the `raw` sine
  wave — a textbook moving-average response from the same code unit-tested on
  the host.

## Scope / what is intentionally not done

- The "sensor" is a faked sine wave (keeps focus on RTOS primitives, not a
  driver).
- The mutex is not under real contention yet (no task changes `window`).
- Output is logs, not UART-to-a-peer or network.
- On-target behavior was checked by a manual flash; automating it is Phase 4
  (pytest-embedded on a self-hosted runner).

## One sentence

Phase 2 turned the skeleton into an actual concurrent RTOS program — two tasks,
a queue, and a mutex — while keeping all real logic in the host-tested pure
component, so the FreeRTOS primitives (tasks, queue, mutex, priorities, timing)
are exercised hands-on without sacrificing testability.
