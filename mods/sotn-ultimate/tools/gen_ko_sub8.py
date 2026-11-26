#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""gen_ko_sub8.py -- familiar voice subtitles, 8px version (new resident area) -> mods/sotn-ultimate/src/easy/sub_str.h

Input : ko_data/번역/번역_패밀리어자막.tsv (VO id hex, speaker, Korean text; the same table gen_ko_sub.py reads)
Output: per line one or two rows (<= ROWMAX cells, split at the space that balances them): a row is bytes: Hangul / ellipsis = glyph code F0xx big endian
        (2 bytes, the same codes as the 12px rows), ASCII raw, space 0x20, end 0.  Plus the "aux cells": the 8px cells (1bpp, 8 rows, MSB = first
        pixel, the plate's format) of the characters the 8px plate (KO_8P block, 0x801F2000, ko_8p_block_250.bin) lacks, so the area carries its own
        and never touches the plate.  Cell rule = gen_ko_8p.py's (v11 MGalmuri8 16x15 cell, columns 0..7, 8 rows from the first ink row;
        a character v11 lacks: Galmuri7.ttf at size 8), same positive control (it must reproduce the plate's cells bit for bit).
"""
import csv
import io
import os
import struct
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ko_paths import KO_WK, KO_FONTS  # noqa: E402
import ko_font_profile as FP  # noqa: E402

P4 = KO_WK + "/P4시험/"
TSV = KO_WK + "/번역/번역_패밀리어자막.tsv"
TSV_SPU = KO_WK + "/번역/번역_사역마음성.tsv"  # optional: SPU shouts (VO id hex, speaker, Korean text, seconds): shown for that long
PLATE = P4 + "ko_8p_block_250.bin"
PLATE_MAP = P4 + "ko_8p_map_250.tsv"
V11B = P4 + "ko_glyph_block_mgalmuri8_v11.bin"
V11M = P4 + "ko_glyph_map_mgalmuri8_v11.tsv"
GMAP = P4 + FP.P["map_name"]
TSV_LYR = KO_WK + "/번역/번역_노래자막.tsv"  # optional: song subtitles (track 0 Nocturne PS1 / 1 Nocturne PSP / 2 I Am the Wind, line no, Korean text); an empty or missing line shows nothing
SEGS = [(0, KO_WK + "/recipe/song_seg_nocturne_ps1.csv"), (1, KO_WK + "/recipe/song_seg_nocturne_psp.csv"), (2, KO_WK + "/recipe/song_seg_staff_ps1.csv")]  # the vocal phrases (demucs stem energy: start / end seconds only)
REPO = os.environ.get("SOTN_KR", os.getcwd())
OUT = os.path.join(REPO, "mods/sotn-ultimate/src/easy/sub_str.h")
ROWMAX = 14   # cells per row: a narrow block under the familiar
ROWHARD = 16


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
    return "가" <= c <= "힣" or c == "…"


def split(text):
    if len(text) <= ROWMAX:
        return [text, ""]
    best = None
    for i, c in enumerate(text):
        if c == " ":
            a, b = text[:i], text[i + 1:]
            if len(a) <= ROWHARD and len(b) <= ROWHARD:
                d = abs(len(a) - len(b))
                if best is None or d < best[0]:
                    best = (d, a, b)
    assert best, ("cannot split into two rows of <= %d cells" % ROWHARD, text)
    return [best[1], best[2]]


def main():
    mp = rd(GMAP)          # char -> glyph-map code (the rows' codes)
    pmap = rd(PLATE_MAP)   # char -> plate code
    plate = open(PLATE, "rb").read()
    n = struct.unpack("<H", plate[:2])[0]
    pcodes = list(struct.unpack("<%dH" % n, plate[2:2 + 2 * n]))
    pcells = {pcodes[i]: plate[2 + 2 * n + 8 * i:2 + 2 * n + 8 * i + 8] for i in range(n)}
    v11b = open(V11B, "rb").read()
    v11 = {}
    for ln in io.open(V11M, encoding="utf-8"):
        q = ln.rstrip(chr(10)).split(chr(9))
        if len(q) >= 3 and len(q[0]) == 1 and q[1].startswith("0x"):
            v11[q[0]] = int(q[2])
    G7 = ImageFont.truetype(KO_FONTS + "/Galmuri/v2.40.4/Galmuri-v2.40.4/Galmuri7.ttf", 8)

    def cell_g7(ch):
        im = Image.new("1", (16, 16), 0)
        d = ImageDraw.Draw(im)
        d.fontmode = "1"
        d.text((0, 0), ch, font=G7, fill=1)
        a = np.array(im, dtype=np.uint8)
        rows = np.where(a.any(axis=1))[0]
        assert len(rows) and rows.max() - rows.min() < 8, ("Galmuri7 glyph empty/too tall", ch)
        t = int(rows.min())
        return np.packbits(a[t:t + 8, 0:8], axis=1).reshape(8).tobytes()

    def cell_of(ch):
        if ch not in v11:
            return cell_g7(ch)
        k = v11[ch]
        a = np.unpackbits(np.frombuffer(v11b[k * 30:k * 30 + 30], dtype=np.uint8)).reshape(15, 16)
        rows = np.where(a.any(axis=1))[0]
        dy = int(rows.min())
        assert rows.max() - dy < 8, ("ink taller than 8 rows", ch)
        return np.packbits(a[dy:dy + 8, 0:8], axis=1).reshape(8).tobytes()

    # positive control: the rule reproduces the plate's Hangul cells
    chk = bad = 0
    for ch, code in pmap.items():
        if "가" <= ch <= "힣" and code in pcells and ch in v11:
            chk += 1
            if cell_of(ch) != pcells[code]:
                bad += 1
    assert chk > 200 and bad == 0, ("cell rule does not reproduce the plate", chk, bad)
    print("positive control: %d plate Hangul cells reproduced, %d differ" % (chk, bad))

    aux = {}   # glyph code -> cell

    used_codes = set(mp.values())
    synth_map = {}

    def synth(ch):  # a character the resident 12px font lacks: the 8px subtitle only needs a code of its own for the aux cell (codes F0xx..F3xx are the two-byte range)
        if ch not in synth_map:
            c = 0xF3FF - len(synth_map)
            while c in used_codes:
                c -= 1
            synth_map[ch] = c
            used_codes.add(c)
            print("synthetic code 0x%04X for %s (not in the resident 12px font)" % (c, ch))
        return synth_map[ch]

    def encode(part):
        b = bytearray()
        for ch in part:
            if is_h(ch):
                code = mp.get(ch) or synth(ch)
                b += bytes([code >> 8, code & 0xFF])
                if code not in pcells and code not in aux:
                    aux[code] = cell_of(ch)
            else:
                assert 0x20 <= ord(ch) < 0x7F, ("not ASCII / Hangul", ch)
                b.append(ord(ch))
                if ord(ch) != 0x20 and ord(ch) not in pcells and ord(ch) not in aux:
                    aux[ord(ch)] = cell_g7(ch)
        return bytes(b)

    rows = [l.rstrip(chr(10)).split(chr(9)) + ["0"] for l in io.open(TSV, encoding="utf-8")][1:]
    if os.path.exists(TSV_SPU):
        for l in io.open(TSV_SPU, encoding="utf-8").read().splitlines()[1:]:
            q = l.split(chr(9))
            if len(q) >= 4 and q[2].strip():
                rows.append([q[0], q[1], q[2], str(int(round(float(q[3]) * 60)) + 12)])
    out = ["// gen_ko_sub8.py: familiar subtitle rows + the aux cells for the 8px subtitle (do not edit)"]
    ents = []
    maxrow = 0
    for r in rows:
        vid, text, frames = r[0], r[2], int(r[-1])
        parts = split(text)
        k = "ko_sub8_%s" % vid
        for pi, part in enumerate(parts):
            bb = encode(part)
            maxrow = max(maxrow, len(part))
            out.append('static const char %s_%d[] = "%s";' % (k, pi, "".join("\\x%02X" % x for x in bb)))
        ents.append("{0x%s, %s_0, %s_1, %d}" % (vid, k, k, frames))
    out.append("typedef struct { u16 id; const char* r0; const char* r1; u16 frames; } KoSub8Line;")
    out.append("static const KoSub8Line g_KoSub8Lines[%d] = {%s};" % (len(ents), ", ".join(ents)))
    # the songs: the vocal phrases' start / end frames (60 per second) and, when a lyric file gives it, the line's Korean text
    lyr = {}
    rows_lyr = {}   # track -> [(start s, end s, text)] : the lyric file's own rows (a line may be split: ids like 6-1, 6-2), when it has any
    if os.path.exists(TSV_LYR):
        for l in io.open(TSV_LYR, encoding="utf-8").read().splitlines()[1:]:
            q = l.split(chr(9))
            if len(q) >= 4 and q[0].strip().isdigit():
                trk_ = int(q[0])
                txt = q[6].strip() if len(q) >= 7 else (q[2].strip() if len(q) == 3 else "")
                if len(q) >= 7:
                    rows_lyr.setdefault(trk_, []).append((float(q[2]), float(q[3]), txt))
                elif len(q) >= 3 and q[2].strip() and q[1].isdigit():
                    lyr[(trk_, int(q[1]))] = q[2].strip()
    ph = os.environ.get("SONG_PLACEHOLDER") == "1"
    out.append('static const char ko_song_empty[] = "";')
    out.append("typedef struct { u16 a; u16 b; const char* r0; const char* r1; } KoSongLine;")
    nsong = []
    def encode12(part):  # a song row: the resident 12px glyph codes (v14h map), ASCII raw; the newarea draws them with the game's 12px glyph source
        b = bytearray()
        for ch in part:
            if is_h(ch):
                code = mp[ch]
                b += bytes([code >> 8, code & 0xFF])
            else:
                assert 0x20 <= ord(ch) < 0x7F, ("not ASCII / Hangul", ch)
                b.append(ord(ch))
        return bytes(b)

    def song_row(trk, a, b, text):
        if text:
            parts = ([t.strip() for t in text.split('|')] if '|' in text else [text.strip(), ''])
            assert len(parts) == 2 and all(len(t) <= 24 for t in parts), ('song line: two rows of <= 24 cells', text)
            names = []
            for pi, part in enumerate(parts):
                if part:
                    out.append('static const char ko_song_%d_%d_%d[] = "%s";' % (trk, song_row.n, pi, "".join("\\x%02X" % x for x in encode12(part))))
                    names.append("ko_song_%d_%d_%d" % (trk, song_row.n, pi))
                else:
                    names.append("ko_song_empty")
            r0, r1 = names
        else:
            r0 = r1 = "ko_song_empty"
        song_row.n += 1
        return "{%d, %d, %s, %s}" % (int(round(a * 60)), int(round(b * 60)), r0, r1)
    song_row.n = 0
    for trk, path in SEGS:
        ent = []
        if trk in rows_lyr:
            for (a, b, text) in sorted(rows_lyr[trk]):
                ent.append(song_row(trk, a, b, text))
        else:
            for r in csv.reader(io.open(path, encoding="utf-8-sig")):
                if len(r) < 4 or r[1] != "phrase":
                    continue
                no = int(r[0])
                text = lyr.get((trk, no), ("%d행" % no) if ph else "")
                ent.append(song_row(trk, float(r[2]), float(r[3]), text))
        out.append("static const KoSongLine g_KoSong%d[%d] = {%s};" % (trk, len(ent), ", ".join(ent)))
        nsong.append(len(ent))
    out.append("#define KO_SONG_N0 %d" % nsong[0])
    out.append("#define KO_SONG_N1 %d" % nsong[1])
    out.append("#define KO_SONG_N2 %d" % nsong[2])
    codes = sorted(aux)
    out.append("#define KO_SUB8_NAUX %d" % len(codes))
    out.append("static const u16 g_KoSub8AuxCode[%d] = {%s};" % (len(codes), ", ".join("0x%04X" % c for c in codes)))
    out.append("static const u8 g_KoSub8AuxCell[%d] = {%s};" % (8 * len(codes), ", ".join("0x%02X" % x for c in codes for x in aux[c])))
    io.open(OUT, "w", encoding="utf-8", newline="").write(chr(10).join(out) + chr(10))
    print("sub_str.h: %d lines, longest row %d cells, %d aux cells (%d B)" % (len(rows), maxrow, len(codes), 10 * len(codes)))


if __name__ == "__main__":
    sys.exit(main())
