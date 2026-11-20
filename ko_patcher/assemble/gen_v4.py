#!/usr/bin/env python3
"""꾸러미 v4 생성기 — 베이스 U'(얼티밋 공식 v1.4) + 일본 PS1 v1.2 T1 -> 목표 이미지.

분류(목표 섹터별, 우선순위):
  same      목표[16:2352] == U'[16:2352] (헤더 MSF 는 조립 때 재계산)
  data_only F2, 목표[24:2348] == U'[24:2348], 목표 EDC 0 (서브헤더는 subs.bin)
  mono      F2 모노 XA, 목표[24:2328] == mono_xa(T1 런) 출력, 꼬리 24B 0
  bcopy     목표[24:2352] == T1 섹터 [24:2352] (헤더 MSF 재계산, sub 는 recipe)
  new       그 외 -> 델타가 운반 (0 채움)
어떤 분류든 틀려도 델타가 보정하므로 sha1 은 보존된다(분류는 델타 크기만 좌우).

사용: gen_v4.py <U'.bin> <T1.bin> <목표.bin> <출력꾸러미폴더> <v3b꾸러미폴더> <도구폴더(v4.py 있는곳)> <델타만들기 0|1>
"""
import hashlib
import json
import mmap
import os
import subprocess
import sys
import time
from collections import Counter

SECT = 2352
SYNC = bytes.fromhex("00" + "ff" * 10 + "00")
ZERO24 = bytes(24)
ZERO4 = bytes(4)

U_PATH, T1_PATH, TGT_PATH, OUT, OLD, TOOLS, MAKE_DELTA = sys.argv[1:8]
sys.path.insert(0, TOOLS)
import v4 as asm      # 조립기(= v3c 사본): build_u2c, msf_head, find_xa_extent
import mono_xa

t0 = time.time()


def log(*a):
    print("[%6.1fs]" % (time.time() - t0), *a, flush=True)


def md(b):
    return hashlib.md5(b).digest()


def sha1_file(p):
    h = hashlib.sha1()
    with open(p, "rb") as f:
        for blk in iter(lambda: f.read(1 << 22), b""):
            h.update(blk)
    return h.hexdigest()


def md5_file(p):
    h = hashlib.md5()
    with open(p, "rb") as f:
        for blk in iter(lambda: f.read(1 << 22), b""):
            h.update(blk)
    return h.hexdigest()


os.makedirs(OUT, exist_ok=True)
fU = open(U_PATH, "rb")
mU = mmap.mmap(fU.fileno(), 0, access=mmap.ACCESS_READ)
nU = len(mU) // SECT
fT = open(TGT_PATH, "rb")
mT = mmap.mmap(fT.fileno(), 0, access=mmap.ACCESS_READ)
nT = len(mT) // SECT
f1 = open(T1_PATH, "rb")
m1 = mmap.mmap(f1.fileno(), 0, access=mmap.ACCESS_READ)
n1 = len(m1) // SECT
log("U'", nU, "target", nT, "T1", n1)

# --- 색인 ---
idx_full, idx_data = {}, {}
for l in range(nU):
    s = mU[l * SECT:(l + 1) * SECT]
    idx_full.setdefault(md(s[16:]), []).append(l)
    idx_data.setdefault(md(s[24:2348]), []).append(l)
log("U' 색인", len(idx_full), len(idx_data))
idx_t1 = {}
for l in range(n1):
    s = m1[l * SECT:(l + 1) * SECT]
    idx_t1.setdefault(md(s[24:2352]), []).append(l)
log("T1 색인", len(idx_t1))

# --- mono 서명 (runs_span.json 은 v3b 것을 재사용: 런표·T1 동일) ---
spanspec = json.load(open(os.path.join(OLD, "runs_span.json"), encoding="utf-8"))
ext = asm.find_xa_extent(f1)
assert ext is not None
sigmap = {}
for cfg, (off, nsec, kind) in spanspec.items():
    if kind != "mono":
        continue
    payload = b"".join(
        m1[(ext + off + 8 * k) * SECT + 24:(ext + off + 8 * k) * SECT + 24 + 2304]
        for k in range(nsec))
    outs = mono_xa.run_to_mono_sectors(payload)
    for s, d in enumerate(outs):
        assert len(d) == 2304
        sigmap.setdefault(md(d), (int(cfg), s))
log("mono 서명", len(sigmap))

# --- 분류 ---
rows = []          # [t, n, u|None, rule, form]
subs = bytearray()
bpairs = []
mono_rec = {}
cnt = Counter()
prev = None        # (rule, u)  직전 섹터의 same/data_only 정보
for t in range(nT):
    S = mT[t * SECT:(t + 1) * SECT]
    form = "F2" if S[18] & 0x20 else "F1"
    hdr_ok = (S[0:12] == SYNC and S[12:16] == asm.msf_head(t, S[15])[12:16])
    rule, u = "new", None
    if hdr_ok:
        tail = S[16:]
        # same
        cand = None
        if prev and prev[0] == "same" and prev[1] + 1 < nU:
            pu = prev[1] + 1
            Us = mU[pu * SECT:(pu + 1) * SECT]
            if Us[15] == S[15] and Us[16:] == tail:
                cand = pu
        if cand is None:
            for c in idx_full.get(md(tail), [])[:64]:
                Us = mU[c * SECT:(c + 1) * SECT]
                if Us[15] == S[15] and Us[16:] == tail:
                    cand = c
                    break
        if cand is not None:
            rule, u = "same", cand
        elif form == "F2" and S[2348:2352] == ZERO4:
            dat = S[24:2348]
            if prev and prev[0] == "data_only" and prev[1] + 1 < nU:
                pu = prev[1] + 1
                Us = mU[pu * SECT:(pu + 1) * SECT]
                if Us[15] == S[15] and Us[24:2348] == dat:
                    cand = pu
            if cand is None:
                for c in idx_data.get(md(dat), [])[:64]:
                    Us = mU[c * SECT:(c + 1) * SECT]
                    if Us[15] == S[15] and Us[24:2348] == dat:
                        cand = c
                        break
            if cand is not None:
                rule, u = "data_only", cand
        if rule == "new" and S[15] == 2:
            if form == "F2" and not (S[19] & 1) and S[2328:2352] == ZERO24:
                loc = sigmap.get(md(S[24:2328]))
                if loc is not None:
                    rule = "mono"
                    mono_rec[str(t)] = [loc[0], loc[1], S[16:24].hex()]
            if rule == "new":
                src = None
                for c in idx_t1.get(md(S[24:2352]), [])[:64]:
                    Ts = m1[c * SECT:(c + 1) * SECT]
                    if Ts[24:2352] == S[24:2352]:
                        src = c
                        break
                if src is not None:
                    rule = "bcopy"
                    bpairs.append({"t45": t, "src_t1": src, "sub_new": S[16:24].hex(),
                                   "rule": "copy-sub-msf-rewrite", "form": form})
    cnt[(rule, form)] += 1
    if rule == "data_only":
        subs += S[16:24]
    prev = (rule, u) if rule in ("same", "data_only") else None
    # 런 병합
    mrule = "new" if rule in ("new", "mono", "bcopy") else rule
    if (rows and rows[-1][3] == mrule and rows[-1][4] == form and
            ((u is None and rows[-1][2] is None) or
             (u is not None and rows[-1][2] is not None and
              rows[-1][2] + rows[-1][1] == u))):
        rows[-1][1] += 1
    else:
        rows.append([t, 1, u, mrule, form])
log("분류 완료", dict(cnt))
log("런", len(rows), "subs", len(subs), "bcopy", len(bpairs), "mono", len(mono_rec))

with open(os.path.join(OUT, "move_runs.tsv"), "w", encoding="utf-8", newline="\n") as g:
    g.write("t45_lba\tn\tu_lba\trule\tform\n")
    for t, n, u, r, fm in rows:
        g.write("%d\t%d\t%s\t%s\t%s\n" % (t, n, "" if u is None else u, r, fm))
open(os.path.join(OUT, "subs.bin"), "wb").write(bytes(subs))
json.dump({"pairs": bpairs,
           "note": "v4: T1 일치 b-copy. src_t1 = 재료2 T1 LBA(T1 sha1 고정), rule=copy-sub-msf-rewrite"},
          open(os.path.join(OUT, "recipe_b.json"), "w", encoding="utf-8"), ensure_ascii=False)
json.dump(mono_rec, open(os.path.join(OUT, "recipe_mono.json"), "w", encoding="utf-8"))
json.dump(spanspec, open(os.path.join(OUT, "runs_span.json"), "w", encoding="utf-8"))

# --- fingerprints.json ---
fp = json.load(open(os.path.join(OLD, "fingerprints.json"), encoding="utf-8"))


def fix(o):
    if isinstance(o, str):
        return o.replace("v1.3", "v1.4")
    if isinstance(o, dict):
        return {k: fix(v) for k, v in o.items()}
    if isinstance(o, list):
        return [fix(v) for v in o]
    return o


fp = fix(fp)
fp["m1_md5"] = md5_file(U_PATH)
fp["m1_sha1"] = sha1_file(U_PATH)
fp["m1_size"] = os.path.getsize(U_PATH)
fp["m1_sectors"] = nU
fp["m1_version"] = "공식 v1.4"
# 공개판 고정 이름 (영문)
fp["files"] = {
    "m1_bin": "SOTN_Ultimate_v1.4.bin",
    "m1_cue": "SOTN_Ultimate_v1.4.cue",
    "m2_cue": "SOTN_JP_PS1_v1.2.cue",
    "m2_t1": "SOTN_JP_PS1_v1.2_T1.bin",
    "m2_t2": "SOTN_JP_PS1_v1.2_T2.ape",
}
fp["result_sha1"] = sha1_file(TGT_PATH)
fp["result_size"] = os.path.getsize(TGT_PATH)
json.dump(fp, open(os.path.join(OUT, "fingerprints.json"), "w", encoding="utf-8"),
          ensure_ascii=False, indent=1)
log("fingerprints", fp["m1_sha1"], fp["result_sha1"])

# --- U2c' + 델타 ---
if MAKE_DELTA == "1":
    u2c = os.path.join(os.path.dirname(os.path.abspath(OUT)), "work_U2c_v4.bin")
    r = asm.build_u2c(U_PATH, T1_PATH, OUT, u2c)
    log("U2c", r)
    xd = asm.XDELTA3 if os.path.isfile(asm.XDELTA3) else "xdelta3"
    delta = os.path.join(OUT, "u2c_to_t45.vcdiff")
    cp = subprocess.run([xd, "-e", "-f", "-B", "805306368", "-s", u2c, TGT_PATH, delta],
                        capture_output=True, text=True)
    log("xdelta rc", cp.returncode, cp.stderr[:300])
    log("델타 크기", os.path.getsize(delta))
    os.remove(u2c)
log("DONE")
