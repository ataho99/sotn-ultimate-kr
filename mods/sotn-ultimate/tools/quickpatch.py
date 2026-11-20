#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""quickpatch.py -- file-level fast build: write rebuilt overlay/data files into a copy of a
finished MODE2/2352 image at the SAME LBA instead of running the full build.

  quickpatch.py BASE.bin OUT.bin DISKPATH=NEWFILE [DISKPATH=NEWFILE ...] [--dra-table]

Per file (all checked, any failure aborts before OUT is written):
  * DISKPATH is an ISO9660 path such as BOSS/MAR/MAR.BIN; it must exist in BASE.
  * the new file must fit the sectors the old file occupies (new sectors <= old sectors); the
    sector tail is zero filled; sectors are rewritten with fresh EDC/ECC (cdsector.fix_form1).
  * the ISO directory record size fields (little + big endian) are updated.
  * DRA.BIN carries the byte length of each stage overlay in its stage table as the u32 pair
    [LBA][LENGTH] (src/dra/lba_stage.c, include/lba_us.h *_BIN_OFF/_BIN_LEN): with --dra-table
    every such pair of the old file is rewritten to the new length (exactly one hit expected).
BASE is never modified.  The result must equal the image of a full build of the same sources
(compare sha1 with the full-build image once).
"""
import os
import struct
import sys
import hashlib
import shutil

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cdsector  # noqa: E402

SEC = 2352


def read_sector(f, lba):
    f.seek(lba * SEC)
    return bytearray(f.read(SEC))


def user(f, lba, n=1):
    out = bytearray()
    for i in range(n):
        out += read_sector(f, lba + i)[24:2072]
    return out


def walk(f):
    """-> {path: dict(lba,size,dirlba,dirsec,recoff)}  (recoff = offset in that directory sector's data)"""
    pvd = user(f, 16)
    root_lba = int.from_bytes(pvd[158:162], "little")
    root_sz = int.from_bytes(pvd[166:170], "little")
    res = {}

    def rec(lba, sz, pre):
        nsec = (sz + 2047) // 2048
        for si in range(nsec):
            b = user(f, lba + si)
            o = 0
            while o < 2048:
                ln = b[o]
                if not ln:
                    break
                el = int.from_bytes(b[o + 2:o + 6], "little")
                es = int.from_bytes(b[o + 10:o + 14], "little")
                fl = b[o + 25]
                nl = b[o + 32]
                nm = bytes(b[o + 33:o + 33 + nl]).decode("latin1")
                if nm not in ("\x00", "\x01"):
                    if fl & 2:
                        rec(el, es, pre + nm + "/")
                    else:
                        res[pre + nm.split(";")[0]] = dict(lba=el, size=es, dirsec=lba + si, recoff=o)
                o += ln
    rec(root_lba, root_sz, "")
    return res


def write_user(f, lba, data2048):
    blk = read_sector(f, lba)
    assert blk[15] == 0x02, "not a mode 2 sector at %d" % lba
    blk[24:2072] = data2048
    cdsector.fix_form1(blk)
    f.seek(lba * SEC)
    f.write(blk)


def main(argv):
    dra_table = "--dra-table" in argv
    args = [a for a in argv if not a.startswith("--")]
    if len(args) < 3:
        print(__doc__)
        return 2
    base, out = args[0], args[1]
    items = []
    for a in args[2:]:
        p, _, nf = a.partition("=")
        items.append((p, nf))
    shutil.copyfile(base, out + ".tmp")
    f = open(out + ".tmp", "r+b")
    try:
        tbl = walk(f)
        by_lba = sorted(v["lba"] for v in tbl.values())
        report = []
        dra = tbl["DRA.BIN"]
        dra_old = bytearray(user(f, dra["lba"], (dra["size"] + 2047) // 2048))[:dra["size"]]
        dra_new = bytearray(dra_old)
        dra_dirty = False
        for path, nf in items:
            if path not in tbl:
                raise SystemExit("ABORT: %s not in image" % path)
            e = tbl[path]
            data = open(nf, "rb").read()
            old_ns = (e["size"] + 2047) // 2048
            new_ns = (len(data) + 2047) // 2048
            if new_ns > old_ns:
                raise SystemExit("ABORT: %s grows %d -> %d sectors (does not fit its extent)" % (path, old_ns, new_ns))
            changed = 0
            for si in range(old_ns):
                chunk = data[si * 2048:(si + 1) * 2048]
                chunk = chunk + bytes(2048 - len(chunk))
                cur = user(f, e["lba"] + si)
                if bytes(cur) != chunk:
                    write_user(f, e["lba"] + si, chunk)
                    changed += 1
            line = "%s lba %d: size %d -> %d, sectors %d -> %d, rewritten %d" % (
                path, e["lba"], e["size"], len(data), old_ns, new_ns, changed)
            if len(data) != e["size"]:
                d = user(f, e["dirsec"])
                o = e["recoff"]
                d[o + 10:o + 14] = struct.pack("<I", len(data))
                d[o + 14:o + 18] = struct.pack(">I", len(data))
                write_user(f, e["dirsec"], bytes(d))
                line += "; dir record size updated (sector %d)" % e["dirsec"]
                if dra_table:
                    pat = struct.pack("<II", e["lba"], e["size"])
                    n = dra_new.count(pat)
                    if n != 1:
                        raise SystemExit("ABORT: DRA table pair [LBA %d][LEN %d] found %d times (expected 1)" % (e["lba"], e["size"], n))
                    i = dra_new.index(pat)
                    dra_new[i + 4:i + 8] = struct.pack("<I", len(data))
                    dra_dirty = True
                    line += "; DRA +0x%X length -> 0x%X" % (i + 4, len(data))
            report.append(line)
        if dra_dirty:
            pad = bytes(dra_new) + bytes((-len(dra_new)) % 2048)
            for si in range(len(pad) // 2048):
                chunk = pad[si * 2048:(si + 1) * 2048]
                if bytes(user(f, dra["lba"] + si)) != chunk:
                    write_user(f, dra["lba"] + si, chunk)
    finally:
        f.close()
    os.replace(out + ".tmp", out)
    for r in report:
        print(r)
    h = hashlib.sha1()
    with open(out, "rb") as g:
        for b in iter(lambda: g.read(8 << 20), b""):
            h.update(b)
    print("OUT %s sha1 %s" % (out, h.hexdigest()))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
