#!/usr/bin/env python3
# gen_ko_wrap.py -- line breaks and dialogue windows from the font profile.
#
# Input : 번역_v14.tsv (never overwritten), ko_font_profile.py (glyph/space px,
#         window width, lines per window), optional overrides tsv
# Output: 번역_v14_<font>_<N>줄.tsv   same columns, 번역 column of the stage
#         dialogue rows re-broken:  "/" = line break (LINE_BREAK),
#         "|" = window break (NEXT_DIALOG + speaker re-set), see gen_ko_splice.py
#         줄끊기_손본행_<font>.tsv    rows whose breaks changed: id, v14 text,
#         auto result, final (override) and reason
# A different font => different profile => rerun => same procedure.
import io
from ko_paths import KO_WK  # noqa: E402
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ko_font_profile as FP  # noqa: E402

OUTD = os.environ.get("KO_OUTD", (KO_WK + "/검수/"))
SRC = os.environ.get("KO_WRAP_SRC") or (OUTD + "번역_v14.tsv")  # 보스 행이 든 작업 원본을 받을 수 있게
SCENES = ("st0_richter_dracula", "sel_ending", "no3_death", "cen_maria",
          "nz0_slogra_gaibon", "lib_librarian", "no4_ferryman_left",
          "no4_ferryman_right", "rcen_shaft", "dre_succubus",
          "dai_huntinggirl", "top_maria_richter")
# boss / Maria dialogue rows (keys bo2_/bo5_/bo6_/rbo2_/rbo6_<hex>:N, mar_maria:N): one TSV row = ONE game window,
# so the window count, speaker order and voice stay as the translator wrote them; only lines wider than the
# window (W px) are re-broken (carry_wrap) -- 0.9.2: boss windows were overflowing the right border
# split rows that stay SEPARATE (not folded into the base row, not blanked): gen_ko_splice puts them in the base row's second window (the retail script has an own window, with the Alucard portrait, behind the base row's last slot)
B_SEPARATE = ("rcen_shaft:5b",)
BOSS_PREFIX = ("bo2_", "bo5_", "bo6_", "rbo2_", "rbo6_", "mar_maria")
TAG = FP.ACTIVE
OVR = OUTD + "줄끊기_수정_%s.tsv" % TAG          # key <TAB> final text <TAB> reason
OUT = OUTD + "번역_v14_%s_%d줄.tsv" % (TAG, FP.P["window_lines"])
HAND = OUTD + "줄끊기_손본행_%s.tsv" % TAG
W = FP.P["window_px"]
NL = FP.P["window_lines"]


def wrap_line(s):
    """one logical line -> list of lines <= W px; spaces only, 2-way split
    balanced when possible, else greedy (a word wider than W is split by char)"""
    if FP.text_px_norm(s) <= W:
        return [s]
    cuts = [i for i, c in enumerate(s) if c == " "]
    best = None
    for i in cuts:
        a, b = s[:i], s[i + 1:]
        wa, wb = FP.text_px_norm(a), FP.text_px_norm(b)
        if wa <= W and wb <= W:
            d = abs(wa - wb)
            if best is None or d < best[0]:
                best = (d, [a, b])
    if best:
        return best[1]
    out, cur = [], ""
    for w in s.split(" "):
        t = (cur + " " + w) if cur else w
        if FP.text_px_norm(t) <= W:
            cur = t
            continue
        if cur:
            out.append(cur)
        cur = w
        while FP.text_px_norm(cur) > W:
            k = len(cur)
            while k > 1 and FP.text_px_norm(cur[:k]) > W:
                k -= 1
            out.append(cur[:k])
            cur = cur[k:]
    if cur:
        out.append(cur)
    return out


def reflow_lines(text):
    """ignore the translator's "/" and fill every line to W px (greedy, word by word)"""
    words = " ".join(x for x in text.split("/") if x).split(" ")
    out, cur = [], ""
    for w in words:
        t = (cur + " " + w) if cur else w
        if FP.text_px_norm(t) <= W:
            cur = t
            continue
        if cur:
            out.append(cur)
        cur = w
        while FP.text_px_norm(cur) > W:
            k = len(cur)
            while k > 1 and FP.text_px_norm(cur[:k]) > W:
                k -= 1
            out.append(cur[:k])
            cur = cur[k:]
    if cur:
        out.append(cur)
    return out


def carry_wrap(text):
    """boss rows: keep the translator's "/" lines and the row as it is when every line fits in W px.
    A line wider than W px: B = cut it at the last space that fits and carry the tail to the front of the
    next line when that still fits in W (line count unchanged); otherwise A = split it into two balanced
    lines (wrap_line). The sentence text is never changed, only where it breaks."""
    lines = text.split("/")
    if not any(FP.text_px_norm(x) > W for x in lines):
        return text
    out = []
    i = 0
    while i < len(lines):
        cur = lines[i]
        i += 1
        if FP.text_px_norm(cur) <= W:
            out.append(cur)
            continue
        cut = None
        for k, c in enumerate(cur):
            if c == " " and FP.text_px_norm(cur[:k]) <= W:
                cut = k
        if cut is not None and i < len(lines) and lines[i] != "":
            tail = cur[cut + 1:]
            if FP.text_px_norm(tail + " " + lines[i]) <= W:
                out.append(cur[:cut])
                lines[i] = tail + " " + lines[i]
                continue
        out.extend(wrap_line(cur))
    return "/".join(out)


def layout(text, windows=True):
    lines = []
    for ln in text.split("/"):
        if ln == "":
            continue
        lines += wrap_line(ln)
    if windows and FP.P.get("reflow") and len(lines) > NL:
        # more lines than a window holds: re-flow (greedy fill) before splitting into windows
        lines = reflow_lines(text)
    if not windows:
        return "/".join(lines)
    wins = [lines[i:i + NL] for i in range(0, len(lines), NL)]
    return "|".join("/".join(w) for w in wins)


def main():
    ovr = {}
    if os.path.exists(OVR):
        for ln in io.open(OVR, encoding="utf-8"):
            p = ln.rstrip(chr(10)).split(chr(9))
            if len(p) >= 2 and ":" in p[0]:
                ovr[p[0]] = (p[1], p[2] if len(p) > 2 else "")
    rows = [l.rstrip(chr(10)).split(chr(9)) for l in io.open(SRC, encoding="utf-8")]
    h = rows[0]
    ti = [i for i, x in enumerate(h) if x.startswith("번역")][0]
    out = []
    hand = [chr(9).join(["행", "v14 원문", "자동안", "최종(수정안 있으면 수정안)", "수정 이유", "원문 줄 수", "최종 창 수", "줄폭px(최종)"])]
    nrow = nwin = nwide = 0
    # v14 split rows (key suffix b): the game script keeps ONE slot, so the b row is
    # folded into its base row BEFORE layout and its own text is emptied
    bt = {}
    for r in rows[1:]:
        if len(r) > ti and r[0].endswith("b") and r[0][:-1].split(":")[-1].isdigit() and r[0] not in B_SEPARATE:
            bt[r[0][:-1]] = r[ti]
    nboss = 0
    for r in rows[1:]:
        if len(r) > ti and r[1] == "대사" and r[0].startswith(BOSS_PREFIX) and not r[0].endswith("b"):
            orig = r[ti]
            final = carry_wrap(orig)
            if final != orig:
                nboss += 1
                lines = final.split("/")
                hand.append(chr(9).join([r[0], orig, final, final, "boss carry_wrap (>%d px)" % W,
                                         str(len([x for x in orig.split("/") if x])), "1",
                                         ",".join(str(FP.text_px_norm(l)) for l in lines)]))
            r[ti] = final
            out.append(chr(9).join(r))
            continue
        if len(r) > ti and r[1] == "대사" and r[0].split(":")[0] in SCENES:
            orig = r[ti]
            isb = r[0].endswith("b") and r[0][:-1].split(":")[-1].isdigit()
            if r[0] in B_SEPARATE:  # one short line: written as it is (gen_ko_splice folds it behind the base row)
                out.append(chr(9).join(r))
                continue
            if isb:
                r[ti] = ""
                out.append(chr(9).join(r))
                continue
            if r[0] in bt:
                orig = orig + "/" + bt[r[0]]
            # rows whose English original has blank lines (sel_ending:0 poem) keep the
            # original scroll style: width wrap only, no window split
            nb = chr(92) + "n" + chr(92) + "n"
            if r[2].startswith("#"):   # public TSV: slot counts; an empty EN segment (count 0) = blank line / leading / trailing newline
                _cs = r[2][1:].split("/")
                scroll = len(_cs) > 1 and "0" in _cs
            else:
                scroll = nb in r[2] or r[2].startswith(chr(92) + "n") or r[2].endswith(chr(92) + "n")
            auto = layout(orig, windows=not scroll)
            final, why = (ovr[r[0]] if r[0] in ovr else (auto, ""))
            if final != orig:
                nrow += 1
                lines = [l for w in final.split("|") for l in w.split("/")]
                if any(FP.text_px_norm(l) > W for l in orig.split("/") if l):
                    nwide += 1
                nwin += final.count("|")
                hand.append(chr(9).join([r[0], orig, auto, final, why or ("자동" if final == auto else ""),
                                         str(len([x for x in orig.split("/") if x])), str(final.count("|") + 1),
                                         ",".join(str(FP.text_px_norm(l)) for l in lines)]))
            r[ti] = final
        out.append(chr(9).join(r))
    io.open(OUT, "w", encoding="utf-8", newline="").write(chr(10).join([chr(9).join(h)] + out) + chr(10))
    io.open(HAND, "w", encoding="utf-8", newline="").write(chr(10).join(hand) + chr(10))
    print("profile %s: window %d px x %d lines; rows changed %d (re-broken wide rows %d, extra windows %d, boss rows re-broken %d) -> %s" % (
        TAG, W, NL, nrow, nwide, nwin, nboss, OUT))


if __name__ == "__main__":
    main()
