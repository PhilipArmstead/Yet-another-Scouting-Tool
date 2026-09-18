#!/usr/bin/env bash
# SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Regenerates every platform icon artefact from the master SVG.
#
# Requires: rsvg-convert (librsvg), python3. `iconutil` (macOS only) is used for the .icns;
# on other hosts the existing .icns is left untouched.

set -euo pipefail

cd "$(dirname "$0")"

MASTER="yast.svg"
SIZES=(16 24 32 48 64 128 256 512 1024)

command -v rsvg-convert >/dev/null || { echo "rsvg-convert not found" >&2; exit 1; }

rm -rf png
mkdir -p png

for size in "${SIZES[@]}"; do
	rsvg-convert -w "${size}" -h "${size}" "${MASTER}" -o "png/${size}.png"
done

# Windows: every entry stored as PNG, which the shell has decoded since Vista.
python3 make-ico.py yast.ico png/16.png png/24.png png/32.png png/48.png png/64.png png/128.png png/256.png

# macOS
if command -v iconutil >/dev/null; then
	rm -rf yast.iconset
	mkdir yast.iconset
	cp png/16.png   yast.iconset/icon_16x16.png
	cp png/32.png   yast.iconset/icon_16x16@2x.png
	cp png/32.png   yast.iconset/icon_32x32.png
	cp png/64.png   yast.iconset/icon_32x32@2x.png
	cp png/128.png  yast.iconset/icon_128x128.png
	cp png/256.png  yast.iconset/icon_128x128@2x.png
	cp png/256.png  yast.iconset/icon_256x256.png
	cp png/512.png  yast.iconset/icon_256x256@2x.png
	cp png/512.png  yast.iconset/icon_512x512.png
	cp png/1024.png yast.iconset/icon_512x512@2x.png
	iconutil --convert icns --output yast.icns yast.iconset
	rm -rf yast.iconset
else
	echo "iconutil unavailable; leaving yast.icns unchanged" >&2
fi

echo "Icons regenerated."
