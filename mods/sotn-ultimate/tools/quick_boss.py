#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""quick_boss.py -- post-link steps of the asm-boss overlays BO2 BO5 BO6 RBO2 RBO6 (+ BO6RIC2) for quick_boss.sh.

The full build (build_ko_trial.sh) gives these BINs these steps after ninja's link:
  gen_ko_wrap.py -> gen_ko_splice.py (plan of the Korean boss script bytes) -> gen_ko_splice.py --apply-boss
  -> psp_script.py --apply -> build_bo6ric2.py (BO6RIC2 = BO6 + RIC2 coat art)
This runs the same code in-process on COPIES in a work dir (nothing under build/us is written):

  quick_boss.py WORKDIR NAME [NAME ...]      NAME: BO2 BO5 BO6 RBO2 RBO6 (BO6 also makes BO6RIC2)

WORKDIR/NAME.BIN must be the freshly linked overlay (objcopy of the elf); results are written there too.
The wrapped translation table is read from $KO_TSV when set (quick_boss.sh points it at a freshly
wrapped copy), else the file gen_ko_wrap.py wrote last.
"""
import hashlib
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.environ.get("SOTN_KR") or os.path.abspath(os.path.join(HERE, "../../.."))
sys.path.insert(0, HERE)
BOSS = ("BO2", "BO5", "BO6", "RBO2", "RBO6")


def main(work, names):
    import gen_ko_splice as S
    import psp_script as P
    for n in names:
        if n not in BOSS:
            sys.exit("quick_boss: %s is not an asm boss overlay (%s)" % (n, " ".join(BOSS)))
    tsv, mp = S.load_tsv(), S.load_map()
    plan, allow = [], {}
    S.splice_boss(tsv, mp, plan, allow)            # the plan gen_ko_splice.py writes to ko_boss_splice.json
    old_plan_path = os.path.join(ROOT, "build/us/ko_boss_splice.json")
    if os.path.exists(old_plan_path):
        same = json.load(open(old_plan_path)) == json.loads(json.dumps(plan))
        print("quick_boss: boss plan %s the last full build's ko_boss_splice.json (%d scripts)" % (
            "IDENTICAL to" if same else "DIFFERS from", len(plan)))
    # allow list: the last full build's plus this plan's regions (psp_script lets those bytes differ from retail)
    allow_path = os.path.join(ROOT, "build/us/mod_ko_psp_allow.json")
    al = json.load(open(allow_path)) if os.path.exists(allow_path) else {}
    for b, rr in allow.items():
        for r in rr:
            if r not in al.setdefault(b, []):
                al[b].append(r)
    apath = os.path.join(work, "allow.json")
    json.dump(al, open(apath, "w"))
    P.KO_ALLOW = apath
    pj = json.load(open(os.path.join(ROOT, "build/us/psp_script/patches.json")))
    for n in names:
        path = os.path.join(work, n + ".BIN")
        b = bytearray(open(path, "rb").read())
        k = 0
        for p in plan:                                # == gen_ko_splice.apply_boss, on the copy
            if p["bin"] != n + ".BIN":
                continue
            old, new, o = bytes.fromhex(p["old"]), bytes.fromhex(p["new"]), p["off"]
            cur = bytes(b[o:o + len(old)])
            if cur == new:
                continue
            if cur != old:
                sys.exit("quick_boss: %s at 0x%X is not the retail script (BIN not freshly linked?)" % (n, o))
            b[o:o + len(new)] = new
            k += 1
        open(path, "wb").write(b)
        ents = [dict(e, bin=path) for e in pj if os.path.basename(e["bin"]) == n + ".BIN"]
        tmp = os.path.join(work, "patches_%s.json" % n)
        json.dump(ents, open(tmp, "w"))
        P.PATCHES = tmp
        P.apply(False)
        print("quick_boss: %s boss splice wrote %d scripts, psp entries %d, sha1 %s" % (
            n, k, len(ents), hashlib.sha1(open(path, "rb").read()).hexdigest()))
        if n == "BO6":
            import build_bo6ric2 as B
            real = B.path
            redirect = {"build/us/BO6.BIN": path, "build/us/BO6RIC2.BIN": os.path.join(work, "BO6RIC2.BIN")}
            B.path = lambda p: redirect.get(p) or real(p)
            B.main()
            print("quick_boss: BO6RIC2 sha1 %s" % hashlib.sha1(open(redirect["build/us/BO6RIC2.BIN"], "rb").read()).hexdigest())


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    main(sys.argv[1], [a.upper().replace(".BIN", "") for a in sys.argv[2:]])
