#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Last build step (trial48): the Japanese Konami logo movie.

Appends the Japanese disc's LOGO15XA.STR (1,680 sectors, Track 1 LBA 23106..)
verbatim to the end of the image and points the US image at it:
  * PVD volume size          (sector 16)
  * root dir record LOGO15XA.STR;1 -> new LBA / size   (sector 22)
  * SEL.BIN and SELRIC2.BIN stream table at file offset 0xB2CC:
      cdOffset -> new LBA, frameCount 143 -> 168
Mode 2 form 1 sectors that change get EDC/ECC rebuilt.  Every expected old
value is asserted, so a drifted image fails loudly instead of being patched.

usage: jp_movie.py IMAGE.bin JP_TRACK1.bin|.ecm
(an .ecm is decoded with tools/unecm.c, compiled on the fly)
"""
import os, struct, subprocess, sys, tempfile

S = 2352
JP_FIRST, COUNT = 23106, 1680
OLD_CD, OLD_FRAMES, NEW_FRAMES, STREAM_OFF = 23113, 143, 168, 0xB2CC

_F = [0] * 256; _B = [0] * 256; _E = [0] * 256
for _i in range(256):
    _j = (_i << 1) ^ (0x11D if _i & 0x80 else 0)
    _F[_i] = _j & 0xFF; _B[_i ^ (_j & 0xFF)] = _i
    _e = _i
    for _ in range(8):
        _e = (_e >> 1) ^ (0xD8018001 if _e & 1 else 0)
    _E[_i] = _e


def edc(b):
    e = 0
    for x in b:
        e = (e >> 8) ^ _E[(e ^ x) & 0xFF]
    return e


def eblock(src, maj, mn, mm, mi):
    size = maj * mn
    lo, hi = bytearray(maj), bytearray(maj)
    for m in range(maj):
        idx = (m >> 1) * mm + (m & 1)
        a = b = 0
        for _ in range(mn):
            t = src[idx]; idx += mi
            if idx >= size: idx -= size
            a ^= t; b ^= t; a = _F[a]
        a = _B[_F[a] ^ b]
        lo[m] = a; hi[m] = a ^ b
    return bytes(lo) + bytes(hi)


def fix_form1(s):
    """rebuild EDC + ECC of a Mode 2 form 1 sector (the ECC ignores the header address;
    Q parity covers the P parity, so P is written before Q is computed)"""
    s[0x818:0x81C] = struct.pack('<I', edc(bytes(s[0x10:0x818])))
    z = bytearray(s[12:2352]); z[0:4] = b'\0\0\0\0'
    s[0x81C:0x81C + 172] = eblock(z, 86, 24, 2, 86)
    z = bytearray(s[12:2352]); z[0:4] = b'\0\0\0\0'
    s[0x8C8:0x8C8 + 104] = eblock(z, 52, 43, 86, 88)


def bcd(n): return ((n // 10) << 4) | (n % 10)


def header(lba):
    f = lba + 150
    return b'\x00' + b'\xff' * 10 + b'\x00' + bytes([bcd(f // 4500), bcd(f // 75 % 60), bcd(f % 75), 2])


def sec(f, lba):
    f.seek(lba * S); return bytearray(f.read(S))


def put(f, lba, s):
    f.seek(lba * S); f.write(bytes(s))


def dir_entries(f, lba, size):
    d = b''.join(bytes(sec(f, lba + k)[24:2072]) for k in range((size + 2047) // 2048))
    o, res = 0, []
    while o < len(d):
        l = d[o]
        if l == 0:
            o = (o // 2048 + 1) * 2048; continue
        res.append((d[o + 33:o + 33 + d[o + 32]].decode('latin1'), struct.unpack('<I', d[o + 2:o + 6])[0],
                    struct.unpack('<I', d[o + 10:o + 14])[0], d[o + 25], lba * 0 + o, l)); o += l
    return res


def find(f, root, path):
    lba, size = root
    ent = None
    for part in path:
        ent = [e for e in dir_entries(f, lba, size) if e[0] == part]
        assert ent, "not found: " + part
        lba, size = ent[0][1], ent[0][2]
    return lba, size


def decode_ecm(ecm):
    here = os.path.dirname(os.path.abspath(__file__))
    tmp = tempfile.mkdtemp(prefix="jpt1_")
    exe, out = os.path.join(tmp, "unecm"), os.path.join(tmp, "jp_t1.bin")
    subprocess.run(["cc", "-O2", "-w", "-o", exe, os.path.join(here, "unecm.c")], check=True)
    subprocess.run([exe, ecm, out], check=True)
    return out


def main(img, jp):
    if jp.endswith(".ecm"):
        jp = decode_ecm(jp)
    n = os.path.getsize(img) // S
    assert os.path.getsize(img) % S == 0
    with open(img, 'r+b') as f, open(jp, 'rb') as fj:
        pvd = sec(f, 16)
        assert pvd[24:24 + 6] == b'\x01CD001', "no PVD"
        vol = struct.unpack('<I', pvd[24 + 80:24 + 84])[0]
        assert vol == n, ("volume size %d != image sectors %d (already patched?)" % (vol, n))
        # 1) movie sectors, payload verbatim, header = new position
        for k in range(COUNT):
            j = sec(fj, JP_FIRST + k)
            assert j[:12] == b'\x00' + b'\xff' * 10 + b'\x00' and j[15] == 2, "JP sector %d not Mode 2" % (JP_FIRST + k)
            put(f, n + k, bytearray(header(n + k)) + j[16:])
        total = n + COUNT
        # 2) PVD volume size (LE + BE)
        pvd[24 + 80:24 + 88] = struct.pack('<I', total) + struct.pack('>I', total)
        fix_form1(pvd); put(f, 16, pvd)
        # 3) root dir record
        root = (struct.unpack('<I', pvd[24 + 156 + 2:24 + 156 + 6])[0], 2048)
        rs = sec(f, root[0]); done = False; o = 0
        while o < 2048 and rs[24 + o]:
            l = rs[24 + o]; name = bytes(rs[24 + o + 33:24 + o + 33 + rs[24 + o + 32]])
            if name == b'LOGO15XA.STR;1':
                p = 24 + o
                assert struct.unpack('<I', rs[p + 2:p + 6])[0] == OLD_CD, "LOGO15XA old LBA"
                rs[p + 2:p + 10] = struct.pack('<I', n) + struct.pack('>I', n)
                rs[p + 10:p + 18] = struct.pack('<I', COUNT * 2048) + struct.pack('>I', COUNT * 2048)
                done = True
            o += l
        assert done, "LOGO15XA.STR;1 not in root dir"
        fix_form1(rs); put(f, root[0], rs)
        # 4) SEL.BIN / SELRIC2.BIN stream table
        for path in (("ST", "SEL", "SEL.BIN;1"), ("BIN", "SELRIC2.BIN;1")):
            lba, size = find(f, root, path)
            lba += STREAM_OFF // 2048; off = 24 + STREAM_OFF % 2048
            s = sec(f, lba)
            cd, fr = struct.unpack('<IH', s[off:off + 6])  # u32 cdOffset, u16 frameCount (+u16 isRGB24)
            assert (cd, fr) == (OLD_CD, OLD_FRAMES), ("%s: old stream0 %r" % (path[-1], (cd, fr)))
            s[off:off + 6] = struct.pack('<IH', n, NEW_FRAMES)
            fix_form1(s); put(f, lba, s)
    print("jp_movie: LOGO15XA.STR %d sectors at LBA %d, image %d -> %d sectors" % (COUNT, n, n, total))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
