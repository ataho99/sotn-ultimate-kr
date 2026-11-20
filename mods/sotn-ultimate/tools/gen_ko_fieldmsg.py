#!/usr/bin/env python3
# gen_ko_fieldmsg.py -- KO stage message-box labels -> src/st/ko_field_msg.h (trial builds only).
#
# Input : ko_data/번역/번역_필드메시지.tsv (키, 분류, 영문, 일판 원문(기준), 한글, 비고).
#         Translation reference = the JP version's wording (일판), not the US text.
#         한글 "TODO" or empty = placeholder: no macro is written, the stage keeps the English label.
# Output: #define KO_FIELD_<KEY> "<w><h><text>"  for e_mist_door.h / e_sealed_door.h / no4 first_c_file.c.
#
# Label format (EntityMessageBox, src/st/e_collect.h): byte 0 = box width px, byte 1 = box height px,
# then the BlitChar stream (Hangul F0xx, ' ' 0x20, 0x01 = second line, NUL end). Width = longest line
# (12 px Hangul / letters, 8 px engtab ASCII, 6 px space) + 8, rounded up to 4; height 0x0E for one
# line, 0x1E for two (retail US values). Encoding: ko_text_enc.enc_show (current glyph map).
# Overlay size: a label that fits the retail US label is NUL-padded to the same size (no data shift);
# a longer one is written as is and reported (NO4 has little overlay margin).
import io
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ko_paths import KO_WK, KO_REPO  # noqa: E402
import ko_text_enc as E  # noqa: E402

REPO = os.environ.get("SOTN_KR") or KO_REPO
TSV = KO_WK + "/번역/번역_필드메시지.tsv"
OUT = os.path.join(REPO, "src", "st", "ko_field_msg.h")
# sizeof of the retail US label each key replaces (UTF-8 "．" is 2 bytes after the SJIS recode)
EN_SIZEOF = {
    "mist_alucard": 20,   # "\x7C\x0EMist could pass．"
    "mist_richter": 44,   # "\x94\x1EI have nothing to do\x01with this wall．．．"
    "sealed_door": 21,    # "\x7C\x0EMagically sealed．"
    "no4_bridge": 49,     # "\xB8\x1ESomething appeared near\x01to the wooden bridge．"
}
MAX_LINES = 2
MAX_W = 0xFC
PLACEHOLDER = ("", "TODO")


FIELD_BASE = 0x8009F300  # dra.h KO_FIELD_BASE (ARC_KO.BIN zero gap, build_ko_trial.sh asserts it)
SLOT = 0x80
ORDER = ["mist_alucard", "mist_richter", "sealed_door", "no4_bridge"]
BIN = KO_WK + "/P4시험/ko_field.bin"


def main():
    rows = [l.rstrip("\n").split("\t") for l in io.open(TSV, encoding="utf-8")][1:]
    out = ["// gen_ko_fieldmsg.py: KO stage message-box labels (do not edit; 번역_필드메시지.tsv).",
           "// The label bytes live in ARC_KO.BIN at KO_FIELD_<KEY>_ADDR (build_ko_trial.sh); the overlays keep the",
           "// retail English array and pick the KO address when its first byte is non-zero.",
           "// A key without an _ADDR macro is still a placeholder: the stage shows the English label.",
           "#pragma once",
           "#define KO_FIELD_PICK(a, en) ((*(volatile u8*)(a)) ? (char*)(a) : (en))"]
    blob = bytearray(SLOT * len(ORDER))
    done = todo = 0
    for r in rows:
        if len(r) < 5 or not r[0].startswith("field:"):
            continue
        key = r[0][6:]
        assert key in EN_SIZEOF and key in ORDER, ("unknown field key", key)
        ko = r[4].strip()
        if ko in PLACEHOLDER:
            out.append("/* KO_FIELD_%s: placeholder (JP reference: %s) */" % (key.upper(), r[3]))
            todo += 1
            continue
        lines = ko.split("/")
        assert len(lines) <= MAX_LINES, ("too many lines", key, ko)
        w = (E.show_px(ko, linebreak="/") + 8 + 3) & ~3
        assert w <= MAX_W, ("label too wide", key, ko, w)
        h = 0x0E + 0x10 * (len(lines) - 1)
        body = bytes([w, h]) + E.enc_show(ko, linebreak="/")
        assert 0 not in body[2:], key
        assert len(body) + 1 <= SLOT, ("label over the slot", key, len(body))
        idx = ORDER.index(key)
        blob[idx * SLOT:idx * SLOT + len(body)] = body
        out.append("#define KO_FIELD_%s_ADDR 0x%08X /* %s (%d B) */" % (key.upper(), FIELD_BASE + idx * SLOT, ko.replace("/", " / "), len(body) + 1))
        done += 1
    io.open(OUT, "w", encoding="utf-8", newline="").write("\n".join(out) + "\n")
    open(BIN, "wb").write(bytes(blob))
    print("ko_field_msg.h: %d labels, %d placeholders; ko_field.bin %d B" % (done, todo, len(blob)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
