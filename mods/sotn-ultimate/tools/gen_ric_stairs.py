#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""Saturn port mod: Richter's stairs stances (Ultimate), from his own art.

Standing on stairs (a steep slope) he takes one of two stances by the way he
faces (src/ric/pl_setstep.c RicSetStand): his idle (frame 1) above the waist
on the legs of frame 211 (facing up the stairs: front foot on the higher
step) or frame 212 (facing down: back foot on the higher step). Frames 211
and 212 are in his set but no animation shows them; their sprites (0xC3,
0xC4) have no arm, so the idle's top half (arm included) goes over them.

RIC.BIN must keep its retail 116 sectors (DRA's file table) and its sprite
sheet its size (DRA and the costume builders use its end), so:
- facing up goes in sprite 0xC3's own slot, at its size (the generated
  src/ric/gen/richter.h is patched after make build_assets; frame 211
  unchanged);
- facing down (larger than 0xC4's slot) goes in src/ric/gen/mod_ric_stairs.h,
  a buffer with its frame; RicInit points sprite 0xC4 / frame 212 there.
build_ric2.py puts the Saturn coat's own stances in RIC2.BIN (compose()).
"""
import json
import os

import numpy as np

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
ASSETS = os.path.join(ROOT, "assets", "ric")
OUT = os.path.join(ROOT, "src", "ric", "gen", "mod_ric_stairs.h")

IDLE, UP, DOWN = 1, 211, 212  # spritebank0 frames
WAIST = 5  # frame y: the idle above, the stair legs from here down
THIGH = 0  # frame y: from here a raised thigh outside the idle shows
FRAME_X, FRAME_Y = -16, -24  # the stance frames' origin (the idle's)
BUF = 4 + 24 * 45 // 2  # facing down's buffer (both stances: 24x44)
SLOT = 0xC3  # facing up: this sprite's slot in the sheet
HDR = os.path.join(ROOT, "src", "ric", "gen", "richter.h")


def compose(top, legs):
    """top / legs: (img, left, top) in frame coordinates (img: 2D palette
    indices, 0 = transparent). Returns (img, left, top) of the stance."""
    (ti, tx, ty), (li, lx, ly) = top, legs
    x0, y0 = min(tx, lx), min(ty, ly)
    x1 = max(tx + ti.shape[1], lx + li.shape[1])
    y1 = max(ty + ti.shape[0], ly + li.shape[0])
    c = np.zeros((y1 - y0, x1 - x0), np.uint8)
    for img, ox, oy, keep in ((li, lx, ly, lambda fy: fy >= WAIST),
                              (ti, tx, ty, lambda fy: fy < WAIST)):
        for r in range(img.shape[0]):
            fy = oy + r
            if not keep(fy):
                continue
            row = img[r]
            m = row != 0
            c[fy - y0, ox - x0:ox - x0 + img.shape[1]][m] = row[m]
    # a raised thigh: above the waist the leg grows up from the waist line
    # through the pixels the idle leaves empty, so the knee shows around
    # and behind his hand / sleeve, which stay in front (user reports
    # 2026-11-20: the front boot had no leg; the coat's hand cut the knee)
    lg = np.zeros_like(c)
    lg[ly - y0:ly - y0 + li.shape[0], lx - x0:lx - x0 + li.shape[1]] = li
    w0 = WAIST - y0
    todo = [(w0, x) for x in np.nonzero(lg[w0])[0]] if 0 <= w0 < len(c) else []
    seen = set(todo)
    while todo:
        y, x = todo.pop()
        for ny, nx in ((y - 1, x), (y, x - 1), (y, x + 1), (y + 1, x)):
            if (ny, nx) in seen or not THIGH - y0 <= ny < w0:
                continue
            if 0 <= nx < c.shape[1] and lg[ny, nx] and not c[ny, nx]:
                seen.add((ny, nx))
                c[ny, nx] = lg[ny, nx]
                todo.append((ny, nx))
    ys, xs = np.nonzero(c)
    c = c[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    left, topy = x0 + xs.min(), y0 + ys.min()
    # the sheet's widths are multiples of 4
    pad = -c.shape[1] % 4
    if pad:
        c = np.pad(c, ((0, 0), (0, pad)))
    return c, left, topy


def pack(img, left, topy, fx=FRAME_X, fy=FRAME_Y, size=None):
    """The sheet's sprite bytes for img placed at (left, topy) in a frame
    whose origin is (fx, fy); size: pad the image to exactly that many
    bytes (same width, more rows), else to BUF with zeros."""
    h, w = img.shape
    if size is not None:
        rows = (size - 4) * 2 // w
        assert rows >= h and 4 + w * rows // 2 == size, (w, h, size)
        img = np.pad(img, ((0, rows - h), (0, 0)))
        h = rows
    px, py = left - fx, topy - fy
    assert 0 <= px <= 255 and 0 <= py <= 255, (px, py)
    flat = img.reshape(-1)
    data = bytes([w, h, px, py]) + bytes((flat[0::2] | (flat[1::2] << 4)).tolist())
    if size is not None:
        return data
    assert len(data) <= BUF, (len(data), BUF)
    return data + bytes(BUF - len(data))


def patch_slot(data):
    """Sprite SLOT's bytes in the generated sheet header (same size)."""
    import re
    t = open(HDR).read()
    m = re.search(r"static unsigned char richter_%d\[\] = \{\n(.*?)\};" % SLOT,
                  t, re.S)
    old = [int(v, 0) for v in re.findall(r"0x[0-9A-Fa-f]+|\d+", m.group(1))]
    assert len(old) == len(data), (len(old), len(data))
    body = ",\n".join(",".join("0x%02X" % v for v in data[i:i + 16])
                      for i in range(0, len(data), 16)) + ",\n"
    new = t[:m.start(1)] + body + t[m.end(1):]
    if new != t:
        open(HDR, "w").write(new)


def slot_size():
    import re
    m = re.search(r"static unsigned char richter_%d\[(\d+)\];" % SLOT, open(HDR).read())
    return int(m.group(1))


def ps1_part(frame):
    """(img, left, top) of a spritebank0 frame, from the extracted assets."""
    from PIL import Image
    import yaml
    anim = json.load(open(os.path.join(ASSETS, "spritebank0.animset.json")))
    sheet = yaml.safe_load(open(os.path.join(ASSETS, "richter.yaml")))
    (part,) = anim[frame]
    e = sheet[part["flags"] & 0x7FFF]
    img = np.array(Image.open(os.path.join(ASSETS, e["name"])))
    return img, part["x"] + e["pivotx"], part["y"] + e["pivoty"]


def main():
    idle = ps1_part(IDLE)
    # facing up, in sprite 0xC3's slot: frame 211 is at (-8, -24)
    up = compose(idle, ps1_part(UP))
    anim = json.load(open(os.path.join(ASSETS, "spritebank0.animset.json")))
    (p211,) = anim[UP]
    patch_slot(pack(*up, p211["x"], p211["y"], size=slot_size()))
    down = pack(*compose(idle, ps1_part(DOWN)))
    L = ["// generated by mods/sotn-ultimate/tools/gen_ric_stairs.py; do not edit",
         "// Richter's Ultimate stairs stance facing down the stairs (src/ric/",
         "// pl_main.c RicInit; facing up is in sprite 0xC3's slot)",
         "#define RIC_STAIR_BUF %d" % BUF,
         "u8 g_RicStairSpr[RIC_STAIR_BUF] = {" + ", ".join("0x%02X" % b for b in down) + "};",
         "s16 g_RicStairFrame[4] = {0xC4 | 0x8000, %d, %d, 0};" % (FRAME_X, FRAME_Y)]
    text = "\n".join(L) + "\n"
    if not os.path.exists(OUT) or open(OUT).read() != text:
        open(OUT, "w").write(text)
    print("ric stairs: facing up %dx%d in sprite 0x%X's slot, facing down %dx%d"
          % (up[0].shape[1], up[0].shape[0], SLOT, down[0], down[1]))


if __name__ == "__main__":
    main()
