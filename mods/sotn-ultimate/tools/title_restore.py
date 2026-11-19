#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Build step (trial49d): put the shipped-title assets back into the image.

The trial46 title / loading / opening-caption art (F_TITLE0, F_TITLE1, F_SEL,
F_GAME, F_GAME2) lives in the repo as sha-pinned files (ko_assets/title46/).
Each file is written over the same-named disc file, sector by sector, with
Mode 2 form 1 EDC/ECC rebuilt.  Sizes must match, so the disc layout never
moves.  Idempotent.  (An out-of-repo post step silently dropped out of the
trial48 build chain; every replacement is a build step now.)

usage: title_restore.py IMAGE.bin ASSET_DIR
"""
import hashlib, json, os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import jp_movie as J


def main(img, adir):
    man = json.load(open(os.path.join(adir, 'manifest.json'), encoding='utf-8'))
    changed = 0
    with open(img, 'r+b') as f:
        pvd = J.sec(f, 16)
        assert pvd[24:30] == b'\x01CD001', "no PVD"
        root = (struct.unpack('<I', pvd[24 + 156 + 2:24 + 156 + 6])[0], 2048)
        for m in man['files']:
            data = open(os.path.join(adir, m['file']), 'rb').read()
            assert len(data) == m['size'], "asset size " + m['file']
            assert hashlib.sha256(data).hexdigest() == m['sha256'], "asset sha256 " + m['file']
            lba, size = J.find(f, root, m['path'])
            assert size == len(data), "disc file size %s: %d != %d" % (m['file'], size, len(data))
            for k in range((size + 2047) // 2048):
                s = J.sec(f, lba + k)
                assert s[15] == 2 and not (s[18] & 0x20), "not Mode 2 form 1: %s +%d" % (m['file'], k)
                s[24:24 + 2048] = data[k * 2048:(k + 1) * 2048].ljust(2048, b'\0')
                J.fix_form1(s)
                J.put(f, lba + k, s)
                changed += 1
            print("title_restore: %s LBA %d, %d sectors" % (m['file'], lba, (size + 2047) // 2048))
    print("title_restore: %d files, %d sectors written" % (len(man['files']), changed))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
