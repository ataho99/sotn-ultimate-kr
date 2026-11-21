// SPDX-License-Identifier: AGPL-3.0-or-later
// ko_equipreloc: resident tail core. Unified selector/overview draw
// (8px Mgalmuri 8x8 cells, same 8px advance/pitch as English).
// Linked at KO_EQUIP_CORE; entries (sel/ov/enter) jump here relatively.
#include "ko_equip.h"
#include "game.h"

#if defined(MOD_KO)

void KO_8pLoad(void);
s32 KO_8pValid(void);
s32 KO_8pFind(u16 code);
s32 KO_8pRow(s32 g);
void KO_8pRC(s32 cell, s32* grp, s32* col, s32 isSel);
u32 KO_EqSig(const char** names, s32 count);
s32 KO_EqParseName(const char* str, s32* idx, s32 cap);
void KO_8pExpand(s32 cell, u8* dst);
void KO_8pUpload(s32* cells, s32 nCells, s32 isSel);
void KO_8pDrawIdx(const char* idx, const s32* spc, s32 x, s32 y, MenuContext* ctx);

// 8px block (KO_8P.BIN layout: u16 count, count x u16 codes (LE; F0xx
// hangul or ascii<0x80), count x 8B cells (1bpp MSB, pixel0 = 0x80,
// 8 rows)). Boot-loaded to KO_8P_BASE via the ARC_KO chain (cd.c),
// so this former menu-open CD loader is a stub. Kept as a symbol: the
// tail link references it.
void KO_8pLoad(void) {
}
// Block present check (pinned count 390 = 371 (283 + the equipment-name chars of the JP-name pass; gen_ko_8p.py) + the 14 cells of the Extra settings popup + 5 cells (U R O F + one Hangul: the Extra settings row, SATURN, ON/OFF); gen_ko_8p_ext.py): boot chain fills this before any
// menu opens. The draw gate uploads only when this passes.
s32 KO_8pValid(void) {
    u16 n = ((u16*)KO_8P_BASE)[0];
    if (n == 390) {
        return 1;
    }
    return 0;
}
s32 KO_8pFind(u16 code) {
    u16* hdr = (u16*)KO_8P_BASE;
    u16* tab;
    s32 n;
    s32 i;
    n = hdr[0];
    if (n <= 0 || n > KO_8P_MAX) {
        return -1;
    }
    tab = hdr + 1;
    for (i = 0; i < n; i++) {
        if (tab[i] == code) {
            return i;
        }
    }
    return -1;
}

// 24 cells per 16px sheet row (24x8 = 192px, same span as 16x12).
// sel: g0=row1, g1=row3, g2=row4 (64 = 24+24+16).
// ov:  g3=row8, g4=row7 (41 = 24+17). Row 2 stays description-only.
s32 KO_8pRow(s32 g) {
    if (g == 0) {
        return 1;
    }
    if (g == 1) {
        return 3;
    }
    if (g == 2) {
        return 4;
    }
    if (g == 3) {
        return 8;
    }
    return 7;
}

void KO_8pRC(s32 cell, s32* grp, s32* col, s32 isSel) {
    if (isSel) {
        if (cell < 24) {
            *grp = 0;
            *col = cell;
            return;
        }
        if (cell < 48) {
            *grp = 1;
            *col = cell - 24;
            return;
        }
        *grp = 2;
        *col = cell - 48;
        return;
    }
    if (cell < 24) {
        *grp = 3;
        *col = cell;
        return;
    }
    *grp = 4;
    *col = cell - 24;
}

u32 KO_EqSig(const char** names, s32 count) {
    s32 i;
    u32 h;
    h = 2166136261u ^ (KO_EQ_EPOCH * 16777619u);
    h ^= (u32)count;
    h *= 16777619u;
    for (i = 0; i < count; i++) {
        h ^= (u32)names[i];
        h *= 16777619u;
    }
    return h;
}

// Parse one _S-style name (F0xx pair or single byte; 0xFF 0x00 ends).
// Out: idx[i] (8px cell index). Unseen -> -1 (blank fallback below).
s32 KO_EqParseName(const char* str, s32* idx, s32 cap) {
    s32 n = 0;
    s32 unseen = 0;
    u8 b;
    while (n < cap) {
        s32 f;
        b = *str++;
        if (b == 0xFF) {
            if (*str == 0) {
                break;
            }
            str++;
            continue;
        }
        if (b >= 0x80) {
            u8 lo = *str++;
            u16 code = ((u16)b << 8) | lo;
            if (code < 0xF040) { /* no upper bound: KO_8pFind returns -1 for a code the plate lacks (0.9.4: the Hangul cell F3F7 was blanked by the old > 0xF375) */
                idx[n] = -1;
                n++;
                continue;
            }
            f = KO_8pFind(code);
            idx[n] = f;
            n++;
        } else {
            u8 ch = (b == 0) ? 0x20 : (u8)(b + 0x20);
            f = KO_8pFind((u16)ch);
            idx[n] = f;
            n++;
        }
        if (f < 0) {
            unseen++;
        }
    }
    // Names that are mostly characters the plate lacks (the English
    // "UNARMED" / "NONE" of the empty left hand and the empty-armor row
    // reach the screen as spaces) are drawn as the empty-slot mark "----",
    // the same as every other empty slot.
    if (unseen >= 2 && cap >= 4) {
        s32 d = KO_8pFind(0x2D);
        n = 4;
        idx[0] = d;
        idx[1] = d;
        idx[2] = d;
        idx[3] = d;
    }
    return n;
}

// Expand one 8B 1bpp cell to 32B 4bpp (leftmost pixel = low nibble,
// same convention as the 12x16 sheet: 0 = transparent, 3 = white core
// (the sheet CLUT is a 0..3 ramp where 1 = dark edge gray; writing 1
// made every 8px name gray. The blob expander emits up to 3 for solid ink).
void KO_8pExpand(s32 cell, u8* dst) {
    u8* blk = (u8*)KO_8P_BASE;
    u16 n;
    u8* src;
    s32 y;
    s32 x;
    n = ((u16*)KO_8P_BASE)[0];
    src = blk + 2 + n * 2 + cell * 8;
    for (y = 0; y < 8; y++) {
        u8 b = src[y];
        for (x = 0; x < 8; x += 2) {
            s32 p0 = (b & (0x80 >> x)) ? 3 : 0;
            s32 p1 = (b & (0x80 >> (x + 1))) ? 3 : 0;
            *dst++ = (u8)((p1 << 4) | p0);
        }
    }
}

// Upload-once: cells changed (sig) -> VRAM top-half of 16px rows.
void KO_8pUpload(s32* cells, s32 nCells, s32 isSel) {
    u8 tmp[32];
    s32 i;
    s32 grp;
    s32 col;
    for (i = 0; i < nCells; i++) {
        KO_8pRC(i, &grp, &col, isSel);
        KO_8pExpand(cells[i], tmp);
        LoadTPage((u_long*)tmp, 0, 0, 0x180 + col * 2, KO_8pRow(grp) * 16,
                  8, 8);
    }
}

// Unified draw (isSel=1: groups 0,1,2; isSel=0: groups 3,4).
// Same union/sig gating as ko_equip12b; row 2 never touched here.
void KO_EqDraw(const char** names, const s32* xs, const s32* ys, s32 count,
               MenuContext* ctx, s32 isSel) {
    s32 cells[KO_EQ_SEL_CELLS]; // 64 covers both unions (41 for overview)
    s32 pi[12];
    char idx[12];
    s32 spc[12];
    s32 spaceCix;
    s32 cap = isSel ? KO_EQ_SEL_CELLS : KO_EQ_OV_CELLS;
    s32 nCells = 0;
    s32 i;
    s32 k;
    s32 pn;
    s32 grp;
    s32 col;
    u32 sig;
    if (count <= 0) {
        return;
    }
    spaceCix = KO_8pFind(0x20);
    for (i = 0; i < count; i++) {
        pn = KO_EqParseName(names[i], pi, 10);
        for (k = 0; k < pn; k++) {
            s32 f;
            s32 cell = -1;
            s32 cix = pi[k];
            if (cix < 0) {
                cix = KO_8pFind(0x20); // unseen -> blank (space cell)
            }
            if (cix < 0) {
                continue; // block missing: char dropped from union
            }
            for (f = 0; f < nCells; f++) {
                if (cells[f] == cix) {
                    cell = f;
                    break;
                }
            }
            if (cell < 0) {
                if (nCells >= cap) {
                    continue; // overflow: char dropped from union
                }
                cell = nCells;
                cells[nCells] = cix;
                nCells++;
            }
        }
    }
    sig = KO_EqSig(names, count);
    if (isSel) {
        if (sig != KO_EQ_SELSIG) {
            if (KO_8pValid()) {
                KO_8pUpload(cells, nCells, 1);
                KO_EQ_SELSIG = sig;
            }
        }
    } else {
        if (sig != KO_EQ_OVSIG) {
            if (KO_8pValid()) {
                KO_8pUpload(cells, nCells, 0);
                KO_EQ_OVSIG = sig;
            }
        }
    }
    for (i = 0; i < count; i++) {
        s32 j = 0;
        pn = KO_EqParseName(names[i], pi, 10);
        for (k = 0; k < pn; k++) {
            s32 f;
            s32 cell = -1;
            s32 cix = pi[k];
            if (cix < 0) {
                cix = KO_8pFind(0x20);
            }
            if (cix < 0) {
                continue;
            }
            for (f = 0; f < nCells; f++) {
                if (cells[f] == cix) {
                    cell = f;
                    break;
                }
            }
            if (cell < 0) {
                continue; // union overflow: drop char, keep alignment
            }
            KO_8pRC(cell, &grp, &col, isSel);
            idx[j++] = (char)((grp << 5) | col);
            spc[j - 1] = (cix == spaceCix);
        }
        idx[j] = (char)0xFF;
        KO_8pDrawIdx(idx, spc, xs[i], ys[i], ctx);
    }
}

// Draw one 8px index string (0xFF ends). 8px advance per cell, 4px for
// space cells (half-width: spaces were a full 8px like 한글 한 칸).
// tpage is pinned to 6. The old (ch & 0x10) ? 7 : 6 copied the 12px
// 16-wide half-row split, but 8px rows are 24 cells (192px < one 256px page)
// uploaded contiguously at 0x180: col>=16 drew from tpage 7 (X 0x1C0) and
// came out blank. u = col*8 stays linear within the same page.
void KO_8pDrawIdx(
    const char* idx, const s32* spc, s32 x, s32 y, MenuContext* ctx) {
    s32 g;
    s32 col;
    s32 u;
    s32 v;
    s32 ch;
    s32 n = 0;
    while (true) {
        ch = *idx++;
        if (ch == 0xFF) {
            return;
        }
        g = (ch >> 5) & 7;
        col = ch & 0x1F;
        u = col * 8;
        v = KO_8pRow(g) * 16;
        MenuDrawImg(ctx, x, y, 8, 8, u, v, 0x1A1, 6, 1,
                    0x40);
        x += spc[n++] ? 4 : 8;
    }
}

#endif
