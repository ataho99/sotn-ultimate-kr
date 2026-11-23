#!/usr/bin/env python3
# gen_ko_sound.py -- Korean strings of the Sound Mode screen (Player Select overlay SEL) ->
#   mods/sotn-ultimate/src/menu/mod_sound_ko_str.h
# Input : ko_data/번역/번역_사운드모드.tsv (번호, 영문, 한국어 제목(비우면 영어), 설명)
# Encoding = SEL's 12px band path (func_801B2608 -> func_801B2108, MOD_KO): Hangul = glyph code F0xx big endian
# (ko_glyph_map), ASCII raw (engtab cells), space 0x20; SND_PROMO=1: a Hangul that is in ko_promo_map.tsv
# (the cutscene 1-byte promotion table, resident at 0x8009F000) is its 1 byte 0x80+slot (func_801B2108 decodes it while
# g_ModSndPromo is set). SND_OFFS=1: the descriptions are one blob + u16 offsets. SND_DESC=0: no descriptions.
# One band holds 144 bytes = 24 Hangul advances; fail closed beyond that.
# The output also holds g_ModSoundTracks (name = the display string; ids read from the English table in
# 2C048_mod_player_select.h) so the KO build keeps no second copy of the names.
import io
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ko_paths import KO_WK  # noqa: E402
import ko_font_profile as FP  # noqa: E402

P4 = KO_WK + "/P4시험/"
TSV = KO_WK + "/번역/번역_사운드모드.tsv"
REPO = os.environ.get("SOTN_KR", os.getcwd())
OUT = os.path.join(REPO, "mods/sotn-ultimate/src/menu/mod_sound_ko_str.h")
SRC = os.path.join(REPO, "mods/sotn-ultimate/src/menu/2C048_mod_player_select.h")
PROMO = os.environ.get("SND_PROMO", "0") == "1"
OFFS = os.environ.get("SND_OFFS", "0") == "1"
DESC = os.environ.get("SND_DESC", "1") == "1"


def rd(path):
    m = {}
    for ln in io.open(path, encoding="utf-8"):
        q = ln.rstrip(chr(10)).split(chr(9))
        if len(q) >= 2 and len(q[0]) == 1:
            try:
                m[q[0]] = int(q[1], 16)
            except ValueError:
                pass
    return m


def is_h(c):
    return "가" <= c <= "힣"


MP = rd(P4 + FP.P["map_name"])
PM = {}
for ln in io.open(P4 + "ko_promo_map.tsv", encoding="utf-8").read().split(chr(10))[1:]:
    q = ln.split(chr(9))
    if len(q) >= 3 and q[0]:
        PM[q[0]] = int(q[1], 16)
        assert MP[q[0]] == int(q[2], 16), ("promo map and glyph map disagree", q[0])


def enc(s, what):
    b = bytearray()
    w = 0
    for ch in s:
        if is_h(ch):
            assert ch in MP, ("no glyph code", ch, what)
            if PROMO and ch in PM:
                b.append(PM[ch])
            else:
                b += bytes([MP[ch] >> 8, MP[ch] & 0xFF])
        else:
            assert 0x20 <= ord(ch) < 0x7F, ("not ASCII / Hangul", ch, what)
            b.append(ord(ch))
        w += 6
    assert w <= 144, ("band too wide (bytes of 12px advance)", w, what)
    return bytes(b)


def esc(b):
    return "".join("\\x%02X" % x for x in b)


def main():
    rows = [l.rstrip(chr(10)).split(chr(9)) for l in io.open(TSV, encoding="utf-8")][1:]
    assert len(rows) == 44, len(rows)
    src = io.open(SRC, encoding="utf-8").read()
    ids = re.findall(r'^    \{_S\("[^"]*"\), ([A-Za-z0-9_| ]+)\},', src, re.M)
    assert len(ids) == 44, len(ids)
    out = ["// gen_ko_sound.py: Sound Mode strings (do not edit)", "#define MOD_SND_DESC %d" % (1 if DESC else 0),
           "#define MOD_SND_PROMO %d" % (1 if PROMO else 0)]
    ARC = os.environ.get("SND_ARC", "dt")  # "0" none, "d" descriptions, "dt" descriptions + titles go to the ARC A-gap zero runs
    # zero runs of the A-gap (ARC_KO.BIN, resident from the title on); the build step asserts they are still zero
    # 0x8009F150 (+176) is NOT used: cutscene_actor_name.h writes the cutscene name glyphs (96 B each) from 0x8009F000 up,
    # which reaches it with a name of 4+ Hangul (the SEL endings draw such names)
    # 0x8009F180 (+128): the name glyph copy reaches 0x8009F000 + 4 * 0x60 = 0x8009F17F (names are at most 4 Hangul = 8 bytes,
    # asserted below); 0x8009F150..F17F (48 B) stays unused
    RUNS = [(0x8009EFBC, 68), (0x8009F180, 128), (0x8009F32F, 81), (0x8009F39B, 101), (0x8009F415, 107),
            (0x8009F4B1, 79), (0x8009FEFC, 68), (0x8009FF48, 184)]
    items = []  # (key, bytes)
    enc_t, enc_d = {}, {}
    for r in rows:
        n = int(r[0])
        title = r[2] if len(r) > 2 and r[2] else r[1]
        desc = r[3] if len(r) > 3 else ""
        enc_t[n] = enc(title, "title%d" % n)
        enc_d[n] = enc(desc, "desc%d" % n) if (desc and DESC) else b""
    placed = {}
    patches = []
    if ARC != "0":
        free = [[a, l] for a, l in RUNS]
        free[0][0] += 1  # the first byte stays a NUL: the empty string every "no description" points at
        free[0][1] -= 1
        want = [("d%02d" % n, e + b"\0") for n, e in enc_d.items() if e]
        if ARC == "dt":
            want += [("t%02d" % n, e + b"\0") for n, e in enc_t.items()]
        want.sort(key=lambda kv: (kv[0][0] != "d", -len(kv[1])))  # descriptions first, then titles; each longest first
        for key, e in want:
            for f in free:
                if f[1] >= len(e):
                    placed[key] = f[0]
                    patches.append((f[0], e))
                    f[0] += len(e)
                    f[1] -= len(e)
                    break
            else:
                # a description must fit (the order puts the long ones first); a title that does not stays in SEL
                assert key.startswith("t"), ("does not fit the A-gap zero runs", key, len(e), [f[1] for f in free])
        out.append("// A-gap placement: %d bytes in %d runs, %d bytes left" % (
            sum(len(e) for _, e in patches), len(RUNS), sum(f[1] for f in free)))
        import json
        json.dump({"runs": RUNS, "patches": [[hex(a), e.hex()] for a, e in patches]},
                  io.open(os.path.join(REPO, "src/dra/gen/us/ko_sound_arc.json"), "w"), indent=0)
    # the cutscene name code (src/st/cutscene_actor_name.h) copies 0x60 bytes per name Hangul from 0x8009F000 up: a longer
    # name would reach the zero runs used above
    import glob
    mx = 0
    for fn in glob.glob(os.path.join(REPO, "src/st/*/*.c")):
        for m in re.finditer(r"actor_names_ko_len\[\] = \{([^}]*)\}", io.open(fn, encoding="utf-8", errors="ignore").read()):
            for v in m.group(1).split(","):
                mx = max(mx, int(v.strip()))
    assert mx <= 8, ("a cutscene actor name has more than 4 Hangul: the A-gap runs from 0x8009F180 up are no longer safe", mx)
    out.append("// cutscene actor names: at most %d bytes (4 Hangul max asserted)" % mx)
    dblob = bytearray(b"\0")
    offs = []
    for r in rows:
        n = int(r[0])
        ek = "t%02d" % n
        if ek in placed:
            pass
        else:
            out.append('static const char ko_snd_t%02d[] = "%s";' % (n, esc(enc_t[n])))
        e = enc_d[n]
        if e and not OFFS and ARC == "0":
            out.append('static const char ko_snd_d%02d[] = "%s";' % (n, esc(e)))
        elif e and OFFS and ARC == "0":
            offs.append(len(dblob))
            dblob += e + b"\0"
        elif OFFS and ARC == "0":
            offs.append(0)
    out.append('static const char ko_snd_play[] = "%s";' % esc(enc("연주 중:", "play")))
    names = []
    for i, r in enumerate(rows):
        n = int(r[0])
        names.append("(const char*)0x%08X" % placed["t%02d" % n] if ("t%02d" % n) in placed else "ko_snd_t%02d" % n)
    out.append("static const ModSoundTrack g_ModSoundTracks[] MOD_RO = {" + ", ".join(
        "{%s, %s}" % (names[i], ids[i]) for i in range(44)) + "};")
    if DESC and ARC != "0":
        lo = ["0x%04X" % ((placed["d%02d" % int(r[0])] if ("d%02d" % int(r[0])) in placed else 0x8009EFBC) & 0xFFFF) for r in rows]
        out.append("static const u16 g_ModSoundDescLo[44] = {" + ", ".join(lo) + "};")
        out.append("#define MOD_SND_DESC_STR(k) ((const char*)(0x80090000 | g_ModSoundDescLo[k]))")
    elif DESC and OFFS:
        out.append('static const char ko_snd_dblob[] = "%s";' % esc(bytes(dblob[:-1])))
        out.append("static const u16 g_ModSoundDescOff[44] = {" + ", ".join(str(o) for o in offs) + "};")
        out.append("#define MOD_SND_DESC_STR(k) (ko_snd_dblob + g_ModSoundDescOff[k])")
    elif DESC:
        out.append('static const char ko_snd_none[] = "";')
        out.append("static const char* const g_ModSoundDescKo[44] = {" + ", ".join(
            ("ko_snd_d%02d" % int(r[0])) if (len(r) > 3 and r[3]) else "ko_snd_none" for r in rows) + "};")
        out.append("#define MOD_SND_DESC_STR(k) (g_ModSoundDescKo[k])")
    io.open(OUT, "w", encoding="utf-8", newline="").write(chr(10).join(out) + chr(10))
    print("mod_sound_ko_str.h: promo=%d offs=%d desc=%d" % (PROMO, OFFS, DESC))


if __name__ == "__main__":
    sys.exit(main())
