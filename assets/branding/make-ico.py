#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Packs PNGs into a Windows .ico.

Entries of 64px and under are stored as 32-bit bottom-up DIBs, which every Windows shell
surface can decode; larger entries are stored as PNG, which is required for the 256px entry
and saves a substantial amount of space.
"""

import struct
import sys
import zlib

DIB_THRESHOLD = 64


def decode_png(path):
    raw = open(path, "rb").read()
    pos, idat, width, height = 8, bytearray(), 0, 0
    while pos < len(raw):
        length = struct.unpack(">I", raw[pos:pos + 4])[0]
        kind = raw[pos + 4:pos + 8]
        data = raw[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            width, height, depth, colour = struct.unpack(">IIBB", data[:10])
            if depth != 8 or colour != 6:
                raise ValueError(f"{path}: expected 8-bit RGBA")
        elif kind == b"IDAT":
            idat += data
        pos += 12 + length

    stream = zlib.decompress(bytes(idat))
    stride = width * 4
    rows, prev, offset = [], bytearray(stride), 0
    for _ in range(height):
        filt = stream[offset]
        offset += 1
        line = bytearray(stream[offset:offset + stride])
        offset += stride
        if filt:
            for i in range(stride):
                a = line[i - 4] if i >= 4 else 0
                b = prev[i]
                c = prev[i - 4] if i >= 4 else 0
                if filt == 1:
                    line[i] = (line[i] + a) & 0xFF
                elif filt == 2:
                    line[i] = (line[i] + b) & 0xFF
                elif filt == 3:
                    line[i] = (line[i] + ((a + b) >> 1)) & 0xFF
                else:
                    pp = a + b - c
                    pa, pb, pc = abs(pp - a), abs(pp - b), abs(pp - c)
                    pred = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                    line[i] = (line[i] + pred) & 0xFF
        rows.append(bytes(line))
        prev = line

    return width, height, rows


def to_dib(width, height, rows):
    header = struct.pack("<IiiHHIIiiII", 40, width, height * 2, 1, 32, 0, 0, 0, 0, 0, 0)
    pixels = bytearray()
    for row in reversed(rows):
        for x in range(0, len(row), 4):
            r, g, b, a = row[x:x + 4]
            pixels += bytes((b, g, r, a))
    mask = bytes(((width + 31) // 32) * 4 * height)
    return header + bytes(pixels) + mask


def main(output, sources):
    images = []
    for path in sources:
        width, height, rows = decode_png(path)
        payload = open(path, "rb").read() if width > DIB_THRESHOLD else to_dib(width, height, rows)
        images.append((width, height, payload))

    offset = 6 + 16 * len(images)
    directory = bytearray(struct.pack("<HHH", 0, 1, len(images)))
    for width, height, payload in images:
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
