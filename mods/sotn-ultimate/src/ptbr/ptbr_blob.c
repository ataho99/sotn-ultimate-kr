typedef unsigned int u32;
typedef int s32;

#define BLOB ((u32*)0x801F0000)
#define MAGIC 0x31425450 // "PTB1"
#define HOOK 0x801F0010
// main.exe: the Krom2RawAdd stub (li t2, 0xB0; jr t2; li t1, 0x51), which
// DRA's func_80106A28 and nothing else calls, and FlushCache's
#define KROM2RAWADD ((volatile u32*)0x80015218)
#define FLUSH_CACHE ((void (*)(void))0x800192CC)

#define SELBLK ((volatile u32*)0x801E8000)
#define SEL_MAGIC 0x314C5350 // "PSL1"
#define SELDATA __attribute__((section(".sel.rodata")))
#define SELCODE __attribute__((section(".sel.text"), noinline))
#define TTL_MAGIC 0x314C5454   // "TTL1"
#define NAMES_MAGIC 0x314D414E // "NAM1"
#define END_MAGIC 0x30444E45   // "END0"
#define ENJ_MAGIC 0x304A4E45   // "ENJ0" (block 4: the ending's English JP copy)
#define TTLDATA __attribute__((section(".ttl.rodata")))
#define TTLCODE __attribute__((section(".ttl.text"), noinline))
// (its variables: back to their first value at each load)
#define TTLVAR __attribute__((section(".ttl.data")))

#include "mod_ptbr_sel.h"

static void HookKrom(void) {
    u32 j = 0x08000000 | ((HOOK >> 2) & 0x03FFFFFF);

    if (KROM2RAWADD[0] != j) {
        KROM2RAWADD[0] = j;
        KROM2RAWADD[1] = 0; // nop (the jump's delay slot)
        FLUSH_CACHE();
    }
}

static void Text(s32 pt) {
    volatile u32* p = (volatile u32*)((char*)BLOB + BLOB[3]);
    volatile u32* field;
    s32 runs, n, stride;

    for (runs = *p++; runs; runs--) {
        field = (volatile u32*)p[0];
        stride = p[1] & 0xFFFF;
        n = p[1] >> 16;
        for (p += 2; n; n--, p += 2) {
            if (p[1]) {
                if (*field != p[1]) {
                    p[0] = *field;
                }
                if (p[0]) {
                    *field = pt ? p[1] : p[0];
                }
            }
            field = (volatile u32*)((char*)field + stride);
        }
    }
}

#define LANG_EN (BLOB[2])

// A gothic word drawn smaller than DrawString16x16's 16x16 (the card
// panel's "sem / cartao"): SEL's sheet letters as scaled quads, through
// the mod Player Select's ModCharselQuad (tpage 0xC, CLUT 0x200)
SELDATA static const unsigned char g_SelEmpty[1] = {0};
SELCODE static void SelSmall(const unsigned char* s, s32 x, s32 y, s32 size) {
    s32 w = size & 0xF, h = size >> 4, i;
    const unsigned char* uv = (const unsigned char*)SEL_GOTHIC_UV;

    for (; *s; s++, x += w) {
        i = ((s32(*)(s32))SEL_GOTHIC_INDEX)(*s);
        ((void (*)(s32, s32, s32, s32, s32, s32, s32, s32, u32, u32, s32,
                   s32))SEL_QUAD)(
            x, y, w, h, uv[i * 3 + 1], uv[i * 3 + 2], 16, 16, 0xC,
            ((unsigned short*)0x8003C104)[0x200], 0x80, 0x20);
    }
}

typedef struct {
    short x, y, w, h;
} SelRect;
SELCODE static void SelHeader(s32 u) {
    SelRect r;

    r.x = SEL_HDR_VRAM_X + u / 4;
    r.y = SEL_HDR_Y;
    r.w = SEL_HDR_W / 4;
    r.h = SEL_HDR_H;
    ((void (*)(SelRect*, const u32*))SEL_LOADIMAGE)(&r, g_SelHdrPix);
    r.x = SEL_HDR_VRAM_X;
    r.y = SEL_HDR_CLUT_Y;
    r.w = 16;
    r.h = 1;
    ((void (*)(SelRect*, const u32*))SEL_LOADIMAGE)(
        &r, (const u32*)SEL_MENU_CLUT);
    ((s32(*)(s32))SEL_DRAWSYNC)(0);
}

__attribute__((used)) void PtbrSelMap(u32* a) {
    s32 i;

    if (LANG_EN || SELBLK[0] != SEL_MAGIC) {
        return;
    }
    for (i = 0; i < SEL_MAP_N; i++) {
        if (a[0] == SEL_BASE + g_SelMap[i].en * 4) {
            if (g_SelMap[i].small == 0xFF) { // the Player Select header
                SelHeader(a[3]);
                a[2] = 0; // (no rows of the Saturn one)
                return;
            }
            a[0] = (u32)&g_SelText[g_SelMap[i].pt];
            a[1] += g_SelMap[i].dx;
            a[2] += g_SelMap[i].dy;
            if (g_SelMap[i].small) {
                SelSmall(
                    (const unsigned char*)a[0], a[1], a[2], g_SelMap[i].small);
                a[0] = (u32)g_SelEmpty;
            }
            return;
        }
    }
}

// Patch the first two instructions of SEL's text functions with a jump to
// their trampoline: only while they hold what the build found there (SEL
// is loaded again from the disc each time, unpatched)
SELCODE static void SelHook(void) {
    volatile u32* f;
    s32 i, n = 0;

    for (i = 0; i < SEL_HOOKS; i++) {
        f = (volatile u32*)g_SelHookAt[i];
        if (f[0] == g_SelHookWord[i * 2] && f[1] == g_SelHookWord[i * 2 + 1]) {
            f[0] = 0x08000000 | (((u32)g_SelHookTo[i] >> 2) & 0x03FFFFFF);
            f[1] = 0;
            n++;
        }
    }
    if (n) {
        FLUSH_CACHE();
    }
}

// the save panel's area names (D_80180128 shares words between areas, so
// its fields are written instead). English: left alone, as SEL comes fresh
// from the disc each time (the language changes in game)
SELCODE static void SelAreas(void) {
    s32 i;

    if (LANG_EN) {
        return;
    }
    for (i = 0; i < SEL_AREA_N; i++) {
        *(volatile u32*)(SEL_BASE + g_SelArea[i].field) =
            (u32)&g_SelText[g_SelArea[i].pt];
    }
}

SELCODE static void SelWords(void) {
    s32 i;

    if (LANG_EN) {
        return;
    }
    for (i = 0; i < SEL_WORD_N; i++) {
        *(volatile u32*)(SEL_BASE + g_SelWords[i].at) = g_SelWords[i].value;
    }
}

// ... and the copy screen's source panel, whose sprites SEL set up from
// those tables at its init, before the title block was there: its
// level / time sprites still showing the retail label (u0) get the new one
#define PRIM_BUF 0x80086FEC // DRA g_PrimBuf (0x34 bytes each)
SELCODE static void SelPanel(void) {
    volatile unsigned char* p;
    s32 n, k, idx;

    if (LANG_EN) {
        return;
    }
    idx = ((volatile s32*)SEL_GFX)[0x13 * 2];
    if (idx <= 0 || idx >= 0x500) {
        return;
    }
    p = (volatile unsigned char*)(PRIM_BUF + idx * 0x34);
    for (n = 0; n < 8 && p; n++) {
        for (k = 0; k < SEL_LABEL_N; k++) {
            if (g_SelLabels[k].i == n && p[0x0C] == g_SelLabels[k].old_u &&
                *(volatile unsigned short*)(p + 0x1A) == 0xC) {
                p[0x0C] = g_SelLabels[k].u;
                p[0x0D] = g_SelLabels[k].v;
                p[0x18] = g_SelLabels[k].w;
            }
        }
        p = *(volatile unsigned char* volatile*)p;
    }
}

#define ENGINE_STEP ((volatile s32*)DRA_ENGINE_STEP)
SELCODE static void SelSoundBtn(void) {
    SelRect r;

    if (LANG_EN || *ENGINE_STEP != 1) { // Upd_Eng_MenuFadeIn
        return;
    }
    r.x = 960;
    r.y = 96;
    r.w = 128 / 4;
    r.h = 32;
    ((void (*)(SelRect*, const u32*))SEL_LOADIMAGE)(&r, g_SelSoundBtn);
}

// the title block's part of a frame (only while it is there)
SELCODE static void SelFrame(void) {
    SelHook();
    SelAreas();
    SelWords();
    SelPanel();
    SelSoundBtn();
}

// the title block's header: SEL_MAGIC, then its frame
__attribute__((section(".sel.hdr"), used)) static const u32 g_SelHdr[] = {
    SEL_MAGIC,
    (u32)SelFrame,
};

static void Heads(s32 pt) {
    volatile u32* f;
    s32 i, n = 0;

    for (i = 0; i < DRA_HEADS; i++) {
        f = (volatile u32*)g_HeadAt[i];
        if (*f == g_HeadWord[i * 2 + !pt]) {
            *f = g_HeadWord[i * 2 + pt];
            n++;
        }
    }
    if (n) {
        FLUSH_CACHE();
    }
}

static s32 Block(s32 id, s32 lba);

static void Frame(s32 step) {
    if (!step) {
        HookKrom();
        if (SELBLK[0] == SEL_MAGIC) {
            SelFrame();
        }
        // the main menu hands over to the game (a save loaded:
        // Upd_Eng_260; a new game: Upd_Eng_18): the area names' block
        // then, as the title's step 5 does (the stages with no room for
        // their banner read it there; DRA's own loads wait for this one)
        if (*ENGINE_STEP == 0x104 || *ENGINE_STEP == 18) {
            Block(2, BLOCK_LBA);
        }
    } else {
        Heads(!LANG_EN);
    }
    Text(step && !LANG_EN);
}

#define CD_STEP ((volatile s32*)0x8006C398)      // g_CdStep
#define LOAD_FILE ((volatile s32*)0x8006BAFC)    // g_LoadFile
#define LOAD_OVL_IDX ((volatile s32*)0x80097918) // g_LoadOvlIdx
#define DEMOKEY_LOC ((volatile s32*)MOD_PTBR_DEMOKEY)
#define CDFILE_DEMOKEY 32 // CdFile_DemoKey
static const u32 g_BlockMagic[5] = {
    SEL_MAGIC, TTL_MAGIC, NAMES_MAGIC, END_MAGIC, ENJ_MAGIC};

static s32 g_BlockBusy, g_BlockLoc, g_BlockFail;

__attribute__((noinline)) static s32 Title(void);

static s32 Block(s32 id, s32 lba) {
    if (id == 1 && *ENGINE_STEP >= 3) {
        if (*ENGINE_STEP != 5) {
            return 0;
        }
        id = 2;
    }
    if (g_BlockBusy || SELBLK[0] != g_BlockMagic[id]) {
        if (*CD_STEP != 0) {
            return 1;
        }
        if (!g_BlockBusy) {
            if (g_BlockFail & (1 << id)) {
                return 0;
            }
            g_BlockLoc = *DEMOKEY_LOC;
            *DEMOKEY_LOC = lba;
            *LOAD_OVL_IDX = id;
            *LOAD_FILE = CDFILE_DEMOKEY;
            *CD_STEP = 1;         // CdStep_LoadInit
            g_BlockBusy = id + 1; // (which block is loading)
            return 1;
        }
        // a load finished: close it (the area names' one, started as the
        // menu hands over to the game, is only closed here, at the next
        // block: the ending's). Another block's: this one loads next call
        *DEMOKEY_LOC = g_BlockLoc;
        lba = g_BlockBusy;
        g_BlockBusy = 0;
        if (lba != id + 1) {
            return 1;
        }
        if (SELBLK[0] != g_BlockMagic[id]) {
            g_BlockFail |= 1 << id;
            return 0;
        }
    }
    return id == 1 ? Title() : 0;
}

#define PADS                                                                   \
    ((volatile unsigned short*)DRA_PADS) // g_pads[0]: pressed, previous, tapped
#define SEL_STAGE_GFX ((volatile u32*)DRA_SEL_GFX_OFF)
#define TIMER (*(volatile s32*)DRA_TIMER)
#define API(off) (*(u32*)(DRA_API + (off)))
#define ALLOC_PRIMS(n) (((short (*)(s32, s32))API(0x44))(6, n)) // PRIM_SPRT
#define FREE_PRIMS(i) (((void (*)(s32))API(0x40))(i))
#define PLAY_SFX(id) (((void (*)(s32))API(0x68))(id))
#define SFX_MOVE 0x67B   // SFX_UI_MOVE
#define SFX_SELECT 0x67D // SFX_UI_MP_FULL (the main menu's select)
#define PAD_MOVE 0xF000  // the d-pad
#define PAD_START 0x0800
#define PAD_OK 0x0860 // PAD_START | PAD_CROSS | PAD_CIRCLE
#define LANG_PRIMS 96
#define DRAW_COLORS 4
#define DRAW_HIDE 8
#define UNPACK                                                                 \
    ((unsigned char*)0x801EA000) // the save buffer (idle on the title)
// a sprite's clut is an index into DRA's g_ClutIds (VRAM CLUT ids, built at
// boot for y 240-255 only): the screen points the last three (stage
// palettes, unused on the title) at its own CLUTs, and back after
#define CLUT_IDS ((unsigned short*)0x8003C104)
#define CI_LETTERS 0x2FD
#define CI_FONT 0x2FE
#define CI_PLATE 0x2FF

typedef struct Prim {
    struct Prim* next;
    unsigned char r0, g0, b0, type;
    short x0, y0;
    unsigned char u0, v0;
    unsigned short clut;
    unsigned char r1, g1, b1, p1;
    short x1, y1;
    unsigned char u1, v1; // (a sprite's width / height)
    unsigned short tpage;
    unsigned char pad0[0xA];
    unsigned short priority;
    unsigned char pad1[0xA];
    unsigned short drawMode;
} Prim; // (DRA's Primitive, 0x34 bytes)

// resident: kept for the boot (0 the screen to come, 1 on it, 2 chosen)
static s32 g_LangState, g_LangSel, g_LangPrim;
static unsigned short g_LangCi[3]; // the borrowed g_ClutIds entries
// per visit (the block comes again with each title visit)
TTLVAR static s32 g_TtlFresh = 1;

TTLCODE static void Piece(s32 i) {
    const TtlPiece* pc = &g_TtlPieces[i];
    const unsigned char* s = g_TtlPack + pc->at;
    unsigned char* d = UNPACK;
    s32 n = pc->w * 2 * pc->h;
    s32 k;
    SelRect r;

    while (n > 0) {
        if (*s) {
            *d++ = *s++;
            n--;
        } else {
            k = s[1];
            s += 2;
            n -= k;
            while (k--) {
                *d++ = 0;
            }
        }
    }
    r.x = pc->x;
    r.y = pc->y;
    r.w = pc->w;
    r.h = pc->h;
    ((void (*)(SelRect*, const u32*))SEL_LOADIMAGE)(&r, (const u32*)UNPACK);
    ((s32(*)(s32))SEL_DRAWSYNC)(0);
}

TTLCODE static void Clut(s32 x, const void* c) {
    SelRect r;

    r.x = x;
    r.y = TTL_CLUT_Y;
    r.w = 16;
    r.h = 1;
    ((void (*)(SelRect*, const u32*))SEL_LOADIMAGE)(&r, (const u32*)c);
    ((s32(*)(s32))SEL_DRAWSYNC)(0);
}

// the next sprite of the screen's list
TTLCODE static Prim* Sprite(Prim* p, s32 x, s32 y, s32 u, s32 v, s32 w, s32 h,
                            s32 tpage, s32 clut, s32 grey, s32 prio) {
    if (!p) {
        return p;
    }
    p->x0 = x;
    p->y0 = y;
    p->u0 = u;
    p->v0 = v;
    p->u1 = w;
    p->v1 = h;
    p->tpage = tpage;
    p->clut = clut;
    p->r0 = p->g0 = p->b0 = grey;
    p->priority = prio;
    p->drawMode = DRAW_COLORS;
    return p->next;
}

TTLDATA static const char g_TtlAsk[2][24] = {
    "Selecione seu idioma", "Choose your language"};

#define WORD_Y 186
#define ASK_Y 158
TTLDATA static const short g_TtlPlate[2][2] = {{76, 184}, {284, 152}};

typedef struct {
    short x, y, w, h;
} TtlRect;
TTLDATA static const TtlRect g_TtlLogoSrc[4] = {
    {TTL_LOGO_SRC_X, TTL_LOGO_SRC_Y, TTL_LOGO_W, TTL_LOGO_H},
    {TTL_LOGO_SRC_X, TTL_LOGO_SRC_Y + TTL_LOGO_H, TTL_LOGO_W, TTL_LOGO_H},
    {TTL_LOGO_SRC_X, TTL_LOGO_SRC_Y + 2 * TTL_LOGO_H, TTL_LOGO_W, TTL_LOGO_H},
    {TTL_LOGO_SRC_X, TTL_LOGO_SRC_Y + 3 * TTL_LOGO_H, TTL_LOGO_W, TTL_LOGO_H},
};
TTLCODE static void TitleLogo(s32 pt) {
    s32 k;

    for (k = 0; k < 2; k++) {
        ((s32(*)(const TtlRect*, s32, s32))MOVE_IMAGE)(
            &g_TtlLogoSrc[pt * 2 + k], TTL_LOGO_DST_X + k * TTL_LOGO_W,
            TTL_LOGO_DST_Y);
    }
}

TTLCODE static void Draw(void) {
    Prim* p = (Prim*)(DRA_PRIMBUF + g_LangPrim * 0x34);
    const unsigned char(*word)[3];
    const char* s;
    s32 i, j, n, x, y, w;

    i = g_LangSel;
    j = g_TtlPlate[i][0] + g_TtlPlate[i][1]; // its right end
    y = WORD_Y - 8; // (both: the same padding under the word)
    p = Sprite(p, g_TtlPlate[i][0], y, TTL_CAP_L_U, TTL_CAP_V, TTL_CAP_W,
               TTL_PLATE_H, TTL_TPAGE, CI_PLATE, 0x80, 3);
    p = Sprite(p, j - TTL_CAP_W, y, TTL_CAP_R_U + 2, TTL_CAP_V, TTL_CAP_W,
               TTL_PLATE_H, TTL_TPAGE, CI_PLATE, 0x80, 3);
    for (x = g_TtlPlate[i][0] + TTL_CAP_W; x < j - TTL_CAP_W; x += w) {
        w = j - TTL_CAP_W - x;
        if (w > TTL_BAR_W) {
            w = TTL_BAR_W;
        }
        p = Sprite(p, x, y, TTL_BAR_U, TTL_BAR_T_V, w, 5, TTL_TPAGE, CI_PLATE,
                   0x80, 3);
        p = Sprite(p, x, y + TTL_BAR_B_Y, TTL_BAR_U, TTL_BAR_B_V, w, 5,
                   TTL_TPAGE, CI_PLATE, 0x80, 3);
    }
    // the words: a dark shadow (1 px right / down), the unchosen one dimmed
    for (i = 0; i < 2; i++) {
        word = i ? g_TtlEn : g_TtlPt;
        n = i ? TTL_EN_LEN : TTL_PT_LEN;
        x = g_TtlPlate[i][0] + 20;
        y = WORD_Y;
        for (j = 0; j < n; j++) {
            s32 h = word[j][2];
            p = Sprite(p, x + j * 16 + 1, y - (h - 16) + 1, word[j][0],
                       word[j][1], 16, h, TTL_TPAGE, CI_LETTERS, 0x08, 4);
            p = Sprite(p, x + j * 16, y - (h - 16), word[j][0], word[j][1], 16,
                       h, TTL_TPAGE, CI_LETTERS, i == g_LangSel ? 0x80 : 0x60,
                       5);
        }
    }
    // the prompt in the chosen language (the title sheet's 8x8 font)
    s = g_TtlAsk[g_LangSel];
    for (n = 0; s[n]; n++) {
    }
    x = 256 - n * 4;
    for (j = 0; j < n; j++) {
        s32 c = s[j] - 0x20;
        if (!c) {
            continue;
        }
        p = Sprite(p, x + j * 8 + 1, ASK_Y + 1, (c & 15) * 8, (c >> 4) * 8, 8,
                   8, TTL_FONT_TPAGE, CI_FONT, 0x08, 4);
        p = Sprite(p, x + j * 8, ASK_Y, (c & 15) * 8, (c >> 4) * 8, 8, 8,
                   TTL_FONT_TPAGE, CI_FONT, 0x80, 5);
    }
    for (; p; p = p->next) {
        p->drawMode = DRAW_HIDE;
    }
    // the plate's glow (the Saturn's ramp, a row every 2 frames)
    i = (TIMER >> 1) % TTL_RAMP_ROWS; // (up the ramp's half, then back)
    Clut(TTL_CLUT_PLATE_X,
         g_TtlRamp[i < TTL_RAMP_ROWS / 2 ? i : TTL_RAMP_ROWS - 1 - i]);
}

// the title's sprites 0-6 (0-2 the prompt, 3-6 the copyright line), on or
// off: the prompt off the screen (the PRESS START step, still running under
// the language screen for the title's animation, blinks it), the copyright
// line hidden
TTLCODE static void TitleText(s32 on) {
    Prim* p = (Prim*)(DRA_PRIMBUF + *(volatile short*)SEL_TITLE_PRIM * 0x34);
    s32 i;

    for (i = 0; i < 7 && p; i++, p = p->next) {
        if (i < 3) {
            p->y0 = on ? 0xA8 : 0x1A8;
        } else {
            p->drawMode = on ? DRAW_COLORS : DRAW_HIDE;
        }
    }
}

TTLCODE static s32 TitleFrame(void) {
    s32 i, tap;

    // the logo's subtitle: the highlighted language on the language
    // screen, else the language (before the fade-in shows it)
    TitleLogo(g_LangState == 1 ? !g_LangSel : !LANG_EN);
    if (*ENGINE_STEP == 1) {
        return 0; // (loaded during the fade-in: the screen once it is done)
    }
    if (g_LangState == 0) {
        for (i = TTL_ART; i < TTL_PIECES; i++) {
            Piece(i);
        }
        Clut(TTL_CLUT_LETTERS_X, (const void*)SEL_MENU_CLUT);
        for (i = 0; i < 3; i++) {
            g_LangCi[i] = CLUT_IDS[CI_LETTERS + i];
        }
        CLUT_IDS[CI_LETTERS] = TTL_CLUT_LETTERS;
        CLUT_IDS[CI_FONT] = TTL_CLUT_FONT;
        CLUT_IDS[CI_PLATE] = TTL_CLUT_PLATE;
        g_LangPrim = ALLOC_PRIMS(LANG_PRIMS);
        if (g_LangPrim < 0) {
            g_LangState = 2;
        } else {
            g_LangSel = 0;
            g_LangState = 1;
        }
    }
    if (g_LangState == 1) {
        tap = PADS[2];
        PADS[2] =
            tap & ~PAD_START; // (START is the screen's, not PRESS START's)
        if (tap & PAD_MOVE) {
            g_LangSel ^= 1;
            PLAY_SFX(SFX_MOVE);
        }
        if (tap & PAD_OK) {
            PLAY_SFX(SFX_SELECT);
            BLOB[2] = g_LangSel;
            FREE_PRIMS(g_LangPrim);
            for (i = 0; i < 3; i++) {
                CLUT_IDS[CI_LETTERS + i] = g_LangCi[i];
            }
            g_LangState = 2;
            g_TtlFresh = 1;
        } else {
            TitleText(0);
            Draw();
            return 0;
        }
    }
    // the menus' art: SEL's stage entry at the language's F_SEL
    *SEL_STAGE_GFX = (LANG_EN || !SEL_GFX_PT) ? SEL_GFX_EN : SEL_GFX_PT;
    if (g_TtlFresh) {
        g_TtlFresh = 0;
        if (!LANG_EN) {
            for (i = 0; i < TTL_ART; i++) {
                Piece(i);
            }
        }
    }
    TitleText(1);
    return 0;
}
__attribute__((section(".ttl.hdr"), used)) static const u32 g_TtlHdr[] = {
    TTL_MAGIC,
    (u32)TitleFrame,
};

__attribute__((noinline)) static s32 Title(void) {
    if (SELBLK[0] == TTL_MAGIC) {
        return ((s32(*)(void))SELBLK[1])();
    }
    return 0;
}

__attribute__((section(".entry"), used)) void* const g_PtbrEntry[] = {
    (void*)Frame,
    (void*)Block,
};
