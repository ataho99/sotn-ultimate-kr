#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""One-off: cut the five title assets out of the live image into ko_assets/title46/
with a sha256 manifest.   usage: title_extract46.py IMAGE46.bin OUT_DIR"""
import hashlib, json, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import jp_movie as J

FILES = [("F_TITLE0.BIN", ("BIN", "F_TITLE0.BIN;1")), ("F_TITLE1.BIN", ("BIN", "F_TITLE1.BIN;1")),
         ("F_SEL.BIN", ("ST", "SEL", "F_SEL.BIN;1")), ("F_GAME.BIN", ("BIN", "F_GAME.BIN;1")),
         ("F_GAME2.BIN", ("BIN", "F_GAME2.BIN;1"))]


def main(img, out):
    os.makedirs(out, exist_ok=True)
    man = {"source": "trial46 live image (sha1 38eb97090501b44779bff36fd8ae454aa6a0f4dc)", "files": []}
    with open(img, 'rb') as f:
        pvd = J.sec(f, 16)
        root = (struct.unpack('<I', pvd[24 + 156 + 2:24 + 156 + 6])[0], 2048)
        for name, path in FILES:
            lba, size = J.find(f, root, path)
            data = b''.join(bytes(J.sec(f, lba + k)[24:2072]) for k in range((size + 2047) // 2048))[:size]
            open(os.path.join(out, name), 'wb').write(data)
            man["files"].append({"file": name, "path": list(path), "size": size,
                                 "sha256": hashlib.sha256(data).hexdigest()})
            print(name, size, man["files"][-1]["sha256"][:12])
    json.dump(man, open(os.path.join(out, 'manifest.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
