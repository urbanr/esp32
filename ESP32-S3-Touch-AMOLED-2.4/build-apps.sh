#!/bin/sh
# Prelozi vsechny aplikace pro desku 2.41 a slozi binarky do adresare,
# jehoz obsah staci nakopirovat na SD kartu do /apps.
#   ./build-apps.sh [vystupni-adresar]      (vychozi: ./sd-apps)
set -e

FQBN="esp32:esp32:waveshare_esp32_s3_touch_amoled_241:CDCOnBoot=cdc,UploadSpeed=115200"
DIR="$(cd "$(dirname "$0")" && pwd)"
OUT="${1:-$DIR/sd-apps}"

# sketch:nazev-na-displeji
APPS="esp32-amoled-sand:pisek
esp32-amoled-starfield:hvezdy
esp32-amoled-bubble-level:vodovaha
esp32-amoled-ship-navigator:lod
esp32-rat-jumper:krysa-skokan
esp32-rat-zombies:krysy-a-zombici"

mkdir -p "$OUT"
for a in $APPS; do
  sketch="${a%%:*}"
  name="${a##*:}"
  echo "=== $sketch -> $name.bin"
  tmp="$(mktemp -d)"
  arduino-cli compile -b "$FQBN" --output-dir "$tmp" "$DIR/$sketch" > /dev/null
  cp "$tmp/$sketch.ino.bin" "$OUT/$name.bin"
  rm -rf "$tmp"
done

echo
echo "hotovo, binarky jsou v $OUT"
ls -la "$OUT"
