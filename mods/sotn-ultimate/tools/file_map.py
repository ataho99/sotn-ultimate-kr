#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""file_map.py -- "file map" of a MODE2/2352 SotN disc image (read only).

  file_map.py IMAGE.bin --out-tsv MAP.tsv [--out-md SUMMARY.md] [--root WT_ROOT]
                        [--dra DRA.BIN] [--hot STEM,STEM,...] [--check PATH:LBA:SECTORS:SIZE ...]
                        [--check-dra-offset PATH:HEXOFFSET:LBA:SIZE ...]

One TSV row per file (directories excluded); the ISO directory is read with quickpatch.walk().
Columns: path, LBA, allocated sectors (ceil(size/2048)), size, last-sector slack bytes,
next file (LBA order), empty sectors up to it (negative = overlap), sha1 (of the 2048 user bytes
per sector, cut to size), kind (xa|data), ISO directory record (dir sector LBA + record offset),
offset of the [LBA][size] u32 pair in DRA.BIN, same pair in other files, build source (build/us
same-name file, SAME/DIFF vs the image) + source folder, RAM load address (build/us/*.ld).
Nothing about a particular image is hard coded: every value comes from the image and, when
--root is given, from the source tree.  Without --root the root-dependent columns are
"unverified" (미확인).

kind: 'xa' when the extension is .STR/.XA (or the name has no extension and ends with '.'
as SD/XA_STR1.) or when any sector of the file has a Mode 2 submode with Form 2 (0x20),
audio (0x04) or video (0x02) set; else 'data'.
The image is never modified.
"""
import argparse
import hashlib
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import quickpatch  # noqa: E402

SEC = 2352
NA = "미확인"
DEFAULT_HOT = ("DRA,SEL,CEN,ST0,NO3,NO4,DRE,RCEN,LIB,MAD,BO2,BO5,BO6,RBO2,RBO6,MAR,"
               "ARC_KO,ARC_F2,KO_JP,F_GAME,F_GAME2,CENP,MARIA2")
# build/us linker script names that do not follow the stem rules below
LD_ALIAS = {"SLUS_000.67": "main"}
XA_EXT = (".STR", ".XA")
OTHER_SEARCH_MAX = 16 << 20


def ld_address(root, path):
    """first non-zero section address of build/us/<name>.ld, or None"""
    base = os.path.basename(path)
    stem = base.split(".")[0].lower()
    top = path.split("/")[0]
    cands = []
    if base in LD_ALIAS:
        cands.append(LD_ALIAS[base])
    if top == "BOSS":
        cands.append("bo" + stem)
    elif top == "ST":
        cands.append("st" + stem)
    cands.append(stem)
    for c in cands:
        p = os.path.join(root, "build", "us", c + ".ld")
        if os.path.isfile(p):
            with open(p, errors="replace") as f:
                for line in f:
                    m = re.match(r"\s*\.\w+\s+(0x[0-9A-Fa-f]+)\s*:", line)
                    if m and int(m.group(1), 16):
                        return "%s (%s.ld)" % (m.group(1).lower().replace("0x", "0x"), c)
            return None
    return None


def src_dir(root, path):
    parts = path.split("/")
    if len(parts) == 3 and parts[0] in ("BOSS", "ST") and parts[2].upper() == parts[1].upper() + ".BIN":
        d = os.path.join(root, "src", parts[0].lower(), parts[1].lower())
        if os.path.isdir(d):
            return "src/%s/%s" % (parts[0].lower(), parts[1].lower())
    return None


def read_file(f, lba, size):
    """-> (user data bytes cut to size, sha1 hex, any_xa_submode)"""
    n = (size + 2047) // 2048
    h = hashlib.sha1()
    xa = False
    chunks = []
    left = size
    done = 0
    while done < n:
        k = min(256, n - done)
        f.seek((lba + done) * SEC)
        raw = f.read(k * SEC)
        for i in range(k):
            s = raw[i * SEC:(i + 1) * SEC]
            if len(s) < SEC:
                raise SystemExit("image truncated at LBA %d" % (lba + done + i))
            if s[15] == 2 and (s[18] & 0x26):
                xa = True
            d = s[24:2072] if s[15] == 2 else s[16:2064]
            d = d[:left]
            left -= len(d)
            h.update(d)
            chunks.append(d)
        done += k
    return b"".join(chunks), h.hexdigest(), xa


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("image")
    ap.add_argument("--out-tsv", required=True)
    ap.add_argument("--out-md")
    ap.add_argument("--root", help="source tree root (has build/us and src/); omitted -> root columns unverified")
    ap.add_argument("--dra", default="DRA.BIN", help="ISO path of the file whose size table is searched")
    ap.add_argument("--hot", default=DEFAULT_HOT, help="comma separated file stems treated as frequently rebuilt")
    ap.add_argument("--check", action="append", default=[], metavar="PATH:LBA:SECTORS:SIZE",
                    help="positive control against the table (reported in the summary)")
    ap.add_argument("--check-dra-offset", action="append", default=[], metavar="PATH:HEXOFF:LBA:SIZE",
                    help="positive control: the [LBA][SIZE] pair of PATH is at this offset of --dra file")
    ap.add_argument("--image-label", default=None)
    a = ap.parse_args(argv)

    f = open(a.image, "rb")
    h = hashlib.sha1()
    for b in iter(lambda: f.read(8 << 20), b""):
        h.update(b)
    img_sha = h.hexdigest()

    tbl = quickpatch.walk(f)
    rows = []
    data = {}
    for path, e in tbl.items():
        ns = (e["size"] + 2047) // 2048
        d, sha, xa = read_file(f, e["lba"], e["size"])
        up = os.path.basename(path).upper()
        ext = os.path.splitext(up)[1]
        by_ext = ext in XA_EXT or up.endswith(".")
        kind = "xa" if (by_ext or xa) else "data"
        why = []
        if by_ext:
            why.append("ext")
        if xa:
            why.append("submode")
        rows.append(dict(path=path, lba=e["lba"], ns=ns, size=e["size"], slack=ns * 2048 - e["size"],
                         sha=sha, kind=kind, why="+".join(why), dirsec=e["dirsec"], recoff=e["recoff"]))
        if kind == "data" and e["size"] <= OTHER_SEARCH_MAX:
            data[path] = d
        if path == a.dra:
            dra_data = d
        if a.root:
            bp = os.path.join(a.root, "build", "us", os.path.basename(path))
            if os.path.isfile(bp):
                with open(bp, "rb") as g:
                    bd = g.read()
                rows[-1]["build"] = "build/us/%s %s" % (
                    os.path.basename(path),
                    "SAME" if bd == d else "DIFF(build %d B, image %d B)" % (len(bd), len(d)))
            else:
                rows[-1]["build"] = "build/us 에 같은 이름 없음"

    rows.sort(key=lambda r: (r["lba"], r["path"]))
    for i, r in enumerate(rows):
        if i + 1 < len(rows):
            n = rows[i + 1]
            r["next"] = n["path"]
            r["gap"] = n["lba"] - (r["lba"] + r["ns"])
        else:
            r["next"] = "(마지막 파일)"
            r["gap"] = ""

    if a.dra not in tbl:
        raise SystemExit("%s not in image (use --dra)" % a.dra)
    for r in rows:
        pat = struct.pack("<II", r["lba"], r["size"])
        if r["size"] == 0:
            r["dra"] = "없음(크기0)"
        else:
            offs = []
            i = dra_data.find(pat)
            while i >= 0:
                offs.append(i)
                i = dra_data.find(pat, i + 1)
            r["dra_offs"] = offs
            r["dra"] = ("0x%X" % offs[0]) if len(offs) == 1 else ("없음" if not offs else "%d회(%s)" % (
                len(offs), ",".join("0x%X" % o for o in offs[:6])))
        oth = []
        if r["size"]:
            for p, d in data.items():
                if p == a.dra or p == r["path"]:
                    continue
                i = d.find(pat)
                k = 0
                while i >= 0 and k < 4:
                    oth.append("%s:0x%X" % (p, i))
                    k += 1
                    i = d.find(pat, i + 1)
        r["others"] = ";".join(oth) if oth else "없음"
        if a.root:
            r["srcdir"] = src_dir(a.root, r["path"]) or "없음"
            ra = ld_address(a.root, r["path"])
            r["ram"] = ra or NA
            r.setdefault("build", NA)
        else:
            r["srcdir"] = NA
            r["ram"] = NA
            r["build"] = NA

    hot = set(s.strip().upper() for s in a.hot.split(",") if s.strip())
    for r in rows:
        stem = os.path.basename(r["path"]).upper().split(".")[0]
        r["hot"] = stem in hot
        reasons = []
        if r["slack"] < 64:
            reasons.append("슬랙<64B")
        if r["gap"] == 0:
            reasons.append("다음 파일과 틈0")
        r["risk"] = "+".join(reasons) if (r["hot"] and reasons) else ""

    os.makedirs(os.path.dirname(os.path.abspath(a.out_tsv)), exist_ok=True)
    hdr = ["경로", "LBA", "할당섹터", "실제크기", "마지막섹터슬랙B", "다음파일", "다음파일까지빈섹터",
           "sha1", "종류", "종류근거", "ISO디렉터리레코드(디렉터리섹터LBA:레코드오프셋)",
           "DRA크기표오프셋", "다른파일의같은쌍", "만드는소스(build/us)", "소스폴더", "RAM적재주소", "교체위험"]
    with open(a.out_tsv, "w", encoding="utf-8", newline="") as o:
        o.write("\t".join(hdr) + "\n")
        for r in rows:
            o.write("\t".join(str(x) for x in (
                r["path"], r["lba"], r["ns"], r["size"], r["slack"], r["next"], r["gap"], r["sha"], r["kind"],
                r["why"] or "없음", "%d:0x%X" % (r["dirsec"], r["recoff"]), r["dra"], r["others"], r["build"],
                r["srcdir"], r["ram"], r["risk"])) + "\n")

    if a.out_md:
        write_md(a, rows, img_sha, tbl, dra_data)
    print("files", len(rows), "tsv", a.out_tsv)
    return 0


def mdrow(cells):
    return "| " + " | ".join(str(c) for c in cells) + " |\n"


def write_md(a, rows, img_sha, tbl, dra_data):
    label = a.image_label or os.path.basename(a.image)
    tot_ns = sum(r["ns"] for r in rows)
    by = {r["path"]: r for r in rows}
    out = []
    out.append("# 파일 지도 요약\n\n")
    out.append("- 이미지: `%s`\n- sha1(전체): `%s`\n\n" % (label, img_sha))
    out.append("## (a) 총계\n\n- 총 파일 수(디렉터리 제외): %d\n- 총 할당 섹터(파일별 ceil(size/2048) 합): %d\n"
               "- 종류별: %s\n\n" % (len(rows), tot_ns, ", ".join(
                   "%s %d개" % (k, sum(1 for r in rows if r["kind"] == k)) for k in ("data", "xa"))))

    def top(title, key, rev):
        cand = [r for r in rows if r["size"] > 0]
        cand.sort(key=lambda r: (r["slack"], r["path"]), reverse=rev)
        out.append("### %s\n\n" % title)
        out.append(mdrow(["경로", "LBA", "섹터", "크기", "슬랙B", "다음 파일", "빈 섹터"]))
        out.append(mdrow(["---"] * 7))
        for r in cand[:20]:
            out.append(mdrow([r["path"], r["lba"], r["ns"], r["size"], r["slack"], r["next"], r["gap"]]))
        out.append("\n")

    out.append("## (b) 슬랙 순위 (크기 0 파일 제외)\n\n")
    top("슬랙 큰 순 상위 20", "slack", True)
    top("슬랙 작은 순 상위 20", "slack", False)

    out.append("## (c) 교체 위험 파일\n\n")
    out.append("기준: 이번 빌드에서 자주 바뀌는 오버레이(`--hot`: %s) 중 슬랙 < 64B 이거나 다음 파일과 틈 0 인 것. "
               "새 파일이 섹터를 더 쓰면 그 자리에 못 들어간다(quickpatch 는 new_sectors <= old_sectors 만 허용).\n\n"
               % a.hot)
    risk = [r for r in rows if r["risk"]]
    out.append(mdrow(["경로", "LBA", "섹터", "슬랙B", "다음 파일", "빈 섹터", "위험 사유"]))
    out.append(mdrow(["---"] * 7))
    for r in risk:
        out.append(mdrow([r["path"], r["lba"], r["ns"], r["slack"], r["next"], r["gap"], r["risk"]]))
    hotrows = [r for r in rows if r["hot"]]
    out.append("\n해당 스템에 맞는 파일 %d개 중 위험 표시 %d개. 위험 표시 아닌 대상: %s\n\n" % (
        len(hotrows), len(risk), ", ".join("%s(슬랙%d,틈%s)" % (r["path"], r["slack"], r["gap"])
                                          for r in hotrows if not r["risk"]) or "없음"))
    neg = [r for r in rows if r["gap"] != "" and r["gap"] < 0]
    out.append("extent 겹침(틈 < 0) 파일: %s\n\n" % (", ".join("%s(%d)" % (r["path"], r["gap"]) for r in neg) or "없음"))

    out.append("## 검증 (양성 대조)\n\n")
    ok_all = True
    if not a.check and not a.check_dra_offset:
        out.append("지정된 대조값 없음.\n\n")
    for c in a.check:
        p, lba, ns, sz = c.split(":")
        r = by.get(p)
        if not r:
            out.append("- %s: 표에 없음 -> 불일치\n" % p)
            ok_all = False
            continue
        good = (r["lba"], r["ns"], r["size"]) == (int(lba), int(ns), int(sz))
        ok_all &= good
        out.append("- %s 기대 LBA %s/%s섹터/크기 %s, 표 LBA %d/%d섹터/크기 %d -> %s\n" % (
            p, lba, ns, sz, r["lba"], r["ns"], r["size"], "일치" if good else "**불일치**"))
    for c in a.check_dra_offset:
        p, off, lba, sz = c.split(":")
        r = by.get(p)
        pat = struct.pack("<II", int(lba), int(sz))
        want = int(off, 16)
        got = dra_data[want:want + 8] == pat
        got_size = (not got) and want >= 4 and dra_data[want - 4:want + 4] == pat
        ok_all &= (got or got_size)
        res = ("일치(쌍 시작 = 지정 오프셋)" if got else
               "쌍 시작은 0x%X, 지정 오프셋 0x%X 는 그 쌍의 크기 필드(+4)" % (want - 4, want) if got_size else "**불일치**")
        out.append("- %s: %s 에서 [LBA %s][크기 %s] 쌍, 지정 오프셋 0x%X -> %s; 표 DRA열(쌍 시작) = %s, 다른파일열 = %s\n" % (
            p, a.dra, lba, sz, want, res, r["dra"] if r else "?", r["others"] if r else "?"))
        # same pair as seen from the table of that file itself
    out.append("\n종합: %s\n\n" % ("전부 일치" if ok_all else "**불일치 있음**"))

    out.append("## (d) 판단 근거와 못 채운 열\n\n")
    xa_rows = [r for r in rows if r["kind"] == "xa"]
    out.append("- 종류: 확장자 .STR/.XA 또는 이름이 '.'로 끝남(SD/XA_STR1.), 또는 파일 섹터 중 하나라도 Mode 2 submode 의 Form2(0x20)/audio(0x04)/video(0x02) 비트가 켜진 경우 'xa'. xa %d개: %s. "
               "근거 열: ext/submode.\n" % (len(xa_rows), ", ".join("%s[%s]" % (r["path"], r["why"]) for r in xa_rows)))
    out.append("- sha1: 섹터 사용자 데이터 2048B 를 이어 붙여 실제 크기에서 자른 값(Form2 섹터는 2324B 중 앞 2048B 만 해당). 디렉터리 레코드 크기와 같은 기준.\n")
    out.append("- 할당 섹터는 ceil(크기/2048). 다음 파일은 LBA 정렬 순 바로 다음 항목(동일 LBA 는 경로순), 빈 섹터 = 다음 LBA - (LBA + 할당). 디렉터리·시스템 영역·CD-DA 는 '다음 파일' 계산에 들지 않으므로 마지막 파일 뒤 빈 공간은 알 수 없음.\n")
    out.append("- DRA 열: %s 의 데이터에서 struct.pack('<II', LBA, size) 를 바이트 검색(비정렬 포함), 1회면 오프셋, 0회 '없음', 여러 회 'N회(오프셋…)'. 크기 0 파일은 검색하지 않음.\n" % a.dra)
    out.append("- 다른파일의같은쌍: 이미지 내 종류 data·크기 16MB 이하 파일(자기 자신·%s 제외)에서 같은 쌍 검색, 파일별 최대 4곳 표기. xa 스트림과 16MB 초과 파일은 검색 안 함.\n" % a.dra)
    if a.root:
        nb = sum(1 for r in rows if r["build"].endswith("SAME"))
        nd = sum(1 for r in rows if "DIFF" in r["build"])
        nn = sum(1 for r in rows if "없음" in r["build"])
        out.append("- 만드는 소스: --root 의 build/us/<파일 기본이름> 과 이미지 바이트 비교. SAME %d / DIFF %d / 같은 이름 없음 %d. 빌드 산출물은 같은 이름으로만 대응시키며 다른 디렉터리의 동명 파일은 구분하지 않음. 소스폴더는 BOSS/<X>/<X>.BIN, ST/<X>/<X>.BIN 에 대해 src/boss|st/<x>/ 가 있을 때만 적음, 그 외 '없음'.\n" % (nb, nd, nn))
        nram = sum(1 for r in rows if r["ram"] == NA)
        out.append("- RAM 적재 주소: build/us/<bo|st><이름>.ld, 또는 <이름>.ld (SLUS_000.67 은 main.ld) 의 첫 비0 섹션 시작. 대응 .ld 가 없어 '미확인' 인 파일 %d개 (F_*, SD_*, XA, ARC_*, MARIA* 등 데이터·벡터 포함; WEAPON*, KO 같은 이름 불일치 파일 포함, 추측으로 채우지 않음).\n" % nram)
        miss = [r["path"] for r in rows if r["ram"] == NA and r["path"].upper().endswith(".BIN") and not os.path.basename(r["path"]).startswith("F_")]
        out.append("  - .BIN 중 RAM 미확인: %s\n" % (", ".join(miss) or "없음"))
    else:
        out.append("- --root 미지정: 만드는 소스·소스폴더·RAM 적재 주소 열은 전부 '미확인'.\n")
    out.append("- 교체위험 열은 TSV 마지막 열에도 있음(빈 값 = 위험 표시 없음).\n")
    with open(a.out_md, "w", encoding="utf-8") as o:
        o.write("".join(out))


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
