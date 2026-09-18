#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Packs PNGs into a Windows .ico.

Every entry is stored as PNG rather than a bottom-up DIB. Windows has decoded PNG icon
entries since Vista and GTK 4 requires Windows 10, so the DIB form buys no compatibility
and costs a great deal of space: the 16-64px entries alone were 34KB uncompressed against
10KB as PNG. That matters more here than for a typical icon, because the .ico is linked
into the executable as a resource rather than shipped alongside it.
"""

import struct
import sys

PNG_MAGIC = b"\x89PNG\r\n\x1a\n"


def read_size(raw, path):
    """Reads the pixel dimensions out of a PNG's IHDR, which is always the first chunk."""
    if raw[:8] != PNG_MAGIC or raw[12:16] != b"IHDR":
        raise ValueError(f"{path}: not a PNG")
    width, height = struct.unpack_from(">II", raw, 16)
    if not (1 <= width <= 256 and 1 <= height <= 256):
        raise ValueError(f"{path}: {width}x{height} is out of range for an icon entry")
    return width, height


def main(output, sources):
    images = []
    for path in sources:
        with open(path, "rb") as handle:
            payload = handle.read()
        width, height = read_size(payload, path)
        images.append((width, height, payload))

    offset = 6 + 16 * len(images)
    # ICONDIR: reserved, type 1 (icon), image count.
    directory = bytearray(struct.pack("<HHH", 0, 1, len(images)))
    for width, height, payload in images:
        # A 256px entry is encoded as 0 in the single-byte dimension fields.
        directory += struct.pack(
            "<BBBBHHII", width & 0xFF, height & 0xFF, 0, 0, 1, 32, len(payload), offset
        )
        offset += len(payload)

    with open(output, "wb") as handle:
        handle.write(directory)
        for _, _, payload in images:
            handle.write(payload)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2:])
