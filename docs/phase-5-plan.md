# Phase 5 plan — security capstone (secure boot v2 + flash encryption)

> **STATUS: PLAN ONLY. Do not execute on a board you care about.**
> This phase burns eFuses, which is **irreversible** and can **permanently
> brick** a board. It is deferred until a sacrificial ESP32 and unhurried time
> are available. Read the Hazards section in full before doing anything.

## Goal

Prove the device rejects unauthorized firmware:
1. **Secure Boot v2** — the ROM verifies the bootloader's signature, and the
   bootloader verifies each app's signature, using an **RSA-3072** public key
   whose digest is burned into eFuses. Unsigned/wrongly-signed images are
   refused at boot.
2. **Flash Encryption** — flash contents are AES-encrypted with a key in eFuse,
   so a flash dump is unreadable.
3. **CI signing** — the firmware is signed in the `release` job with a private
   key stored as a **GitHub Actions secret**, never in the repo.

**Done = the board boots our signed image, and refuses a deliberately
unsigned/modified image.**

## ⚠️ Hazards (read before executing)

- **eFuse burning is one-way.** Secure Boot and Flash Encryption eFuses cannot
  be un-set. A mistake can brick the board with no recovery.
- **Use development mode, not release mode.** Dev mode keeps re-flashing and
  debugging possible so you can iterate; release mode locks the board down and
  is for production only. This project is strictly dev mode.
- **Uninterrupted power.** A power failure *during* eFuse burning can
  permanently brick the chip. Use a stable supply; do not do this on a flaky
  USB cable.
- **Sacrificial board only.** Use one of the spare ESP32s you can afford to
  lose. Never the daily-dev board.
- **Order matters** when enabling both: **Flash Encryption first, then Secure
  Boot v2** (SB v2 needs its key eFuse left readable; doing FE after could
  conflict with that). Follow the ESP-IDF "security features enablement
  workflow" for the installed IDF version exactly.
- **Keys are forever.** The signing key's public digest is burned in; lose the
  private key and you can never sign an update that board will accept.

## Key management

- Generate the Secure Boot signing key on a host:
  `espsecure.py generate_signing_key --version 2 secure_boot_signing_key.pem`
  (RSA-3072 for ESP32).
- **The private key never enters the repo.** `.gitignore` already blocks
  `*.pem`/`*.key` as a backstop. Store it in a password manager / offline.
- For CI, add the key as a GitHub Actions **secret** (e.g. `SECURE_BOOT_KEY`,
  base64-encoded). The `release` job decodes it to a temp file, signs, then
  deletes it. Scope: only the release job needs it.
- The flash-encryption key for dev mode can be device-generated; keep a copy if
  you want to decrypt dumps during development.

## Implementation outline (execute later, in order)

1. **Pre-flight (no eFuses yet):** on the sacrificial board, confirm normal
   build/flash/boot works. Record the chip + MAC for your notes.
2. **Signing locally first (no eFuses):** generate the key; enable Secure Boot
   v2 in `sdkconfig` in a way that *builds a signed image* but **do not burn
   eFuses yet**. Verify the signed image boots on a normal board. This
   de-risks the toolchain before touching anything irreversible.
3. **Flash Encryption (dev mode):** enable via `menuconfig` (Security features
   → Enable flash encryption on boot → Development). Flash; the first boot
   encrypts. Verify the app still runs and a flash dump is unreadable.
4. **Secure Boot v2 (dev mode):** enable in `menuconfig`; flash the signed
   bootloader + app; on first boot the key digest burns. Verify boot.
5. **The money shot:** build a deliberately **unsigned** (or wrong-key) app and
   confirm the board **refuses to boot it**. That is the pass condition.
6. **CI signing:** add the key as a secret; have the `release` job sign the app
   (and bootloader) during packaging; record signing status in `manifest.json`.

## How it plugs into what already exists

- The `release` job (Phase 3.5) is where CI signing slots in — sign before
  `tools/package.sh` bundles, so released artifacts are signed.
- `manifest.json` should gain a `signed: true/false` (and key id) field.
- `docs/adr/` should get an ADR recording the secure-boot decisions (RSA-3072,
  dev mode, key-in-CI-secret) when this is executed.

## References

- ESP-IDF Secure Boot v2 (per installed IDF version).
- ESP-IDF Flash Encryption (development mode section).
- ESP-IDF "Security Features Enablement Workflow" — the ordering + eFuse steps.
