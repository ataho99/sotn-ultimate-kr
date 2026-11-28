#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
# ko_preflight.py -- static KO preflight: every problem of the KO build inputs AT ONCE, before the long
# build (build_ko_trial.sh runs it first; KO_SKIP_PREFLIGHT=1 skips it). Writes nothing in the tree
# (generator dry runs go to a temp folder). Exit 0 = no FAIL, 2 = problems (--strict: WARN counts too).
#
# Each check reads the SAME inputs the build's generators read, through their own modules where the
# module imports cleanly (ko_font_profile, ko_text_enc, gen_ko_splice, gen_ko_wrap, gen_ko_promo,
# gen_ko_names, gen_ko_equip, gen_ko_fieldmsg, gen_ko_dlg, gen_ko_sheet2, gen_ko_cfg, gen_ko_save,
# ko_entry_offsets, check_ko_cutscene_struct; gen_ko_mar is loaded without its top-level main()).
#   1 glyph coverage  every non-ASCII char of every row the build uses has a cell in the profile glyph
#                     map (encoder by encoder: cutscene / ShowText / _S 8px / equipment v4 desc / 8px plate)
#   2 keys / EN       splicer / gen_ko_dlg / gen_ko_mar / relic / field / cfg / save key sets, orphans,
#                     EN column vs retail (cutscene yamls via a splice dry run, field labels vs C source)
#   3 half/full width ShowText-family encoders need an engtab cell for every half-width non-letter;
#                     8px _S names and cutscene dialogue want half-width; LIB descs want full-width
#   4 slot lengths    wrapped dialogue lines / window lines, splice KO<=EN per entry segment, field box,
#                     relic px / buffer, enemy+relic DRA growth vs MOD_DRA_PAD, equipment names / bundle,
#                     cfg labels, sheet-2 titles, gen_ko_mar line counts, TSV 폭한계 column
#   5 cell budget     block cells x 23 B vs KO_POP_TEXT (0x5640), dra.h KO_GLYPH_COUNT, KO_SHA, engtab /
#                     map codes inside the block, promo / plate / sheet-2 sizes, ARC layout defines
#   6 paths / env     every input the build opens (env overrides expanded), tools on PATH, env mixups
#   7 entry offsets   C SetCutsceneScript(+const) vs ko_entry_offsets table, psp_script split, and
#                     ko_entry_offsets.verify on the dry-run splice (needs extracted yamls; else skipped)
# Usage: python3 mods/sotn-ultimate/tools/ko_preflight.py [--strict] [-v]
import ast
import contextlib
import hashlib
import importlib
import importlib.util
import io
import os
import re
import shutil
import struct
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
os.environ.pop("KO_DUMP_EN", None)  # gen_ko_splice / gen_ko_mar would append to it during the dry run
from ko_paths import KO_REPO, KO_WK, KO_FONTS  # noqa: E402

REPO = os.environ.get("SOTN_KR") or KO_REPO
P4 = KO_WK + "/P4시험/"
STRICT = "--strict" in sys.argv
VERBOSE = "-v" in sys.argv
BUILD_SH = os.path.join(HERE, "build_ko_trial.sh")

CHECKS = [("1", "glyph coverage"), ("2", "keys / EN vs retail"), ("3", "half / full width"),
          ("4", "slot lengths"), ("5", "glyph cell budget"), ("6", "paths / env"),
          ("7", "entry offsets")]
FIND = []
COUNT = dict((c, 0) for c, _ in CHECKS)
SKIPS = dict((c, []) for c, _ in CHECKS)


def add(c, sev, msg):
    FIND.append((c, sev, msg))


def fail(c, msg):
    add(c, "FAIL", msg)


def warn(c, msg):
    add(c, "WARN", msg)


def info(c, msg):
    add(c, "INFO", msg)


def skip(c, msg):
    SKIPS[c].append(msg)


def seen(c, n=1):
    COUNT[c] += n


def rp(p):
    try:
        r = os.path.relpath(p, REPO)
        return p if r.startswith("..") else r
    except ValueError:
        return p


def kl(keys, n=6):
    keys = list(keys)
    s = ", ".join(keys[:n])
    return s + (" (+%d more)" % (len(keys) - n) if len(keys) > n else "")


def exc(ex):
    a = ex.args[0] if len(ex.args) == 1 else ex.args
    return "%s: %r" % (type(ex).__name__, a)


@contextlib.contextmanager
def quiet():
    with contextlib.redirect_stdout(io.StringIO()):
        yield


def guarded(c, label):
    """decorator: a check part that crashes is a FAIL of its check, the others still run"""
    def deco(fn):
        def run(*a, **k):
            try:
                return fn(*a, **k)
            except Exception as ex:  # noqa: BLE001 -- report, never stop the preflight
                fail(c, "%s stopped (preflight could not finish this part): %s" % (label, exc(ex)))
                return None
        return run
    return deco


def read_rows(path):
    return [l.rstrip("\n").split("\t") for l in io.open(path, encoding="utf-8")]


def rd_map(path):
    m = {}
    for ln in io.open(path, encoding="utf-8"):
        q = ln.rstrip("\n").split("\t")
        if len(q) >= 2 and len(q[0]) == 1 and q[1].lower().startswith("0x"):
            m[q[0]] = int(q[1], 16)
    return m


def rd_engtab(path):
    raw = open(path, "rb").read()
    assert len(raw) == 188, (path, len(raw))
    return dict((chr(0x21 + i), v) for i, v in enumerate(struct.unpack("<94H", raw)) if v)


def is_h(ch):
    return "가" <= ch <= "힣"


def is_fw(ch):
    """full-width ASCII forms / ideographic space and CJK punctuation"""
    return 0xFF01 <= ord(ch) <= 0xFF5E or ch in "　。、・～"


def ctext(ch):
    return "%r U+%04X" % (ch, ord(ch))


def imp(name, c="6"):
    try:
        with quiet():
            return importlib.import_module(name)
    except Exception as ex:  # noqa: BLE001
        fail(c, "import %s.py failed: %s (its build step would fail the same way)" % (name, exc(ex)))
        return None


# ===================================================================== setup: profile, map, engtabs
try:
    import ko_font_profile as FP  # noqa: E402
except Exception as ex:  # noqa: BLE001
    print("FAIL 6 ko_font_profile: %s (KO_FONT_PROFILE=%r)" % (exc(ex), os.environ.get("KO_FONT_PROFILE")))
    sys.exit(2)

PROFILE_MAP = FP.P["map_name"]
PROFILE_BLK = FP.P["blk_name"]
MAP_USED = PROFILE_MAP
if not os.path.exists(P4 + PROFILE_MAP):
    fb = re.sub(r"v14d", "v14c", PROFILE_MAP)
    if fb != PROFILE_MAP and os.path.exists(P4 + fb):
        FP.P["map_name"] = fb  # every module imported below reads the fallback (same Hangul as v14d)
        MAP_USED = fb
        fail("6", "profile glyph map %s missing (made by `gen_ko_glyph_lat.py d`, not by the build); "
             "checks 1-5 use %s (v14d map == v14c map)" % (rp(P4 + PROFILE_MAP), fb))
    else:
        fail("6", "profile glyph map %s missing; glyph checks skipped" % rp(P4 + PROFILE_MAP))

E = imp("ko_text_enc")
MAP = {}
if E is not None:
    try:
        MAP = E.glyph_map()
    except Exception as ex:  # noqa: BLE001
        fail("1", "glyph map %s unreadable: %s" % (MAP_USED, exc(ex)))
INV = dict((v, k) for k, v in MAP.items())
SYM = {}
if os.path.exists(P4 + "ko_glyph_map_v12f_new.tsv"):
    SYM = rd_map(P4 + "ko_glyph_map_v12f_new.tsv")

ENG253 = {}
try:
    ENG253 = rd_engtab(P4 + "ko_engtab_253.bin")  # what ko_text_enc / gen_ko_save / gen_ko_equip encode with
except Exception as ex:  # noqa: BLE001
    fail("6", "ko_engtab_253.bin (read by ko_text_enc / gen_ko_save / gen_ko_equip): %s" % exc(ex))


def build_exports():
    out = {}
    txt = io.open(BUILD_SH, encoding="utf-8").read()
    for m in re.finditer(r"(?m)^export (KO_[A-Z0-9_]+)=(\S+)", txt):
        v = m.group(2).strip('"').replace("${KO_WK}", KO_WK).replace("$KO_WK", KO_WK)
        out[m.group(1)] = v
    return out, txt


BX, BUILD_TXT = build_exports()


def lat_new_d():
    """gen_ko_glyph_lat.py NEW_D (the ASCII cells stage d appends), read from its source (no import:
    the module loads a TTF and numpy at import)"""
    src = io.open(os.path.join(HERE, "gen_ko_glyph_lat.py"), encoding="utf-8").read()
    for node in ast.parse(src).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "NEW_D" for t in node.targets):
            return eval(compile(ast.Expression(node.value), "gen_ko_glyph_lat.py", "eval"),
                        {"__builtins__": {}}, {"list": list, "chr": chr})
    return []


NEW_D = []
try:
    NEW_D = lat_new_d()
except Exception as ex:  # noqa: BLE001
    warn("5", "gen_ko_glyph_lat.py NEW_D unreadable: %s" % exc(ex))
ENG_BUILD_PATH = BX.get("KO_ENGTAB", P4 + "ko_engtab_253.bin")
ENG_BUILD_PREDICTED = False
try:
    ENG_BUILD = rd_engtab(ENG_BUILD_PATH)
except Exception:  # noqa: BLE001
    ENG_BUILD = dict(ENG253)
    for ch in NEW_D:
        ENG_BUILD.setdefault(ch, -1)  # predicted (code unknown until gen_ko_glyph_lat.py d runs)
    ENG_BUILD_PREDICTED = True

PLATE = set()
try:
    _pb = open(P4 + "ko_8p_block_250.bin", "rb").read()
    _pn = struct.unpack("<H", _pb[:2])[0]
    PLATE = set(struct.unpack("<%dH" % _pn, _pb[2:2 + _pn * 2]))
except Exception as ex:  # noqa: BLE001
    fail("6", "ko_8p_block_250.bin (8px plate, gen_ko_cfg / check_ko_glyph / KO_8P.BIN): %s" % exc(ex))

# ===================================================================== aggregators (checks 1 / 3)
MISS = {}      # (char, encoder) -> [count, keys]   no glyph cell
HALF = {}      # (char, encoder) -> [count, keys]   half-width without the cell the encoder needs (FAIL)
FULL = {}      # (char, encoder) -> [count, keys]   full-width where half-width is expected
KROM = {}      # encoder -> keys                    raw ASCII letters -> console BIOS Kanji ROM (grey)
DLG_ASCII = {}  # char -> [count, keys]             dialogue ASCII without a KO_ENGTAB cell (8x8 plate fallback)


def _agg(d, k, key):
    e = d.setdefault(k, [0, []])
    e[0] += 1
    if key not in e[1]:
        e[1].append(key)


def miss(ch, key, enc):
    _agg(MISS, (ch, enc), key)


def chk_cut(key, text, enc="cutscene (gen_ko_splice / gen_ko_dlg / gen_ko_mar)"):
    """cutscene encoders: ASCII = raw byte, … = F1A0, everything else needs a map cell"""
    seen("1")
    for ch in FP.norm(text.replace("\\n", "")):
        if ch in "/|":
            continue
        o = ord(ch)
        if o >= 128:
            if ch not in MAP and ch != "…":
                miss(ch, key, enc)
            if is_fw(ch):
                _agg(FULL, (ch, "dialogue (half-width ASCII + engtab cells)"), key)
        elif 0x21 <= o < 0x7F and ch not in ENG_BUILD:
            _agg(DLG_ASCII, ch, key)


def chk_show(key, s, enc, linebreak=None):
    """ko_text_enc.enc_show (ShowText / BlitChar): mirror of its branches, then the real encoder"""
    seen("1")
    seen("3")
    found = False
    for ch in s:
        if ch == " " or (linebreak is not None and ch == linebreak):
            continue
        if ch == "." and "." in ENG253:
            continue
        if ch == '"' and "“" in MAP and "”" in MAP:
            continue
        if ch in "[]" and "「" in MAP and "」" in MAP:
            continue
        if ord(ch) >= 128:
            if ch not in MAP and ch not in SYM:
                miss(ch, key, enc)
                found = True
                if is_fw(ch):
                    _agg(FULL, (ch, enc), key)
            continue
        if ch in ENG253:
            continue
        if ch.isalpha():
            KROM.setdefault(enc, [])
            if key not in KROM[enc]:
                KROM[enc].append(key)
            continue
        _agg(HALF, (ch, enc), key)
        found = True
    if E is not None and not found:
        try:
            E.enc_show(s, linebreak=linebreak)
        except Exception as ex:  # noqa: BLE001
            fail("1", "%s: %s encoder refuses %r: %s" % (key, enc, s, exc(ex)))
            return False
    return not found


def chk_s(key, s, enc):
    """ko_text_enc.enc_s (_S 8px names): ' ' 0x00, ASCII 0x21..0x7E, the rest a map cell"""
    seen("1")
    seen("3")
    ok = True
    for ch in s:
        o = ord(ch)
        if ch == " " or 0x20 < o < 0x7F:
            continue
        if o < 128:
            _agg(HALF, (ch, enc), key)
            ok = False
        elif ch not in MAP:
            miss(ch, key, enc)
            ok = False
            if is_fw(ch):
                _agg(FULL, (ch, enc), key)
    return ok


# ===================================================================== module handles
GW = imp("gen_ko_wrap")
GP = imp("gen_ko_promo")
GN = imp("gen_ko_names")
GE = imp("gen_ko_equip")
GF = imp("gen_ko_fieldmsg")
GD = imp("gen_ko_dlg")
GS2 = imp("gen_ko_sheet2")
GC = imp("gen_ko_cfg")
GSV = imp("gen_ko_save")
EO = imp("ko_entry_offsets")
CS = imp("check_ko_cutscene_struct")
S = None
if os.path.exists(P4 + "ko_promo_map.tsv"):
    S = imp("gen_ko_splice")  # loads ko_promo_map.tsv at import
else:
    fail("6", "%s missing: gen_ko_splice.py fails at import" % rp(P4 + "ko_promo_map.tsv"))

DEF_V14 = KO_WK + "/검수/번역_v14.tsv"
V14 = []
V14_TI = 3
if os.path.exists(DEF_V14):
    V14 = read_rows(DEF_V14)
    try:
        V14_TI = [i for i, x in enumerate(V14[0]) if x.startswith("번역")][0]
    except IndexError:
        fail("2", "번역_v14.tsv header has no 번역 column")
else:
    fail("6", "translation table %s missing" % rp(DEF_V14))

SPLICE_PREFIX = {}  # prefix -> (asset, overlay, optional)
if S is not None:
    for _a, _o, _sym, _prefs in S.STAGES:
        for _p in _prefs:
            SPLICE_PREFIX[_p] = (_a, _o, _o in S.OPTIONAL)
DLG_STAGE = set(GD.STAGE) if GD is not None else set()
MAR_PREFIX = ("mod_maria_boss_data.h:g_MbScriptPre", "mod_maria_boss_data.h:g_MbScriptPost")
BOSS_RE = re.compile(r"^(bo2|bo5|bo6|rbo2|rbo6)_([0-9a-f]+)$")
STRUCT_STAGES = {"st0_richter_dracula", "dai_huntinggirl", "dre_succubus", "rcen_shaft"}  # check_ko_cutscene_struct


def dlg_rows():
    for r in V14[1:]:
        if len(r) > V14_TI and len(r) > 1 and r[1] == "대사":
            yield r


def prefix(key):
    return key.rsplit(":", 1)[0]


# ===================================================================== check 1 / 3: coverage per source
@guarded("1", "dialogue coverage")
def c1_dialogue():
    for r in dlg_rows():
        p = prefix(r[0])
        if p in SPLICE_PREFIX or p in DLG_STAGE or BOSS_RE.match(p) or p in MAR_PREFIX:
            chk_cut(r[0], r[V14_TI])


@guarded("1", "relic / enemy names")
def c1_names():
    if GN is None:
        return
    tbl = GN.load_table()
    suffix = tbl.get("config_us.h:g_RelicDefs:card:suffix", "")
    for k, v in sorted(tbl.items()):
        if not v:
            continue
        if k.startswith("config_us.h:g_RelicDefs:") and (k.endswith(":name") or k.endswith(":desc")):
            chk_show(k, v.replace(GN.CARD_TOKEN, suffix), "ShowText (relic, ko_text_enc.enc_show)")
        elif k.startswith("enemydefs.h:g_EnemyDefs:") or k.startswith("enemydefs_mod.h:g_EnemyDefs:"):
            chk_s(k, v, "_S 8px (enemy name, ko_text_enc.enc_s)")
    if suffix:
        chk_show("config_us.h:g_RelicDefs:card:suffix", suffix, "ShowText (relic, ko_text_enc.enc_show)")


@guarded("1", "field messages")
def c1_field():
    if GF is None:
        return
    for r in read_rows(GF.TSV)[1:]:
        if len(r) < 5 or not r[0].startswith("field:"):
            continue
        ko = r[4].strip()
        if ko in GF.PLACEHOLDER:
            continue
        chk_show(r[0], ko, "BlitChar (field message, ko_text_enc.enc_show)", linebreak="/")


EQ_KO = {}


@guarded("1", "equipment names / descs")
def c1_equip():
    if GE is None:
        return
    EQ_KO.update(GE.load_tsv())
    mp4 = GE.load_map()  # ko_glyph_map_v4.tsv (the desc encoder's map, 529 Hangul)
    for key, (en, s) in sorted(EQ_KO.items()):
        if key.endswith(":name"):
            seen("1")
            seen("3")
            for ch in s:
                o = ord(ch)
                if ch == " " or 0x20 <= o < 0x7F:
                    if ch != " " and o not in PLATE:
                        miss(ch, key, "8px plate (equipment name, ko_8p_block_250)")
                    continue
                if not is_h(ch):
                    _agg(FULL if is_fw(ch) else HALF, (ch, "_S 8px (equipment name: Hangul + ASCII only)"), key)
                    continue
                if ch not in MAP:
                    miss(ch, key, "_S 8px (equipment name, profile map)")
                elif MAP[ch] not in PLATE:
                    miss(ch, key, "8px plate (equipment name, ko_8p_block_250)")
                elif MAP[ch] > 0xF375:
                    fail("5", "%s: %s code %04X > F375 (gen_ko_equip enc_name assert)" % (key, ctext(ch), MAP[ch]))
        elif key.endswith(":desc"):
            seen("1")
            seen("3")
            enc = "ShowText (equipment desc, gen_ko_equip v4 map + engtab_253)"
            i = 0
            while i < len(s):
                ch = s[i]
                if ch == ".":
                    j = i
                    while j < len(s) and s[j] == ".":
                        j += 1
                    if j - i < 2 and "." not in ENG253:
                        _agg(HALF, (ch, enc), key)
                    i = j if j - i >= 2 else i + 1
                    continue
                i += 1
                if ch == " " or ch in '[]"':
                    continue
                if is_h(ch):
                    if ch not in mp4:
                        miss(ch, key, "ko_glyph_map_v4.tsv (equipment desc encoder)")
                    elif MAP and MAP.get(ch) != mp4[ch]:
                        fail("1", "%s: %s desc code %04X (v4 map) is %s in the loaded block (%s) -- draws "
                             "the wrong glyph (the 뼈/F05F class; gen_ko_equip.MAP = v4)"
                             % (key, ctext(ch), mp4[ch], "'%s'" % INV.get(mp4[ch], "no cell"), MAP_USED))
                    continue
                if ord(ch) < 128 and ch in ENG253:
                    continue
                _agg(FULL if is_fw(ch) else HALF, (ch, enc), key)


CFG_ROWS = []


@guarded("1", "config screen")
def c1_cfg():
    if GC is None:
        return
    for r in read_rows(GC.TSV)[1:]:
        if len(r) < 5 or not r[0].startswith("cfg:"):
            continue
        CFG_ROWS.append(r)
        key, kind, ko = r[0], r[1], r[4]
        seen("1")
        seen("3")
        for ch in ko:
            o = ord(ch)
            if kind.startswith("메뉴"):
                if ch == " " or o < 128:
                    continue
                if not is_h(ch):
                    _agg(FULL if is_fw(ch) else HALF, (ch, "_S 8px (cfg label: Hangul + ASCII only)"), key)
                elif ch not in MAP:
                    miss(ch, key, "_S 8px (cfg label, profile map)")
                elif MAP[ch] not in PLATE:
                    miss(ch, key, "8px plate (cfg label, ko_8p_block_250)")
            else:
                if o < 128:
                    if ch != " " and not ch.isalpha():
                        # gen_ko_cfg writes info-bar ASCII raw; ShowText pairs a non-letter byte with the next one
                        _agg(HALF, (ch, "ShowText raw (cfg info bar: gen_ko_cfg emits ASCII raw)"), key)
                    elif ch.isalpha():
                        KROM.setdefault("ShowText raw (cfg info bar)", []).append(key)
                    continue
                if not is_h(ch):
                    try:
                        sj = ch.encode("cp932")
                    except UnicodeEncodeError:
                        sj = b""
                    if len(sj) == 2 and 0x81 <= sj[0] <= 0x84:
                        continue  # a full-width symbol gen_ko_cfg writes as its SJIS pair (the retail font has it)
                    _agg(FULL if is_fw(ch) else HALF, (ch, "ShowText (cfg info bar: Hangul only)"), key)
                elif ch not in MAP:
                    miss(ch, key, "ShowText (cfg info bar, profile map)")


@guarded("1", "save texts")
def c1_save():
    if GSV is None:
        return
    for n, r in enumerate(read_rows(GSV.TSV)[1:], 2):
        if len(r) != 4:
            fail("2", "번역_저장문구.tsv line %d has %d columns (gen_ko_save unpacks exactly 4: 키 영문 일판 한글)" % (n, len(r)))
            continue
        key, ko = r[0], r[3]
        seen("1")
        seen("3")
        for ch in ko:
            if ch == " ":
                continue
            if ord(ch) > 127:
                if ch not in MAP:
                    miss(ch, key, "save text (gen_ko_save, profile map)")
            elif ch not in ENG253:
                _agg(HALF, (ch, "save text (gen_ko_save: every ASCII needs an engtab_253 cell)"), key)


SHEET = {}


@guarded("1", "sheet-2 labels")
def c1_sheet2():
    if GS2 is None:
        return
    for ch in GS2.UPLOAD2:
        seen("1")
        if ch not in MAP:
            miss(ch, "gen_ko_sheet2.UPLOAD2", "sheet-2 upload list (profile map)")
    titles = GS2.load_titles()
    SHEET["titles"] = titles
    for name, text in titles:
        for ch in text:
            if ch not in GS2.IDX:
                seen("1")
                if ch not in MAP:
                    miss(ch, "sheet-2 title %s" % name, "sheet-2 UPLOAD3 (profile map)")


@guarded("1", "hand-coded sources")
def c1_handcoded():
    pat = re.compile(r"\\x([Ff][0-3])\\x([0-9A-Fa-f]{2})")
    expf = P4 + "ko_handcoded_chars.txt"
    exp = set(io.open(expf, encoding="utf-8").read().strip()) if os.path.exists(expf) else None
    for f in ("src/st/lib/e_shop.c", "src/st/lib/ko_lib_names.h", "src/st/sel/2C048.c"):
        p = os.path.join(REPO, f)
        if not os.path.exists(p):
            fail("6", "%s missing (check_ko_glyph.py reads it)" % f)
            continue
        bad = {}
        for mm in pat.finditer(io.open(p, encoding="utf-8").read()):
            c = (int(mm.group(1), 16) << 8) | int(mm.group(2), 16)
            if not (0xF040 <= c <= 0xF3FF):
                continue
            seen("1")
            ch = INV.get(c)
            if ch is None:
                bad.setdefault("code %04X not in %s" % (c, MAP_USED), 0)
                bad["code %04X not in %s" % (c, MAP_USED)] += 1
            elif exp is not None and is_h(ch) and ch not in exp:
                bad.setdefault("%s decodes outside ko_handcoded_chars.txt" % ctext(ch), 0)
                bad["%s decodes outside ko_handcoded_chars.txt" % ctext(ch)] += 1
        for msg, n in sorted(bad.items()):
            fail("1", "%s: %s (x%d; check_ko_glyph FAILs)" % (f, msg, n))


@guarded("3", "LIB hand-coded strings")
def c3_lib():
    esc = re.compile(r"\\x[0-9A-Fa-f]{2}")
    # the Extra settings popup info bar (xset: rows that are not 8px labels) goes through the same ShowText
    for r in read_rows(GC.TSV)[1:]:
        if len(r) < 5 or not r[0].startswith("xset:") or r[1].startswith("메뉴"):
            continue
        seen("3")
        for ch in r[4]:
            o = ord(ch)
            if ch == " " or (o < 128 and ch.isalpha()):
                continue
            if o < 128:
                _agg(HALF, (ch, "ShowText raw (xset info bar: gen_ko_cfg refuses ASCII non-letters)"), r[0])
                continue
            if is_h(ch):
                if ch not in MAP:
                    miss(ch, r[0], "ShowText (xset info bar, profile map)")
                continue
            try:
                sj = ch.encode("cp932")
            except UnicodeEncodeError:
                sj = b""
            if not (len(sj) == 2 and 0x81 <= sj[0] <= 0x84):
                _agg(FULL if is_fw(ch) else HALF, (ch, "ShowText (xset info bar: Hangul + full-width symbols)"), r[0])
    p = os.path.join(REPO, "src/st/lib/e_shop.c")
    if os.path.exists(p):
        t = io.open(p, encoding="utf-8").read()
        m = re.search(r"D_us_80180FB4\[\] = \{(.*?)\n\};", t, re.S)
        if not m:
            fail("3", "e_shop.c: D_us_80180FB4 not found (check_ko_glyph FAILs)")
        else:
            for n, ln in enumerate(m.group(1).split("\n")):
                q = re.match(r'\s*"(.*)",?\s*$', ln)
                if q and re.search(r"\\x[Ff][0-3]", q.group(1)):
                    seen("3")
                    hw = sorted(set(re.findall(r"[0-9.,?!():]", esc.sub("", q.group(1)))))
                    if hw:
                        fail("3", "e_shop.c D_us_80180FB4 line %d: half-width %s in a KO bestiary desc "
                             "(func_us_801B0C40 reads non-letters as SJIS pairs: full-width required)" % (n, "".join(hw)))
    p = os.path.join(REPO, "src/st/lib/ko_lib_names.h")
    if os.path.exists(p):
        for q in re.finditer(r'^\s*"(.+)", //', io.open(p, encoding="utf-8").read(), re.M):
            seen("3")
            s = q.group(1)
            if not s.endswith("\\xFF\\x00") or "\\x20" in s:
                fail("3", "ko_lib_names.h entry not in func_us_801B1064 form (\\x00 space, \\xFF\\x00 end): %s" % s)


def flush_13():
    if not MAP:
        MISS.clear()
        fail("1", "no glyph map loaded: per-character coverage not reported (see check 6)")
    for (ch, enc), (n, keys) in sorted(MISS.items(), key=lambda x: (-x[1][0], x[0])):
        fail("1", "no glyph cell: %s x%d [%s] rows: %s" % (ctext(ch), n, enc, kl(keys)))
    for (ch, enc), (n, keys) in sorted(HALF.items()):
        note = ""
        if ch in ENG_BUILD and ch not in ENG253:
            note = " -- %s has it, but the encoder reads ko_engtab_253.bin" % os.path.basename(ENG_BUILD_PATH)
        fail("3", "%s x%d: this encoder has no cell / form for it [%s]%s rows: %s" % (ctext(ch), n, enc, note, kl(keys)))
    for (ch, enc), (n, keys) in sorted(FULL.items()):
        warn("3", "full-width %s x%d where half-width is expected [%s] rows: %s" % (ctext(ch), n, enc, kl(keys)))
    for enc, keys in sorted(KROM.items()):
        warn("3", "raw ASCII letters drawn by the BIOS Kanji ROM (grey) [%s] rows: %s" % (enc, kl(keys)))
    for ch, (n, keys) in sorted(DLG_ASCII.items()):
        warn("3", "dialogue ASCII %s x%d has no cell in %s%s (8x8 plate fallback) rows: %s" % (
            ctext(ch), n, os.path.basename(ENG_BUILD_PATH), " (predicted)" if ENG_BUILD_PREDICTED else "", kl(keys)))


# ===================================================================== check 2: keys
@guarded("2", "dialogue keys")
def c2_dialogue():
    byp = {}
    for r in dlg_rows():
        byp.setdefault(prefix(r[0]), []).append(r)
    for p, rows in sorted(byp.items()):
        seen("2", len(rows))
        users = []
        if p in SPLICE_PREFIX:
            users.append("gen_ko_splice")
        if p in DLG_STAGE:
            users.append("gen_ko_dlg")
        if p in MAR_PREFIX:
            users.append("gen_ko_mar")
        if BOSS_RE.match(p):
            users.append("gen_ko_splice(boss)")
        if not users:
            fail("2", "orphan dialogue prefix %s (%d rows): no generator reads it, and gen_ko_dlg.py asserts "
                 "every 대사 prefix is in its STAGE table" % (p, len(rows)))
            continue
        if p not in DLG_STAGE:
            fail("2", "dialogue prefix %s (%d rows, %s) is not in gen_ko_dlg.STAGE: gen_ko_dlg.py "
                 "(build step before the splice) stops on `assert pre in STAGE`" % (p, len(rows), "+".join(users)))
        idx = []
        for r in rows:
            m = re.match(r"^(\d+)(b?)$", r[0].rsplit(":", 1)[1])
            if not m:
                fail("2", "%s: key suffix is not N / Nb (splicer sorts by int)" % r[0])
                continue
            if m.group(2):
                if not any(x[0] == r[0][:-1] for x in rows):
                    fail("2", "%s: split row without its base row %s (gen_ko_splice.load_tsv asserts)" % (r[0], r[0][:-1]))
            else:
                idx.append(int(m.group(1)))
        gaps = sorted(set(range(max(idx) + 1)) - set(idx)) if idx else []
        if gaps:
            warn("2", "%s: index gaps %s (splicer takes rows in index order; a gap shifts nothing but is suspicious)" % (p, gaps[:10]))
        if len(set(idx)) != len(idx):
            fail("2", "%s: duplicate keys" % p)
        cm = [r[2].startswith("#") for r in rows if len(r) > 2]
        if any(cm) and not all(cm):
            fail("2", "%s: EN column mixes slot counts (#n/..) and text (splicer asserts)" % p)
        for r in rows:
            if len(r) > 2 and r[2].startswith("#") and not re.match(r"^#\d+(/\d+)*$", r[2]):
                fail("2", "%s: EN slot-count column %r malformed" % (r[0], r[2]))
    for p, (asset, ovl, opt) in sorted(SPLICE_PREFIX.items()):
        if p not in byp and not opt:
            fail("2", "splicer stage %s (%s) has no rows %s:N (gen_ko_splice `assert keys`)" % (ovl, asset, p))
        elif p not in byp:
            info("2", "optional stage %s: no %s:N rows (placeholder, retail kept)" % (ovl, p))
    for p in MAR_PREFIX:
        n = len(byp.get(p, []))
        if n != 7:
            fail("2", "%s: %d rows, gen_ko_mar.py asserts exactly 7" % (p, n))
    # gen_ko_wrap overrides naming rows that do not exist
    if GW is not None and os.path.exists(GW.OVR):
        keys = set(r[0] for r in V14[1:])
        for r in read_rows(GW.OVR)[1:]:
            if len(r) >= 2 and ":" in r[0] and r[0] not in keys:
                warn("2", "%s: override row %s names no 번역_v14 row" % (os.path.basename(GW.OVR), r[0]))


@guarded("2", "table use")
def c2_unused():
    used = ("enemydefs.h:g_EnemyDefs:", "enemydefs_mod.h:g_EnemyDefs:", "config_us.h:g_RelicDefs:",
            "config_us.h:g_MenuStr:97", "config_us.h:g_MenuStr:98")
    other = {}
    for r in V14[1:]:
        if not r or (len(r) > 1 and r[1] == "대사"):
            continue
        if not r[0].startswith(used):
            other.setdefault(r[0].split(":")[0] + ":" + (r[0].split(":")[1] if ":" in r[0] else ""), 0)
            other[r[0].split(":")[0] + ":" + (r[0].split(":")[1] if ":" in r[0] else "")] += 1
    if other:
        info("2", "번역_v14 rows no build tool reads (hand-ported in C or reference): %s"
             % ", ".join("%s %d" % kv for kv in sorted(other.items())))
    # equipment: the build reads 번역_v4_장비.tsv, not v14
    if EQ_KO:
        v14 = dict((r[0], r[V14_TI].strip()) for r in V14[1:] if len(r) > V14_TI and r[0].split(":")[0] in
                   ("equipment.h", "accessories.h", "equipment_mod.h", "accessories_mod.h"))
        diff = [k for k, (en, s) in EQ_KO.items() if k in v14 and v14[k] and v14[k] != s.strip()]
        if diff:
            warn("2", "equipment: 번역_v14.tsv and 번역_v4_장비.tsv disagree on %d rows (the build bakes 번역_v4_장비): %s"
                 % (len(diff), kl(sorted(diff))))


@guarded("2", "relic keys")
def c2_relic():
    if GN is None:
        return
    tbl = GN.load_table()
    en = GN.relic_en_rows()
    seen("2", 63)
    if len(en) != 31:
        fail("2", "config_us.h g_RelicDefs: %d EN rows, gen_ko_names asserts 31" % len(en))
    if not tbl.get("config_us.h:g_RelicDefs:card:suffix", ""):
        fail("2", "config_us.h:g_RelicDefs:card:suffix is empty (gen_ko_names asserts)")
    for i in range(31):
        for part in ("name", "desc"):
            k = "config_us.h:g_RelicDefs:%d:%s" % (i, part)
            v = tbl.get(k, "")
            if not v:
                fail("2", "%s empty / missing (gen_ko_names asserts)" % k)
            elif part == "desc" and GN.CARD_TOKEN in v:
                fail("2", "%s holds %s (only names may)" % (k, GN.CARD_TOKEN))
            elif part == "name" and "{" in v.replace(GN.CARD_TOKEN, ""):
                fail("2", "%s holds a stray '{'" % k)
    if os.path.exists(GN.TSV):
        n = sum(1 for r in read_rows(GN.TSV)[1:] if r[0].startswith("config_us.h:g_RelicDefs:") and len(r) > 2 and r[2])
        if not n:
            skip("2", "relic EN vs retail: the table's EN column is empty (public TSV)")


@guarded("2", "field keys / EN")
def c2_field():
    if GF is None:
        return
    rows = [r for r in read_rows(GF.TSV)[1:] if len(r) >= 5 and r[0].startswith("field:")]
    have = set()
    srcs = [os.path.join(REPO, f) for f in ("src/st/e_mist_door.h", "src/st/e_sealed_door.h", "src/st/no4/first_c_file.c")]
    blob = "\n".join(io.open(p, encoding="utf-8").read() for p in srcs if os.path.exists(p))
    todo = []
    for r in rows:
        seen("2")
        key = r[0][6:]
        have.add(key)
        if key not in GF.EN_SIZEOF:
            fail("2", "%s: unknown field key (gen_ko_fieldmsg asserts; EN_SIZEOF has %s)" % (r[0], ", ".join(sorted(GF.EN_SIZEOF))))
            continue
        if r[4].strip() in GF.PLACEHOLDER:
            todo.append(r[0])
        macro = "KO_FIELD_" + key.upper()
        m = re.search(r"#if defined\(%s\)(.*?)#else(.*?)#endif" % macro, blob, re.S)
        if not m:
            (fail if r[4].strip() not in GF.PLACEHOLDER else warn)(
                "2", "%s: no `#if defined(%s)` site in the stage sources (the label is never used)" % (r[0], macro))
            continue
        body = "".join(re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(2)))
        size, text = lit_size_text(body)
        if size != GF.EN_SIZEOF[key]:
            fail("4", "%s: gen_ko_fieldmsg EN_SIZEOF %d != sizeof the US label in C %d (pad would shift overlay data)"
                 % (r[0], GF.EN_SIZEOF[key], size))
        want = r[2].replace("．", ".")
        if want and want != text:
            warn("2", "%s: EN column %r != the retail US label %r" % (r[0], want, text))
    for key in sorted(set(GF.EN_SIZEOF) - have):
        info("2", "field:%s has no row (placeholder, English kept)" % key)
    if todo:
        info("2", "field messages still placeholders (TODO / empty, English shown): %s" % kl(todo))


def lit_size_text(body):
    """C literal body (sotn_str / SJIS sizes: non-ASCII char = 2 B) -> (sizeof incl NUL, text after the
    2 header bytes, \\x01 -> '/', '．' -> '.')"""
    out = []
    i = 0
    while i < len(body):
        c = body[i]
        if c == "\\" and i + 1 < len(body):
            n = body[i + 1]
            if n == "x":
                j = i + 2
                while j < len(body) and j < i + 4 and body[j] in "0123456789abcdefABCDEF":
                    j += 1
                out.append(chr(int(body[i + 2:j], 16)))
                i = j
                continue
            if n in "01234567":
                j = i + 1
                while j < len(body) and j < i + 4 and body[j] in "01234567":
                    j += 1
                out.append(chr(int(body[i + 1:j], 8)))
                i = j
                continue
            out.append({"n": "\n", "t": "\t", "0": "\0"}.get(n, n))
            i += 2
            continue
        out.append(c)
        i += 1
    size = sum(1 if ord(c) < 256 else 2 for c in out) + 1
    text = "".join(out[2:]).replace("\x01", "/").replace("．", ".")
    return size, text


@guarded("2", "cfg / save symbols")
def c2_symbols():
    menu = os.path.join(REPO, "src/dra/menu.c")
    if CFG_ROWS and os.path.exists(menu):
        refs = set(re.findall(r"\bko_((?:str|desc)\d+)\b", io.open(menu, encoding="utf-8").read()))
        keys = set(r[0][4:] for r in CFG_ROWS)
        seen("2", len(keys))
        for k in sorted(refs - keys):
            fail("2", "src/dra/menu.c uses ko_%s but 번역_설정화면.tsv has no cfg:%s (compile error)" % (k, k))
        for k in sorted(keys - refs):
            warn("2", "cfg:%s is baked but src/dra/menu.c never uses ko_%s" % (k, k))
    sv = os.path.join(REPO, "src/dra/62DEC.c")
    if GSV is not None and os.path.exists(sv) and os.path.exists(GSV.TSV):
        refs = set(re.findall(r"\b(KS_[A-Z0-9_]+)\b", io.open(sv, encoding="utf-8").read()))
        keys = set(r[0] for r in read_rows(GSV.TSV)[1:] if r and r[0])
        seen("2", len(keys))
        for k in sorted(refs - keys):
            fail("2", "src/dra/62DEC.c uses %s but 번역_저장문구.tsv has no row (compile error)" % k)
        for k in sorted(keys - refs):
            warn("2", "save text %s is baked but src/dra/62DEC.c never uses it" % k)


# ===================================================================== check 4: lengths
@guarded("4", "relic / enemy lengths")
def c4_names():
    if GN is None or E is None:
        return
    tbl = GN.load_table()
    en = GN.relic_en_rows()
    suffix = tbl.get("config_us.h:g_RelicDefs:card:suffix", "")
    total = 0
    ok = True
    for i in range(min(31, len(en))):
        name = tbl.get("config_us.h:g_RelicDefs:%d:name" % i, "").replace(GN.CARD_TOKEN, suffix)
        desc = tbl.get("config_us.h:g_RelicDefs:%d:desc" % i, "")
        if not name or not desc:
            ok = False
            continue
        if en[i].startswith(" "):
            name = " " + name
        seen("4", 2)
        px = E.show_px(name)
        if px > GN.RELIC_NAME_PX_MAX:
            fail("4", "relic %d name %r %d px > %d (relic list row)" % (i, name.strip(), px, GN.RELIC_NAME_PX_MAX))
        px = E.show_px(desc)
        if px > GN.RELIC_DESC_PX_MAX:
            fail("4", "relic %d desc %r %d px > %d (info bar)" % (i, desc, px, GN.RELIC_DESC_PX_MAX))
        try:
            nb, db = E.enc_show(name), E.enc_show(desc)
        except Exception:  # noqa: BLE001 -- reported by check 1 / 3
            ok = False
            continue
        if len(nb) >= 38:
            fail("4", "relic %d name %d B >= 38 (func_800F9E18 strcpy buffer[38])" % (i, len(nb)))
        total += GN.al4(len(nb) + 1) + GN.al4(len(db) + 1)
    rgrow = total - GN.RELIC_BASELINE_ALIGNED
    egrow = None
    gen = os.path.join(GN.GEN, "enemydefs.h")
    if os.path.exists(gen):
        egrow = 0
        try:
            kolib = GN.load_ko_lib()
        except Exception as ex:  # noqa: BLE001
            fail("1", "src/st/lib/ko_lib_names.h fallback names: %s" % exc(ex))
            kolib = {}
        missing = []
        for src in ("enemydefs.h", "enemydefs_mod.h"):
            path = os.path.join(GN.GEN, src)
            if not os.path.exists(path):
                continue
            rows = [m for m in (GN.ROW_RE.match(ln) for ln in io.open(path, encoding="latin-1").read().split("\n")) if m]
            ko, share = {}, {}
            for m in rows:
                idx = int(m.group(2), 16)
                e = GN.en_unescape(m.group(3))
                k = tbl.get("%s:g_EnemyDefs:0x%03X" % (src, idx), "")
                if not k and src == "enemydefs.h" and kolib.get(idx):
                    k = kolib[idx]
                if k:
                    ko[idx] = k
                    if e:
                        share.setdefault(e, k)
            ids = set(int(m.group(2), 16) for m in rows)
            for k in tbl:
                mm = re.match(r"^%s:g_EnemyDefs:0x([0-9A-Fa-f]+)$" % re.escape(src), k)
                if mm and int(mm.group(1), 16) not in ids:
                    warn("2", "%s: no such id in src/dra/gen/us/%s (orphan row)" % (k, src))
            for m in rows:
                idx = int(m.group(2), 16)
                e = GN.en_unescape(m.group(3))
                k = ko.get(idx) or (share.get(e) if e else None)
                if not k:
                    if e:
                        missing.append("%s 0x%03X %s" % (src, idx, e))
                    continue
                seen("4")
                try:
                    kb = E.enc_s(k)
                except Exception:  # noqa: BLE001 -- check 1
                    continue
                slot = GN.en_len(e) + 2
                if len(kb) > slot:
                    egrow += GN.al4(len(kb) + 1) - GN.al4(slot + 1)
        if missing:
            warn("2", "%d enemy ids with an EN name and no Korean anywhere (gen_ko_names lists them; --strict fails): %s"
                 % (len(missing), kl(missing, 4)))
    else:
        skip("4", "enemy name slots / DRA growth: src/dra/gen/us/enemydefs.h not extracted yet (relic part checked alone)")
    grow = rgrow + (egrow or 0)
    if ok and grow > 696:
        fail("4", "KO names grow DRA rodata by %d B (relics %+d, enemies %s) > MOD_DRA_PAD 696 (gen_ko_names FAILs)"
             % (grow, rgrow, "%+d" % egrow if egrow is not None else "not measured"))
    else:
        info("4", "KO_NAMES_DRA_GROW = %d (relics %+d, enemies %s) <= 696" % (grow, rgrow, egrow if egrow is not None else "n/a"))


@guarded("4", "field box")
def c4_field():
    if GF is None or E is None:
        return
    for r in read_rows(GF.TSV)[1:]:
        if len(r) < 5 or not r[0].startswith("field:") or r[4].strip() in GF.PLACEHOLDER:
            continue
        key, ko = r[0][6:], r[4].strip()
        seen("4")
        lines = ko.split("/")
        if len(lines) > GF.MAX_LINES:
            fail("4", "%s: %d lines > %d" % (r[0], len(lines), GF.MAX_LINES))
        w = (E.show_px(ko, linebreak="/") + 8 + 3) & ~3
        if w > GF.MAX_W:
            fail("4", "%s: box %d px > 0x%X" % (r[0], w, GF.MAX_W))
        try:
            body = E.enc_show(ko, linebreak="/")
        except Exception:  # noqa: BLE001 -- check 1 / 3
            continue
        if 0 in body:
            fail("4", "%s: NUL inside the label bytes" % r[0])
        size = len(body) + 3
        if key in GF.EN_SIZEOF and size > GF.EN_SIZEOF[key]:
            warn("4", "%s: %d B > US label %d B: the overlay's data grows (NO4 has little margin)" % (r[0], size, GF.EN_SIZEOF[key]))


@guarded("4", "equipment lengths")
def c4_equip():
    if GE is None:
        return
    for key, (en, s) in sorted(EQ_KO.items()):
        if key.endswith(":name"):
            seen("4")
            if len(s) > GE.KO_EQ_NAME_MAX:
                fail("4", "%s: %r %d chars > %d (12 px x 10 name plate; gen_ko_equip asserts)" % (key, s, len(s), GE.KO_EQ_NAME_MAX))
    gen = GE.GEN
    if not (os.path.exists(os.path.join(gen, "equipment_mod.h")) and os.path.exists(os.path.join(gen, "accessories_mod.h"))):
        skip("4", "equipment in-place vs KO_JP bundle size: src/dra/gen/us/equipment_mod.h not generated yet")
        skip("2", "equipment EN (#h sha) vs retail: src/dra/gen/us/equipment_mod.h not generated yet")
        return
    mp11 = GE.load_map_v11()
    row = re.compile(r'^(/\* 0x([0-9A-F]+) \*/) \{_S\("((?:[^"\\]|\\.)*)"\),(.*)$')
    bundle = 0
    nwarn = []
    for mod_src, retail_src, tag in (("equipment_mod.h", "equipment.h", "g_EquipDefs"),
                                     ("accessories_mod.h", "accessories.h", "g_AccessoryDefs")):
        for line in io.open(os.path.join(gen, mod_src), encoding="utf-8").read().split("\n"):
            m = row.match(line)
            if not m:
                continue
            idx, en = "0x" + m.group(2), m.group(3)
            if not all(0x20 <= ord(c) < 0x7F for c in en):
                continue
            nkey = GE.mod_key(mod_src, retail_src, tag, idx, EQ_KO, "name")
            if nkey is None:
                continue
            want, s = EQ_KO[nkey]
            match = GE.en_match(want, en)
            if not match:
                nwarn.append(nkey)
            try:
                kb = GE.ko_name_bytes(s, mp11)
            except Exception:  # noqa: BLE001 -- check 1
                continue
            if not (len(kb) + 2 <= len(en) + 1 and match):
                bundle += len(kb) + 3
    seen("2", len(EQ_KO))
    if nwarn:
        warn("2", "equipment names whose EN (#h sha) does not match the generated header (go to the bundle): %s" % kl(nwarn))
    if not 1 <= bundle <= 2048:
        fail("4", "KO_JP.BIN equipment-name bundle %d B (build asserts 1..2048, one sector)" % bundle)
    else:
        info("4", "KO_JP.BIN bundle %d / 2048 B" % bundle)


@guarded("4", "cfg / sheet-2 lengths")
def c4_menu():
    for r in CFG_ROWS:
        if r[1].startswith("메뉴"):
            seen("4")
            if len(r[4]) > 10:
                fail("4", "%s: %r %d chars > 10 (8px plate parse; gen_ko_cfg asserts)" % (r[0], r[4], len(r[4])))
    if GS2 is not None and "titles" in SHEET:
        up3 = []
        for name, text in SHEET["titles"]:
            seen("4")
            if not 0 < len(text) < GS2.TITLE_SLOT:
                fail("4", "sheet-2 title %s %r: %d cells, slot holds %d (gen_ko_sheet2 asserts)" % (name, text, len(text), GS2.TITLE_SLOT - 1))
            up3 += [c for c in text if c not in GS2.IDX and c not in up3]
        if len(up3) > GS2.UPLOAD3_MAX:
            fail("4", "sheet-2 UPLOAD3 needs %d glyphs > %d (one sheet row): %s" % (len(up3), GS2.UPLOAD3_MAX, "".join(up3)))


@guarded("4", "TSV 폭한계 column")
def c4_tsv_limits():
    for path in (DEF_V14, KO_WK + "/번역/번역_v4_장비.tsv"):
        if not os.path.exists(path):
            continue
        rows = read_rows(path)
        h = rows[0]
        if "폭한계" not in h:
            continue
        li = h.index("폭한계")
        ti = [i for i, x in enumerate(h) if x.startswith("번역")][0]
        oi = h.index("넘침여부") if "넘침여부" in h else None
        over = []
        marked = []
        for r in rows[1:]:
            if len(r) <= max(li, ti) or (len(r) > 1 and r[1] == "대사"):
                continue
            t = r[ti].strip()
            if not t or not r[li].strip().isdigit():
                continue
            seen("4")
            b = sum(1 if ord(c) < 128 else 2 for c in t)  # the table's 글자수(B) rule
            if b > int(r[li]):
                over.append("%s %dB>%s" % (r[0], b, r[li]))
            elif oi is not None and len(r) > oi and "넘침" in r[oi]:
                marked.append(r[0])
        if over:
            warn("4", "%s: %d rows over their 폭한계 (bytes): %s" % (os.path.basename(path), len(over), kl(over, 8)))
        if marked:
            warn("4", "%s: rows the table itself marks 넘침: %s" % (os.path.basename(path), kl(marked)))


# ===================================================================== dialogue dry run (checks 2 / 4 / 7)
TMP = tempfile.mkdtemp(prefix="ko_preflight_")


def ops_of(lines):
    ops = []
    for ln in lines:
        m = CS.ROW.match(ln)
        if m:
            ops.append((m.group(1), (m.group(2) or "").strip()))
    return ops


def classify(label, ex, where="splice"):
    a = ex.args[0] if len(ex.args) == 1 else ex.args
    s = repr(a)
    if isinstance(ex, ValueError) or "straddles" in s or "C entry" in s or "jump target" in s:
        fail("7", "%s: %s" % (label, s))
    elif isinstance(a, tuple) and len(a) >= 2 and a[1] == "end":
        fail("4", "%s: KO bytes exceed the EN script (%s); the 0x0D pad cannot absorb it" % (label, s))
    elif isinstance(a, tuple) and len(a) == 5:
        fail("2", "%s: EN segment %s:%s does not match the retail script: TSV %s, script %s"
             % (label, a[1], a[2], a[3], a[4]))
    elif isinstance(a, tuple) and len(a) == 4 and where == "mar":
        fail("4", "%s: %s has %d KO lines > %d EN groups (gen_ko_mar asserts)" % (label, a[1], a[2], a[3]))
    elif isinstance(a, str) and len(a) <= 4 and not isinstance(ex, KeyError):
        fail("1", "%s: no glyph cell for %s (%s encoder)" % (label, a, where))
    else:
        fail("2", "%s: %s stops: %s" % (label, where, exc(ex)))


@guarded("4", "gen_ko_wrap dry run")
def wrap_dry():
    if GW is None:
        return None
    if not os.path.exists(GW.SRC):
        fail("6", "gen_ko_wrap source %s missing (KO_OUTD=%r)" % (rp(GW.SRC), os.environ.get("KO_OUTD")))
        return None
    GW.OUT = os.path.join(TMP, "wrapped.tsv")
    GW.HAND = os.path.join(TMP, "hand.tsv")
    with quiet():
        GW.main()
    ovr = set()
    if os.path.exists(GW.OVR):
        ovr = set(r[0] for r in read_rows(GW.OVR))
    rows = read_rows(GW.OUT)
    ti = [i for i, x in enumerate(rows[0]) if x.startswith("번역")][0]
    for r in rows[1:]:
        if len(r) <= ti or r[1] != "대사" or r[0].split(":")[0] not in GW.SCENES or not r[ti]:
            continue
        seen("4")
        if r[2].startswith("#"):
            cs = r[2][1:].split("/")
            scroll = len(cs) > 1 and "0" in cs
        else:
            scroll = "\\n\\n" in r[2] or r[2].startswith("\\n") or r[2].endswith("\\n")
        src = " (override %s)" % os.path.basename(GW.OVR) if r[0] in ovr else ""
        for wi, win in enumerate(r[ti].split("|")):
            lines = [l for l in win.split("/") if l]
            for l in lines:
                px = FP.text_px_norm(l)
                if px > GW.W:
                    fail("4", "%s window %d: line %r %d px > %d px%s" % (r[0], wi, l, px, GW.W, src))
            if not scroll and len(lines) > GW.NL:
                (fail if r[0].split(":")[0] in STRUCT_STAGES else warn)(
                    "4", "%s window %d: %d lines > window_lines %d%s" % (r[0], wi, len(lines), GW.NL, src))
    return GW.OUT


@guarded("2", "gen_ko_promo dry run")
def promo_dry():
    if GP is None or S is None:
        return False
    GP.OUT_BIN = os.path.join(TMP, "promo.bin")
    GP.OUT_MAP = os.path.join(TMP, "promo_map.tsv")
    try:
        with quiet():
            GP.main()
    except AssertionError as ex:
        fail("1", "gen_ko_promo.py stops: %s (a frequent dialogue char has no cell)" % exc(ex))
        return False
    S.PROMO_MAP = GP.OUT_MAP
    S.PROMO = S.load_promo()
    if open(GP.OUT_MAP, "rb").read() != open(P4 + "ko_promo_map.tsv", "rb").read():
        info("5", "gen_ko_promo will rewrite ko_promo_map.tsv / ko_promo_tbl.bin (dialogue frequencies changed)")
    return True


def retail_yaml(asset):
    """(lines or None, why): retail script lines as gen_ko_splice._read_yaml_retail would see them"""
    path = os.path.join(REPO, "assets", asset)
    bak = path + ".ko_retail"
    if os.path.exists(bak):
        return io.open(bak, encoding="utf-8").read(), None
    if not os.path.exists(path):
        return None, "not extracted"
    cur = io.open(path, encoding="utf-8").read()
    if S.MARK in cur:
        return None, "spliced yaml without .ko_retail backup (gen_ko_splice asserts)"
    return cur, None


BOSS_PREFIXES = set()


@guarded("7", "splice dry run")
def splice_dry(wrapped):
    if S is None or EO is None or wrapped is None:
        skip("7", "splice dry run: gen_ko_splice / gen_ko_wrap not usable")
        return
    S.TSV = wrapped
    try:
        tsv = S.load_tsv()
    except Exception as ex:  # noqa: BLE001
        classify("gen_ko_splice.load_tsv", ex)
        return
    mp = S.load_map()
    sizes, _bins = S.config_sizes()
    done = 0
    for asset, overlay, symbol, prefs in S.STAGES:
        cur, why = retail_yaml(asset)
        if cur is None:
            if why == "not extracted":
                skip("7", "%s not extracted (make build_assets / a previous build): EN match, KO<=EN, entry offsets skipped" % asset)
            else:
                fail("2", "%s: %s" % (asset, why))
            continue
        lines = [l for l in cur.split("\n") if l.strip() != ""]
        rows = S.split_rows(lines)
        nm0 = asset.split("/")[-1].rsplit(".", 1)[0]
        key0 = ("assets/" + asset.rsplit("/", 1)[0], nm0)
        if key0 not in sizes:
            fail("2", "%s: no cutscene size in config/assets.us.yaml (gen_ko_splice asserts)" % asset)
            continue
        start0, size0 = sizes[key0]
        entries = EO.offsets_for("assets/" + asset)
        seen("2")
        seen("4")
        seen("7")
        try:
            out, st = S.splice_rows(asset, rows, start0, size0, entries, prefs, tsv, mp, overlay in S.OPTIONAL)
        except S.Untranslated:
            info("2", "%s: no translated %s rows (placeholder, retail kept)" % (asset, prefs[0]))
            continue
        except Exception as ex:  # noqa: BLE001
            classify(asset, ex)
            continue
        done += 1
        new_rows = S.split_rows(out)
        for e in EO.verify(rows, new_rows, S._row_size, entries, asset):
            fail("7", e)
        if CS is not None:
            bad, _n = CS.compare(asset, ops_of(lines), ops_of(out))
            for b in bad:
                (fail if prefs[0] in STRUCT_STAGES else warn)("4", "cutscene struct: %s" % b)
        info("4", "%s: EN %d B, KO %d B, pad %d B, %d keys" % (asset, st[0], st[1], st[2], st[3]))
    for k, msg in S.WARN:
        warn("4", "%s: %s" % (k, msg))
    # LINKED_ENTRIES: the left ferryman yaml must keep its retail size (verify above checks every size)
    # boss (asm) overlays
    for ovl in sorted(EO.BOSS_SCRIPTS):
        binp = EO.BOSS_SCRIPTS[ovl][0]
        if not os.path.exists(os.path.join(REPO, "disks", "us", binp)):
            skip("7", "boss %s: disks/us/%s absent (the splicer skips it too)" % (ovl, binp))
            for a in EO.BOSS_SCRIPTS[ovl][2]:
                BOSS_PREFIXES.add(S.boss_prefix(ovl, a - EO.BASE))
            continue
        try:
            retail, regs = S.boss_regions(ovl)
        except Exception as ex:  # noqa: BLE001
            fail("7", "boss %s regions: %s" % (ovl, exc(ex)))
            continue
        for off, toks, ents in regs:
            BOSS_PREFIXES.add(S.boss_prefix(ovl, off))
            label = "%s@0x%X" % (binp, off)
            seen("7")
            rows = S.toks_to_rows(toks)
            old = S.rows_to_bytes(rows)
            if old != retail[off:off + len(old)]:
                fail("7", "%s: token round trip differs from the retail BIN" % label)
                continue
            try:
                out, st = S.splice_rows(label, rows, off, len(old), ents, [S.boss_prefix(ovl, off)], tsv, mp, True)
            except S.Untranslated:
                continue
            except Exception as ex:  # noqa: BLE001
                classify(label, ex)
                continue
            if not S.BOSS_ASM_OK:
                fail("2", "%s: KO rows for an asm-rendered boss (no KO glyph path): gen_ko_splice stops unless "
                     "KO_BOSS_ASM_RENDER=1" % label)
            new = S.rows_to_bytes(S.split_rows(out))
            for e in EO.verify_bytes(old, new, [0] + ents, label):
                fail("7", e)
    if not done:
        skip("2", "cutscene EN vs retail (splicer's strict EN match): no retail yaml extracted yet")
        skip("4", "cutscene KO <= EN byte budget per entry segment: no retail yaml extracted yet")


@guarded("2", "boss key prefixes")
def c2_boss():
    for r in dlg_rows():
        m = BOSS_RE.match(prefix(r[0]))
        if m and BOSS_PREFIXES and prefix(r[0]) not in BOSS_PREFIXES:
            fail("2", "%s: no boss script region starts at BIN +0x%s (known: %s)" % (r[0], m.group(2), kl(sorted(BOSS_PREFIXES))))


@guarded("2", "gen_ko_mar dry run")
def mar_dry():
    path = os.path.join(HERE, "gen_ko_mar.py")
    tree = ast.parse(io.open(path, encoding="utf-8").read())
    tree.body = [n for n in tree.body if not (isinstance(n, ast.Expr) and isinstance(n.value, ast.Call)
                                              and isinstance(n.value.func, ast.Name) and n.value.func.id == "main")]
    ns = {"__name__": "gen_ko_mar_preflight", "__file__": path}
    exec(compile(tree, path, "exec"), ns)
    hdr, bak = ns["HDR"], ns["BAK"]
    # static: KO lines <= EN groups (count mode), no header needed
    for p in MAR_PREFIX:
        for k, (en, ko) in sorted(ns["load_rows"](p + ":").items()):
            seen("4")
            if en.startswith("#"):
                groups = len([c for c in en[1:].split("/") if c != "0"])
                knon = len([x for x in ko.split("/") if x != ""])
                if knon > groups:
                    fail("4", "%s: %d KO lines > %d EN groups (gen_ko_mar asserts; no extra-line path there)" % (k, knon, groups))
    if not os.path.exists(hdr):
        skip("2", "Saturn Maria EN vs retail: %s is made by gen_cen_maria.py during the build" % rp(hdr))
        return
    src = io.open(hdr, encoding="utf-8").read()
    if "g_MbScriptPre[] MOD_RO = {\n#ifdef MOD_KO" in src:
        if not os.path.exists(bak):
            fail("2", "%s KO-patched without its .ko_en backup (gen_ko_mar asserts)" % rp(hdr))
            return
        src = io.open(bak, encoding="utf-8").read()
    mp = ns["load_map"]()
    for arr, want in (("g_MbScriptPre", "mod_maria_boss_data.h:g_MbScriptPre:"),
                      ("g_MbScriptPost", "mod_maria_boss_data.h:g_MbScriptPost:")):
        rows = ns["load_rows"](want)
        m = re.search(r"u8 " + arr + r"\[\] MOD_RO = \{(.*?)\n\};", src, re.S)
        if not m:
            fail("2", "%s: %s not found (gen_cen_maria output changed?)" % (rp(hdr), arr))
            continue
        seen("2")
        try:
            ns["convert"](m.group(1), list(rows.keys()), rows, mp, arr)
        except Exception as ex:  # noqa: BLE001
            classify("gen_ko_mar %s" % arr, ex, "mar")


# ===================================================================== check 5: cell budget
def dra_define(txt, name):
    m = re.search(r"#define %s (0x[0-9A-Fa-f]+|\d+)" % name, txt)
    return int(m.group(1), 0) if m else None


@guarded("5", "glyph block")
def c5_cells():
    dra_p = os.path.join(REPO, "src/dra/dra.h")
    dra = io.open(dra_p, encoding="utf-8").read()
    cb = FP.P["cell_bytes"]
    blk_path = P4 + PROFILE_BLK
    want_blk = BX.get("KO_BLK")
    if want_blk and os.path.normpath(want_blk) != os.path.normpath(blk_path):
        fail("5", "build_ko_trial.sh KO_BLK %s != profile blk_name %s" % (rp(want_blk), rp(blk_path)))
    predicted = False
    if os.path.exists(blk_path):
        blk = open(blk_path, "rb").read()
        if len(blk) % cb:
            fail("5", "%s: %d B not a multiple of %d" % (PROFILE_BLK, len(blk), cb))
        cells = len(blk) // cb
        sha = hashlib.sha256(blk).hexdigest()
        ks = BX.get("KO_SHA", "")
        if ks != sha:
            fail("5", "build_ko_trial.sh KO_SHA %s != block sha256 %s%s" % (
                ks[:16] + "..", sha, " (placeholder: paste the sha)" if set(ks) == {"0"} else ""))
    else:
        prev = re.sub(r"v14d", "v14c", PROFILE_BLK)
        if prev != PROFILE_BLK and os.path.exists(P4 + prev):
            cells = len(open(P4 + prev, "rb").read()) // cb + len(NEW_D)
            predicted = True
            fail("5", "glyph block %s missing (run `gen_ko_glyph_lat.py d`); budget below is predicted: "
                 "%s + %d NEW_D cells = %d" % (PROFILE_BLK, prev, len(NEW_D), cells))
        else:
            fail("5", "glyph block %s missing; no budget" % PROFILE_BLK)
            return
        if set(BX.get("KO_SHA", "")) == {"0"}:
            fail("5", "build_ko_trial.sh KO_SHA is the all-zero placeholder (the ARC_KO step asserts the sha)")
    seen("5")
    tag = " (predicted)" if predicted else ""
    cnt = dra_define(dra, "KO_GLYPH_COUNT")
    if cnt != cells:
        fail("5", "dra.h KO_GLYPH_COUNT %s != block cells %d%s" % (cnt, cells, tag))
    if dra_define(dra, "KO_GLYPH_CELL_BYTES") != cb:
        fail("5", "dra.h KO_GLYPH_CELL_BYTES %s != profile cell_bytes %d" % (dra_define(dra, "KO_GLYPH_CELL_BYTES"), cb))
    if bool(re.search(r"(?m)^#define KO_GLYPH_12COL", dra)) != (cb == 23):
        fail("5", "dra.h KO_GLYPH_12COL vs profile cell_bytes %d disagree" % cb)
    if cnt is not None and not 819 <= cnt <= 960:
        fail("5", "dra.h KO_GLYPH_COUNT %d outside 819..960 (ARC_KO step asserts)" % cnt)
    limit = 0x5640  # KO_POP_TEXT - KO_GLYPH_BLOCK_ADDR: cells must end before the resident KO_PopupText
    pt = dra_define(dra, "KO_POP_TEXT")
    if pt is not None and pt - 0x80099000 != limit:
        fail("5", "dra.h KO_POP_TEXT 0x%X moved (build asserts block offset 0x5640)" % pt)
    used = cells * cb
    free = (limit - used) // cb
    need = len(set(ch for (ch, enc) in MISS if "plate" not in enc and "v4" not in enc))
    (fail if used > limit else info)("5", "glyph cells %d x %d B = %d B of %d B (0x5640)%s; %d cells free, %d missing chars from check 1%s"
                                     % (cells, cb, used, limit, tag, free, need,
                                        " -- adding them would NOT fit" if need > free else ""))
    if cells > 960:
        fail("5", "cells %d > 960 (code space F040..F3FF)" % cells)
    if used - 23280 > (dra_define(dra, "KO_BLOB_SLOT") or 896):
        fail("5", "cells past 23280 B do not fit the 896 B blob slot")
    # map / engtab codes inside the block, one char per cell
    codes = {}
    for ch, code in MAP.items():
        codes.setdefault(code, []).append(ch)
        if not 0xF040 <= code < 0xF040 + cells:
            fail("5", "map %s: %s code %04X outside the %d cells" % (MAP_USED, ctext(ch), code, cells))
    for code, chs in sorted(codes.items()):
        if len(chs) > 1:
            fail("5", "map %s: cell %04X shared by %s (check_ko_glyph FAILs)" % (MAP_USED, code, "".join(chs)))
    if not ENG_BUILD_PREDICTED:
        for ch, code in sorted(ENG_BUILD.items()):
            if not 0xF040 <= code < 0xF040 + cells:
                fail("5", "%s: %r -> %04X outside the %d cells" % (os.path.basename(ENG_BUILD_PATH), ch, code, cells))
        old = ENG253
        chg = [ch for ch, c in old.items() if ENG_BUILD.get(ch) != c]
        if chg:
            fail("5", "%s changes existing engtab_253 entries %s" % (os.path.basename(ENG_BUILD_PATH), "".join(chg)))
    # ARC_KO layout defines the build asserts
    for name, val in (("KO_GLYPH_BASE", 0xF040), ("KO_ARC_SIZE", 57344), ("KO_EQUIP_BASE", 0x8009F500),
                      ("KO_EQUIP_SIZE", 0xB00), ("KO_SHEET2_ADDR", 0x8009F100), ("KO_PROMO_TBL_SLOTS", 112)):
        v = dra_define(dra, name)
        if v != val:
            fail("5", "dra.h %s = %s, the ARC_KO step asserts 0x%X" % (name, v, val))
    pa = dra_define(dra, "KO_PROMO_TBL_ADDR")
    if pa is None or pa - 0x80099000 != 0x6000:
        fail("5", "dra.h KO_PROMO_TBL_ADDR %s (ARC_KO step asserts offset 0x6000)" % pa)


@guarded("5", "tables")
def c5_tables():
    for name, size in (("ko_promo_tbl.bin", 224), ("ko_font8_adv.bin", None)):
        p = P4 + name
        if os.path.exists(p) and size is not None and len(open(p, "rb").read()) != size:
            fail("5", "%s: %d B, build asserts %d" % (name, len(open(p, "rb").read()), size))
    if S is not None and getattr(S, "PROMO", None):
        seen("5")
        bad = [ch for ch, b in S.PROMO.items() if ch not in MAP]
        if bad:
            fail("5", "promotion map chars without a map cell: %s" % "".join(bad))
    sh = P4 + "ko_sheet2.bin"
    if os.path.exists(sh) and len(open(sh, "rb").read()) > 0x100:
        fail("5", "ko_sheet2.bin > 0x100 B (KO_FONT8_ADV follows at 0x6200)")
    pb = P4 + "ko_8p_block_250.bin"
    if os.path.exists(pb):
        b = open(pb, "rb").read()
        pn = struct.unpack("<H", b[:2])[0]
        seen("5")
        eqc = os.path.join(REPO, "mods/sotn-ultimate/src/menu/eq_core.c")
        pin = re.search(r"if [(]n == ([0-9]+)[)]", io.open(eqc, encoding="utf-8").read()) if os.path.exists(eqc) else None
        if len(b) != 2 + pn * 10 or len(b) > 4096 or not pin or int(pin.group(1)) != pn:
            fail("5", "8px plate: %d cells / %d B (want 2+10n <= 4096), eq_core.c pins %s" % (pn, len(b), pin and pin.group(1)))
        m = re.search(r"ko_8p_block_250\.bin', 'rb'\)\.read\(\)\nassert hashlib\.sha256\(b\)\.hexdigest\(\) == '([0-9a-f]{64})'", BUILD_TXT)
        if m and hashlib.sha256(b).hexdigest() != m.group(1):
            fail("5", "ko_8p_block_250.bin sha256 != the KO_8P.BIN pin in build_ko_trial.sh")


# ===================================================================== check 6: paths / env
@guarded("6", "paths")
def c6_paths():
    W = KO_WK + "/"
    items = [
        # (path, kind, used by)   kind: in = must exist before the build, gen = an earlier build step makes it,
        #                         ext = extraction / retail disc, regen = only for regenerating glyphs
        (P4 + PROFILE_MAP, "in", "profile map (every KO encoder)"),
        (P4 + PROFILE_BLK, "in", "profile glyph block (KO_BLK, ARC_KO.BIN)"),
        (P4 + "ko_glyph_map_v4.tsv", "in", "gen_ko_equip desc map"),
        (P4 + "ko_glyph_map_v12f_new.tsv", "in", "ko_text_enc / gen_ko_equip symbols"),
        (P4 + "ko_glyph_lat_v14d.tsv", "in", "ko_text_enc Latin cells"),
        (P4 + "ko_engtab_253.bin", "in", "ko_text_enc / gen_ko_save / gen_ko_equip engtab"),
        (ENG_BUILD_PATH, "in", "build KO_ENGTAB (ARC_KO.BIN)"),
        (BX.get("KO_FONT8_ADV", P4 + "ko_font8_adv.bin"), "gen", "build KO_FONT8_ADV (gen_ko_font8.py --patch)"),
        (BX.get("KO_PROMO", P4 + "ko_promo_tbl.bin"), "gen", "build KO_PROMO (gen_ko_promo)"),
        (BX.get("KO_SHEET2", P4 + "ko_sheet2.bin"), "gen", "build KO_SHEET2 (gen_ko_sheet2)"),
        (P4 + "ko_promo_map.tsv", "in", "gen_ko_splice import (gen_ko_promo rewrites)"),
        (P4 + "ko_8p_block_250.bin", "in", "KO_8P.BIN / gen_ko_cfg / check_ko_glyph"),
        (P4 + "ko_8p_block_243.bin", "in", "gen_ko_font8"),
        (P4 + "ko_8p_map_243.tsv", "in", "gen_ko_font8"),
        (P4 + "ko_handcoded_chars.txt", "in", "check_ko_glyph hand-coded sources"),
        (DEF_V14, "in", "gen_ko_dlg / gen_ko_promo / gen_ko_sheet2 / gen_ko_wrap"),
        (W + "번역/번역_v4_장비.tsv", "in", "gen_ko_equip / check_ko_glyph"),
        (W + "번역/번역_설정화면.tsv", "in", "gen_ko_cfg"),
        (W + "번역/번역_저장문구.tsv", "in", "gen_ko_save"),
        (W + "번역/번역_필드메시지.tsv", "in", "gen_ko_fieldmsg"),
        (KO_FONTS + "/Galmuri/v2.40.4/Galmuri-v2.40.4/Galmuri7.ttf", "in", "gen_ko_font8.py --patch (regenerates the sheet every build)"),
        (KO_FONTS + "/MonadABXY/mgalmuri-font/_ttf/MGalmuri12.ttf", "regen", "gen_ko_glyph_lat.py d"),
        (os.path.join(REPO, "disks/us/BIN/F_GAME.BIN"), "in", "gen_ko_font8 --patch / retail disc"),
        (os.path.join(REPO, "config/assets.us.yaml"), "in", "gen_ko_splice sizes"),
        (os.path.join(REPO, "src/dra/dra.h"), "in", "KO defines"),
        (os.path.join(REPO, "src/st/ko_font_params.h"), "in", "check_ko_glyph"),
        (os.path.join(REPO, "mods/sotn-ultimate/ko_assets/voice_v14/table.tsv"), "in", "voice_v14.py"),
        (os.path.join(REPO, "src/dra/gen/us/enemydefs.h"), "ext", "gen_ko_names"),
        (os.path.join(REPO, "src/dra/gen/us/equipment.h"), "ext", "gen_ko_equip"),
        (os.path.join(REPO, "mods/sotn-ultimate/src/maria_saturn/mod_maria_boss_data.h"), "gen", "gen_ko_mar (gen_cen_maria)"),
        (os.path.join(REPO, "build/us/psp_dub/dub.json"), "gen", "gen_ko_timing (psp_dub)"),
    ]
    if S is not None:
        for asset, _o, _s, _p in S.STAGES:
            items.append((os.path.join(REPO, "assets", asset), "ext", "gen_ko_splice"))
    if GW is not None:
        items.append((GW.OVR, "opt", "gen_ko_wrap overrides"))
    for p, kind, who in items:
        seen("6")
        if os.path.exists(p):
            continue
        if kind == "in":
            fail("6", "missing input %s (%s)" % (rp(p), who))
        elif kind == "regen":
            info("6", "missing %s (%s; only for regenerating glyphs)" % (rp(p), who))
        else:
            info("6", "not yet present %s (%s; made by %s)" % (rp(p), who, {"gen": "an earlier build step", "ext": "make build_assets / extraction", "opt": "optional"}[kind]))
    # title46 restore files (last build step)
    man = os.path.join(REPO, "mods/sotn-ultimate/ko_assets/title46/manifest.json")
    if os.environ.get("TITLE_RESTORE", "1") != "0" and os.path.exists(man):
        import json
        for f in json.load(io.open(man, encoding="utf-8")).get("files", []):
            seen("6")
            p = os.path.join(os.path.dirname(man), f["file"])
            if not os.path.exists(p):
                fail("6", "title46 restore file %s missing (last build step aborts; TITLE_RESTORE=0 skips)" % rp(p))
            elif os.path.getsize(p) != f.get("size", os.path.getsize(p)):
                fail("6", "title46 %s size %d != manifest %d" % (f["file"], os.path.getsize(p), f["size"]))
            elif f.get("sha256") and hashlib.sha256(open(p, "rb").read()).hexdigest() != f["sha256"]:
                fail("6", "title46 %s sha256 != manifest" % f["file"])
    # env-gated build inputs
    psp = os.environ.get("MOD_PSP_DUB", "1") == "1"
    if psp and os.environ.get("PSP_DUB_JP", "0") == "1" and os.environ.get("VOICE_V14", "1") != "0":
        v = os.environ.get("V14_BIN", "build/v14/v14.bin")
        if not os.path.exists(os.path.join(REPO, v)) and not os.path.exists(v):
            fail("6", "V14_BIN %s missing (PSP_DUB_JP=1: the voice step aborts; VOICE_V14=0 skips)" % v)
    if os.environ.get("JP_MOVIE", "1") != "0":
        v = os.environ.get("JP_T1", "build/jp_t1/jp_t1.bin")
        if not os.path.exists(os.path.join(REPO, v)) and not os.path.exists(v):
            fail("6", "JP_T1 %s missing (the last step aborts; JP_MOVIE=0 skips)" % v)
    fl = os.environ.get("REPACK_XA_FLAGS", "").split()
    for i, a in enumerate(fl):
        if a in ("--swap-jp", "--swap-span") and i + 1 < len(fl):
            seen("6")
            if not os.path.exists(fl[i + 1]) and not os.path.exists(os.path.join(REPO, fl[i + 1])):
                fail("6", "REPACK_XA_FLAGS %s %s missing (repack_xa.py)" % (a, fl[i + 1]))
    # tools
    node = os.environ.get("NODE", "node")
    tools = ["mipsel-linux-gnu-cpp", "mipsel-linux-gnu-as", "mipsel-linux-gnu-ld", "mipsel-linux-gnu-objdump",
             "mipsel-linux-gnu-objcopy", "mipsel-linux-gnu-nm", "go", node, "iconv", "make"]
    for t in tools:
        seen("6")
        if not (shutil.which(t) or os.path.exists(t)):
            fail("6", "tool %s not on PATH" % t)
    if not os.path.exists(os.path.join(REPO, "build/us/mod/mar2/sfx_jp/SD_RIH.VB")) and not shutil.which("cargo"):
        fail("6", "cargo not on PATH (the JP Maria-2 sfx step builds tools/maria-sfx)")
    for f in (".venv/bin/python", "bin/cc1-psx-26", "tools/sotn_str/target/release/sotn_str", "tools/maspsx/maspsx.py"):
        seen("6")
        if not os.path.exists(os.path.join(REPO, f)):
            fail("6", "%s missing" % f)
    for mod in ("numpy", "PIL"):
        if importlib.util.find_spec(mod) is None:
            fail("6", "python3 module %s missing (gen_ko_font8.py --patch imports it)" % mod)


@guarded("6", "env")
def c6_env():
    tree = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
    kr = os.environ.get("SOTN_KR")
    if kr and os.path.realpath(kr) != os.path.realpath(tree):
        warn("6", "SOTN_KR=%s is not this tree %s: generators read/write the other tree (#250)" % (kr, tree))
    sr = os.environ.get("SOTN_REPO")
    if sr and os.path.realpath(sr) != os.path.realpath(REPO):
        warn("6", "SOTN_REPO=%s != SOTN_KR %s: gen_ko_equip prefers SOTN_REPO" % (sr, REPO))
    for name in ("KO_V14_TSV", "KO_V14"):
        v = os.environ.get(name)
        if v and os.path.realpath(v) != os.path.realpath(DEF_V14):
            warn("6", "%s=%s: only %s reads it; gen_ko_dlg / gen_ko_promo / gen_ko_sheet2 / gen_ko_wrap still read %s "
                 "(mixed translation inputs)" % (name, v, "gen_ko_names" if name == "KO_V14_TSV" else "gen_ko_mar", rp(DEF_V14)))
            if not os.path.exists(v):
                fail("6", "%s=%s does not exist" % (name, v))
    v = os.environ.get("KO_OUTD")
    if v and os.path.realpath(v.rstrip("/")) != os.path.realpath(KO_WK + "/검수"):
        warn("6", "KO_OUTD=%s: gen_ko_wrap reads 번역_v14.tsv + overrides there, the other generators read %s" % (v, rp(DEF_V14)))
    if os.environ.get("KO_TSV"):
        warn("6", "KO_TSV=%s: gen_ko_splice reads it instead of gen_ko_wrap's output (the wrap step is bypassed)" % os.environ["KO_TSV"])
        if not os.path.exists(os.environ["KO_TSV"]):
            fail("6", "KO_TSV=%s does not exist" % os.environ["KO_TSV"])
    v = os.environ.get("KO_EQUIP_TSV")
    if v and not os.path.exists(v):
        fail("6", "KO_EQUIP_TSV=%s does not exist" % v)
    for name, real in (("KO_WRAP_SRC", "KO_OUTD (folder of 번역_v14.tsv for gen_ko_wrap)"),
                       ("MB_HDR", "gen_ko_timing --hdr / gen_ko_mar HDR (fixed path under SOTN_KR)")):
        if os.environ.get(name):
            warn("6", "%s is set but no KO tool reads it (the real knob: %s)" % (name, real))
    # encoders vs the engtab the build loads
    if os.path.basename(ENG_BUILD_PATH) != "ko_engtab_253.bin":
        extra = sorted(set(ENG_BUILD) - set(ENG253))
        warn("6", "build loads %s (%d more ASCII cells: %s) but ko_text_enc / gen_ko_save / gen_ko_equip encode with "
             "ko_engtab_253.bin (and ko_text_enc's Latin list is ko_glyph_lat_v14d.tsv): text using those chars is refused "
             "or drawn by the BIOS font" % (os.path.basename(ENG_BUILD_PATH), len(extra), "".join(extra)))
    info("6", "profile %s, map %s%s, block %s, KO_WK %s, SOTN_KR %s" % (
        FP.ACTIVE, PROFILE_MAP, " (using %s)" % MAP_USED if MAP_USED != PROFILE_MAP else "", PROFILE_BLK, KO_WK, REPO))


# ===================================================================== check 7: static entry offsets
@guarded("7", "C entry offsets")
def c7_static():
    if EO is None or S is None:
        return
    rx = re.compile(r"SetCutsceneScript\(\s*([A-Za-z_][A-Za-z_0-9]*)\s*\+\s*(0x[0-9A-Fa-f]+|\d+)\s*\)")
    for base in ("src/st", "src/boss"):
        root = os.path.join(REPO, base)
        if not os.path.isdir(root):
            continue
        for dp, _dn, fns in os.walk(root):
            for fn in fns:
                if not fn.endswith((".c", ".h")):
                    continue
                p = os.path.join(dp, fn)
                for m in rx.finditer(io.open(p, encoding="utf-8", errors="replace").read()):
                    seen("7")
                    ovl = os.path.relpath(dp, root).split(os.sep)[0]
                    sym, off = m.group(1), int(m.group(2), 0)
                    assets = [a for a, o, s, _p in S.STAGES if o in ("st" + ovl, "bo" + ovl) and s == sym]
                    if not assets:
                        warn("7", "%s: SetCutsceneScript(%s + 0x%X) in a script the splicer does not rewrite" % (rp(p), sym, off))
                        continue
                    for a in assets:
                        if off not in EO.offsets_for("assets/" + a):
                            fail("7", "%s: SetCutsceneScript(%s + 0x%X) is not in ko_entry_offsets.ENTRY_OFFSETS[assets/%s] "
                                 "(the splice would move it: freeze)" % (rp(p), sym, off, a))
    for rel in EO.ENTRY_OFFSETS:
        if rel[len("assets/"):] not in [a for a, _o, _s, _p in S.STAGES]:
            warn("7", "ko_entry_offsets names %s, which gen_ko_splice.STAGES does not splice" % rel)
    try:
        with quiet():
            errs = EO.check_psp_split(REPO)
        for e in errs:
            fail("7", e)
    except Exception as ex:  # noqa: BLE001
        warn("7", "psp_script split check: %s" % exc(ex))


# ===================================================================== run
def main():
    c6_env()
    c6_paths()
    c1_dialogue()
    c1_names()
    c1_field()
    c1_equip()
    c1_cfg()
    c1_save()
    c1_sheet2()
    c1_handcoded()
    c3_lib()
    c2_dialogue()
    c2_unused()
    c2_relic()
    c2_field()
    c2_symbols()
    c4_names()
    c4_field()
    c4_equip()
    c4_menu()
    c4_tsv_limits()
    wrapped = wrap_dry()
    if promo_dry():
        splice_dry(wrapped)
    else:
        skip("7", "splice dry run: promotion table not regenerated")
    c2_boss()
    mar_dry()
    c7_static()
    flush_13()
    c5_cells()
    c5_tables()
    shutil.rmtree(TMP, ignore_errors=True)

    sev = dict((c, {"FAIL": 0, "WARN": 0, "INFO": 0}) for c, _ in CHECKS)
    for c, s, _m in FIND:
        sev[c][s] += 1
    print("KO preflight  tree %s  KO_WK %s  profile %s" % (REPO, KO_WK, FP.ACTIVE))
    print("%-26s %8s %6s %6s %6s %6s" % ("check", "items", "FAIL", "WARN", "INFO", "SKIP"))
    for c, name in CHECKS:
        print("%-26s %8d %6d %6d %6d %6d" % ("%s %s" % (c, name), COUNT[c], sev[c]["FAIL"], sev[c]["WARN"],
                                             sev[c]["INFO"], len([x for x in SKIPS[c] if x])))
    order = {"FAIL": 0, "WARN": 1, "INFO": 2}
    shown = [f for f in FIND if f[1] != "INFO" or VERBOSE]
    for c, s, m in sorted(shown, key=lambda f: (f[0], order[f[1]])):
        print("%s %s  %s" % (s, c, m))
    for c, _n in CHECKS:
        for x in SKIPS[c]:
            if x:
                print("SKIP %s  %s" % (c, x))
    nf = sum(v["FAIL"] for v in sev.values())
    nw = sum(v["WARN"] for v in sev.values())
    bad = nf + (nw if STRICT else 0)
    print("ko_preflight: %d FAIL, %d WARN%s -> %s" % (nf, nw, " (--strict)" if STRICT else "",
                                                     "problems (exit 2)" if bad else "OK"))
    return 2 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
