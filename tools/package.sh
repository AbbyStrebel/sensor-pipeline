#!/usr/bin/env bash
# Package a complete, flashable, verifiable firmware bundle from a build/.
#
# Produces dist/<name>/ containing:
#   - bootloader.bin, partition-table.bin, <app>.bin  (the flashable set)
#   - flash_args, flasher_args.json                   (offsets + flash settings)
#   - <app>.elf, <app>.map                            (debug symbols / memory map)
#   - SHA256SUMS                                       (integrity)
#   - manifest.json                                   (build metadata / traceability)
#   - flash.sh                                        (one-command flash helper)
# ...and a dist/<name>.tar.gz of that directory.
#
# Usage: tools/package.sh [output_dir]
#   Requires a prior `idf.py build`. Default output_dir is ./dist.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build"
OUT_BASE="${1:-$ROOT/dist}"

FA_JSON="$BUILD/flasher_args.json"
if [ ! -f "$FA_JSON" ]; then
  echo "error: $FA_JSON not found. Run 'idf.py build' first." >&2
  exit 1
fi

# Derive names/metadata from the build.
APP_BIN_REL="$(python3 -c "import json;print(json.load(open('$FA_JSON'))['app']['file'])")"
APP_NAME="$(basename "${APP_BIN_REL%.bin}")"             # e.g. sensor_pipeline
TARGET="$(python3 -c "import json;print(json.load(open('$FA_JSON')).get('extra_esptool_args',{}).get('chip','esp32'))")"

GIT_COMMIT="$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || echo unknown)"
GIT_REF="$(git -C "$ROOT" describe --tags --always --dirty 2>/dev/null || echo unknown)"
IDF_VERSION="$(idf.py --version 2>/dev/null | head -1 || echo unknown)"
BUILD_TIME="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

OUT="$OUT_BASE/${APP_NAME}-${GIT_REF}"
rm -rf "$OUT"
mkdir -p "$OUT"

# --- Copy the flashable set (flattened filenames) + offsets/settings files ---
cp "$BUILD/bootloader/bootloader.bin"                 "$OUT/bootloader.bin"
cp "$BUILD/partition_table/partition-table.bin"       "$OUT/partition-table.bin"
cp "$BUILD/${APP_NAME}.bin"                            "$OUT/${APP_NAME}.bin"
cp "$FA_JSON"                                          "$OUT/flasher_args.json"

# Generate a bundle-local flash_args using FLATTENED filenames (the build's
# flash_args references subdirs like bootloader/bootloader.bin, but we flatten
# everything into the bundle root, so rewrite the paths to match).
python3 - "$FA_JSON" "$APP_NAME" > "$OUT/flash_args" <<'PY'
import json, sys
fa, app = sys.argv[1], sys.argv[2]
d = json.load(open(fa))
s = d["flash_settings"]
# First line: flash settings, mirroring IDF's flash_args format.
print(f"--flash-mode {s['flash_mode']} --flash-freq {s['flash_freq']} --flash-size {s['flash_size']}")
# Map each original (possibly nested) file to its flattened basename.
import os
for offset, path in d["flash_files"].items():
    print(f"{offset} {os.path.basename(path)}")
PY
# --- Debug artifacts ---
cp "$BUILD/${APP_NAME}.elf"                            "$OUT/${APP_NAME}.elf"
cp "$BUILD/${APP_NAME}.map"                            "$OUT/${APP_NAME}.map"

# --- Checksums (over everything copied so far) ---
( cd "$OUT" && sha256sum ./* > SHA256SUMS )

# --- manifest.json (build metadata + per-file sizes/sha) ---
python3 - "$OUT" "$APP_NAME" "$TARGET" "$GIT_COMMIT" "$GIT_REF" "$IDF_VERSION" "$BUILD_TIME" <<'PY'
import json, os, sys, hashlib
out, app, target, commit, ref, idf, ts = sys.argv[1:8]
def sha256(p):
    h = hashlib.sha256()
    with open(p, 'rb') as f:
        for chunk in iter(lambda: f.read(8192), b''):
            h.update(chunk)
    return h.hexdigest()
files = {}
for name in sorted(os.listdir(out)):
    p = os.path.join(out, name)
    if os.path.isfile(p) and name not in ('manifest.json',):
        files[name] = {"size": os.path.getsize(p), "sha256": sha256(p)}
manifest = {
    "project": app,
    "target": target,
    "git_commit": commit,
    "git_ref": ref,
    "idf_version": idf,
    "build_time_utc": ts,
    "files": files,
}
with open(os.path.join(out, "manifest.json"), "w") as f:
    json.dump(manifest, f, indent=2)
    f.write("\n")
print("wrote manifest.json with", len(files), "files")
PY

# --- flash.sh: one-command flash helper (reads flash_args) ---
cat > "$OUT/flash.sh" <<EOF
#!/usr/bin/env bash
# Flash this bundle to an ESP32. Usage: ./flash.sh [PORT]
# Requires esptool (pip install esptool, or from ESP-IDF).
set -euo pipefail
PORT="\${1:-/dev/ttyUSB0}"
cd "\$(dirname "\$0")"
echo "Flashing ${APP_NAME} (${GIT_REF}) to \$PORT ..."
exec esptool --chip ${TARGET} -p "\$PORT" -b 460800 \\
  --before default-reset --after hard-reset write-flash \\
  @flash_args
EOF
chmod +x "$OUT/flash.sh"

# --- Tarball ---
TARBALL="$OUT_BASE/${APP_NAME}-${GIT_REF}.tar.gz"
tar -C "$OUT_BASE" -czf "$TARBALL" "$(basename "$OUT")"

echo
echo "Bundle:  $OUT"
echo "Tarball: $TARBALL"
echo "Contents:"
ls -1 "$OUT" | sed 's/^/  /'
