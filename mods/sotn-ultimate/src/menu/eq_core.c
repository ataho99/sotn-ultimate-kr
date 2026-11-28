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
void KO_8pDrawIdx(const char* idx, s32 spIdx, s32 x, s32 y, MenuContext* ctx);

// 8px block (KO_8P.BIN layout: u16 count, count x u16 codes (LE; F0xx
// hangul or ascii<0x80), count x 8B cells (1bpp MSB, pixel0 = 0x80,
// 8 rows)). Boot-loaded to KO_8P_BASE via the ARC_KO chain (cd.c),
// so this former menu-open CD loader is a stub. Kept as a symbol: the
// tail link references it.
void KO_8pLoad(void) {
}
// Block present check (pinned count 394 (390 + 템 상 전 밋 of the Richter/Maria pause menu, gen_ko_8p_ext249.py) = 371 (283 + the equipment-name chars of the JP-name pass; gen_ko_8p.py) + the 14 cells of the Extra settings popup + 5 cells (U R O F + one Hangul: the Extra settings row, SATURN, ON/OFF); gen_ko_8p_ext.py): boot chain fills this before any
// menu opens. The draw gate uploads only when this passes.
s32 KO_8pValid(void) {
    u16 n = ((u16*)KO_8P_BASE)[0];
    if (n == 399) {
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
        // LoadTPage only queues the upload and tmp is this frame's stack: with the GPU busy (the
        // Richter / Maria Commands list before it) every cell sampled the last one (095_13: the status
        // name showed one glyph three times). As eq_pop.c
        DrawSync(0);
    }
}

// Unified draw (isSel=1: groups 0,1,2; isSel=0: groups 3,4).
// Same union/sig gating as ko_equip12b; row 2 never touched here.
// 3877: the cell indexes of every name are computed only when the list signature (names pointers + count + epoch, the same gate
// the VRAM upload uses) changed, and kept as idx rows in the cache (ko_equip.h); every frame just draws the cached rows. Before, the
// whole table search (KO_8pFind, 399 codes per character, twice) ran every frame and the equipment screen no longer fit in one frame.
// The callers pass at most 8 names (the 7 / 8 button rows, 8 overview names, 7 familiar names) except the Korean Extra settings / Cheats popup (PopDrawKo:
// label + value per row, up to 20 names); the cache has 31 rows (KO_EQ_CACHE_N), the space cell's byte sits after them (KO_EQ_SPIDX). (3958: it was at row 12's place, +0xC0,
// so the 13th name's first cell was overwritten by the space cell's idx and drawn blank.)
void KO_EqDraw(const char** names, const s32* xs, const s32* ys, s32 count,
               MenuContext* ctx, s32 isSel) {
    s32 cells[KO_EQ_SEL_CELLS]; // 64 covers both unions (41 for overview)
    s32 pi[12];
    u8* cidx = (u8*)(isSel ? KO_EQ_CACHE_SEL : KO_EQ_CACHE_OV);
    u8* irow;
    volatile u32* sigp = isSel ? &KO_EQ_SELSIG : &KO_EQ_OVSIG;
    s32 spaceCix;
    s32 spCell;
    s32 cap = isSel ? KO_EQ_SEL_CELLS : KO_EQ_OV_CELLS;
    s32 nCells = 0;
    s32 i;
    s32 j;
    s32 k;
    s32 f;
    s32 pn;
    s32 cix;
    s32 grp;
    s32 col;
    u32 sig;
    if (count <= 0) {
        return;
    }
    sig = KO_EqSig(names, count);
    if (sig != *sigp) {
        spaceCix = KO_8pFind(0x20);
        spCell = -1;
        for (i = 0; i < count; i++) {
            irow = cidx + (i << 4);
            j = 0;
            pn = KO_EqParseName(names[i], pi, 10);
            for (k = 0; k < pn; k++) {
                cix = pi[k];
                if (cix < 0) {
                    cix = spaceCix; // unseen -> blank (space cell)
                }
                if (cix < 0) {
                    continue; // block missing: char dropped from union
                }
                for (f = 0; f < nCells; f++) {
                    if (cells[f] == cix) {
                        break;
                    }
                }
                if (f == nCells) {
                    if (nCells >= cap) {
                        continue; // overflow: char dropped from union (keeps alignment of the rest)
                    }
                    cells[nCells++] = cix;
                }
                if (cix == spaceCix) {
                    spCell = f;
                }
                irow[j++] = (u8)f; // the union cell number for now; turned into the idx byte below
            }
            irow[j] = 0xFF; // j <= 10 (pn is capped at 10): j cells + the end mark fit the 16-byte row
        }
        if (KO_8pValid()) {
            KO_8pUpload(cells, nCells, isSel);
            *sigp = sig;
        }
        // the union is final: cell number -> idx byte ((group << 5) | column)
        for (i = 0; i < count; i++) {
            irow = cidx + (i << 4);
            for (j = 0; irow[j] != 0xFF; j++) {
                KO_8pRC(irow[j], &grp, &col, isSel);
                irow[j] = (u8)((grp << 5) | col);
            }
        }
        // the space cell's idx byte (the draw advances 4 px for it, 8 for the rest); 0xFF = no space cell
        cidx[KO_EQ_SPIDX] = 0xFF;
        if (spCell >= 0) {
            KO_8pRC(spCell, &grp, &col, isSel);
            cidx[KO_EQ_SPIDX] = (u8)((grp << 5) | col);
        }
    }
    for (i = 0; i < count; i++) {
        KO_8pDrawIdx((const char*)(cidx + (i << 4)), cidx[KO_EQ_SPIDX], xs[i], ys[i], ctx);
    }
}

// Draw one 8px index string (0xFF ends). 8px advance per cell, 4px for
// space cells (half-width: spaces were a full 8px like 한글 한 칸).
// tpage is pinned to 6. The old (ch & 0x10) ? 7 : 6 copied the 12px
// 16-wide half-row split, but 8px rows are 24 cells (192px < one 256px page)
// uploaded contiguously at 0x180: col>=16 drew from tpage 7 (X 0x1C0) and
// came out blank. u = col*8 stays linear within the same page.
void KO_8pDrawIdx(
    const char* idx, s32 spIdx, s32 x, s32 y, MenuContext* ctx) {
    s32 g;
    s32 col;
    s32 u;
    s32 v;
    s32 ch;
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
        x += (ch == spIdx) ? 4 : 8;
    }
}

#endif
