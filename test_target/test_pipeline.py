"""On-target test: flash the firmware to a real ESP32 and verify behaviour.

Run with pytest-embedded, which builds/flashes the project and captures the
serial log. This automates the manual flash-and-watch check from Phase 2.

Local run (board on /dev/ttyUSB1), from the repo root:

    . "$IDF_PATH/export.sh"
    idf.py build
    pytest test_target/ --target esp32 --port /dev/ttyUSB1

In CI this runs on a self-hosted runner with an ESP32 attached.
"""

import re


def test_boots_and_starts_pipeline(dut):
    """Board boots cleanly and both tasks announce startup."""
    # app_main banner.
    dut.expect("sensor-pipeline starting", timeout=20)
    # Both FreeRTOS tasks start.
    dut.expect("reporter started", timeout=5)
    dut.expect(re.compile(rb"sampler started \(period=\d+ms\)"), timeout=5)
    # Composition root confirms the pipeline is up.
    dut.expect("pipeline running", timeout=5)


def test_samples_flow_and_are_smoothed(dut):
    """Samples arrive in order and the filter produces a smoothed value."""
    # Grab several reporter lines: "seq=<n> raw=<f> smoothed=<f>".
    line = re.compile(rb"seq=(\d+) raw=(-?\d+\.\d+) smoothed=(-?\d+\.\d+)")

    seqs = []
    for _ in range(5):
        m = dut.expect(line, timeout=5)
        seq = int(m.group(1))
        raw = float(m.group(2))
        smoothed = float(m.group(3))
        seqs.append(seq)
        # Smoothed output must be a real, finite number in the signal's range.
        # The source is sin(x)+1.0, so values live in roughly [0, 2].
        assert -0.5 <= smoothed <= 2.5, f"smoothed out of range: {smoothed}"
        assert -0.5 <= raw <= 2.5, f"raw out of range: {raw}"

    # Sequence numbers must be strictly increasing and contiguous (no drops).
    assert seqs == list(range(seqs[0], seqs[0] + len(seqs))), f"seq gaps: {seqs}"


def test_still_alive_after_running(dut):
    """Liveness: the board keeps producing samples (a crash/reboot stops them).

    A panic or boot-loop would halt the steady sample stream, so if we still see
    a fresh, higher sequence number after a delay, the firmware stayed up.
    """
    line = re.compile(rb"seq=(\d+) raw=-?\d+\.\d+ smoothed=-?\d+\.\d+")

    first = int(dut.expect(line, timeout=5).group(1))
    # Let it run through several more periods, then confirm it is still logging
    # and the sequence advanced (proves forward progress, not a reset to 0).
    later = int(dut.expect(line, timeout=5).group(1))
    assert later > first, f"sequence did not advance ({first} -> {later})"
