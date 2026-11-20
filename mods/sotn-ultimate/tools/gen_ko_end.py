#!/usr/bin/env python3
# gen_ko_end.py — ko_end ending/shop KO byte bake (trial builds only).
# 입력: 한글작업/번역/번역_v13_통합.tsv (sel_ending:0-29, e_shop.c:D_us_80181310:0-5)
#       + P4시험/ko_glyph_map_*.tsv (프로필 map_name) (ARC_KO 776자, KO_GLYPH_BASE 0xF040).
# 출력: stdout 검증 리포트. --emit-shop 검증용 C 리터럴, --emit-end YAML BYTE 단편.
# sel 대사 '/'는 줄바꿈 표시(BR재생성)이므로 TEXT 분절점으로 취급, 바이트열에는 넣지 않는다.
# 생성물(gen/*.h, assets YAML)에는 쓰지 않는다 — 빌드 턴에서 YAML 적용 후 풀빌드 검증.
import io
from ko_paths import KO_WK, KO_REPO  # noqa: E402
import os
import sys

REPO = KO_REPO
WK = KO_WK
TSV = (KO_WK + "/검수/번역_v14.tsv")
MAP = os.path.join(WK, "P4시험", "" + __import__("ko_font_profile").P["map_name"] + "")


def load_map():
    mp = {}
    for line in io.open(MAP, encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        if len(p) >= 2 and len(p[0]) == 1 and "가" <= p[0] <= "힣":
            mp[p[0]] = int(p[1], 16)
    assert len(mp) >= 700, len(mp)  # glyph-map size sanity check
    return mp


def enc_bytes(s, mp):
    out = bytearray()
    s = s.replace('\u2026', '...')
    for ch in s:
        o = ord(ch)
        if o < 128:
            out.append(o)
        else:
            assert "가" <= ch <= "힣", repr(ch)
            code = mp[ch]
            out.append((code >> 8) & 0xFF)
            out.append(code & 0xFF)
    return bytes(out)


def c_lit(b):
    return "".join("\\x%02X" % x for x in b)


def load_rows():
    end, shop = {}, {}
    for line in io.open(TSV, encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        if len(p) < 4 or not p[0]:
            continue
        if p[0].startswith("sel_ending:"):
            end[p[0]] = p[3]
        elif p[0].startswith("e_shop.c:D_us_80181310:"):
            shop[p[0]] = p[3]
    return end, shop


def main():
    mp = load_map()
    end, shop = load_rows()
    assert len(end) == 30, len(end)
    assert len(shop) == 6, len(shop)
    # 상점: 선행 제어 1B(원판 x좌표) + KO 바이트. e_shop.c 커밋 리터럴과 대조용.
    shop_ctrl = ["\x07", "\x07", "\x06", "\x01", "\x06", "\x04"]  # 0.9.2: centred in the 21-hw cell
    print("shop rows=%d" % len(shop))
    for i in range(6):
        key = "e_shop.c:D_us_80181310:%d" % i
        b = enc_bytes(shop[key], mp)
        lead = ord(shop_ctrl[i])
        # 제어 충돌 검사: KO lead F0-F3은 ASCII/제어 대역과 겹치지 않는다
        assert all(x < 0x80 or 0xF0 <= x <= 0xF3 for x in b[0:1]), key
        print("  [%d] %s bytes=%d lit=\"%s%s\"" % (i, shop[key], len(b), "\\x%02X" % lead, c_lit(b)))
    # 엔딩: '/' 분절별 바이트 수. 행당 한글 수/제어 충돌 검사.
    print("end rows=%d" % len(end))
    total = 0
    for i in range(30):
        key = "sel_ending:%d" % i
        segs = end[key].split("/")
        nbytes = 0
        for s in segs:
            b = enc_bytes(s, mp)
            nbytes += len(b)
        total += nbytes
        print("  [%d] segs=%d bytes=%d" % (i, len(segs), nbytes))
    print("end total KO bytes=%d" % total)
    if "--emit-shop" in sys.argv:
        for i in range(6):
            key = "e_shop.c:D_us_80181310:%d" % i
            sys.stdout.write('    "\\x%02X%s",\n' % (ord(shop_ctrl[i]), c_lit(enc_bytes(shop[key], mp))))
    if "--emit-end" in sys.argv:
        for i in range(30):
            key = "sel_ending:%d" % i
            sys.stdout.write("  # %s\n" % key)
            for s in end[key].split("/"):
                b = enc_bytes(s, mp)
                sys.stdout.write("  - [BYTE, %s]\n" % ", ".join("0x%02X" % x for x in b))
                sys.stdout.write("  - [LINE_BREAK]\n")


if __name__ == "__main__":
    main()
