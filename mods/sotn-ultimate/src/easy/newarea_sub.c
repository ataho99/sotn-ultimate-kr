// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate (Korean build): the familiars' voice subtitles in 8px, right under the familiar's body and following it (new resident area,
// build_newarea.sh; the replacement of the 12px bottom-of-screen subtitle of tt_002 / tt_003, 094_17). Hooks: the servant overlays' ProcessSfxState
// (src/servant/play_sfx.h) call SubCmd (header entry +20) when the XA voice starts / ends; R1Tick (every frame of play) calls SubFollow.
//  * Text: the 8px plate (KO_8P, resident at 0x801F2000: u16 count, count x u16 glyph code, count x 8 B 1bpp cell) + the aux cells of sub_str.h
//    (gen_ko_sub8.py) for the characters the plate lacks. Every glyph is expanded to a 4 bit 8x8 tile (ink = index 8 of the dialogue window's clut
//    0x1A1, like the 12px subtitle), uploaded with LoadImage into VRAM tpage 0x10 rows y 480 / 488 (x half-words 18 + 2 * column, clear of the
//    dialogue buffers y 384..447, the name cells y 448..463, the pickup popup y 464 and Maria's x 0..17) and drawn as an 8x8 SPRT.
//  * Primitive marker (type SPRT + priority + tpage 0x10): after a room / stage load the buffer is rebuilt; an index is freed only while the head still
//    carries it. Failure = no subtitle, never a wait.
//  * SUB_SHADOW: a second set of sprites, black (DRAW_COLORS, colour 0) one pixel right and down, behind the text.
//  * The state is .data (non-zero start values): the area has no .bss.
#include <game.h>
#include "sub_str.h"

#ifndef SUB_SHADOW
#define SUB_SHADOW 1 // 1 = black shadow behind the white text, 0 = plain
#endif
#ifndef SUB_DY
#define SUB_DY 18 // from the familiar's position (posY) down to the top of the first row: under the body (measured on the faerie / the demon)
#endif
#define SUB_ENTITY 4 // SERVANT_ENTITY_INDEX
#define SUB_PRIO_TEXT 0x1F0
#define SUB_PRIO_SHADOW 0x1EF
#define SUB_COLMAX 23 // cells per row in VRAM (46 half-words from x 18 to 63)
#define SUB_PLATE 0x801F2000

static s32 g_SubPrim = -1; // the first sprite (shadows first when SUB_SHADOW, then the text); -1: none
static s32 g_SubCount = 1; // glyph sprites per set (non-zero start: a zero static would be .bss, which this area does not have)
static s32 g_SubLine = -1; // the shown line's index in g_KoSub8Lines
static s32 g_SubDbg = 1;   // SUB_DEBUG: a test writes a voice id here (> 1) and the next SubFollow shows it

static s32 SubAdv(u8 c) {
    if ((c & 0xFC) == 0xF0) {
        return 8; // Hangul / ellipsis
    }
    return c == '?' ? 6 : 4; // space, , . ! ?
}

static void SubFree(void) {
    s32 i = g_SubPrim;

    g_SubPrim = -1;
    g_SubLine = -1;
    if (i < 0 || i >= MAX_PRIM_COUNT) {
        return;
    }
    if (g_PrimBuf[i].type != PRIM_SPRT || g_PrimBuf[i].tpage != 0x10 ||
        (g_PrimBuf[i].priority != SUB_PRIO_TEXT && g_PrimBuf[i].priority != SUB_PRIO_SHADOW)) {
        return;
    }
    FreePrimitives(i);
}

// the 8 B cell of a glyph code: the aux cells first, then the plate; NULL when neither has it
static const u8* SubCell(u16 code) {
    const u8* plate = (const u8*)SUB_PLATE;
    s32 i, n;

    for (i = 0; i < KO_SUB8_NAUX; i++) {
        if (g_KoSub8AuxCode[i] == code) {
            return &g_KoSub8AuxCell[i * 8];
        }
    }
    n = *(const u16*)plate;
    for (i = 0; i < n; i++) {
        if (((const u16*)(plate + 2))[i] == code) {
            return plate + 2 + n * 2 + i * 8;
        }
    }
    return NULL;
}

// walks the line's rows and calls back per glyph: mode 0 = count + upload, mode 1 = place. (one routine so SHOW and FOLLOW lay out identically)
static void SubLayout(s32 mode) {
    const KoSub8Line* ln = &g_KoSub8Lines[g_SubLine];
    const u8* row[2];
    const u8* p;
    Primitive* t;
    Primitive* sh;
    Entity* e;
    const u8* cell;
    RECT rc;
    u32 tile[8]; // one glyph's 4 bit tile (32 B), uploaded with a DrawSync before the next
    s32 r, w, x, y, rows, i, k, n, col, px, py;
    u16 code;

    row[0] = (const u8*)ln->r0;
    row[1] = (const u8*)ln->r1;
    rows = *row[1] ? 2 : 1;
    e = &g_Entities[SUB_ENTITY];
    px = e->posX.i.hi;
    py = e->posY.i.hi + SUB_DY;
    if (py > 232 - rows * 8) { // not under the bottom edge
        py = 232 - rows * 8;
    }
    if (py < 8) {
        py = 8;
    }
    t = &g_PrimBuf[g_SubPrim];
    sh = t;
    if (SUB_SHADOW) {
        for (i = 0; i < g_SubCount; i++) { // the text sprites follow the shadow ones in the list
            t = t->next;
        }
    }
    k = 0;
    for (r = 0; r < rows; r++) {
        w = 0;
        for (p = row[r]; *p; p += ((*p & 0xFC) == 0xF0) ? 2 : 1) {
            w += SubAdv(*p);
        }
        x = px - w / 2;
        if (x > 256 - w) { // not past the right edge
            x = 256 - w;
        }
        if (x < 0) {
            x = 0;
        }
        y = py + r * 8;
        col = 0;
        for (p = row[r]; *p; p += ((*p & 0xFC) == 0xF0) ? 2 : 1) {
            if (*p == 0x20) {
                x += SubAdv(*p);
                continue;
            }
            if (mode == 0) {
                code = ((*p & 0xFC) == 0xF0) ? (u16)((p[0] << 8) | p[1]) : *p;
                cell = SubCell(code);
                if (cell != NULL) {
                    for (i = 0; i < 8; i++) { // 1bpp (MSB first) -> 4 bit, ink = 8, pixel 0 in the low nibble
                        tile[i] = 0;
                        for (n = 0; n < 8; n++) {
                            if (cell[i] & (0x80 >> n)) {
                                tile[i] |= 8 << (n * 4);
                            }
                        }
                    }
                    rc.x = 18 + col * 2;
                    rc.y = 480 + r * 8;
                    rc.w = 2;
                    rc.h = 8;
                    LoadImage(&rc, (u_long*)tile);
                    DrawSync(0); // the tile buffer is reused
                }
                t->u0 = 72 + col * 8;
                t->v0 = 224 + r * 8;
                t->u1 = 8;
                t->v1 = 8;
                t->tpage = 0x10;
                t->clut = PAL_UNK_1A1; // the dialogue window's clut: ink = index 8 = white
                t->r0 = t->g0 = t->b0 = 0x80;
                t->priority = SUB_PRIO_TEXT;
                t->drawMode = cell != NULL ? DRAW_ABSPOS : DRAW_HIDE;
                if (SUB_SHADOW) {
                    sh->u0 = t->u0;
                    sh->v0 = t->v0;
                    sh->u1 = 8;
                    sh->v1 = 8;
                    sh->tpage = 0x10;
                    sh->clut = PAL_UNK_1A1;
                    sh->r0 = sh->g0 = sh->b0 = 0; // black: the colour is applied (DRAW_COLORS)
                    sh->priority = SUB_PRIO_SHADOW;
                    sh->drawMode = cell != NULL ? (DRAW_ABSPOS | DRAW_COLORS) : DRAW_HIDE;
                }
            }
            t->x0 = x;
            t->y0 = y;
            t = t->next;
            if (SUB_SHADOW) {
                sh->x0 = x + 1;
                sh->y0 = y + 1;
                sh = sh->next;
            }
            x += SubAdv(*p);
            col++;
            k++;
        }
    }
}

static void SubShow(s32 id) {
    const KoSub8Line* ln;
    const u8* p;
    s32 i, r, n, cnt;

    SubFree();
    for (i = 0; i < (s32)(sizeof(g_KoSub8Lines) / sizeof(g_KoSub8Lines[0])); i++) {
        if (g_KoSub8Lines[i].id == id) {
            break;
        }
    }
    if (i >= (s32)(sizeof(g_KoSub8Lines) / sizeof(g_KoSub8Lines[0]))) {
        return;
    }
    ln = &g_KoSub8Lines[i];
    cnt = 0;
    for (r = 0; r < 2; r++) {
        n = 0;
        for (p = (const u8*)(r ? ln->r1 : ln->r0); *p; p += ((*p & 0xFC) == 0xF0) ? 2 : 1) {
            if (*p != 0x20) {
                n++;
            }
        }
        if (n > SUB_COLMAX) {
            return;
        }
        cnt += n;
    }
    if (cnt == 0) {
        return;
    }
    g_SubPrim = AllocPrimitives(PRIM_SPRT, SUB_SHADOW ? cnt * 2 : cnt);
    if (g_SubPrim < 0) {
        g_SubPrim = -1;
        return;
    }
    g_SubCount = cnt;
    g_SubLine = i;
    SubLayout(0);
}

// header entry +20: cmd 1 = show the voice id, 2 = hide
void SubCmd(s32 cmd, s32 id) {
    if (cmd == 1) {
        SubShow(id);
    } else {
        SubFree();
    }
}

// every frame of play (R1Tick): follow the familiar
void SubFollow(void) {
#ifdef SUB_DEBUG
    if (g_SubDbg > 1) {
        SubShow(g_SubDbg);
        g_SubDbg = 1;
    }
#endif
    if (g_SubPrim < 0) {
        return;
    }
    if (g_PrimBuf[g_SubPrim].type != PRIM_SPRT || g_PrimBuf[g_SubPrim].tpage != 0x10) { // the buffer was rebuilt
        g_SubPrim = -1;
        g_SubLine = -1;
        return;
    }
    SubLayout(1);
}
