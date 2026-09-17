#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
command -v pio >/dev/null || { echo "PlatformIO Core fehlt: https://platformio.org/install/cli" >&2; exit 1; }
command -v zip >/dev/null || { echo "zip fehlt." >&2; exit 1; }

environment=seeed_xiao_esp32s3
build_dir=".pio/build/$environment"
rm -rf dist
mkdir -p dist/installer

pio run -e "$environment"
pio run -e "$environment" -t buildfs

cp "$build_dir/bootloader.bin" dist/installer/
cp "$build_dir/partitions.bin" dist/installer/
cp "$build_dir/firmware.bin" dist/installer/
cp "$build_dir/littlefs.bin" dist/installer/
if [[ -f "$build_dir/boot_app0.bin" ]]; then
  cp "$build_dir/boot_app0.bin" dist/installer/
elif [[ -f "$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin" ]]; then
  cp "$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin" dist/installer/
else
  echo "boot_app0.bin wurde nicht gefunden." >&2
  exit 1
fi

cp web-installer/index.html web-installer/manifest.json dist/installer/
(cd browser-extension && zip -qr ../dist/redbutton-extension.zip . -x '*.pyc' -x '__pycache__/*')
tar -czf dist/redbutton-linux.tar.gz linux scripts/install-linux.sh
(cd dist && sha256sum installer/*.bin redbutton-extension.zip redbutton-linux.tar.gz > SHA256SUMS)

echo "Fertig. Öffne dist/installer/index.html über einen lokalen HTTP-Server:"
echo "  python3 -m http.server 8000 --directory dist"
echo "Danach: http://localhost:8000/installer/"
