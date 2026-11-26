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
//  * The short SPU shouts of the sword / Nose Devil / Sprite familiars (ids 0x885-0x8B8) have no end event: they go through the game's PlaySfx slot in
//    g_api (RAM 0x8003C7DC), which SubFollow points at SpuSfxHook (the original address is read from the slot and called through), and a line with a
//    frame count (g_KoSub8Lines.frames) is shown for that long.
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
#define SPU_SLOT ((volatile u32*)0x8003C7DC) /* g_api.PlaySfx */
extern s32 VSync(s32 mode);

static s32 g_SubPrim = -1; // the first sprite (shadows first when SUB_SHADOW, then the text); -1: none
static s32 g_SubCount = 1; // glyph sprites per set (non-zero start: a zero static would be .bss, which this area does not have)
static s32 g_SubLine = -1; // the shown line's index in g_KoSub8Lines
static const char* g_SubR0 = ko_song_empty; // the rows on screen (non-null start: no .bss)
static const char* g_SubR1 = ko_song_empty;
static s32 g_SubTimer = -1; // frames left of a timed (SPU) line; -1: none
static u32 g_SpuOrig = 1;   // the PlaySfx the slot held before our hook (non-zero start)
static s32 g_SubDbg = 1;   // SUB_DEBUG
static s32 g_SpuDbg = 1;   // SUB_DEBUG: a test writes an SPU id here: it is played through the game's PlaySfx slot like a familiar overlay does   // SUB_DEBUG: a test writes a voice id here (> 1) and the next SubFollow shows it

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
    g_SubTimer = -1;
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

    row[0] = (const u8*)g_SubR0;
    row[1] = (const u8*)g_SubR1;
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

// shows two rows (r1 may be empty) under the familiar; frames > 0: for that long (a timed SPU shout), else until SubFree; tag = the line's number
static void SubShowRows(const char* r0, const char* r1, s32 frames, s32 tag) {
    const u8* p;
    s32 r, n, cnt;

    SubFree(); // (the line on screen is replaced)
    cnt = 0;
    for (r = 0; r < 2; r++) {
        n = 0;
        for (p = (const u8*)(r ? r1 : r0); *p; p += ((*p & 0xFC) == 0xF0) ? 2 : 1) {
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
    g_SubLine = tag;
    g_SubR0 = r0;
    g_SubR1 = r1;
    g_SubTimer = frames > 0 ? frames : -1;
    SubLayout(0);
}

static void SubShow(s32 id) {
    s32 i;

    for (i = 0; i < (s32)(sizeof(g_KoSub8Lines) / sizeof(g_KoSub8Lines[0])); i++) {
        if (g_KoSub8Lines[i].id == id) {
            SubShowRows(g_KoSub8Lines[i].r0, g_KoSub8Lines[i].r1, g_KoSub8Lines[i].frames, i);
            return;
        }
    }
}

// ---- the song subtitle, 12px (the resident KO glyph block via the game's func_80106A28), fixed bottom centre, petal-like effect.
// One sprite set per line: SG_N shadows then SG_N texts (the same order as the 8px lines). VRAM tpage 0x10, bands y 480 (glyphs 0..14) / 496 (15..29), x half-words 18 + 3 * column
// (the same free strip as the 8px version; the two never show together unless a familiar shouts during a song: that shout then overwrites the strip).
// Effect (frames of play): in 30 = glyph by glyph from the left, each rises 3 px and sharpens (alpha); hold = a very slow 1 px sway per glyph; out 36 = glyph by glyph from
// the left, each drifts up and to the right by a few px and fades. Alpha = additive (text) / subtractive (shadow) blend with the colour scaled; full alpha = opaque text + black shadow.
#define SG_MAX 30
#define SG_IN 30
#define SG_OUT 36
#ifndef SG_Y1
#define SG_Y1 192 /* the lower (or the only) row's top; two rows are 16 px apart */
#endif
#ifndef SG_BLEND_ADD
#define SG_BLEND_ADD 0x20
#define SG_BLEND_SUB 0x40
#endif
static s32 g_SgPrim = -1;
static s32 g_SgN = 1;
static s32 g_SgPhase = 0; // 0 none, 1 in, 2 hold, 3 out
static s32 g_SgT = 1;
static s16 g_SgX[SG_MAX] = {1};
static s16 g_SgY[SG_MAX] = {1};
static const s8 g_SgSway[16] = {0, 0, 1, 1, 1, 1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0};
#define SG_GLYPH_FN (*(u16* (**)(u32, u16))0x8003C800) /* g_api.func_80106A28 */

static s32 SgAdv(u8 c) {
    return (c & 0xFC) == 0xF0 ? 12 : 6;
}

static void SgFree(void) {
    s32 i = g_SgPrim;

    g_SgPrim = -1;
    g_SgPhase = 0;
    if (i < 0 || i >= MAX_PRIM_COUNT) {
        return;
    }
    if (g_PrimBuf[i].type != PRIM_SPRT || g_PrimBuf[i].tpage != 0x10 ||
        (g_PrimBuf[i].priority != SUB_PRIO_TEXT && g_PrimBuf[i].priority != SUB_PRIO_SHADOW)) {
        return;
    }
    FreePrimitives(i);
}

static void SgShow(const char* r0, const char* r1) {
    const u8* row[2];
    const u8* p;
    Primitive* sh;
    Primitive* t;
    u32 buf[24];
    u16* glyph;
    RECT rc;
    s32 r, cnt, w, x, y, k, i, rows, col, band;
    u16 code;

    SgFree();
    row[0] = (const u8*)r0;
    row[1] = (const u8*)r1;
    rows = *row[1] ? 2 : 1;
    cnt = 0;
    for (r = 0; r < rows; r++) {
        for (p = row[r]; *p; p += ((*p & 0xFC) == 0xF0) ? 2 : 1) {
            if (*p != 0x20) {
                cnt++;
            }
        }
    }
    if (cnt == 0 || cnt > SG_MAX) {
        return;
    }
    g_SgPrim = AllocPrimitives(PRIM_SPRT, cnt * 2);
    if (g_SgPrim < 0) {
        g_SgPrim = -1;
        return;
    }
    g_SgN = cnt;
    sh = &g_PrimBuf[g_SgPrim];
    t = sh;
    for (i = 0; i < cnt; i++) {
        t = t->next;
    }
    k = 0;
    for (r = 0; r < rows; r++) {
        w = 0;
        for (p = row[r]; *p; p += ((*p & 0xFC) == 0xF0) ? 2 : 1) {
            w += SgAdv(*p);
        }
        x = (256 - w) / 2;
        if (x > 248 - w) {
            x = 248 - w;
        }
        if (x < 8) {
            x = 8;
        }
        y = SG_Y1 - (rows - 1 - r) * 16;
        for (p = row[r]; *p; p += ((*p & 0xFC) == 0xF0) ? 2 : 1) {
            if (*p == 0x20) {
                x += 6;
                continue;
            }
            code = ((*p & 0xFC) == 0xF0) ? (u16)((p[0] << 8) | p[1]) : ((const u16*)0x8009EF00)[*p - 0x21]; // ASCII: the KO engtab cell
            glyph = code ? SG_GLYPH_FN(code, 1) : NULL;
            if (glyph != NULL) {
                for (i = 0; i < 24; i++) {
                    buf[i] = ((u32*)glyph)[i];
                }
                col = k % 15;
                band = k / 15;
                rc.x = 18 + col * 3;
                rc.y = 480 + band * 16;
                rc.w = 3;
                rc.h = 16;
                LoadImage(&rc, (u_long*)buf);
                DrawSync(0); // the glyph buffer is the game's single shared one
            }
            t->tpage = 0x10;
            t->clut = PAL_UNK_1A1; // ink = index 8 = white
            t->u0 = 72 + (k % 15) * 12;
            t->v0 = 224 + (k / 15) * 16;
            t->u1 = 12;
            t->v1 = 16;
            t->priority = SUB_PRIO_TEXT;
            t->drawMode = DRAW_HIDE;
            sh->tpage = 0x10;
            sh->clut = PAL_UNK_1A1;
            sh->u0 = t->u0;
            sh->v0 = t->v0;
            sh->u1 = 12;
            sh->v1 = 16;
            sh->priority = SUB_PRIO_SHADOW;
            sh->drawMode = DRAW_HIDE;
            g_SgX[k] = x;
            g_SgY[k] = y;
            t = t->next;
            sh = sh->next;
            x += SgAdv(*p);
            k++;
        }
    }
    g_SgPhase = 1;
    g_SgT = 0;
}

// every frame of play: alpha / offsets of every glyph by the phase
static void SgTick(void) {
    Primitive* sh;
    Primitive* t;
    s32 i, a, dx, dy, n, lt, st, v;

    if (g_SgPrim < 0 || g_SgPhase == 0) {
        return;
    }
    if (g_PrimBuf[g_SgPrim].type != PRIM_SPRT || g_PrimBuf[g_SgPrim].tpage != 0x10) { // the buffer was rebuilt
        g_SgPrim = -1;
        g_SgPhase = 0;
        return;
    }
    n = g_SgN;
    sh = &g_PrimBuf[g_SgPrim];
    t = sh;
    for (i = 0; i < n; i++) {
        t = t->next;
    }
    g_SgT++;
    if (g_SgPhase == 1 && g_SgT >= SG_IN + 12) {
        g_SgPhase = 2;
        g_SgT = 0;
    } else if (g_SgPhase == 3 && g_SgT >= SG_OUT + 12) {
        SgFree();
        return;
    }
    for (i = 0; i < n; i++) {
        a = 16;
        dx = 0;
        dy = 0;
        if (g_SgPhase == 1) {
            st = i * SG_IN / n;
            lt = g_SgT - st;
            a = lt <= 0 ? 0 : (lt >= 12 ? 16 : lt * 16 / 12);
            dy = (16 - a) * 3 / 16;
        } else if (g_SgPhase == 2) {
            dy = g_SgSway[(g_SgT / 4 + i * 3) & 15];
        } else {
            st = i * SG_OUT / n;
            lt = g_SgT - st;
            if (lt < 0) {
                lt = 0;
            }
            a = lt >= 12 ? 0 : 16 - lt * 16 / 12;
            dx = lt / 3;
            dy = -(lt / 3);
            if (dx > 5) {
                dx = 5;
            }
            if (dy < -5) {
                dy = -5;
            }
        }
        if (a <= 0) {
            t->drawMode = DRAW_HIDE;
            sh->drawMode = DRAW_HIDE;
        } else if (a >= 16) {
            t->r0 = t->g0 = t->b0 = 0x80;
            t->drawMode = DRAW_ABSPOS;
            sh->r0 = sh->g0 = sh->b0 = 0;
            sh->drawMode = DRAW_ABSPOS | DRAW_COLORS;
        } else {
            v = a * 8; // 0..127: the colour scale
            t->r0 = t->g0 = t->b0 = v;
            t->drawMode = DRAW_ABSPOS | DRAW_COLORS | DRAW_TRANSP | SG_BLEND_ADD;
            sh->r0 = sh->g0 = sh->b0 = v / 2;
            sh->drawMode = DRAW_ABSPOS | DRAW_COLORS | DRAW_TRANSP | SG_BLEND_SUB;
        }
        t->x0 = g_SgX[i] + dx;
        t->y0 = g_SgY[i] + dy;
        sh->x0 = g_SgX[i] + dx + 1;
        sh->y0 = g_SgY[i] + dy + 1;
        t = t->next;
        sh = sh->next;
    }
}

static void SgHide(void) { // a line ends: the glyphs drift away
    if (g_SgPrim >= 0 && g_SgPhase != 3) {
        g_SgPhase = 3;
        g_SgT = 0;
    }
}

// ---- the songs (Nocturne, I Am the Wind): the vocal phrases' frames (gen_ko_sub8.py) against the real-time clock; a phrase without a text shows nothing.
// g_PlayingXaBgmId (0x8013901C) = the XA music's id - 0x300: 0x3F = Nocturne (the PSP dub = both dub bits clear sings the English recording: its own table),
// 0x3C = I Am the Wind. The clock is VSync(-1) (vblanks, real time: unaffected by pauses and by the ring's slow motion).
#define SONG_XA_ID (*(volatile s32*)0x8013901C)
static s32 g_SongXa = -1;
static s32 g_SongTrk = -1;
static s32 g_SongV0 = 1;
static s32 g_SongCur = -1;
static s32 g_SongShown = -1; // -1: the line on screen is not a song's

static void SongTick(void) {
    s32 id, t, i, n;
    const KoSongLine* tb;

    id = SONG_XA_ID;
    if (id != g_SongXa) {
        g_SongXa = id;
        g_SongCur = -1;
        g_SongTrk = id == 0x3F ? ((MOD_STATUS_FLAGS & (MOD_FLAG_PS1_DUB | MOD_FLAG_JP_DUB)) ? 0 : 1) : (id == 0x3C ? 2 : -1);
        g_SongV0 = VSync(-1);
        if (g_SongShown >= 0) {
            g_SongShown = -1;
            SgFree();
        }
    }
    if (g_SongTrk < 0) {
        return;
    }
    t = VSync(-1) - g_SongV0;
    if (g_SongTrk == 0) {
        tb = g_KoSong0;
        n = KO_SONG_N0;
    } else if (g_SongTrk == 1) {
        tb = g_KoSong1;
        n = KO_SONG_N1;
    } else {
        tb = g_KoSong2;
        n = KO_SONG_N2;
    }
    for (i = 0; i < n; i++) {
        if (t >= tb[i].a && t < tb[i].b) {
            break;
        }
    }
    if (i == g_SongCur) {
        return;
    }
    g_SongCur = i;
    if (i < n && (*tb[i].r0 || *tb[i].r1)) {
        SgShow(tb[i].r0, tb[i].r1);
        g_SongShown = 0;
    } else if (g_SongShown >= 0) {
        g_SongShown = -1;
        SgHide();
    }
}

// header entry +20: cmd 1 = show the voice id, 2 = hide
void SubCmd(s32 cmd, s32 id) {
    if (cmd == 1) {
        SubShow(id);
    } else {
        SubFree();
    }
}

// the game's PlaySfx for the familiars' shouts: show the line of the id (if it has one), then play the sound as usual
s32 SpuSfxHook(s32 id) {
    if (id >= 0x885 && id <= 0x8B8) {
        SubShow(id);
    }
    return ((s32(*)(s32))g_SpuOrig)(id);
}

// every frame of play (R1Tick): keep the PlaySfx slot on our hook, count a timed line down, follow the familiar
void SubFollow(void) {
    SongTick();
    SgTick();
    if (*SPU_SLOT != (u32)SpuSfxHook) { // (the table is rebuilt at stage loads)
        g_SpuOrig = *SPU_SLOT;
        *SPU_SLOT = (u32)SpuSfxHook;
    }
#ifdef SUB_DEBUG
    if (g_SubDbg > 1) {
        SubShow(g_SubDbg);
        g_SubDbg = 1;
    }
    if (g_SpuDbg > 1) {
        s32 id = g_SpuDbg;

        g_SpuDbg = 1;
        ((s32(*)(s32))*SPU_SLOT)(id);
    }
#endif
    if (g_SubPrim < 0) {
        return;
    }
    if (g_SubTimer > 0 && --g_SubTimer == 0) {
        SubFree();
        return;
    }
    if (g_PrimBuf[g_SubPrim].type != PRIM_SPRT || g_PrimBuf[g_SubPrim].tpage != 0x10) { // the buffer was rebuilt
        g_SubPrim = -1;
        g_SubLine = -1;
        return;
    }
    SubLayout(1);
}
