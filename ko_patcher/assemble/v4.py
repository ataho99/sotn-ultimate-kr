#!/usr/bin/env python3
"""조립 도구 v4 — U 공식 v1.4 + 일본 PS1 v1.2 원판(T1+T2) + 꾸러미_v3 -> 한글 베타1.

v2 대비 변경 2건:
  (a) b-copy 확대 137 -> 1784 (T1 /LOGO15XA.STR verbatim 1,647 추가).
      재료 LBA는 ISO 파일+섹터인덱스로 해결 (런타임 ISO 탐색, LBA 고정 없음).
  (b) mono 삽입 20B shift 버그 수정: 모듈 출력 2304B를
      [o+24:o+2328]에 넣고 [o+2328:o+2348] 20B 0패딩 + EDC 0.
      v2는 2304B를 2324B 슬롯에 대입해 bytearray가 섹터당 20B씩 줄어
      U2c 전체가 어긋나 있었다 (9378*20=187,560B, 실측 일치).
      델타는 같은 소스로 만들고 적용하므로 v2 시험은 통과했으나
      델타가 불필요하게 컸다. v3 소스는 섹터 정렬 정상 (691,353,936B).

흐름: 재료 검사(리젝트) -> U 재배치+헤더 재계산(U2c) -> b-copy 1784 삽입
  -> mono-ize 9,378 생성·삽입(v3 수정) -> xdelta 적용(외부 xdelta3)
  -> 결과 sha1 검증.
재배치 규칙(v1과 동일, 오라클 전수 검증):
  same      = U[16:2352] 그대로 + sync/MSF(t45) 재계산 (EDC/ECC는 MSF 무관 — 복사)
  data_only = 서브헤더(subs.bin) + U 데이터 + Form2 EDC 0 규칙(전수 0 확인)
              Form1은 표준 EDC/ECC 재계산(해당 0건, 경로만 유지)
  new       = 0 채움(델타가 운반)
툴체인 사용 없음(표준 라이브러리만; numpy 없음). xdelta3 외부 바이너리 필요.
"""
import hashlib
import json
import mmap
import os
import shutil
import subprocess
import sys

SECT = 2352
SYNC = bytes.fromhex("00" + "ff" * 10 + "00")
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import iso9660
import mono_xa

# xdelta 3.2.2 바이너리를 도구 폴더 상대경로로 사용 (v3b 방식).
XDELTA3 = os.path.normpath(os.path.join(HERE, "..", "_도구", "xdelta31", "xdelta3"))
# 배포 시: PATH의 xdelta3 또는 동봉 안내. 버전·조건은 README_v3.md에 명시.


def find_file_extent(f, path_parts):
    """재료2 T1에서 ISO 경로(path_parts 리스트, 대소문자 무시)의 extent 반환.

    LBA 고정 금지: recipe의 iso.file + sector_index를
    런타임에 extent로 환산한다. 없으면 None.
    """
    pvd = iso9660.pvd(f)
    if pvd is None:
        return None
    import struct

    def find_in(lba, want):
        s = iso9660.read_sector(f, lba)
        u = s[24:24 + 2048]
        off = 0
        while off < 2048:
            ln = u[off]
            if ln == 0:
                break
            rec = u[off:off + ln]
            nlen = rec[32]
            nm = bytes(rec[33:33 + nlen]).decode("ascii", "replace")
            if nm.split(";")[0].upper() == want.upper():
                return (struct.unpack("<I", rec[2:6])[0],
                        struct.unpack("<I", rec[10:14])[0],
                        rec[25])
            off += ln
        return None

    root = iso9660.list_dir(f, pvd["root_lba"], 1)
    cur = None
    want0 = path_parts[0].upper()
    for nm, elba, size, flags in root:
        if bytes(nm).decode("ascii", "replace").split(";")[0].upper() == want0:
            cur = (elba, size, flags)
            break
    if cur is None:
        return None
    for part in path_parts[1:]:
        elba, size, flags = cur
        if flags != 2:
            return None
        import math
        nsec = max(1, (size + 2047) // 2048)
        got = None
        for k in range(nsec):
            r = find_in(elba + k, part)
            if r is not None:
                got = r
                break
        if got is None:
            return None
        cur = got
    elba, size, flags = cur
    if flags == 2:
        return None
    return elba


def find_xa_extent(f):
    """하위 호환용: SD/XA_STR1. extent (find_file_extent 래퍼)."""
    return find_file_extent(f, ["SD", "XA_STR1."])


def bcd(v):
    return ((v // 10) << 4) | (v % 10)


def msf_head(t45, mode=0x02):
    f = t45 + 150
    return SYNC + bytes([bcd(f // 75 // 60), bcd((f // 75) % 60), bcd(f % 75), mode])


# --- 표준 EDC/ECC (CD-ROM 표준 알고리즘) ---
_EDC_LUT = None
_ECC_F = None
_ECC_B = None


def _init_tables():
    global _EDC_LUT, _ECC_F, _ECC_B
    if _EDC_LUT is not None:
        return
    _EDC_LUT = [0] * 256
    _ECC_F = [0] * 256
    _ECC_B = [0] * 256
    for i in range(256):
        j = (i << 1) ^ (0x11D if i & 0x80 else 0)
        _ECC_F[i] = j & 0xFF
        _ECC_B[j & 0xFF] = i
        edc = i
        for _ in range(8):
            edc = (edc >> 1) ^ (0xD8018001 if edc & 1 else 0)
        _EDC_LUT[i] = edc & 0xFFFFFFFF


def edc_bytes(data):
    """data 바이트열의 EDC 4B (리틀엔디안)."""
    _init_tables()
    edc = 0
    lut = _EDC_LUT
    for b in data:
        edc = (edc >> 8) ^ lut[(edc ^ b) & 0xFF]
    return bytes([edc & 0xFF, (edc >> 8) & 0xFF, (edc >> 16) & 0xFF, (edc >> 24) & 0xFF])


def eccedc_form1(sec):
    """Mode2 Form1 섹터(bytearray 2352)의 EDC+ECC 재계산. (해당 0건)"""
    sec[0x818:0x81C] = edc_bytes(sec[0x10:0x818])
    _init_tables()
    f = _ECC_F
    b = _ECC_B
    tmp = bytearray(sec)
    for i in range(4):
        tmp[12 + i] = 0  # 주소 0으로 계산
    # P 코드
    for major in range(86):
        index = (major >> 1) * 2 + (major & 1)
        ea = eb = 0
        for _ in range(24):
            t = tmp[12 + index]
            index += 86
            if index >= 86 * 24:
                index -= 86 * 24
            ea ^= t
            eb ^= t
            ea = f[ea]
        ea = b[f[ea] ^ eb]
        sec[0x81C + major] = ea
        sec[0x81C + major + 86] = ea ^ eb
    # Q 코드
    for major in range(52):
        index = (major >> 1) * 86 + (major & 1)
        ea = eb = 0
        for _ in range(43):
            t = tmp[12 + index]
            index += 88
            if index >= 52 * 43:
                index -= 52 * 43
            ea ^= t
            eb ^= t
            ea = f[ea]
        ea = b[f[ea] ^ eb]
        sec[0x8C8 + major] = ea
        sec[0x8C8 + major + 52] = ea ^ eb


def sha1_file(path):
    h = hashlib.sha1()
    with open(path, "rb") as f:
        for blk in iter(lambda: f.read(1 << 20), b""):
            h.update(blk)
    return h.hexdigest()


def fail(kind, msg):
    print(f"REJECT {kind}: {msg}")
    return 2


def check_materials(matdir, fp):
    for key in ("m1_cue", "m1_bin", "m2_cue", "m2_t1", "m2_t2"):
        p = os.path.join(matdir, fp["files"][key])
        if not os.path.isfile(p):
            return fail("없음",
                        f"{fp['files'][key]}이(가) 없습니다. {fp['need'][key]}")
    r = []
    # 1번: 공식 v1.3 고정
    p1 = os.path.join(matdir, fp["files"]["m1_bin"])
    with open(p1, "rb") as f:
        nsec = os.fstat(f.fileno()).st_size // SECT
    if nsec != fp["m1_sectors"]:
        return fail("지문다름",
                    f"1번 이미지 섹터 수 {nsec} != 기대 {fp['m1_sectors']}. "
                    f"얼티밋 공식 방법·{fp['m1_version']}로 만든 이미지가 필요합니다.")
    h1 = sha1_file(p1)
    if h1 != fp["m1_sha1"]:
        return fail("지문다름",
                    f"1번 이미지 sha1 {h1[:12]}… != 기대 {fp['m1_sha1'][:12]}…. "
                    f"얼티밋 공식 방법·{fp['m1_version']}로 만든 이미지가 필요합니다.")
    cue1 = open(os.path.join(matdir, fp["files"]["m1_cue"]), encoding="utf-8",
                errors="replace").read()
    if cue1.count("TRACK ") != fp["m1_tracks"]:
        return fail("지문다름",
                    f"{fp['files']['m1_cue']} 트랙 수 != 기대 {fp['m1_tracks']}. "
                    f"{fp['need']['m1_cue']}")
    # 2번: v1.2 원판 (T1 bin + T2 ape + 2트랙 cue)
    t1 = os.path.join(matdir, fp["files"]["m2_t1"])
    sz1 = os.path.getsize(t1)
    if sz1 != fp["m2_t1_size"]:
        return fail("지문다름",
                    f"2번 T1 크기 {sz1} != 기대 {fp['m2_t1_size']}. "
                    f"{fp['need']['m2_t1']}")
    h2 = sha1_file(t1)
    if h2 != fp["m2_t1_sha1"]:
        return fail("지문다름",
                    f"2번 T1 sha1 {h2[:12]}… != 기대 {fp['m2_t1_sha1'][:12]}…. "
                    f"v1.2 판만 받습니다. {fp['need']['m2_t1']}")
    t2 = os.path.join(matdir, fp["files"]["m2_t2"])
    sz2 = os.path.getsize(t2)
    if sz2 != fp["m2_t2_size"]:
        return fail("지문다름",
                    f"2번 T2 크기 {sz2} != 기대 {fp['m2_t2_size']}. "
                    f"v1.2 판만 받습니다. {fp['need']['m2_t2']}")
    if sha1_file(t2) != fp["m2_t2_sha1"]:
        return fail("지문다름",
                    f"2번 T2 sha1 != 기대. v1.2 판만 받습니다. {fp['need']['m2_t2']}")
    cue2 = open(os.path.join(matdir, fp["files"]["m2_cue"]), encoding="utf-8",
                errors="replace").read()
    if cue2.count("TRACK ") != fp["m2_tracks"]:
        return fail("지문다름",
                    f"{fp['files']['m2_cue']} 트랙 수 != 기대 {fp['m2_tracks']}. "
                    f"v1.2 판만 받습니다. {fp['need']['m2_cue']}")
    with open(t1, "rb") as f:
        pvd = iso9660.pvd(f)
    if pvd is None:
        return fail("지문다름",
                    f"2번 T1에서 ISO9660 PVD를 못 읽었습니다. v1.2 판만 받습니다.")
    r.append(f"m1 sha1 {h1} sectors {nsec}")
    r.append(f"m2t1 sha1 {h2} size {sz1} vol {pvd['vol_id']}")
    return r


def build_u2c(u_path, m2_t1, pkgdir, out_path):
    """U + move_runs.tsv + subs.bin -> U2c (b-copy 1784 + mono-ize 9,378)."""
    with open(os.path.join(pkgdir, "move_runs.tsv"), encoding="utf-8") as g:
        header = g.readline()
        assert header.startswith("t45_lba"), header
        runs = [l.rstrip("\n").split("\t") for l in g]
    subs = open(os.path.join(pkgdir, "subs.bin"), "rb").read()
    f = open(u_path, "rb")
    mU = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
    nU = len(mU) // SECT
    # T45 섹터 수 = runs 끝
    nT = 0
    for p in runs:
        nT = max(nT, int(p[0]) + int(p[1]))
    out = bytearray(nT * SECT)
    si = 0
    for p in runs:
        t, n = int(p[0]), int(p[1])
        rule, form = p[3], p[4]
        has_u = p[2] != ""
        u = int(p[2]) if has_u else -1
        for k in range(n):
            tl = t + k
            o = tl * SECT
            if has_u and form in ("F1", "F2"):
                ul = u + k
                assert ul < nU, (tl, ul)
                S = mU[ul * SECT:(ul + 1) * SECT]
                out[o:o + 16] = msf_head(tl, S[15])
                if rule == "same":
                    out[o + 16:o + SECT] = S[16:SECT]
                elif form == "F2":
                    # subs.bin 서브헤더 + U 데이터 + EDC 0 (전수 0 확인)
                    out[o + 16:o + 24] = subs[si:si + 8]
                    si += 8
                    out[o + 24:o + 2348] = S[24:2348]
                    out[o + 2348:o + 2352] = b"\0\0\0\0"
                else:
                    # Form1 data_only (0건): 표준 재계산
                    sec = bytearray(SECT)
                    sec[0:16] = msf_head(tl, S[15])
                    sec[16:24] = subs[si:si + 8]
                    si += 8
                    sec[24:2072] = S[24:2072]
                    eccedc_form1(sec)
                    out[o:o + SECT] = sec
            else:
                for b in range(o, o + SECT):
                    out[b] = 0
    assert si == len(subs), (si, len(subs))
    # b-copy 1784 삽입 (기존 137 + LOGO15XA 1,647).
    # 재료 위치는 ISO 파일+섹터인덱스로 해결 (런타임 ISO 탐색, LBA 고정 없음).
    # src_t1은 폴백용으로만 보관.
    recipe = json.load(open(os.path.join(pkgdir, "recipe_b.json"), encoding="utf-8"))
    f2 = open(m2_t1, "rb")
    m2 = mmap.mmap(f2.fileno(), 0, access=mmap.ACCESS_READ)
    n2 = len(m2) // SECT
    nb = 0
    ext_cache = {}
    for pr in recipe["pairs"]:
        t45 = pr["t45"]
        newsub = bytes.fromhex(pr["sub_new"])
        iso = pr.get("iso")
        src = None
        if iso is not None and iso.get("file"):
            fp = iso["file"]
            if fp not in ext_cache:
                parts = [p for p in fp.split("/") if p]
                ext_cache[fp] = find_file_extent(f2, parts)
            ext = ext_cache[fp]
            if ext is not None:
                if "sector_index" in iso:
                    src = ext + iso["sector_index"]
                else:
                    # 구 137 형식: t1_lba - extent = sector_index
                    src = pr["src_t1"]
                    assert src >= ext, (t45, src, ext)
        if src is None:
            src = pr["src_t1"]  # 폴백: recipe 기록 LBA
        assert src < n2, (t45, src)
        if pr.get("rule") == "copy-sub-msf-rewrite":
            pass  # sub 동일 verbatim (assert 없음)
        else:
            assert newsub[2] & 0x20
        o = t45 * SECT
        out[o:o + 16] = msf_head(t45)
        out[o + 16:o + 24] = newsub
        out[o + 24:o + 2348] = m2[src * SECT + 24:src * SECT + 2348]
        out[o + 2348:o + 2352] = m2[src * SECT + 2348:src * SECT + 2352]
        nb += 1
    # mono-ize 9,378 생성·삽입 (mono_xa + recipe_mono.json).
    # 입력 = 재료2 T1의 SD/XA_STR1. (extent는 ISO에서 탐색, LBA 고정 금지).
    mono = json.load(open(os.path.join(pkgdir, "recipe_mono.json"),
                           encoding="utf-8"))
    spanspec = json.load(open(os.path.join(pkgdir, "runs_span.json"),
                              encoding="utf-8"))
    f2 = open(m2_t1, "rb")
    m2 = mmap.mmap(f2.fileno(), 0, access=mmap.ACCESS_READ)
    n2 = len(m2) // SECT
    ext = find_xa_extent(f2)
    f2.seek(0)
    if ext is None:
        f.close()
        print("REJECT 지문다름: 2번 T1에서 SD/XA_STR1. extent를 못 찾았습니다. "
              "v1.2 판만 받습니다.")
        return 2
    cache = {}
    nm = 0
    for t45s, loc in mono.items():
        t45 = int(t45s)
        cfg, s = loc[0], loc[1]
        sub = bytes.fromhex(loc[2])
        if cfg not in cache:
            off, nsec, _kind = spanspec[str(cfg)]
            payload = b"".join(
                m2[(ext + off + 8 * k) * SECT + 24:
                   (ext + off + 8 * k) * SECT + 24 + 2304]
                for k in range(nsec))
            assert ext + off + 8 * (nsec - 1) < n2, (cfg, off, nsec)
            cache[cfg] = mono_xa.run_to_mono_sectors(payload)
        data = cache[cfg][s]
        assert len(data) == 2304, (t45, len(data))  # 2304B 고정
        o = t45 * SECT
        out[o:o + 16] = mono_xa.msf_head(t45)
        out[o + 16:o + 24] = sub
        out[o + 24:o + 2328] = data
        out[o + 2328:o + 2348] = bytes(20)  # F2 spare 20B 0 (전수 0 확인)
        out[o + 2348:o + 2352] = b"\0\0\0\0"  # F2 EDC 0 (전수 확인)
        nm += 1
    f2.close()
    with open(out_path, "wb") as g:
        g.write(out)
    f.close()
    return [f"u2c built: {nT} sectors, b-insert {nb} ok, mono-insert {nm} ok"]


def main(argv):
    if len(argv) != 4:
        print("usage: v4.py <재료폴더> <꾸러미폴더> <출력bin>")
        return 2
    matdir, pkgdir, outbin = argv[1], argv[2], argv[3]
    fp = json.load(open(os.path.join(pkgdir, "fingerprints.json"), encoding="utf-8"))
    delta = [d for d in (os.path.join(pkgdir, "u2c_to_t45.vcdiff"),
                         os.path.join(pkgdir, "u2b_to_t45.vcdiff"),
                         os.path.join(pkgdir, "uprime_to_t45.vcdiff"))
             if os.path.isfile(d)]
    if not delta:
        print("REJECT 없음: 꾸러미에 델타가 없습니다.")
        return 2
    delta = delta[0]
    r = check_materials(matdir, fp)
    if isinstance(r, int):
        return r
    for line in r:
        print("OK", line)
    work = outbin + ".work_U2c.bin"
    r2 = build_u2c(os.path.join(matdir, fp["files"]["m1_bin"]),
                   os.path.join(matdir, fp["files"]["m2_t1"]), pkgdir, work)
    if isinstance(r2, int):
        return r2
    for line in r2:
        print("OK", line)
    xd = XDELTA3 if os.path.isfile(XDELTA3) else shutil.which("xdelta3")
    if not xd:
        print("REJECT 없음: xdelta3 바이너리가 없습니다(README_v3.md 참조).")
        return 2
    cp = subprocess.run([xd, "-d", "-s", work, delta, outbin],
                        capture_output=True, text=True)
    if cp.returncode != 0:
        print(f"REJECT 적용실패: xdelta 적용 실패(rc={cp.returncode}): "
              f"어느 단계·구간에서 달랐는지 제보 양식으로 보내주십시오. {cp.stderr[:300]}")
        return 2
    h = sha1_file(outbin)
    os.remove(work)
    if h != fp["result_sha1"]:
        print(f"REJECT 불일치: 결과 sha1 {h} != 기대 {fp['result_sha1']}. "
              "어느 단계·구간에서 달랐는지 제보 양식으로 보내주십시오.")
        return 2
    print(f"OK result sha1 {h} == 기대치. 합격.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
