#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
# gen_ko_cards.py -- KO trial: Korean area-name card pictures into the linked
# stage overlays (trial builds only; build_ko_trial.sh runs it after the
# overlays are linked and before the disc image is made).
#
# Each area name card is a 128x128 4bpp picture compressed with the asset
# tool's cmpgfx (builder/src/sotn_cmpgfx.ts = byte-exact port) inside the
# stage overlay's .data. The Korean picture (ko_data/cards/<KEY>_ko.png,
# same 16 indices as the English one: index = glyph brightness, the game's
# CLUTs colour it) is compressed the same way. The stream ends with the
# cmpgfx terminator nibble and DecompressData (src/dra/4AEA4.c) stops there,
# so the Korean bytes are written over the English slot and the rest of the
# slot is zero padded: same position, same length, nothing after it moves.
#
# Fail closed:
#   * the English PNG compressed here must equal the overlay bytes at the
#     asset's offset (positive control of the compressor and of the slot) --
#     or the slot must already hold this tool's Korean bytes (re-run)
#   * map symbol address == offset + 0x80180000
#   * Korean compressed size <= English slot size
# The slot is registered (overlay, symbol, size) in build/us/mod_data_edits.json
# (merged, as gen_ko_splice.py / gen_card_layouts.py) so check_stage_data.py
# accepts the in-place edit of the .data section.
#
# Not here: the mod's own Garden / Prison cards (synthesised by gdn_assets.py / prs_assets.py),
# Castle Entrance (NO3: no card slot found in the overlay by the 0.9.4 scan). Outer Wall (NO1) is the 16th row
# (0.9.4 / 094_08): its English pixels are decoded from the retail overlay (no assets/ PNG).
import json
import os
import re
import subprocess
import sys

from PIL import Image

from ko_paths import KO_WK, KO_REPO  # noqa: E402

REPO = os.environ.get("SOTN_KR", KO_REPO)
NODE = os.environ.get("NODE", "node")
CARDS = os.path.join(KO_WK, "cards")
EDITS_PATH = os.path.join(REPO, "build", "us", "mod_data_edits.json")
WORK = os.path.join(REPO, "build", "us", "ko_cards")
BASE = 0x80180000

# (card key, stage dir, asset name = map symbol, BIN offset, picture file)
CARDS_TBL = [
    ("LIB", "lib", "gfx_stage_name_en", 0x15388, "LIB_ko.png"),    # Long Library
    ("CAT", "cat", "gfx_stage_name_en", 0x80D0, "CAT_ko.png"),     # Catacombs
    ("NO4", "no4", "D_80185400", 0x5400, "NO4_ko.png"),            # Underground Caverns
    ("ARE", "are", "gfx_stage_name_en", 0x521C, "ARE_ko.png"),     # Colosseum
    ("DAI", "dai", "gfx_stage_name_en", 0x16554, "DAI_ko.png"),    # Royal Chapel
    ("TOP", "top", "gfx_stage_name_en", 0x10714, "TOP_ko.png"),    # Castle Keep
    ("NO0", "no0", "stage_title_no0", 0x48A0, "NO0_ko.png"),       # Marble Gallery
    ("NO2", "no2", "stage_title_no2", 0x14014, "NO2_ko.png"),      # Olrox's Quarters
    ("NZ0", "nz0", "stage_title_nz0_us", 0x156A4, "NZ0_ko.png"),   # Alchemy Laboratory
    ("DRE", "dre", "gfx_stage_name_en", 0xA88C, "DRE_ko.png"),     # Nightmare
    ("CHI", "chi", "bn_gfx_stage_name_en", 0x9B38, "CHI_ko.png"),  # Abandoned Mine
    ("NZ1", "nz1", "D_80186530", 0x6530, "NZ1_ko.png"),            # Clock Tower
    ("ST0", "st0", "stage_title_st0_us", 0x1A40C, "ST0b_ko.png"),  # Final Stage "Bloodlines"
    ("NO1E", "no1", "D_us_80194E14", 0x14E14, "NO1_elev_ko.png"),   # Outer Wall elevator message picture "Elevator activated." (0.9.4 / 094_08; same decode-from-overlay path)
    ("NO1", "no1", "D_us_8019519C", 0x1519C, "NO1_ko.png"),        # Outer Wall (0.9.4 / 094_08: no assets/ PNG -> English pixels decoded from the retail overlay)
]

MJS = """import fs from "node:fs";
import path from "node:path";
import { pathToFileURL } from "node:url";
const [repo, manifest, out] = process.argv.slice(2);
const mod = await import(pathToFileURL(path.join(repo, "mods/sotn-ultimate/tools/builder/src/sotn_cmpgfx.ts")).href);
const man = JSON.parse(fs.readFileSync(manifest, "utf8"));
const res = {};
for (const m of man) {
  const px = new Uint8Array(fs.readFileSync(m.idx));
  res[m.name] = Buffer.from(mod.buildCmpGfx(px, 128, 128, 4)).toString("hex");
}
fs.writeFileSync(out, JSON.stringify(res));
"""


def decode_card(retail_bin, off):
    """DecompressData (src/dra/4AEA4.c) port: the 128x128 4bpp pixel indices of the cmpgfx stream at off.
    Used for an English card that has no assets/ PNG (NO1: the splat config lists no asset there); the
    positive control below (English compress == overlay bytes) then checks this decoder + the compressor."""
    d = open(retail_bin, "rb").read()
    buf = list(d[off:off + 8])
    p, half, out = off + 8, False, []

    def rn():
        nonlocal p, half
        if not half:
            half = True
            return d[p] >> 4
        half = False
        v = d[p] & 15
        p += 1
        return v

    while True:
        op = rn()
        if op == 0:
            c, c2 = rn(), rn()
            out += [0] * (c * 16 + c2 + 0x13)
        elif op == 1:
            out.append(rn())
        elif op == 2:
            v = rn()
            out += [v, v]
        elif op == 3:
            out += [rn(), rn()]
        elif op == 4:
            out += [rn(), rn(), rn()]
        elif op == 5:
            c, c2 = rn(), rn()
            out += [c] * (c2 + 3)
        elif op == 6:
            out += [0] * (rn() + 3)
        elif 7 <= op <= 14:
            ch = buf[op - 7]
            t = ch & 0xF0
            if t == 0x20:
                out += [ch & 15] * 2
            elif t == 0x10:
                out += [ch & 15]
            elif t == 0x60:
                out += [0] * ((ch & 15) + 3)
        else:
            break
        assert len(out) <= 0x4100, "decode runaway"
    assert len(out) == 0x4000, ("not a 128x128 4bpp card", len(out))
    return bytes(out)


def indices(path):
    im = Image.open(path)
    assert im.mode == "P" and im.size == (128, 128), (path, im.mode, im.size)
    raw = im.tobytes()
    assert len(raw) == 128 * 128 and max(raw) < 16, path
    return raw


def main():
    check = "--check" in sys.argv  # report only, write nothing
    os.makedirs(WORK, exist_ok=True)
    man = []
    for key, stage, asset, off, ko_png in CARDS_TBL:
        for tag, path in (("en", os.path.join(REPO, "assets", "st", stage, asset + ".png")),
                          ("ko", os.path.join(CARDS, ko_png))):
            idx = os.path.join(WORK, "%s_%s.idx" % (key, tag))
            if tag == "en" and not os.path.exists(path):
                # no extracted PNG for this asset: decode the retail overlay (disks/us/ST/<S>/<S>.BIN)
                open(idx, "wb").write(decode_card(os.path.join(REPO, "disks", "us", "ST", stage.upper(), stage.upper() + ".BIN"), off))
            else:
                open(idx, "wb").write(indices(path))
            man.append({"name": key + "_" + tag, "idx": idx})
    mjs = os.path.join(WORK, "cmp_cards.mjs")
    open(mjs, "w").write(MJS)
    mf, outj = os.path.join(WORK, "manifest.json"), os.path.join(WORK, "cmp.json")
    json.dump(man, open(mf, "w"))
    subprocess.run([NODE, "--no-warnings", mjs, REPO, mf, outj], check=True)
    comp = {k: bytes.fromhex(v) for k, v in json.load(open(outj)).items()}

    edits = json.load(open(EDITS_PATH)) if os.path.exists(EDITS_PATH) else {}
    status = 0
    for key, stage, asset, off, ko_png in CARDS_TBL:
        ovl = "st" + stage
        binpath = os.path.join(REPO, "build", "us", stage.upper() + ".BIN")
        data = bytearray(open(binpath, "rb").read())
        en, ko = comp[key + "_en"], comp[key + "_ko"]
        slot = len(en)
        mp = open(os.path.join(REPO, "build", "us", ovl + ".map")).read()
        m = re.search(r"0x([0-9a-f]+)\s+%s\n" % re.escape(asset), mp)
        if not m or int(m[1], 16) - BASE != off:
            print("FAIL %s: map symbol %s at %s, expected %#x" % (key, asset, m and m[1], BASE + off))
            status = 1
            continue
        if len(ko) > slot:
            print("FAIL %s: Korean %dB > slot %dB" % (key, len(ko), slot))
            status = 1
            continue
        new = ko + bytes(slot - len(ko))
        cur = bytes(data[off:off + slot])
        if cur != en and cur != new:
            print("FAIL %s: %s@%#x is neither the English card (%dB) nor this tool's Korean bytes"
                  % (key, stage.upper() + ".BIN", off, slot))
            status = 1
            continue
        data[off:off + slot] = new
        if not check:
            open(binpath, "wb").write(bytes(data))
        lst = edits.setdefault(ovl, [])
        if [asset, slot] not in lst:
            lst.append([asset, slot])
        print("OK   %s: %s@%#x slot %dB, EN %dB -> KO %dB + %dB zero pad (%s)"
              % (key, stage.upper() + ".BIN", off, slot, len(en), len(ko), slot - len(ko),
                 "already Korean" if cur == new else "spliced"))
    if status == 0 and not check:
        json.dump(edits, open(EDITS_PATH, "w"), indent=2)
    sys.exit(status)


if __name__ == "__main__":
    main()
