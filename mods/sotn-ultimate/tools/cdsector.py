#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
# Raw 2352-byte CD sector helpers for build_ko_trial.sh's last step (JP Konami
# movie, 시험48): EDC/ECC of Mode 2 sectors and a range reader for .bin.ecm
# images (Neill Corlett's ECM format), so the Japanese disc's LOGO15XA.STR
# sectors can be read without unpacking the whole image.
import struct

_F = [0] * 256
_B = [0] * 256
_EDC = [0] * 256
for _i in range(256):
    _j = (_i << 1) ^ (0x11D if _i & 0x80 else 0)
    _F[_i] = _j & 0xFF
    _B[_i ^ _j & 0xFF] = _i
    _e = _i
    for _ in range(8):
        _e = (_e >> 1) ^ (0xD8018001 if _e & 1 else 0)
    _EDC[_i] = _e


def edc(data):
    e = 0
    for b in data:
        e = (e >> 8) ^ _EDC[(e ^ b) & 0xFF]
    return e


def _ecc(src, major_count, minor_count, major_mult, minor_inc):
    size = major_count * minor_count
    dest = bytearray(major_count * 2)
    for major in range(major_count):
        index = (major >> 1) * major_mult + (major & 1)
        a = b = 0
        for _ in range(minor_count):
            t = src[index]
            index += minor_inc
            if index >= size:
                index -= size
            a ^= t
            b ^= t
            a = _F[a]
        a = _B[_F[a] ^ b]
        dest[major] = a
        dest[major + major_count] = a ^ b
    return dest


def fix_form1(sec):
    """rewrite EDC + ECC of a Mode 2 Form 1 sector in place (bytearray, 2352)"""
    sec[0x818:0x81C] = struct.pack("<I", edc(sec[0x10:0x818]))
    hdr = bytes(sec[12:16])
    sec[12:16] = b"\0\0\0\0"  # Mode 2: the address is not covered
    src = bytes(sec[0xC:0x81C])
    sec[0x81C:0x81C + 172] = _ecc(src, 86, 24, 2, 86)
    src = bytes(sec[0xC:0x81C + 172])
    sec[0x8C8:0x8C8 + 104] = _ecc(src, 52, 43, 86, 88)
    sec[12:16] = hdr


def fix_form2(sec):
    sec[0x92C:0x930] = struct.pack("<I", edc(sec[0x10:0x92C]))


def set_msf(sec, lba):
    pos = lba + 150
    bcd = lambda n: ((n // 10) << 4) | (n % 10)
    sec[12:15] = bytes((bcd(pos // 4500), bcd(pos // 75 % 60), bcd(pos % 75)))


SYNC = b"\x00" + b"\xff" * 10 + b"\x00"


def ecm_sectors(path, first, count):
    """sectors [first, first+count) of the image inside a .bin.ecm, as
    2352-byte bytearrays (Mode 2 addresses are rebuilt from the position)"""
    buf = bytearray(count * 2352)
    lo, hi = first * 2352, (first + count) * 2352
    pos = 0  # output bytes so far
    with open(path, "rb") as f:
        assert f.read(4) == b"ECM\0"
        while pos < hi:
            c = f.read(1)[0]
            typ, num, shift = c & 3, (c >> 2) & 0x1F, 5
            while c & 0x80:
                c = f.read(1)[0]
                num |= (c & 0x7F) << shift
                shift += 7
            if num == 0xFFFFFFFF:
                break
            num += 1
            if typ == 0:
                # raw bytes (sectors that do not fit the EDC/ECC types)
                a, b = max(pos, lo), min(pos + num, hi)
                if a < b:
                    f.seek(a - pos, 1)
                    buf[a - lo:b - lo] = f.read(b - a)
                    f.seek(pos + num - b, 1)
                else:
                    f.seek(num, 1)
                pos += num
                continue
            stored = {1: 2051, 2: 2052, 3: 2328}[typ]
            for _ in range(num):
                lba = pos // 2352
                pos += 2352
                if lba < first or lba >= first + count:
                    f.seek(stored, 1)
                    continue
                if typ == 1:
                    raise SystemExit("mode 1 sector %d inside the range" % lba)
                s = bytearray(2352)
                s[0:12] = SYNC
                s[15] = 2
                s[0x14:0x14 + stored] = f.read(stored)
                s[0x10:0x14] = s[0x14:0x18]
                if typ == 2:
                    fix_form1(s)
                else:
                    fix_form2(s)
                set_msf(s, lba)
                buf[(lba - first) * 2352:(lba - first + 1) * 2352] = s
    return [buf[i * 2352:(i + 1) * 2352] for i in range(count)]
