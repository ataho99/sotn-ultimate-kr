#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Build step (시험50): the 15 PSP-Japanese voice slots take their sectors from the
official Ultimate v1.4 image.

The PSP Japanese voices (psp_dub.py, PSP_DUB_JP=1) are encoded here from the PSP
files; fifteen of those clips exist as the same recording on the v1.4 disc.  The
shipped image uses the v1.4 take.  Fixed table (ko_assets/voice_v14/table.tsv):
slot, first v1.4 sector (stride 16 = the interleave of one XA stream), sector
count of the slot, sha256 of the finished slot file.  Each slot file
(build/us/psp_jp_dub/<slot>.xa, named in dub.json) keeps its own sector headers
and trailers; only the 18 sound groups of each sector come from v1.4 (a v1.4
run shorter than the slot is zero-filled).  Same size, so the layout never
moves.  Runs after psp_dub.py's cache and before repack_xa.py, every build, and
is idempotent: a cache rebuild can no longer bring the old voice back (an
in-tree one-off swap did exactly that).

usage: voice_v14.py DUB_JSON V14_IMAGE TABLE_TSV
"""
import hashlib, json, os, sys

S, DATA, GROUPS, GB = 2352, 24, 18, 128


def main(dub_json, v14, table):
    rows = [l.rstrip("\n").split("\t") for l in open(table, encoding="utf-8")][1:]
    rows = [r for r in rows if len(r) >= 4]
    dub = json.load(open(dub_json))
    by_name = {}
    for e in dub["replace"].values():
        by_name.setdefault(e["name"], e["xa"])
    with open(v14, "rb") as f:
        size = os.fstat(f.fileno()).st_size

        def sec(lba):
            f.seek(lba * S)
            return f.read(S)

        changed = 0
        for slot, lba, n, want in rows:
            lba, n = int(lba), int(n)
            assert slot in by_name, "slot not in dub.json: " + slot
            path = by_name[slot]
            raw = open(path, "rb").read()
            assert len(raw) == n * S, "slot size %s: %d sectors, table %d" % (slot, len(raw) // S, n)
            first = sec(lba)
            key = (first[16], first[17])
            take = b""
            for q in range(n):
                p = lba + 16 * q
                if (p + 1) * S > size:
                    break
                s = sec(p)
                if (s[16], s[17]) != key or not (s[18] & 0x04) or (s[19] & 1):
                    break
                take += s[DATA:DATA + GROUPS * GB]
            take += bytes(n * GROUPS * GB - len(take))
            out = bytearray()
            for q in range(n):
                tm = raw[q * S:(q + 1) * S]
                assert not tm[19] & 1, "slot sector not audio: " + slot
                out += tm[:DATA] + take[q * GROUPS * GB:(q + 1) * GROUPS * GB] + tm[DATA + GROUPS * GB:]
            got = hashlib.sha256(out).hexdigest()
            assert got == want, "voice_v14 %s: sha256 %s != table %s (wrong v1.4 image?)" % (slot, got[:12], want[:12])
            if bytes(out) != raw:
                open(path, "wb").write(bytes(out))
                changed += 1
            print("voice_v14: %s v1.4 LBA %d, %d sectors%s" % (slot, lba, n, "" if bytes(out) == raw else " (written)"))
    print("voice_v14: %d slots, %d written" % (len(rows), changed))


if __name__ == "__main__":
    main(*sys.argv[1:4])
