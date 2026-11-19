#!/usr/bin/env python3
# check_xa_table.py -- the DRA song table (g_XaMusicConfigs) must be repack_xa's table (fail closed).
#
# Background (sihum39b, 월하 BGM 무음): repack_xa.py rewrites src/dra/gen/us/xa_music_config.h
# (and XA_STR1.), then "make build_assets" regenerated that header from the RETAIL yaml, so DRA was
# compiled with the retail table (file/channel/LBA of the retail disc) while the disc's XA_STR1 was
# repacked -> CdlSetfilter file mismatch -> every XA song silent (since sihum24).
#
# Usage: check_xa_table.py dra         compile-side: build/us/dra.elf table == src/dra/gen/us/xa_music_config.h
#        check_xa_table.py image BIN   disc-side: for every song row the XA sector at
#                                      \SD\XA_STR1 + cd_addr + channel has subheader file/channel == the row
import re
import struct
import subprocess
import sys

HDR = "src/dra/gen/us/xa_music_config.h"
ELF = "build/us/dra.elf"
ROW = re.compile(r"^\{(0x[0-9A-Fa-f]+|\d+),\s*(0x[0-9A-Fa-f]+|\d+),\s*(0x[0-9A-Fa-f]+|\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(0x[0-9A-Fa-f]+|\d+)\}")


def header_rows():
    rows = []
    for ln in open(HDR, encoding="utf-8"):
        m = ROW.match(ln.strip())
        if m:
            rows.append(tuple(int(x, 0) for x in m.groups()))
    return rows


def elf_bytes(path, addr, size):
    d = open(path, "rb").read()
    shoff, = struct.unpack_from("<I", d, 0x20)
    shentsize, shnum = struct.unpack_from("<HH", d, 0x2E)
    for i in range(shnum):
        o = shoff + i * shentsize
        name, typ, flags, saddr, off, sz = struct.unpack_from("<IIIIII", d, o)
        if typ == 1 and saddr and saddr <= addr and addr + size <= saddr + sz:
            return d[off + addr - saddr:off + addr - saddr + size]
    raise SystemExit("FAIL check_xa_table: address 0x%08X not inside a PROGBITS section of %s" % (addr, path))


def check_dra():
    rows = header_rows()
    if not rows:
        raise SystemExit("FAIL check_xa_table: no rows parsed from %s" % HDR)
    out = subprocess.check_output(["mipsel-linux-gnu-nm", ELF]).decode()
    m = re.search(r"^([0-9a-f]+) \w g_XaMusicConfigs$", out, re.M)
    if not m:
        raise SystemExit("FAIL check_xa_table: g_XaMusicConfigs not in %s" % ELF)
    addr = int(m.group(1), 16)
    tbl = elf_bytes(ELF, addr, len(rows) * 16)
    bad = []
    for i, r in enumerate(rows):
        cd, u228, ff, ch, vol, u22f, u230 = r
        got = struct.unpack_from("<IiBBBBI", tbl, i * 16)
        want = (cd & 0xFFFFFFFF, u228, ff, ch, vol, u22f, u230)
        if got != want:
            bad.append((i, want, got))
    if bad:
        print("FAIL check_xa_table: DRA song table != %s in %d of %d rows; first: row %d want %s got %s"
              % (HDR, len(bad), len(rows), bad[0][0], bad[0][1], bad[0][2]))
        sys.exit(1)
    print("OK   check_xa_table dra: %d song rows of %s == DRA g_XaMusicConfigs (0x%08X)" % (len(rows), HDR, addr))


def sec(f, l):
    f.seek(l * 2352 + 24)
    return f.read(2048)


def xa_lba(f):
    pvd = sec(f, 16)
    root = struct.unpack_from("<I", pvd, 158)[0]
    rs = struct.unpack_from("<I", pvd, 166)[0]

    def ents(d):
        o = 0
        r = []
        while o < len(d):
            n = d[o]
            if n == 0:
                o = (o // 2048 + 1) * 2048
                if o >= len(d):
                    break
                continue
            lba = struct.unpack_from("<I", d, o + 2)[0]
            sz = struct.unpack_from("<I", d, o + 10)[0]
            r.append((d[o + 33:o + 33 + d[o + 32]].decode("latin1"), lba, sz))
            o += n
        return r

    def rd(lba, sz):
        return b"".join(sec(f, lba + i) for i in range((sz + 2047) // 2048))
    sd = [e for e in ents(rd(root, rs)) if e[0].startswith("SD")][0]
    xa = [e for e in ents(rd(sd[1], sd[2])) if e[0].startswith("XA_STR1")][0]
    return xa[1]


def check_image(path):
    rows = header_rows()
    f = open(path, "rb")
    base = xa_lba(f)
    bad = []
    n = 0
    for i, (cd, u228, ff, ch, vol, u22f, u230) in enumerate(rows):
        if ff == 0 and ch == 0 and cd == 0:
            continue  # empty row
        n += 1
        f.seek((base + cd + ch) * 2352 + 16)
        sh = f.read(8)
        sm = sh[2]
        if not (sm & 0x04) or sh[0] != ff or sh[1] != (ch % 16):
            bad.append((i, "row file %d ch %d cd %d -> sector subheader file %d ch %d submode %02X" % (ff, ch, cd, sh[0], sh[1], sm)))
    if bad:
        print("FAIL check_xa_table: image XA sector != song table in %d of %d rows; first: row %d %s" % (len(bad), n, bad[0][0], bad[0][1]))
        sys.exit(1)
    print("OK   check_xa_table image: %d song rows match the XA sectors (file/channel) of %s (XA_STR1 LBA %d)" % (n, path, base))


if __name__ == "__main__":
    if len(sys.argv) >= 2 and sys.argv[1] == "dra":
        check_dra()
    elif len(sys.argv) >= 3 and sys.argv[1] == "image":
        check_image(sys.argv[2])
    else:
        raise SystemExit(__doc__)
