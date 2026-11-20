#!/usr/bin/env python3
# ko_text_enc.py -- shared KO byte encoders + the "current glyph table" check.
#
# One place for the two KO string encodings the game reads, so every
# generator (gen_ko_names.py, gen_ko_fieldmsg.py, check_ko_text_codes.py)
# encodes with the SAME map as the glyph block that is actually loaded
# (ko_font_profile.P["map_name"], v14c at the moment). The old hand-written
# relic bytes used the v4 map, where 뼈 = F05F; in v14c F05F is the '!'
# engtab cell, so 「갈비뼈」 drew as 「갈비!」.
#
#   enc_show(s)  ShowText (func_800F99B8, menu info bar / relic list) and
#                BlitChar (stage message box) stream: Hangul = F0xx pair,
#                ' ' = 0x20, A-Z/a-z raw (KROM full-width) unless engtab has a
#                cell, every other ASCII = engtab pair (ShowText/BlitChar pair a
#                non-letter byte with the next one, so raw digits/punctuation
#                would eat the next byte). C-string, NUL-terminated by C.
#   enc_s(s)     _S-style 8px name stream (KO_PopupText, LIB list, equipment
#                names): ' ' = 0x00, ASCII = ord-0x20, Hangul = F0xx pair,
#                end = FF 00.
#
# Codes allowed on screen = profile map (Hangul + symbols) + Latin cells
# (ko_glyph_lat_v14d.tsv) + engtab cells (ko_engtab_253.bin). Anything else fails closed.
import io
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ko_paths import KO_WK  # noqa: E402
import ko_font_profile as FP  # noqa: E402

P4 = KO_WK + "/P4시험/"
MAP_PATH = P4 + FP.P["map_name"]
LAT_PATH = P4 + "ko_glyph_lat_v14d.tsv"
ENGTAB_PATH = P4 + "ko_engtab_253.bin"
B1_NEWMAP = P4 + "ko_glyph_map_v12f_new.tsv"

_C = {}


def _rd_map(path):
    m = {}
    for ln in io.open(path, encoding="utf-8"):
        q = ln.rstrip("\n").split("\t")
        if len(q) >= 2 and len(q[0]) == 1 and q[1].lower().startswith("0x"):
            m[q[0]] = int(q[1], 16)
    return m


def glyph_map():
    """char -> code of the glyph block that is loaded (profile map)."""
    if "map" not in _C:
        _C["map"] = _rd_map(MAP_PATH)
        assert len(_C["map"]) >= 700, (MAP_PATH, len(_C["map"]))
    return _C["map"]


def engtab():
    """ASCII char -> engtab cell code (0 = no cell)."""
    if "eng" not in _C:
        raw = open(ENGTAB_PATH, "rb").read()
        assert len(raw) == 188, len(raw)
        e = {}
        for i in range(94):
            v = struct.unpack("<H", raw[2 * i:2 * i + 2])[0]
            if v:
                e[chr(0x21 + i)] = v
        _C["eng"] = e
    return _C["eng"]


def known_codes():
    """every 2-byte code that has a cell in the current glyph block."""
    if "known" not in _C:
        k = set(glyph_map().values())
        if os.path.exists(LAT_PATH):
            k |= set(_rd_map(LAT_PATH).values())
        k |= set(engtab().values())
        _C["known"] = k
    return _C["known"]


def code_name(code):
    """reverse lookup for messages."""
    for ch, c in glyph_map().items():
        if c == code:
            return ch
    for ch, c in engtab().items():
        if c == code:
            return "engtab:" + ch
    return "?"


def is_hangul(ch):
    return "가" <= ch <= "힣"


def _pair(code):
    assert code in known_codes(), ("code not in current glyph table", hex(code))
    return bytes([code >> 8, code & 0xFF])


def enc_show(s, linebreak=None):
    """ShowText / BlitChar stream (see header). linebreak: char that becomes 0x01."""
    mp = glyph_map()
    e = engtab()
    sym = _rd_map(B1_NEWMAP) if os.path.exists(B1_NEWMAP) else {}
    out = bytearray()
    q = 0
    i = 0
    while i < len(s):
        ch = s[i]
        if linebreak is not None and ch == linebreak:
            out.append(0x01)
        elif ch == " ":
            out.append(0x20)
        elif ch == "." and s[i:i + 2] == ".." and "…" in mp:
            j = i
            while j < len(s) and s[j] == ".":
                j += 1
            out += _pair(mp["…"])
            i = j
            continue
        elif ch == '"' and "“" in mp and "”" in mp:
            out += _pair(mp["“" if q % 2 == 0 else "”"])
            q += 1
        elif ch in "[]" and "「" in mp and "」" in mp:
            out += _pair(mp["「" if ch == "[" else "」"])
        elif is_hangul(ch) or (ord(ch) >= 128 and ch in mp):
            assert ch in mp, ("no glyph cell in %s" % FP.P["map_name"], ch, s)
            out += _pair(mp[ch])
        elif ord(ch) >= 128 and ch in sym:
            out += _pair(sym[ch])
        elif ch in e:
            out += _pair(e[ch])
        elif ("A" <= ch <= "Z") or ("a" <= ch <= "z"):
            out.append(ord(ch))  # KROM full-width letter
        else:
            raise AssertionError(("no cell for", ch, s))
        i += 1
    return bytes(out)


def enc_s(s, max_chars=None):
    """_S-style 8px stream, FF 00 terminated (see header)."""
    mp = glyph_map()
    if max_chars is not None:
        assert len(s) <= max_chars, (len(s), s)
    out = bytearray()
    for ch in s:
        o = ord(ch)
        if ch == " ":
            out.append(0x00)
        elif o < 128:
            assert 0x20 < o < 0x7F, repr(ch)
            out.append(o - 0x20)
        else:
            assert ch in mp, ("no glyph cell in %s" % FP.P["map_name"], ch, s)
            out += _pair(mp[ch])
    out += b"\xFF\x00"
    return bytes(out)


def show_px(s, linebreak=None):
    """conservative pixel width of the longest line of an enc_show text."""
    best = cur = 0
    for ch in s:
        if linebreak is not None and ch == linebreak:
            best = max(best, cur)
            cur = 0
        elif ch == " ":
            cur += 6
        elif is_hangul(ch) or ord(ch) >= 128:
            cur += 12
        else:
            cur += 8 if ch not in "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz" else 12
    return max(best, cur)


def esc(bs):
    return "".join("\\x%02X" % b for b in bs)


def scan_codes(bs):
    """yield (offset, code) of every 2-byte code in a KO byte string (either
    encoding): a byte F0..F3 starts a pair, FF starts the FF xx terminator/escape."""
    i = 0
    while i < len(bs):
        b = bs[i]
        if 0xF0 <= b <= 0xF3 and i + 1 < len(bs):
            yield i, (b << 8) | bs[i + 1]
            i += 2
        elif b == 0xFF:
            i += 2
        else:
            i += 1


def c_literal_bytes(lit):
    """bytes of a C string literal body (\\xHH, \\NNN octal, \\\\, \\", plain chars as UTF-8)."""
    out = bytearray()
    i = 0
    while i < len(lit):
        c = lit[i]
        if c == "\\" and i + 1 < len(lit):
            n = lit[i + 1]
            if n == "x":
                j = i + 2
                while j < len(lit) and j < i + 4 and lit[j] in "0123456789abcdefABCDEF":
                    j += 1
                out.append(int(lit[i + 2:j], 16))
                i = j
                continue
            if n in "01234567":
                j = i + 1
                while j < len(lit) and j < i + 4 and lit[j] in "01234567":
                    j += 1
                out.append(int(lit[i + 1:j], 8))
                i = j
                continue
            out += {"n": b"\n", "t": b"\t", "r": b"\r", "0": b"\0"}.get(n, n.encode("utf-8"))
            i += 2
            continue
        out += c.encode("utf-8")
        i += 1
    return bytes(out)
