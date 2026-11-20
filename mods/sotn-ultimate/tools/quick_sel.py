#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""quick_sel.py -- post-link steps of ST/SEL/SEL.BIN and BIN/SELRIC2.BIN for quick_sel.sh.

What the full build does to SEL after ninja links it:
  make disk (relink SEL against the regenerated include/lba_us.h) -> psp_script.py --apply
  -> build_selric2.py (SELRIC2 = final SEL + RIC2 art) -> jp_movie.py (last step: the Japanese logo movie
  is appended to the image and SEL.BIN / SELRIC2.BIN stream 0 is pointed at it: 6 bytes at 0xB2CC).
SEL.BIN depends on include/lba_us.h ONLY through src/st/sel/lba_stage.c: the VH/VB file lengths of every stage
overlay's audio (ST_*_VH_LEN, ST_*_VB_LEN, BOSS_*_VH_LEN, BOSS_*_VB_LEN).  Those are the sizes of audio files
and do not change with code or translation edits, so the header of the last full build is exact.  With an image
the values are checked against it (`lba` below); a mismatch means the disc's audio files changed: full build.

  quick_sel.py post WORKDIR [MOVIE_IMAGE]   psp copies + SELRIC2 + movie patch on WORKDIR/SEL.BIN (the linked BIN)
  quick_sel.py lba IMAGE                    compare the header's VH/VB lengths with the image's files
"""
import hashlib
import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.environ.get("SOTN_KR") or os.path.abspath(os.path.join(HERE, "../../.."))
sys.path.insert(0, HERE)
STREAM_OFF, OLD_CD, OLD_FRAMES, NEW_FRAMES = 0xB2CC, 23113, 143, 168   # tools/jp_movie.py


def movie_values(image):
    """(cdOffset, frames) the full build's jp_movie step wrote into SEL.BIN: read back from a finished image
    (or, with none, from the size of build/sotn.us.bin = the image before the movie is appended)"""
    if image:
        import quickpatch as Q
        with open(image, "rb") as f:
            t = Q.walk(f)
            e = t["ST/SEL/SEL.BIN"]
            d = bytes(Q.user(f, e["lba"], (e["size"] + 2047) // 2048))
        cd, fr = struct.unpack("<IH", d[STREAM_OFF:STREAM_OFF + 6])
        if cd == OLD_CD:
            return None                      # image without the movie step: nothing to patch
        return cd, fr
    p = os.path.join(ROOT, "build/sotn.us.bin")
    if not os.path.exists(p):
        sys.exit("quick_sel: no image given and build/sotn.us.bin missing: cannot know the movie sector")
    return os.path.getsize(p) // 2352, NEW_FRAMES


def lba_check(image):
    import quickpatch as Q
    hdr = open(os.path.join(ROOT, "include/lba_us.h")).read()
    used = set(re.findall(r"\b((?:ST|BOSS)_[A-Z0-9]+_V[HB]_LEN)\b", open(os.path.join(ROOT, "src/st/sel/lba_stage.c")).read()))
    vals = {m.group(1): int(m.group(2), 0) for m in re.finditer(r"#define (\w+) (0x[0-9A-Fa-f]+|\d+)\b", hdr)}
    with open(image, "rb") as f:
        t = Q.walk(f)
    bad, ok = [], 0
    for name in sorted(used):
        m = re.match(r"(ST|BOSS)_([A-Z0-9]+)_(V[HB])_LEN", name)
        d = "%s/%s/" % (m.group(1), m.group(2))
        files = [(k, v) for k, v in t.items() if k.startswith(d) and k.endswith("." + m.group(3))]
        if name not in vals or len(files) != 1:
            bad.append("%s: header %s, image files %s" % (name, vals.get(name), [k for k, _ in files]))
        elif files[0][1]["size"] != vals[name]:
            bad.append("%s: header %#x, image %s %#x" % (name, vals[name], files[0][0], files[0][1]["size"]))
        else:
            ok += 1
    print("quick_sel lba: %d of %d VH/VB lengths used by SEL agree with the image%s" % (
        ok, len(used), "" if not bad else "; MISMATCH: " + "; ".join(bad)))
    return not bad


def post(work, image):
    import psp_script as P
    sel = os.path.join(work, "SEL.BIN")
    pj = json.load(open(os.path.join(ROOT, "build/us/psp_script/patches.json")))
    ents = [dict(e, bin=sel) for e in pj if os.path.basename(e["bin"]) == "SEL.BIN"]
    tmp = os.path.join(work, "patches_SEL.json")
    json.dump(ents, open(tmp, "w"))
    P.PATCHES = tmp
    P.apply(False)
    import build_bo6ric2 as B
    real = B.path
    redirect = {"build/us/SEL.BIN": sel, "build/us/SELRIC2.BIN": os.path.join(work, "SELRIC2.BIN")}
    B.path = lambda p: redirect.get(p) or real(p)
    import build_selric2 as R
    R.main()
    mv = movie_values(image)
    for n in ("SEL.BIN", "SELRIC2.BIN"):
        p = os.path.join(work, n)
        b = bytearray(open(p, "rb").read())
        cd, fr = struct.unpack("<IH", b[STREAM_OFF:STREAM_OFF + 6])
        assert (cd, fr) == (OLD_CD, OLD_FRAMES), (n, cd, fr)
        if mv:
            b[STREAM_OFF:STREAM_OFF + 6] = struct.pack("<IH", *mv)
        open(p, "wb").write(b)
        print("quick_sel: %s sha1 %s (movie patch %s)" % (n, hashlib.sha1(b).hexdigest(), mv))


if __name__ == "__main__":
    if len(sys.argv) >= 3 and sys.argv[1] == "post":
        post(sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else "")
    elif len(sys.argv) == 3 and sys.argv[1] == "lba":
        sys.exit(0 if lba_check(sys.argv[2]) else 1)
    else:
        sys.exit(__doc__)
