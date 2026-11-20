#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""quick_bin.py -- helper of quick_bin.sh: the post-link steps of the Korean full build
(build_ko_trial.sh) that apply to ONE overlay BIN, in the full build's order, on the
BIN that ninja just linked (build/us/<NAME>.BIN, edited in place -- the same state a full
build leaves there).

  quick_bin.py post NAME            run the post-link steps that apply to NAME
  quick_bin.py diskpath NAME        print NAME's path on the disc (from config/disk.us.lba)
  quick_bin.py plan NAME            print which steps apply / whether NAME is supported
  quick_bin.py verify NAME IMAGE    compare build/us/NAME.BIN with the file inside a finished
                                    MODE2/2352 image (full-build result); exit 0 when equal

Steps (the only ones of build_ko_trial.sh that write into a stage overlay BIN):
  * gen_ko_cards.py   Korean area-name card over the English slot (only the card of NAME)
  * psp_script.py --apply   PSP / JP script copies into g_ModPspSpill (only NAME's patch entry)
Both are idempotent and work on build/us/<NAME>.BIN.

Not supported here (their final bytes come from later steps of the full build, so the
quick path would NOT equal a full build): see NEEDS_FULL below.
"""
import hashlib
import json
import os
import re
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.environ.get("SOTN_KR") or os.path.abspath(os.path.join(HERE, "../../.."))
sys.path.insert(0, HERE)

# NAME -> why the quick path cannot reproduce the full build's file
NEEDS_FULL = {
    "SEL": "use quick_sel.sh (link + psp copies + movie patch; also makes SELRIC2)",
    "SELRIC2": "use quick_sel.sh (made together with SEL)",
    "DRA": "pads / lba_us.h / symbols checked against the relinked image",
    "BO2": "use quick_boss.sh (boss splice + psp copies)",
    "BO5": "use quick_boss.sh (boss splice + psp copies)",
    "BO6": "use quick_boss.sh (boss splice + psp copies; also makes BO6RIC2)",
    "BO6RIC2": "use quick_boss.sh BO6 (made together with BO6)",
    "RBO2": "use quick_boss.sh (boss splice + psp copies)",
    "RBO6": "use quick_boss.sh (boss splice + psp copies)",
    "CENP": "own build script (build_cenp.sh)",
    "MARIA": "own build script (build_mar.sh)",
    "MARIA2": "own build script (build_mar2.sh)",
    "TE1": "own build script (build_gdn.sh)",
    "TE2": "own build script (build_prs.sh)",
    "TE3": "own build script (build_rgdn.sh)",
    "TE4": "own build script (build_rprs.sh)",
}


def cards_rows(name):
    import gen_ko_cards as C
    return C, [r for r in C.CARDS_TBL if r[1].upper() == name.upper()]


def psp_entries(name):
    p = os.path.join(ROOT, "build/us/psp_script/patches.json")
    if not os.path.exists(p):
        return None, []
    d = json.load(open(p))
    return d, [e for e in d if os.path.basename(e["bin"]) == name.upper() + ".BIN"]


def plan(name):
    if name.upper() in NEEDS_FULL:
        return None, "NEEDS_FULL: " + NEEDS_FULL[name.upper()]
    steps = []
    _, rows = cards_rows(name)
    if rows:
        steps.append("cards")
    d, ents = psp_entries(name)
    if d is None:
        return None, "build/us/psp_script/patches.json missing (run a full build once)"
    if ents:
        steps.append("psp")
    if name.upper() == "TOP":
        steps.append("topric2")
    return steps, "ok"


def post(name):
    steps, why = plan(name)
    if steps is None:
        sys.exit("quick_bin: %s: %s" % (name, why))
    binpath = os.path.join(ROOT, "build/us", name.upper() + ".BIN")
    before = hashlib.sha1(open(binpath, "rb").read()).hexdigest()
    if "cards" in steps:
        C, rows = cards_rows(name)
        C.CARDS_TBL = rows
        C.EDITS_PATH = os.path.join(tempfile.gettempdir(), "quick_bin_edits_%s.json" % name)
        sys.argv = [sys.argv[0]]
        try:
            C.main()
        except SystemExit as e:
            if e.code:
                sys.exit("quick_bin: gen_ko_cards failed for %s" % name)
    if "topric2" in steps:
        # Richter's coat art into g_ModTopRic2 (needs build/us/RIC2.BIN current); the step asserts the
        # slot is empty, i.e. it only runs on a freshly linked BIN
        import build_topric2 as T
        try:
            T.main()
        except AssertionError as e:
            if "not empty" not in str(e):
                raise
            print("quick_bin: TOP g_ModTopRic2 already filled (BIN was not relinked)")
    if "psp" in steps:
        import psp_script as P
        _, ents = psp_entries(name)
        tmp = os.path.join(tempfile.gettempdir(), "quick_bin_patches_%s.json" % name)
        json.dump(ents, open(tmp, "w"))
        P.PATCHES = tmp
        P.apply(False)
    after = hashlib.sha1(open(binpath, "rb").read()).hexdigest()
    print("quick_bin: %s steps=%s sha1 %s -> %s" % (name, "+".join(steps) or "none", before[:12], after[:12]))


def diskpath(name):
    pat = re.compile(r"^((?:[^/;]+/)*)%s\.BIN;1" % re.escape(name.upper()))
    for ln in open(os.path.join(ROOT, "config/disk.us.lba"), encoding="utf-8"):
        m = pat.match(ln)
        if m:
            return m.group(0)[:-2]
    sys.exit("quick_bin: %s.BIN not in config/disk.us.lba" % name)


def verify(name, image):
    import quickpatch as Q
    with open(image, "rb") as f:
        t = Q.walk(f)
        e = t[diskpath(name)]
        data = bytes(Q.user(f, e["lba"], (e["size"] + 2047) // 2048))[:e["size"]]
    mine = open(os.environ.get("QUICK_VERIFY_FILE") or os.path.join(ROOT, "build/us", name.upper() + ".BIN"), "rb").read()
    same = mine == data
    print("quick_bin verify %s: quick %d B %s / image %d B %s -> %s" % (
        name, len(mine), hashlib.sha1(mine).hexdigest(), len(data), hashlib.sha1(data).hexdigest(),
        "IDENTICAL" if same else "DIFFERENT"))
    return 0 if same else 1


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else ""
    if cmd == "post":
        post(sys.argv[2])
    elif cmd == "diskpath":
        print(diskpath(sys.argv[2]))
    elif cmd == "plan":
        s, w = plan(sys.argv[2])
        print(w if s is None else "steps: " + ("+".join(s) or "none"))
        sys.exit(0 if s is not None else 3)
    elif cmd == "verify":
        sys.exit(verify(sys.argv[2], sys.argv[3]))
    else:
        sys.exit(__doc__)
