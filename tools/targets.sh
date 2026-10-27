#!/usr/bin/env bash
# Single source of truth for the chips this project builds for.
#
# Everything that iterates over targets (CI matrix, local helper scripts)
# should read this list instead of hardcoding chip names. Adding a new
# variant (e.g. esp32s3, esp32c3, esp32p4) is a one-line edit here.
#
# Usage:
#   source tools/targets.sh        # exposes $SUPPORTED_TARGETS
#   tools/targets.sh --json        # prints the list as a JSON array
#                                  # (handy for a GitHub Actions matrix)
#
# Each name must be a value accepted by `idf.py set-target <name>`.

# Space-separated list of supported ESP32 variants.
#   esp32    — Xtensa LX6, dual-core (original)
#   esp32c6  — RISC-V, single-core (+ LP core), Wi-Fi 6 / BLE / 802.15.4
SUPPORTED_TARGETS="esp32 esp32c6"

# When executed directly (not sourced), support a couple of output formats.
if [ "${BASH_SOURCE[0]}" = "${0}" ]; then
  case "${1:-}" in
    --json)
      # Emit ["esp32","esp32c6"] for use as a CI matrix value.
      printf '['
      first=1
      for t in $SUPPORTED_TARGETS; do
        [ $first -eq 1 ] || printf ','
        printf '"%s"' "$t"
        first=0
      done
      printf ']\n'
      ;;
    ""|--list)
      # One target per line.
      for t in $SUPPORTED_TARGETS; do echo "$t"; done
      ;;
    *)
      echo "usage: $0 [--list|--json]" >&2
      exit 2
      ;;
  esac
fi
