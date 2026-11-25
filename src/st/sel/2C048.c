// SPDX-License-Identifier: AGPL-3.0-or-later
#include "sel.h"

extern u8* D_8018C404; // unknown type
extern char g_AsciiSet[];
extern s32 g_MainMenuCursor;
extern s32 g_InputCursorPos;
extern s32 D_801BC3E0; // on-screen keyboard key position
extern s32 D_801BC3EC; // selected memory card block
extern s32 D_801D6B04;
extern s32 g_MemCardSelectorX;
extern s32 g_MemCardSelectorY;
extern char g_InputSaveName[12];

// File-select confirm/cancel buttons. US retail: CROSS confirm, TRIANGLE
// cancel. MOD_KO (Japanese layout): CIRCLE confirm, CROSS cancel.
#if defined(MOD_KO)
#define SEL_CONFIRM PAD_CIRCLE
#define SEL_CANCEL PAD_CROSS
#else
#define SEL_CONFIRM PAD_CROSS
#define SEL_CANCEL PAD_TRIANGLE
#endif
#if defined(MOD_KO)
#define SEL_DECIDEV 0x60 /* red circle slot (F_SEL patched), was 0x80 = right-arrow */
#define SEL_CANCELV 0x70
#define SEL_CANCELCLUT 0x200
#else
#define SEL_DECIDEV 0x70
#define SEL_CANCELV 0x60
#define SEL_CANCELCLUT 0x202
#endif

// BSS
static s32 D_801BAF08; // block icon animation index
static s32 D_801BAF0C;
static s32 D_801BAF10;
static s32 D_801BAF14;
static s32 D_801BAF18[NUM_GFX][2];
static s32 D_801BAFC0;
static s32 MainMenuMaskPrimIndex;
static s32 g_SelNextCrossPressEngStep;
static s32 g_SelEng220NextStep;

void Update(void);
void HandleTitleScreen(void);
void func_801B9C80(void);
void InitSel(s32 objLayoutId);
void func_801B60D4(void);
void func_801B17C8(void);

// DATA
Overlay g_Overlay = {
    /* 0x00 */ Update,
    /* 0x04 */ HandleTitleScreen,
    /* 0x08 */ func_801B9C80,
    /* 0x0C */ InitSel,
    /* 0x10 */ NULL,
    /* 0x14 */ g_SpriteBanks,
    /* 0x18 */ g_Cluts,
    /* 0x1C */ NULL,
    /* 0x20 */ NULL,
    /* 0x24 */ g_EntityGfxs,
    /* 0x28 */ func_801B60D4,
    /* 0x2C */ NULL,
    /* 0x30 */ &D_8018C404,
    /* 0x34 */ NULL,
    /* 0x38 */ NULL,
    /* 0x3C */ func_801B17C8,
};

// The five possible header options that are displayed on the top-left for each
// sub-menu in the main menu. The same graphics is also re-used for the main
// menu selectable options.
static s32 g_MenuHeadGfxU[] = {0x00, 0x80, 0x00, 0x00, 0x00};
static s32 g_MenuHeadGfxV[] = {0xC0, 0x90, 0xE0, 0x80, 0xA0};

static s32 g_MenuUnk084X[] = {0x10, 0x10, 0x18, 0x3D, 0x68, 0x80, 0x18, 0x98};
static s32 g_MenuUnk084Y[] = {0x08, 0x18, 0x38, 0x38, 0x38, 0x38, 0x40, 0x40};
static s32 g_MenuUnk084U0[] = {0x80, 0xA8, 0xE0, 0xE8, 0xE0, 0xE0, 0xE8, 0xF0};
static s32 g_MenuUnk084V0[] = {0xF0, 0xF0, 0x80, 0x80, 0x88, 0x88, 0x88, 0x88};
static s32 g_MenuUnk084U1[] = {0x28, 0x28, 0x08, 0x18, 0x08, 0x08, 0x08, 0x08};
static s32 g_MenuUnk084V1[] = {0x10, 0x10, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08};

StageName D_80180128[80] = {
    #if defined(MOD_KO)
    {"\xF0\x4A\xF0\x50\xF0\x62\x20\xF0\xCD\xF1\x41\x00\x00\x00", ""}, /* 0 대리석 복도 */
#else
    {_S("Marble"), _S("Gallery")},
#endif,
    #if defined(MOD_KO)
    {"\xF1\x96\xF1\x58\x20\xF2\xFE\x20\xF1\xD3\xF1\x96", ""}, /* 1 벽랑 측 외벽 (한패 표기, 이전 외벽) */
#else
    {_S("Outer"), _S("Wall")},
#endif,
    #if defined(MOD_KO)
    {"\xF0\x80\xF0\x61\xF0\xBD\x00\x00\x00\x00\x00\x00", ""}, /* 2 장서고 (한패 표기, 이전 긴 도서관) */
#else
    {_S("Long"), _S("Library")},
#endif,
    #if defined(MOD_KO)
    {"\xF0\x85\xF0\xA3\xF2\xA4\xF0\x85\x00\x00", ""}, /* 3 지하묘지 (한패 표기, 이전 카타콤베) */
#else
    {_S("Catacombs"), _S("")},
#endif,
    #if defined(MOD_KO)
    {"\xF1\xCF\xF0\x4E\xF0\x7C\x20\xF0\xCA\x00\x00\x00\x00\x00\x00\x00", ""}, /* 4 올록의 방 */
#else
    {_S("Olrox's"), _S("Quarters")},
#endif,
    #if defined(MOD_KO)
    {"\xF3\xF6\xF1\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", ""}, /* 5 폐광 (이전 연도 F0D8 F141, 버려진 광산) */
#else
    {_S("Abandoned"), _S("Mine")},
#endif,
    #if defined(MOD_KO)
    {"\xF1\xCD\xF0\xA8\xF2\x7A\x00\x00\x00\x00\x00\x00", ""}, /* 6 예배당 (한패 표기, 이전 왕실 예배당) */
#else
    {_S("Royal"), _S("Chapel")},
#endif,
    #if defined(MOD_KO)
    {"\xF0\x63\x20\xF0\xDD\xF0\x44\x00\x00", ""}, /* 7/65 shared */
#else
    {_S("Entrance"), _S("")},
#endif,
    {_S(""), _S("")},
    #if defined(MOD_KO)
    {"\xF0\x85\xF0\xA3\xF0\xD2\xF3\x2C\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", ""}, /* 9 지하수맥 (한패 표기, 이전 지하 동굴) */
#else
    {_S("Underground"), _S("Caverns")},
#endif,
    #if defined(MOD_KO)
    {"\xF2\x26\xF0\x46\xF0\x80\x00\x00\x00\x00", ""}, /* 10 투기장 */
#else
    {_S("Colosseum"), _S("")},
#endif,
    #if defined(MOD_KO)
    {"\xF0\x6D\xF0\x52\xF0\x63\x20\xF1\xFF\xF0\x60\xF0\x5C", ""}, /* 11 악마성 최상부 (한패 표기, 이전 성 본관) */
#else
    {_S("Castle"), _S("Keep")},
#endif,
    #if defined(MOD_KO)
    {"\xF0\xD8\xF1\x0A\xF0\xD8\xF0\x44\xF0\x9C\x00\x00\x00\x00\x00\x00\x00\x00", ""}, /* 12 연금연구동 (한패 표기, 이전 연금연구실) */
#else
    {_S("Alchemy"), _S("Laboratory")},
#endif,
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
#if defined(MOD_SATURN)
    #if defined(MOD_KO)
    {"\xF0\xDE\xF0\x82\xF1\x8B\xF1\xD9\x20\xF0\xF3\xF2\xCD", ""}, /* 16 저주받은 감옥 */
#else
    {_S("Cursed"), _S("Prison")},
#endif, // Saturn port: MOD_STAGE_PRS
#else
    {_S(""), _S("")},
#endif
#if defined(MOD_SATURN)
    #if defined(MOD_KO)
    {"\xF0\x85\xF0\xA3\x20\xF0\x81\xF1\xD6\x00\x00\x00\x00\x00\x00\x00\x00\x00", ""}, /* 17 지하 정원 */
#else
    {_S("Underground"), _S("Garden")},
#endif, // Saturn port: MOD_STAGE_GDN
#else
    {_S(""), _S("")},
#endif
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    #if defined(MOD_KO)
    {"\xF0\x5E\xF1\x01\xF0\x62\x20\xF0\xCD\xF1\x41\x00\x00\x00\x00\x00\x00\x00\x00\x00", ""}, /* 32 비광석 복도 (한패 표기, 이전 흑대리석 복도) */
#else
    {_S("Black Marble"), _S("Gallery")},
#endif,
    #if defined(MOD_KO)
    {"\xF1\xCC\x20\xF1\xD3\xF1\x96\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", ""}, /* 33 역 외벽 */
#else
    {_S("Reverse"), _S("Outer Wall")},
#endif,
    #if defined(MOD_KO)
    {"\xF1\x0A\xF0\x61\xF0\xA5\xF1\x00\xF0\xBD\x00\x00\x00\x00\x00\x00\x00", ""}, /* 34 금서보관고 (한패 표기, 이전 금지된 도서관) */
#else
    {_S("Forbidden"), _S("Library")},
#endif,
    #if defined(MOD_KO)
    {"\xF0\xA9\xF0\xA6\xF2\xA4\xF0\x85\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", ""}, /* 35 공중묘지 (한패 표기, 이전 떠다니는 카타콤베) */
#else
    {_S("Floating"), _S("Catacombs")},
#endif,
    #if defined(MOD_KO)
    {"\xF0\xAB\xF1\xDA\xF0\x7C\x20\xF0\xCA\x00\x00\x00\x00\x00\x00\x00\x00", ""}, /* 36 사익의 방 (한패 표기, 이전 데스윙의 소굴) */
#else
    {_S("Death Wing's"), _S("Lair")},
#endif,
    #if defined(MOD_KO)
    {"\xF0\x9C\xF2\x93\x00", ""}, /* 37 동굴 */
#else
    {_S("Cave"), _S("")},
#endif,
    #if defined(MOD_KO)
    {"\xF0\x7D\xF0\xC0\xF1\xCD\xF0\xA8\xF2\x7A\x00\x00", ""}, /* 38 이단예배당 (한패 표기, 이전 뒤집힌예배당) */
#else
    {_S("Anti-Chapel"), _S("")},
#endif,
    #if defined(MOD_KO)
    {"\xF0\xC3\xF0\xE1\xF2\x4F\xF0\x63\x20\xF0\xDD\xF0\x44\x00\x00\x00", ""}, /* 39 뒤집힌성 입구 (한패 표기, 이전 역 성 입구) */
#else
    {_S("Reverse"), _S("Entrance")},
#endif,
    {_S(""), _S("")},
#if defined(MOD_SATURN)
    // Saturn port: the retail "Reverce" typo, corrected
    #if defined(MOD_KO)
    {"\xF1\xFC\xF0\x81\xF0\xD2\xF3\x2C\x00\x00\x00\x00\x00\x00\x00", ""}, /* 41 천정수맥 (한패 표기, 이전 역 지하 동굴) */
#else
    {_S("Reverse"), _S("Caverns")},
#endif,
    #if defined(MOD_KO)
    {"\xF0\x6E\xF2\xF2\x20\xF2\x26\xF0\x46\xF0\x80\x00\x00\x00\x00\x00\x00", ""}, /* 42 안쪽 투기장 (한패 표기, 이전 역 투기장) */
#else
    {_S("Reverse"), _S("Colosseum")},
#endif,
    #if defined(MOD_KO)
    {"\xF0\xC3\xF0\xE1\xF2\x4F\xF0\x63\xF1\xFF\xF0\xA3\xF0\x5C", ""}, /* 43 뒤집힌성최하부 (한패 표기, 이전 역 성 본관) */
#else
    {_S("Reverse"), _S("Keep")},
#endif,
#else
    {_S("Reverce"), _S("Caverns")},
    {_S("Reverce"), _S("Colosseum")},
    {_S("Reverce"), _S("Keep")},
#endif
    #if defined(MOD_KO)
    {"\xF2\x4B\xF0\x52\xF0\x67\x20\xF0\xD8\xF0\x44\xF0\x9C\x00\x00\x00\x00\x00\x00\x00\x00", ""}, /* 44 흑마술 연구동 (한패 표기, 이전 강령 연구실) */
#else
    {_S("Necromancy"), _S("Laboratory")},
#endif,
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
#if defined(MOD_SATURN)
    #if defined(MOD_KO)
    {"\xF0\x76\xF0\x97\xF0\x7C\x20\xF0\xF3\xF2\xCD", ""}, /* 48 영혼의 감옥 */
#else
    {_S("Soul"), _S("Prison")},
#endif, // Saturn port: MOD_STAGE_RPRS
    #if defined(MOD_KO)
    {"\xF0\x85\xF2\xCD\x20\xF0\x81\xF1\xD6\x00\x00", ""}, /* 49 지옥 정원 */
#else
    {_S("Hell"), _S("Garden")},
#endif, // Saturn port: MOD_STAGE_RGDN
#else
    {_S(""), _S("")},
    {_S(""), _S("")},
#endif
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    #if defined(MOD_KO)
    {"\xF0\x63\x20\xF0\xDD\xF0\x44\x00\x00", ""}, /* 7/65 shared */
#else
    {_S("Entrance"), _S("")},
#endif,
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
    {_S(""), _S("")},
};

void SetupFileChoose(void) {
    g_MainMenuCursor = 1;
    g_InputCursorPos = 0;
    D_801BC3E0 = 0;
    D_801D6B04 = 0;
    g_MemCardSelectorX = 0;
    g_MemCardSelectorY = 0;
}

static void func_801AC084(s32 arg0, s32 ypos) {
    Primitive* prim;
    s32 i;

    prim = &g_PrimBuf[D_801BAF18[arg0][0]];
    for (i = 0; i < 8; i++, prim = prim->next) {
        prim->x0 = g_MenuUnk084X[i] + 0x68;
        prim->y0 = g_MenuUnk084Y[i] + 0x58 + ypos;
        prim->u0 = g_MenuUnk084U0[i];
        prim->v0 = g_MenuUnk084V0[i];
        prim->u1 = g_MenuUnk084U1[i];
        prim->v1 = g_MenuUnk084V1[i];
        prim->tpage = 0xC;
        prim->clut = 0x200;
        prim->priority = 0x11;
        prim->drawMode = DRAW_HIDE;
    }
}

#if defined(MOD_ULTIMATE_MENU)
#include "../../../mods/sotn-ultimate/src/menu/2C048_mod_sound_button.h"
#else
#define MOD_MENU_BTN_X(i) ((i) * 0x40 - 0x20)
#define MOD_MENU_BTN_Y(i) ((i) * 0x28)
#endif

// Creates the buttons, displays, etc on main menu
void InitMainMenuUI(void) {
    Primitive* prim;
    s32 i;

#if defined(MOD_SATURN)
    ModSoundBtnUpload();
    ModDramaBtnUpload();
#endif

    // Decoration at the top left for "Select Your Destiny"
    D_801BAF18[GFX_UNK_0][0] = g_api.AllocPrimitives(PRIM_GT4, 1);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_0][0]];
    SetTexturedPrimRect(prim, 0xD8, 0x10, 0x90, 0x48, 0, 0);
    func_801B1D88(prim);
    prim->tpage = 0xD;
    prim->clut = 0x201;
    prim->priority = 0x10;
    prim->drawMode = DRAW_DEFAULT;

    for (i = 0; i < NUM_MENU_OPTIONS; i++) {
        D_801BAF18[i + 1][0] = g_api.AllocPrimitives(PRIM_GT4, 1);
        prim = &g_PrimBuf[D_801BAF18[i + 1][0]];
        SetTexturedPrimRect(prim, MOD_MENU_BTN_X(i), MOD_MENU_BTN_Y(i), 0x7F,
                            0x1F, g_MenuHeadGfxU[i], g_MenuHeadGfxV[i]);
        func_801B1D88(prim);
        prim->tpage = 0xC;
        prim->clut = 0x200;
        prim->priority = 0x10;
        if (i == 0) {
            prim->drawMode = DRAW_HIDE;
        } else {
            prim->drawMode = DRAW_COLORS;
        }
    }

    D_801BAF18[GFX_UNK_6][0] = g_api.AllocPrimitives(PRIM_GT4, 4);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_6][0]];
    SetTexturedPrimRect(prim, 0x20, 0xB0, 0x10, 0x10, 0xA0, 0x50);
    func_801B1D88(prim);
    prim->tpage = 0xC;
    prim->clut = 0x200;
    prim->priority = 0x10;
    prim->drawMode = DRAW_DEFAULT;

    prim = prim->next;
    SetTexturedPrimRect(prim, 0x20, 0xB0, 0x10, 0x10, 0xA0, SEL_DECIDEV);
    func_801B1D88(prim);
    prim->tpage = 0xC;
    prim->clut = 0x200;
    prim->priority = 0x10;
    prim->drawMode = DRAW_DEFAULT;

    prim = prim->next;
    SetTexturedPrimRect(prim, 0x20, 0xB0, 0x10, 0x10, 0xA0, SEL_CANCELV);
    func_801B1D88(prim);
    prim->tpage = 0xC;
    prim->clut = SEL_CANCELCLUT;
    prim->priority = 0x10;
    prim->drawMode = DRAW_DEFAULT;

    prim = prim->next;
    SetTexturedPrimRect(prim, 0x20, 0xB0, 0x10, 0x10, 0xA0, 0x80);
    func_801B1D88(prim);
    prim->tpage = 0xC;
    prim->clut = 0x200;
    prim->priority = 0x10;
    prim->drawMode = DRAW_DEFAULT;

    D_801BAF18[GFX_UNK_7][0] = g_api.AllocPrimitives(PRIM_GT4, 1);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_7][0]];
    SetTexturedPrimRect(prim, 0x70, 0x30, 0xA0, 0x30, 0, 0x50);
    func_801B1D88(prim);
    prim->tpage = 0xC;
    prim->clut = 0x202;
    prim->priority = 0x10;
    prim->drawMode = DRAW_HIDE;

    D_801BAF18[GFX_UNK_8][0] = g_api.AllocPrimitives(PRIM_GT4, 1);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_8][0]];
    SetTexturedPrimRect(prim, 0x80, 0x48, 0xF, 0xF, 0xF0, 0xF0);
    func_801B1D88(prim);
    prim->tpage = 0xC;
    prim->clut = 0x200;
    prim->priority = 0x30;
    prim->drawMode = DRAW_HIDE;

    D_801BAF18[GFX_UNK_9][0] = g_api.AllocPrimitives(PRIM_SPRT, 3);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_9][0]];
    for (i = 0; i < 3; i++, prim = prim->next) {
        prim->x0 = (i * 0x80) + 0x30;
        prim->y0 = 0x60;
        prim->u0 = i * 0x80;
        prim->v0 = 0x78;
        prim->u1 = 0x80;
        prim->v1 = 0x78;
        if (i == 2) {
            prim->u0 = 0xE0;
            prim->v0 = 0;
            prim->u1 = 0x20;
        }
        prim->tpage = 0xD;
        prim->clut = 0x202;
        prim->priority = 0x10;
        prim->drawMode = DRAW_HIDE;
    }

    D_801BAF18[GFX_UNK_10][0] = g_api.AllocPrimitives(PRIM_GT4, 1);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_10][0]];
    SetTexturedPrimRect(prim, 0x40, 0x68, 0x20, 0x20, 0, 0x48);
    func_801B1D88(prim);
    prim->tpage = 0xD;
    prim->clut = 0x204;
    prim->priority = 0x30;
    prim->drawMode = DRAW_HIDE;

    D_801BAF18[GFX_WND_SAVE_SUMMARY][0] = g_api.AllocPrimitives(PRIM_GT4, 1);
    prim = &g_PrimBuf[D_801BAF18[GFX_WND_SAVE_SUMMARY][0]];
    SetTexturedPrimRect(prim, 0x68, 0x58, 0xB0, 0x50, 0, 0);
    func_801B1D88(prim);
    prim->tpage = 0xC;
    prim->clut = 0x202;
    prim->priority = 0x10;
    prim->drawMode = DRAW_HIDE;

    D_801BAF18[GFX_UNK_12][0] = g_api.AllocPrimitives(PRIM_SPRT, 8);
    func_801AC084(0xC, 0);

    D_801BAF18[GFX_WND_CARD_1][0] = g_api.AllocPrimitives(PRIM_SPRT, 1);
    prim = &g_PrimBuf[D_801BAF18[GFX_WND_CARD_1][0]];
    prim->x0 = 0x18;
    prim->y0 = 0x38;
    prim->u0 = 0xB0;
    prim->v0 = 0;
    prim->u1 = 0x50;
    prim->v1 = 0x78;
    func_801B1D88(prim);
    prim->tpage = 0xC;
    prim->clut = 0x202;
    prim->priority = 0x10;
    prim->drawMode = DRAW_HIDE;

    D_801BAF18[GFX_WND_CARD_2][0] = g_api.AllocPrimitives(PRIM_SPRT, 1);
    prim = &g_PrimBuf[D_801BAF18[GFX_WND_CARD_2][0]];
    prim->x0 = 0x118;
    prim->y0 = 0x38;
    prim->u0 = 0x90;
    prim->v0 = 0;
    prim->u1 = 0x50;
    prim->v1 = 0x78;
    func_801B1D88(prim);
    prim->tpage = 0xD;
    prim->clut = 0x202;
    prim->priority = 0x10;
    prim->drawMode = DRAW_HIDE;

    D_801BAF18[GFX_UNK_15][0] = g_api.AllocPrimitives(PRIM_SPRT, 1);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_15][0]];
    prim->x0 = 0x94;
    prim->y0 = 0x10;
    prim->u0 = 0;
    prim->v0 = 0x40;
    prim->u1 = 0xD0;
    prim->v1 = 0x20;
    func_801B1D88(prim);
    prim->tpage = 6;
    prim->clut = PAL_UNK_1A1;
    prim->priority = 0x10;
    prim->drawMode = DRAW_HIDE;

    D_801BAF18[GFX_UNK_16][0] = g_api.AllocPrimitives(PRIM_SPRT, 1);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_16][0]];
    prim->x0 = 0xA0;
    prim->y0 = 0x60;
    prim->u0 = 0;
    prim->v0 = 0x60;
    prim->u1 = 0x70;
    prim->v1 = 0x30;
    func_801B1D88(prim);
    prim->tpage = 6;
    prim->clut = PAL_UNK_1A1;
    prim->priority = 0x20;
    prim->drawMode = DRAW_HIDE;

    D_801BAF18[GFX_UNK_17][0] = g_api.AllocPrimitives(PRIM_SPRT, 1);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_17][0]];
    prim->x0 = 0x60;
    prim->y0 = 0xC0;
    prim->u0 = 0;
    prim->v0 = 0x90;
    prim->u1 = 1;
    prim->v1 = 0x10;
    func_801B1D88(prim);
    prim->tpage = 6;
    prim->clut = PAL_UNK_1A1;
    prim->priority = 0x20;
    prim->drawMode = DRAW_DEFAULT;

    D_801BAF18[GFX_UNK_18][0] = g_api.AllocPrimitives(PRIM_GT4, 2);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_18][0]];
    SetTexturedPrimRect(prim, 0x68, 0x30, 0xB0, 0x50, 0, 0);
    func_801B1D88(prim);
    prim->tpage = 0xC;
    prim->clut = 0x202;
    prim->priority = 0x10;
    prim->drawMode = DRAW_HIDE;
    prim = prim->next;
    SetTexturedPrimRect(prim, 0xA8, 0x80, 0x30, 0x10, 0xB0, 0x80);
    func_801B1D88(prim);
    prim->tpage = 0xC;
    prim->clut = 0x200;
    prim->priority = 0x10;
    prim->drawMode = DRAW_HIDE;

    D_801BAF18[GFX_UNK_19][0] = g_api.AllocPrimitives(PRIM_SPRT, 8);
    func_801AC084(0x13, -0x28);

    D_801BAF18[GFX_UNK_20][0] = g_api.AllocPrimitives(PRIM_SPRT, 1);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_20][0]];
    prim->x0 = 0xA0;
    prim->y0 = 0x38;
    prim->u0 = 0;
    prim->v0 = 0xA0;
    prim->u1 = 0x70;
    prim->v1 = 0x30;
    func_801B1D88(prim);
    prim->tpage = 6;
    prim->clut = PAL_UNK_1A1;
    prim->priority = 0x20;
    prim->drawMode = DRAW_HIDE;
}

static void func_801ACBE4(s32 gfxId, u16 drawMode) {
    Primitive* prim;

    prim = &g_PrimBuf[D_801BAF18[gfxId][0]];
    while (prim != NULL) {
        prim->drawMode = drawMode;
        prim = prim->next;
    }
}

void MenuHideAllGfx(void) {
    s32 i;

    for (i = 0; i < NUM_GFX; i++) {
        func_801ACBE4(i, 8);
    }
}

void InitMainMenuBackgroundAndFadeMask(void) {
    Primitive* prim;
    s32 i;

    // Seems to be the background on the main menu (dark blue/grey door thing?)
    D_801BAFC0 = g_api.AllocPrimitives(PRIM_GT4, 3);
    prim = &g_PrimBuf[D_801BAFC0];
    for (i = 0; i < 3; i++, prim = prim->next) {
        SetTexturedPrimRect(prim, i * 0x80, 0, 0x80, 0xF0, 0, 0);
        func_801B1D88(prim);
        prim->tpage = i + 0x89;
        prim->clut = 0x210;
        prim->drawMode = DRAW_COLORS;
    }

    // When the main menu is loaded in, it is covered by a black mask that fades
    // away. This part creates the initial mask, then MainMenuFadeIn deals
    // with fading it out.
    MainMenuMaskPrimIndex = g_api.AllocPrimitives(PRIM_TILE, 2);
    prim = &g_PrimBuf[MainMenuMaskPrimIndex];
    for (i = 0; prim != NULL; i++, prim = prim->next) {
        prim->x0 = (i & 1) * 0xC0;
        prim->u0 = 0xC0;
        prim->v0 = 0xF0;
        SetPrimGrey(prim, 0xFF);
        prim->priority = 0x1FD;
        prim->drawMode = DRAW_UNK_40 | DRAW_TPAGE | DRAW_TRANSP;
    }
}

// On title screen, you press START. Then the screen goes black,
// and fades in gradually. This function handles that fade. Returns false
// as long as the fade-in is ongoing.
// Odd that this function can return false but not true. Perhaps there is a
// default-true behavior when a bool function exits without an explicit value?
// PSP version returns true at the end. True means the fade is complete.
bool MainMenuFadeIn(void) {
    Primitive* prim;
    s32 greyLevel;

    prim = &g_PrimBuf[MainMenuMaskPrimIndex];
    greyLevel = prim->r0;
    greyLevel -= 0x10;
    if (greyLevel < 0) {
        greyLevel = 0;
    }

    SetPrimGrey(prim, greyLevel);
    prim = prim->next;
    SetPrimGrey(prim, greyLevel);

    if (greyLevel != 0) {
        return false;
    }
    // Once the greyLevel is exhaused, we hide them.
    prim = &g_PrimBuf[MainMenuMaskPrimIndex];
    prim->drawMode = DRAW_HIDE;
    prim = prim->next;
    prim->drawMode = DRAW_HIDE;
#ifdef FIX_UB
    // BUG! return value 1 is never returned
    return true;
#endif
}

s32 func_801ACEC0(void) {
    Primitive* prim;
    s32 greyLevel;

    prim = &g_PrimBuf[MainMenuMaskPrimIndex];
    prim->drawMode = DRAW_UNK_40 | DRAW_TPAGE | DRAW_TRANSP;
    greyLevel = prim->r0;
    greyLevel += 0x10;
    if (greyLevel > 0xFF) {
        greyLevel = 0xFF;
    }

    SetPrimGrey(prim, greyLevel);
    prim = prim->next;
    prim->drawMode = DRAW_UNK_40 | DRAW_TPAGE | DRAW_TRANSP;
    SetPrimGrey(prim, greyLevel);

    if (g_api.func_80131F68()) {
        return 0;
    } else {
        return greyLevel == 0xFF;
    }
}

void func_801ACF7C(void) {
    func_801B1ED0();
#ifndef VERSION_PSP
    func_801B25D4("…選択決定取消入力未初期化確認", 0);
    func_801B25D4("はいえ不良", 1);
#endif
}

char* D_801803A8[10] = {
#if defined(MOD_KO)
    _S(""), _S(""), _S(""), _S(""), _S("Not for-"),
#else
    _S("Select"), _S("Decide"), _S("Cancel"),  _S("Input"), _S("Not for-"),
#endif
    _S("Yes"),    _S("No"),     _S("Confirm"), _S("Error"), _S("matted"),
};

#if defined(MOD_PTBR)
#define MOD_TIP_X 0x18
#define MOD_TIP_TX 0x2C
#define MOD_TIP_IN_X 0x116
#define MOD_TIP_IN_TX 0x128
#else
#define MOD_TIP_X 0x20
#define MOD_TIP_TX 0x34
#define MOD_TIP_IN_X 0x120
#define MOD_TIP_IN_TX 0x134
#endif
#define STR_SELECT D_801803A8[0]
#define STR_DECIDE D_801803A8[1]
#define STR_CANCEL D_801803A8[2]
#define STR_INPUT D_801803A8[3]
#define STR_NOTFOR D_801803A8[4]
#define STR_YES D_801803A8[5]
#define STR_NO D_801803A8[6]
#define STR_CONFIRM D_801803A8[7]
#define STR_ERROR D_801803A8[8]
#define STR_MATTED D_801803A8[9]

#if defined(MOD_KO)
/* Menu legends via the top-message band path (func_801B2608 ->
 * func_801B2108 12px upload to VRAM (0x180,id*16), one dynamic SPRT
 * per row). Hook path (dead) removed. */
/* Words = MGalmuri12 v12c codes. Bands = free ids 2,3,13,14.
 * SPRT y = button-icon top (EN 8px-text y minus 4): icon 16px and SPRT
 * 16px share the same center. */
static const char SelLegW[] = "\xF1\xAA\xF2\x1C\x00"
                              "\xF3\x01\xF0\x81\x00"
                              "\xF3\x29\xF0\x65\x00"
                              "\xF0\xDD\xF0\xA4\x00"
                              /* Sound Mode hint words (재생, 뒤로) */
                              "\xF1\xE3\xF1\xA9\x00"
                              "\xF0\xC3\xF0\xA0\x00";
static const u8 SelLegFile[] = {0, 13, 5, 14, 10, 2};
static const u8 SelLegSnd[] = {20, 14, 25, 2};
static const u8 SelLegName[] = {0, 13, 15, 14, 10, 2, 5, 3};
static void SelLegRows(s32 x, s32 y, const u8 *t, s32 n) {
    /* diet: x/y live in the incoming arg slots (reloaded each
     * row), id is re-read from the table after the first call (jal
     * clobbers v0). Same rows/ids/positions as before. */
    s32 wi;
    while (n > 0) {
        wi = *t++;
        func_801B2608((char *)&SelLegW[wi], *t);
        func_801B27A8(x, y, 0x30, 0x10, 0, (*t++) * 16, 0x1A1, 6, 1, 0x80);
        y += 16;
        n--;
    }
}
#endif

#if defined(MOD_KO)
/* save name as ASCII -> func_801B2108 (engtab MGalmuri12 cells, proportional advance)
 * instead of fullwidth SJIS -> BIOS Kanji ROM (grey, 12 px fixed). Same 33 entries / order. */
static char* D_801803D0[] = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K",
    "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V",
    "W", "X", "Y", "Z", "&", "!", "-",  ".", "'",  "?", " ",
};
#else
static char* D_801803D0[] = {
    "Ａ", "Ｂ", "Ｃ", "Ｄ", "Ｅ", "Ｆ", "Ｇ", "Ｈ", "Ｉ", "Ｊ", "Ｋ",
    "Ｌ", "Ｍ", "Ｎ", "Ｏ", "Ｐ", "Ｑ", "Ｒ", "Ｓ", "Ｔ", "Ｕ", "Ｖ",
    "Ｗ", "Ｘ", "Ｙ", "Ｚ", "＆", "！", "−",  "．", "’",  "？", "　",
};
#endif

void func_801ACFBC(s32 port, s32 slot, s32 textId) {
    char playerName[0x20];
    s32 i;

    playerName[0] = '\0';
    for (i = 0; i < 8; i++) {
        s32 charIndex = func_801B2984(g_SaveSummary[port].name[slot][i]);
        strcat(playerName, D_801803D0[charIndex]);
    }
    func_801B263C(playerName, textId);
    func_801B25D4("　", textId + 1);
#if defined(MOD_SATURN)
    // mod: Maria saves (Saturn or PSP) are labelled "Maria", not "Richter"
    if (g_SaveSummary[port].character[slot] == PLAYER_MARIA) {
        func_801B2608("Maria", textId + 2);
    } else
#endif
        if (g_SaveSummary[port].character[slot]) {
        func_801B2608("Richter", textId + 2);
    } else if (g_SaveSummary[port].kind[slot] & SAVE_FLAG_CLEAR) {
        func_801B2608("Clear", textId + 2);
    } else if (g_SaveSummary[port].kind[slot] & SAVE_FLAG_REPLAY) {
        func_801B2608("Replay", textId + 2);
    } else {
        func_801B2608("　", textId + 2);
    }
}

void PrintFileSelectPlaceName(s32 port, s32 slot, s32 y) {
    volatile u32 pad; // !FAKE:

    s32 stage = g_SaveSummary[port].stage[slot];
#if defined(MOD_KO)
    /* ko_place: single-line 12x16 textId path, same renderer as the
     * file-select top message (func_801B2608 -> func_801B2108, 12px).
     * Slot rides the existing name-block rows: id 7 (summary card,
     * y+0x70) and id 11 (detail box, y == 0x48). No new prims. */
    func_801B2608(D_80180128[stage].line1, (y == 0x48) ? 11 : 7);
#else
    DrawImages8x8(D_80180128[stage].line1, 0xA0, y, 1);
    DrawImages8x8(D_80180128[stage].line2, 0xA0, y + 8, 1);
#endif
}

void func_801AD1D0(void) {
    DrawImages8x8(STR_YES, MOD_TIP_TX, 0xC4, 1);
    DrawImages8x8(STR_NO, MOD_TIP_TX, 0xD4, 1);
}

void func_801AD218(void) {
    DrawImages8x8(STR_NO, MOD_TIP_TX, 0xC4, 1);
    DrawImages8x8(STR_YES, MOD_TIP_TX, 0xD4, 1);
}

void DrawNavigationTips(NavigationTips mode) {
    Primitive* prim;

    func_801ACBE4(GFX_UNK_6, 8);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_6][0]];

    switch (mode) {
    case Tips_Generic:
        SetPrimRect(prim, MOD_TIP_X, 0xB0, 0x10, 0x10);
        prim->drawMode = DRAW_DEFAULT;
        prim = prim->next;
        SetPrimRect(prim, MOD_TIP_X, 0xC0, 0x10, 0x10);
        prim->drawMode = DRAW_DEFAULT;
        prim = prim->next;
        SetPrimRect(prim, MOD_TIP_X, 0xD0, 0x10, 0x10);
        prim->drawMode = DRAW_DEFAULT;
        break;

    case Tips_Input:
        SetPrimRect(prim, MOD_TIP_IN_X, 0x20, 0x10, 0x10);
        prim->drawMode = DRAW_DEFAULT;
        prim = prim->next;
        SetPrimRect(prim, MOD_TIP_IN_X, 0x30, 0x10, 0x10);
        prim->drawMode = DRAW_DEFAULT;
        prim = prim->next;
        SetPrimRect(prim, MOD_TIP_IN_X, 0x40, 0x10, 0x10);
        prim->drawMode = DRAW_DEFAULT;
        prim = prim->next;
        SetPrimRect(prim, MOD_TIP_IN_X, 0x50, 0x10, 0x10);
        prim->drawMode = DRAW_DEFAULT;
        break;

    case Tips_YesNo:
    case Tips_NoYes:
        prim = prim->next;
        SetPrimRect(prim, MOD_TIP_X, 0xC0, 0x10, 0x10);
        prim->drawMode = DRAW_DEFAULT;
        prim = prim->next;
        SetPrimRect(prim, MOD_TIP_X, 0xD0, 0x10, 0x10);
        prim->drawMode = DRAW_DEFAULT;
        if (mode == Tips_YesNo) {
            func_801AD1D0();
        } else {
            func_801AD218();
        }
        break;

    case Tips_Confirm:
        prim = prim->next;
        SetPrimRect(prim, MOD_TIP_X, 0xC0, 0x10, 0x10);
        prim->drawMode = DRAW_DEFAULT;
        DrawImages8x8(STR_CONFIRM, MOD_TIP_TX, 0xC4, 1);
        break;

    case Tips_MenuNavigation:
        SetPrimRect(prim, MOD_TIP_X, 0xB8, 0x10, 0x10);
        prim->drawMode = DRAW_DEFAULT;
        prim = prim->next;
        SetPrimRect(prim, MOD_TIP_X, 0xC8, 0x10, 0x10);
        prim->drawMode = DRAW_DEFAULT;
        break;
    }
}

#if defined(MOD_ULTIMATE_MENU)
#include "../../../mods/sotn-ultimate/src/menu/2C048_mod_sound_menu.h"
#endif

void func_801AD490(void) {
    Primitive* prim;
    s32 i;

    DrawNavigationTips(Tips_MenuNavigation);
#if defined(MOD_KO)
    SelLegRows(0x34, 0xB8, SelLegFile, 2);
#else
    DrawImages8x8(STR_SELECT, MOD_TIP_TX, 0xBC, 1);
    DrawImages8x8(STR_DECIDE, MOD_TIP_TX, 0xCC, 1);
#endif
    DrawString16x16("select", 0xF0, 0x20, 1);
    DrawString16x16("your", 0x100, 0x30, 1);
    DrawString16x16("destiny", 0xE8, 0x40, 1);
#if defined(MOD_SATURN)
    func_801B27A8(
        MOD_MENU_BTN_X(MOD_MENU_SOUND), MOD_MENU_BTN_Y(MOD_MENU_SOUND),
        MOD_SOUND_BTN_W, MOD_SOUND_BTN_H, 0, MOD_SOUND_BTN_V,
        g_MainMenuCursor == MOD_MENU_SOUND ? 0x203 : 0x200, MOD_SOUND_BTN_TPAGE,
        1, 0x80);
    if (MOD_DRAMA_ON()) {
        func_801B27A8(
            MOD_MENU_BTN_X(MOD_MENU_DRAMA), MOD_MENU_BTN_Y(MOD_MENU_DRAMA),
            MOD_DRAMA_BTN_W, MOD_DRAMA_BTN_H, 0, 0,
            g_MainMenuCursor == MOD_MENU_DRAMA ? 0x203 : 0x200, MOD_DRAMA_BTN_TPAGE,
            1, 0x80);
    }
#endif

    for (i = 0; i < NUM_MENU_OPTIONS; i++) {
        prim = &g_PrimBuf[D_801BAF18[i + 1][0]];
        if (i == g_MainMenuCursor) {
            prim->clut = 0x203;
        } else {
            prim->clut = 0x200;
        }
    }
}

const char* D_80180454[] = {
    "−＊＊＊＊＊−",  
#if defined(MOD_KO)
    "− \xF1\xA7 \xF0\xBC\xF1\xDB −"
#else
    "− New Game −"
#endif
,   
#if defined(MOD_KO)
    "− \xF0\x7D\xF0\x4F \xF0\xA1\xF0\xF9 −"
#else
    "− Change Name −"
#endif
,
    
#if defined(MOD_KO)
    "− \xF0\x92\xF0\x7F \xF0\xCD\xF0\xAB −"
#else
    "− Copy File −"
#endif
, 
#if defined(MOD_KO)
    "− \xF0\x92\xF0\x7F \xF0\x85\xF0\xDA\xF0\x46 −"
#else
    "− Erase File −"
#endif
,
};
#if defined(MOD_SATURN)
// (not in D_80180454: SEL's .data must not move, psp_script.py patches it)
static const char* const g_ModMenuSoundHelp MOD_RO = "− Sound Mode −";
static const char* const g_ModMenuDramaHelp MOD_RO = "− Drama Mode −";
#endif

#if defined(MOD_SATURN)
#define MOD_MENU_LAST (MOD_DRAMA_ON() ? MOD_MENU_DRAMA : MOD_MENU_SOUND) // 095_36: the flag byte (2C048_mod_sound_button.h)
#else
#define MOD_MENU_LAST 4
#endif
void func_801AD590(void) {
    if (g_pads[0].tapped & (PAD_RIGHT | PAD_DOWN)) {
        g_api.PlaySfx(SFX_UI_MP_FULL); // MP sfx also used for Main Menu Select
        g_MainMenuCursor++;
        if (g_MainMenuCursor == MOD_MENU_LAST + 1) {
            g_MainMenuCursor = 1;
        }
    }
    if (g_pads[0].tapped & (PAD_LEFT | PAD_UP)) {
        g_api.PlaySfx(SFX_UI_MP_FULL);
        g_MainMenuCursor--;
        if (g_MainMenuCursor == 0) {
            g_MainMenuCursor = MOD_MENU_LAST;
        }
    }
#if defined(MOD_SATURN)
    if (g_MainMenuCursor == MOD_MENU_SOUND) {
        func_801B2608((char*)g_ModMenuSoundHelp, 9);
        return;
    }
    if (g_MainMenuCursor == MOD_MENU_DRAMA) {
        func_801B2608((char*)g_ModMenuDramaHelp, 9);
        return;
    }
#endif
    func_801B2608(D_80180454[g_MainMenuCursor], 9);
}

static char* D_80180468 = "richter ";

void func_801AD66C(void) {
    s32 i;
    s32 nSpaces;
    char* strPtr;

    // check if the name only contain spaces
    nSpaces = 0;
    for (i = 0; i < 8; i++) {
        g_Status.saveName[i] = g_InputSaveName[i];
        if (g_InputSaveName[i] == ' ') {
            nSpaces++;
        }
    }

    // if it only contain spaces, set a default name
    if (nSpaces == 8) {
        strcpy(g_Status.saveName, "alucard ");
    }

    g_Status.saveName[8] = 0;
    g_Status.saveName[9] = 0;

    // check if the name is Richter
    strPtr = D_80180468;
    for (i = 0; i < 8; i++) {
        if (g_Status.saveName[i] != *strPtr++) {
            break;
        }
    }

    if (g_GameClearFlag != SAVE_FLAG_NORMAL) {
        g_GameClearFlag = SAVE_FLAG_REPLAY;
    }

    // play as Richter only if the game was previously cleared
    if (i == 8 && g_GameClearFlag != SAVE_FLAG_NORMAL) {
        g_PlayableCharacter = PLAYER_RICHTER;
    } else {
        g_PlayableCharacter = PLAYER_ALUCARD;
    }
}

#if defined(MOD_ULTIMATE_MENU)
#include "../../../mods/sotn-ultimate/src/menu/2C048_mod_player_select.h"
#endif

#if defined(MOD_KO)
#define KO_SAVE(en, ko) ko
#else
#define KO_SAVE(en, ko) en
#endif

void func_801AD78C(void) {
    Primitive* prim;

#if defined(MOD_KO)
    SelLegRows(0x134, 0x20, SelLegName, 4);
#else
    DrawImages8x8(STR_SELECT, MOD_TIP_IN_TX, 0x24, 1);
    DrawImages8x8(STR_INPUT, MOD_TIP_IN_TX, 0x34, 1);
    DrawImages8x8(STR_CANCEL, MOD_TIP_IN_TX, 0x44, 1);
    DrawImages8x8(STR_DECIDE, MOD_TIP_IN_TX, 0x54, 1);
#endif
    DrawString16x16("a b c d e f g h", 0x48, 0x70, 1);
    DrawString16x16("i j k l m n o p", 0x48, 0x88, 1);
    DrawString16x16("q r s t u v w x", 0x48, 0xA0, 1);
    DrawString16x16("y z & ! - . '  ", 0x48, 0xB8, 1);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_8][0]];
    SetTexturedPrimRect(
        prim, g_InputCursorPos * 0x10 + 0x80, 0x48, 0xF, 0xF, 0xF0, 0xF0);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_10][0]];
    SetTexturedPrimRect(prim, (D_801BC3E0 & 7) * 0x20 + 0x40,
                        (D_801BC3E0 & 0x18) * 3 + 0x68, 0x20, 0x20, 0, 0x48);
    if (g_Timer & 8) {
        func_801ACBE4(GFX_UNK_8, 0);
    } else {
        func_801ACBE4(GFX_UNK_8, 8);
    }
    DrawString16x16(g_InputSaveName, 0x80, 0x48, 1);
}

void UpdateNameEntry(void) {
    if (g_pads[0].repeat & PAD_RIGHT) {
        g_api.PlaySfx(SFX_UI_MOVE);
        D_801BC3E0 = (D_801BC3E0 & 0x18) | ((D_801BC3E0 + 1) & 7);
    }

    if (g_pads[0].repeat & PAD_DOWN) {
        g_api.PlaySfx(SFX_UI_MOVE);
        D_801BC3E0 = ((D_801BC3E0 + 8) & 0x18) | (D_801BC3E0 & 7);
    }

    if (g_pads[0].repeat & PAD_LEFT) {
        g_api.PlaySfx(SFX_UI_MOVE);
        D_801BC3E0 = (D_801BC3E0 & 0x18) | ((D_801BC3E0 - 1) & 7);
    }

    if (g_pads[0].repeat & PAD_UP) {
        g_api.PlaySfx(SFX_UI_MOVE);
        D_801BC3E0 = ((D_801BC3E0 - 8) & 0x18) | (D_801BC3E0 & 7);
    }

    if (g_pads[0].tapped & (PAD_R1 | PAD_R2)) {
        g_api.PlaySfx(SFX_UI_MP_FULL);
        g_InputCursorPos++;
        if (g_InputCursorPos == 8) {
            g_InputCursorPos = 0;
        }
    }

    if (g_pads[0].tapped & (PAD_L1 | PAD_L2)) {
        g_api.PlaySfx(SFX_UI_MP_FULL);
        g_InputCursorPos--;
        if (g_InputCursorPos == -1) {
            g_InputCursorPos = 7;
        }
    }

    if (g_pads[0].tapped & SEL_CONFIRM) { // Input Character
        g_api.PlaySfx(SFX_UI_NAME_ENTRY);
        g_InputSaveName[g_InputCursorPos] = g_AsciiSet[D_801BC3E0];
        g_InputCursorPos++;
        if (g_InputCursorPos == 8) {
            g_InputCursorPos = 0;
        }
    }

    if (g_pads[0].tapped & SEL_CANCEL) { // Backspace
        g_InputCursorPos--;
        if (g_InputCursorPos == -1) {
            g_InputCursorPos = 7;
        }
        g_InputSaveName[g_InputCursorPos] = ' ';
    }
}

void UpdateFileSelect(void) {
    if (g_SaveSummary[0].padding >= 0 || g_SaveSummary[1].padding >= 0) {
        if (g_pads[0].repeat & PAD_RIGHT) { // move selector to the right
            g_api.PlaySfx(SFX_UI_MOVE);
            g_MemCardSelectorX = (g_MemCardSelectorX + 1) % 6;
        }

        if (g_pads[0].repeat & PAD_DOWN) { // move selector down
            g_api.PlaySfx(SFX_UI_MOVE);
            g_MemCardSelectorY = (g_MemCardSelectorY + 4) % 5;
        }

        if (g_pads[0].repeat & PAD_LEFT) { // move selector to the left
            g_api.PlaySfx(SFX_UI_MOVE);
            g_MemCardSelectorX = (g_MemCardSelectorX + 5) % 6;
        }

        if (g_pads[0].repeat & PAD_UP) { // move selector up
            g_api.PlaySfx(SFX_UI_MOVE);
            g_MemCardSelectorY = (g_MemCardSelectorY + 1) % 5;
        }

        if (g_SaveSummary[0].padding > 0 && g_SaveSummary[1].padding > 0 &&
            (g_pads[0].tapped & (PAD_L2 + PAD_R2 + PAD_L1 + PAD_R1))) {
            g_api.PlaySfx(SFX_UI_MP_FULL);
            g_MemCardSelectorX = (g_MemCardSelectorX + 3) % 6;
        }

        if (g_SaveSummary[0].padding < 0) {
            g_MemCardSelectorX = (g_MemCardSelectorX % 3) + 3;
        }

        if (g_SaveSummary[1].padding < 0) {
            g_MemCardSelectorX = g_MemCardSelectorX % 3;
        }

        D_801D6B04 = (g_MemCardSelectorX % 3) + (g_MemCardSelectorY * 3) +
                     (g_MemCardSelectorX / 3) * 0xF;
    }
}

void func_801ADF94(s32 flags, s32 yOffset) {
    s32 slot;
    Primitive* prim;
    s32 percLo;
    s32 hideButtons;
    s32 icon;
    s32 x;
    s32 y;
    s32 port;
    s32 saveDescriptorString;
    s32 percHi;
    s32 color;
    s32 tge;
    s32 i;

    saveDescriptorString = flags & 0x7F;
    hideButtons = flags & 0x80;
    port = D_801D6B04 / 15;
    slot = D_801D6B04 % 15;
    y = yOffset * 0x38;
    prim = &g_PrimBuf[D_801BAF18[GFX_WND_SAVE_SUMMARY][0]];
    SetTexturedPrimRect(prim, 0x68, y + 0x58, 0xB0, 0x50, 0, 0);
    if (saveDescriptorString > 0) {
        if (g_SaveSummary[port].padding == -3 || saveDescriptorString == 3) {
            func_801ACBE4(GFX_UNK_12, 8);
            func_801ACBE4(GFX_UNK_16, 8);
            DrawString16x16("new game", 128, 120 + y, 1);
        } else {
            icon = g_SaveSummary[port].icon[slot];
            if (icon >= 0) {
                func_801AC084(0xC, y);
                func_801ACBE4(GFX_UNK_12, 0);
                func_801ACBE4(GFX_UNK_16, 0);
                prim = &g_PrimBuf[D_801BAF18[GFX_UNK_16][0]];
                prim->y0 = y + 0x60;
                func_801ACFBC(port, slot, 6);
                PrintFileSelectPlaceName(port, slot, y + 0x70);
                func_801B2BD4(
                    g_SaveSummary[port].level[slot], 0x94, y + 0x90, 1);
                func_801B2BD4(
                    g_SaveSummary[port].playHours[slot], 0xC8, y + 0x90, 1);
                func_801B2C70(
                    g_SaveSummary[port].playMinutes[slot], 0xE0, y + 0x90, 1);
                func_801B2C70(
                    g_SaveSummary[port].playSeconds[slot], 0xF8, y + 0x90, 1);
                func_801B2BD4(
                    g_SaveSummary[port].gold[slot], 0xB4, y + 0x98, 1);
                percHi = g_SaveSummary[port].nRoomsExplored[slot];
                percHi = (percHi * 1000) / 942;
                percLo = percHi % 10;
                percHi = percHi / 10;
                func_801B2BD4(percHi, 0xE8, y + 0x98, 1);
                func_801B27A8(
                    0xF0, y + 0x9C, 8, 4, 0xE0, 0x8C, 0x200, 0xC, 1, 0x80);
                func_801B2BD4(percLo, 0xF8, y + 0x98, 1);
            } else {
                func_801ACBE4(GFX_UNK_12, 8);
                func_801ACBE4(GFX_UNK_16, 8);
                if (icon == -2) {
                    DrawString16x16("used", 0xA0, y + 0x78, 1);
                } else if (saveDescriptorString == 2) {
                    DrawString16x16("no data", 0x88, y + 0x78, 1);
                } else {
                    DrawString16x16("new game", 0x80, y + 0x78, 1);
                }
            }
        }
    }

    if (!hideButtons) {
#if defined(MOD_KO)
        SelLegRows(0x34, 0xB0, SelLegFile, 3);
#else
        DrawImages8x8(STR_SELECT, MOD_TIP_TX, 0xB4, 1);
        DrawImages8x8(STR_DECIDE, MOD_TIP_TX, 0xC4, 1);
        DrawImages8x8(STR_CANCEL, MOD_TIP_TX, 0xD4, 1);
#endif
    }

    for (port = 0; port < PORT_COUNT; port++) {
        switch (g_SaveSummary[port].padding) {
        case -1:
            DrawString16x16("no", (port * 0x100) + 0x30, 0x58, 1);
            DrawString16x16("card", (port * 0x100) + 0x20, 0x68, 1);
            break;
        case -2:
            DrawImages8x8(STR_NOTFOR, (port * 0x100) + 0x1E, 0x6C, 1);
            DrawImages8x8(STR_MATTED, (port * 0x100) + 0x1E, 0x74, 1);
            break;
        case -3:
            DrawImages8x8(STR_ERROR, (port * 0x100) + 0x36, 0x6C, 1);
            break;
        default:
            for (i = 0; i < BLOCK_PER_CARD; i++) {
                icon = g_SaveSummary[port].icon[i];
                x = 0x20 + (i % 3) * 0x18 + port * 0x100;
                y = 0x90 - (i / 3) * 0x10;
                if (((i % 3) + port * 3 == g_MemCardSelectorX) &&
                    saveDescriptorString != 3 &&
                    ((i / 3) == g_MemCardSelectorY) &&
                    saveDescriptorString > 0 && !D_801BAF10) {
                    tge = 0;

                    if (g_Timer & 0x10) {
                        color = g_Timer & 0xF;
                    } else {
                        color = 0xF - (g_Timer & 0xF);
                    }
                    color = color * 8 + 0x80;
                } else {
                    tge = 0;
                    color = 0x40;
                }
                if (icon == -2) {
                    func_801B27A8(
                        x, y, 0x10, 0x10, 0x90, 0x80, 0x200, 0xC, tge, color);
                }
                if (icon == -3) {
                    func_801B27A8(
                        x, y, 0x10, 0x10, 0x80, 0x80, 0x200, 0xC, tge, color);
                }
                if (icon >= 0) {
                    func_801B27A8(
                        x, y, 0x10, 0x10, icon * 0x10, (D_801BAF08 % 3) * 0x10,
                        icon + 0x220, 0x16, tge, color);
                }
            }
            break;
        }
    }
}

void func_801AE6D0(void) {
    Primitive* prim;
    s32 port;
    s32 slot;
    s32 percLo;
    s32 percHi;
    s32 icon;

    port = D_801BC3EC / 15;
    slot = D_801BC3EC % 15;
    icon = g_SaveSummary[port].icon[slot];
    func_801ACBE4(GFX_UNK_18, 0x11);
    prim = &g_PrimBuf[D_801BAF18[GFX_UNK_18][0]];
    prim = prim->next;
    SetTexturedPrimRect(
        prim, 0xA8, (D_801BAF08 % 3) + 0x7F, 0x30, 0x10, 0xB0, 0x80);
    prim->drawMode = DRAW_DEFAULT;
    func_801ACBE4(GFX_UNK_19, 0);
    func_801ACBE4(GFX_UNK_20, 0);
    func_801ACFBC(port, slot, 0xA);
    PrintFileSelectPlaceName(port, slot, 0x48);
    func_801B2BD4(g_SaveSummary[port].level[slot], 0x94, 0x68, 1);
    func_801B2BD4(g_SaveSummary[port].playHours[slot], 0xC8, 0x68, 1);
    func_801B2C70(g_SaveSummary[port].playMinutes[slot], 0xE0, 0x68, 1);
    func_801B2C70(g_SaveSummary[port].playSeconds[slot], 0xF8, 0x68, 1);
    func_801B2BD4(g_SaveSummary[port].gold[slot], 0xB4, 0x70, 1);
    percHi = g_SaveSummary[port].nRoomsExplored[slot];
    percHi = (percHi * 1000) / 942;
    percLo = percHi % 10;
    percHi = percHi / 10;
    func_801B2BD4(percHi, 0xE8, 0x70, 1);
    func_801B27A8(0xF0, 0x74, 8, 4, 0xE0, 0x8C, 0x200, 0xC, 1, 0x80);
    func_801B2BD4(percLo, 0xF8, 0x70, 1);
}

void func_801AE9A8(void) {
    Primitive* prim;
    s32 i;

    MenuHideAllGfx();
    func_801ACBE4(GFX_UNK_0, 0);

    for (i = 1; i < NUM_MENU_OPTIONS; i++) {
        func_801ACBE4(i + 1, 4);
        prim = &g_PrimBuf[D_801BAF18[i + 1][0]];
        SetTexturedPrimRect(prim, MOD_MENU_BTN_X(i), MOD_MENU_BTN_Y(i), 0x7F,
                            0x1F, g_MenuHeadGfxU[i], g_MenuHeadGfxV[i]);
    }

    DrawNavigationTips(Tips_Generic);
    func_801ACBE4(GFX_UNK_17, 0);
}

void func_801AEA8C(s32 arg0) {
    Primitive* prim;

    g_InputCursorPos = 0;
    D_801BC3E0 = 0;
    MenuHideAllGfx();
    func_801ACBE4(GFX_UNK_7, 0x11);
    func_801ACBE4(GFX_UNK_8, 0);
    func_801ACBE4(GFX_UNK_9, 0x11);
    func_801ACBE4(GFX_UNK_10, 0);

    if (arg0 == 0) {
        func_801ACBE4(GFX_UNK_1, 0);
        prim = &g_PrimBuf[D_801BAF18[GFX_UNK_1][0]];
        SetPrimRect(prim, 0x18, 0x18, 0x7F, 0x1F);
    } else {
        func_801ACBE4(GFX_FILE_DELETE, 0);
        prim = &g_PrimBuf[D_801BAF18[GFX_FILE_DELETE][0]];
        SetPrimRect(prim, 0x18, 0x18, 0x7F, 0x1F);
    }

    DrawNavigationTips(Tips_Input);
}

void SelectMainMenuOption(MainMenuCursor cursor) {
    Primitive* prim;
    s32 gfxId;

    g_SaveSummary[0].padding = 0;
    g_SaveSummary[1].padding = 0;
    MenuHideAllGfx();
    func_801ACBE4(GFX_WND_SAVE_SUMMARY, 0x11);
    func_801ACBE4(GFX_WND_CARD_1, 0x11);
    func_801ACBE4(GFX_WND_CARD_2, 0x11);
    func_801ACBE4(GFX_UNK_15, 0);

    switch (cursor) {
    case MAIN_MENU_CURSOR_FILE_SELECT:
        gfxId = GFX_FILE_SELECT;
        break;
    case MAIN_MENU_CURSOR_NAME_CHANGE:
        gfxId = GFX_NAME_CHANGE;
        break;
    case MAIN_MENU_CURSOR_FILE_COPY:
        gfxId = GFX_FILE_COPY;
        break;
    case MAIN_MENU_CURSOR_FILE_DELETE:
        gfxId = GFX_FILE_DELETE;
        break;
    }
    func_801ACBE4(gfxId, 0);

    // Relocate the graphics at the top-left of the screen
    prim = &g_PrimBuf[D_801BAF18[gfxId][0]];
    SetPrimRect(prim, 0x10, 0x10, 0x7F, 0x1F);
    prim->clut = 0x200;
}

void func_801AECA0(void) {
    s32 i;
    u8* ptr;

    for (i = 0; i < 0x10; i++) {
        ptr = g_saveIconTexture[i];
        LoadTPage((u_long*)ptr, 0, 0, (i * 4) + 0x180, 0x100, 0x10, 0x30);
    }

    g_api.func_800EA5E4(ANIMSET_OVL(4));
}

static void CheckIfMemcardsCanBeUsed(void) {
    s32 isCard0Full;
    s32 isCard1Full;
    s32 i;

    isCard0Full = true;
    isCard1Full = true;
    for (i = 0; i < BLOCK_PER_CARD; i++) {
        if (g_SaveSummary[0].icon[i] != -2) {
            isCard0Full = false;
        }
        if (g_SaveSummary[1].icon[i] != -2) {
            isCard1Full = false;
        }
    }

    D_801BAF10 = 0;
    if ((g_SaveSummary[0].padding < 0 || isCard0Full) &&
        (g_SaveSummary[1].padding < 0 || isCard1Full)) {
        D_801BAF10 = 1;
    }

    D_801BAF14 = 0;
    for (i = 0; i < BLOCK_PER_CARD; i++) {
        if ((g_SaveSummary[0].padding > 0 && g_SaveSummary[0].icon[i] == -3) ||
            (g_SaveSummary[1].padding > 0 && g_SaveSummary[1].icon[i] == -3)) {
            break;
        }
    }
    if (i == BLOCK_PER_CARD) {
        D_801BAF14 = 1;
    }
}

static void func_801AEE74(void) {
    s32 i;

    for (i = 0; i < BLOCK_PER_CARD; i++) {
        if (g_SaveSummary[0].icon[i] >= 0 || g_SaveSummary[1].icon[i] >= 0) {
            break;
        }
    }

    if (i == BLOCK_PER_CARD) {
        D_801BAF10 = 1;
    }
}

#if defined(MOD_PTBR)
#include "disk.h"
#include "lba.h"
#include "../../../mods/sotn-ultimate/src/menu/2C048_mod_ptbr.h"
#endif

void Update(void) {
    s32 i;
    s32 port;
    s32 slot;
    s32 temp_s1;
    s32 var_s4;

#if defined(MOD_PTBR)
    if (MOD_PTBR_BLOB[1] == MOD_PTBR_MAGIC) {
        MOD_PTBR_FRAME(0);
    }
#endif
#if defined(MOD_SATURN) && defined(VERSION_US)
    ModJpDubCopy();
#endif
#if defined(MOD_PTBR)
    if (MOD_PTBR_BLOB[1] == MOD_PTBR_MAGIC) {
        MOD_PTBR_FRAME(1);
    }
#endif
    func_801B1F34();
    switch (g_GameEngineStep) {
    case Upd_Eng_Init:
        func_801B18F4();
        SetTitleDisplayBuffer256();
        g_GameEngineStep--;
        break;
    case Upd_Eng_MenuInit:
#if defined(MOD_PTBR)
        if (ModPtbrLoad(MOD_PTBR_BLOCK_MENU)) {
            break;
        }
#endif
        g_api.PlaySfx(MU_PRAYER);
        g_GameTimer = 0;
        D_801BAF08 = 0;
        func_801B2D6C();
        g_api.func_800EA5E4(ANIMSET_DRA(0));
        g_api.func_800EA5E4(ANIMSET_OVL(0));
        g_api.func_800EA5E4(ANIMSET_OVL(1));
        g_api.func_800EA5E4(ANIMSET_OVL(2));
        g_api.func_800EA5E4(ANIMSET_OVL(3));
        g_api.func_800EA5E4(ANIMSET_OVL(6));
        SetupFileChoose();
        InitMainMenuBackgroundAndFadeMask();
        InitMainMenuUI();
        func_801ACF7C();
        func_801AECA0();
        func_801B1F4C(9);
        func_801AD490();
        g_GameEngineStep += 2;
        break;
    case Upd_Eng_MenuFadeIn:
        func_801AD590();
        func_801AD490();
        if (MainMenuFadeIn()) {
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_MainMenuIdle:
        func_801AD590();
        func_801AD490();
        if (g_pads[0].tapped & SEL_CONFIRM) {
            switch (g_MainMenuCursor) {
            case 0:
                g_api.PlaySfx(SFX_UI_CONFIRM);
                g_GameEngineStep = Upd_Eng_0x10;
                break;
            case 1:
                g_api.PlaySfx(SFX_UI_CONFIRM);
                g_GameEngineStep = Upd_Eng_FileSelect;
                break;
            case 2:
                g_api.PlaySfx(SFX_UI_CONFIRM);
                g_GameEngineStep = Upd_Eng_NameChange;
                break;
            case 3:
                g_api.PlaySfx(SFX_UI_CONFIRM);
                g_GameEngineStep = Upd_Eng_FileCopy;
                break;
            case 4:
                g_api.PlaySfx(SFX_UI_CONFIRM);
                g_GameEngineStep = Upd_Eng_FileDelete;
                break;
#if defined(MOD_SATURN)
            case MOD_MENU_SOUND:
                g_api.PlaySfx(SFX_UI_CONFIRM);
                ModSndSelect(0);
                ModSoundInit();
                g_GameEngineStep = Upd_Eng_ModSound;
                break;
            case MOD_MENU_DRAMA:
                if (!MOD_DRAMA_ON()) {
                    g_api.PlaySfx(SFX_UI_ERROR); // the line is not on this menu
                    break;
                }
                g_api.PlaySfx(SFX_UI_CONFIRM);
                ModSndSelect(1);
                ModSoundInit();
                g_GameEngineStep = Upd_Eng_ModSound;
                break;
#endif
            default:
                g_api.PlaySfx(SFX_UI_ERROR);
                break;
            }
        }
        break;
    case Upd_Eng_3:
        func_801AD590();
        func_801AD490();
        if (func_801ACEC0()) {
            SetGameState(Game_Title);
        }
        break;
    case Upd_Eng_0x10:
        strcpy(g_InputSaveName, "        ");
        func_801AEA8C(0);
        g_GameEngineStep++;
        /* fallthrough */
    case Upd_Eng_17:
        for (i = 0; i < 8; i++) {
            if (g_InputSaveName[i] != ' ') {
                break;
            }
        }
        if ((g_pads[0].tapped & PAD_SELECT) ||
            ((g_pads[0].tapped & SEL_CANCEL) && i == 8 &&
             g_InputCursorPos == 0)) {
            g_GameEngineStep = Upd_Eng_FileSelect;
        } else {
            UpdateNameEntry();
            func_801AD78C();
            if (g_pads[0].tapped & PAD_START) {
                g_api.PlaySfx(SFX_UI_CONFIRM);
                func_801AD66C();
#if defined(MOD_SATURN)
                g_ModCharselShown = false;
                MOD_STATUS_FLAGS = 0;
                // Saturn name code "STAFF" (TITLE.PRG, no clear needed):
                // roll the staff credits through SEL's ending handler.
                {
                    const char* staff = "staff   ";
                    for (i = 0; i < 8; i++) {
                        if (g_Status.saveName[i] != staff[i]) {
                            break;
                        }
                    }
                    g_ModStaffRoll = i == 8;
                }
                if (g_ModStaffRoll) {
                    D_800978B4 = MOD_ENDING_STAFF;
                } else if (g_PlayableCharacter == PLAYER_ALUCARD) {
                    if (ModCharselInit()) {
                        g_GameEngineStep = Upd_Eng_ModCharSelect;
                        break;
                    }
                } else {
                    // the retail "RICHTER" name: normal costume
                    g_Status.D_80097C40 = 0;
                }
#endif
                if (g_PlayableCharacter == PLAYER_ALUCARD) {
                    g_StageId = STAGE_ST0;
                } else {
                    g_StageId = STAGE_NO3;
                }
                D_8003C730 = 0;
                D_8006C378 = -1;
                if (D_801BAF10) {
                    D_80097924 = -1;
                } else {
                    D_80097924 = D_801D6B04 / 15;
                }
                g_api.PlaySfx(SET_UNK_80);
                g_GameEngineStep++;
            }
        }
        break;
#if defined(MOD_SATURN)
    case Upd_Eng_ModCharSelect:
        ModCharselUpdate();
        break;
    case Upd_Eng_ModSound:
        ModSoundUpdate();
        break;
#endif
    case Upd_Eng_18:
#if defined(MOD_SATURN)
        if (g_ModCharselShown) {
            ModCharselDraw();
        } else {
            func_801AD78C();
        }
#else
        func_801AD78C();
#endif
        if (func_801ACEC0()) {
            func_801B18F4();
            func_801B19F4();
            func_801B1DA8();
            func_801B2D1C();
            g_api.func_800EA538(0);
#if defined(MOD_SATURN)
            if (g_ModStaffRoll) {
                SetGameState(Game_Ending);
            } else
#endif
                if (g_PlayableCharacter == PLAYER_ALUCARD) {
                SetGameState(Game_VideoPlayback);
            } else {
                g_GameStep++;
            }
        }
        break;
    case Upd_Eng_FileSelect:
        SelectMainMenuOption(MAIN_MENU_CURSOR_FILE_SELECT);
        func_801B2608(KO_SAVE("Checking Memory Card．", "\xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2 \xF2\x48\xF0\x7E \xF0\xA6．"), 4);
        func_801B2608(KO_SAVE("Do not remove Memory Card．", "\xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2\xF0\xC6 \xF2\xB4\xF0\x85 \xF0\x52\xF0\x69\xF0\x77．"), 5);
        func_801B3120();
        g_GameEngineStep++;
        /* fallthrough */
    case Upd_Eng_49:
        func_801ADF94(0x80, 0);
        D_800978C4 = 0;
        if (func_801B3164()) {
            D_800978C4 = 1;
            g_GameEngineStep++;
            CheckIfMemcardsCanBeUsed();
            if (g_SaveSummary[0].padding == -2 ||
                g_SaveSummary[1].padding == -2) {
                g_SelNextCrossPressEngStep = Upd_Eng_FileSelect;
                g_SelEng220NextStep = Upd_Eng_FileSelect + 2;
                g_GameEngineStep = Upd_Eng_0x200;
            }
        }
        break;
    case Upd_Eng_50:
        if (g_GameClearFlag == SAVE_FLAG_NORMAL) {
            D_801BAF0C = 0xFF;
        } else {
            D_801BAF0C = 0x2FF;
        }
        if (g_SaveSummary[0].padding < 0) {
            g_MemCardSelectorX = (g_MemCardSelectorX % 3) + 3;
        }
        if (g_SaveSummary[1].padding < 0) {
            g_MemCardSelectorX = g_MemCardSelectorX % 3;
        }
        if (D_801BAF10) {
            func_801B2608(KO_SAVE("You won’t be able to save", "\xF0\xDE\xF0\x80\xF0\xB4 \xF3\x18 \xF0\xD2 \xF0\xD6\xF2\xC0\xF1\x36\xF0\x49"), 4);
            func_801B2608(KO_SAVE("your game． Is that OK？", "\xF0\xFB\xF0\xD0\xF0\xA3\xF2\x59\xF2\xC0\xF1\x36\xF1\x11？"), 5);
            func_801ADF94(0x83, 0);
            DrawNavigationTips(Tips_YesNo);
            g_GameEngineStep = Upd_Eng_64;
        } else {
            DrawNavigationTips(Tips_Generic);
            func_801ADF94(1, 0);
            func_801B25D4("　", 4);
            func_801B25D4("　", 5);
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_64:
        func_801ADF94(0x81, 0);
        DrawNavigationTips(Tips_YesNo);
        if (g_pads[0].tapped & SEL_CANCEL) {
            func_801AE9A8();
            func_801AD490();
            g_GameEngineStep = Upd_Eng_MainMenuIdle;
        } else if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            g_GameEngineStep = Upd_Eng_0x10;
        }
        break;
    case Upd_Eng_65:
        func_801ADF94(0x83, 0);
        DrawNavigationTips(Tips_YesNo);
        if (g_pads[0].tapped & SEL_CANCEL) {
            D_801BAF0C = 0xFF;
            DrawNavigationTips(Tips_Generic);
            func_801B2608(KO_SAVE("Select file to be loaded．", "\xF1\x9C\xF1\x5D\xF1\xCF \xF0\x92\xF0\x7F\xF0\xB4"), 4);
            func_801B2608("\xF1\xAA\xF2\x1C\xF0\xB9\xF0\x82\xF1\xAE\xF0\x78", 5);
            g_GameEngineStep = Upd_Eng_51;
        } else if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            g_GameEngineStep = Upd_Eng_0x10;
        }
        break;
    case Upd_Eng_51:
        if (g_pads[0].tapped & SEL_CANCEL) {
            func_801AE9A8();
            func_801AD490();
            g_GameEngineStep = Upd_Eng_MainMenuIdle;
        } else {
            UpdateFileSelect();
            func_801ADF94(1, 0);
            D_801BAF0C++;
            if (D_801BAF0C == 0x100) {
                func_801B2608(KO_SAVE("Select file to be loaded．", "\xF1\x9C\xF1\x5D\xF1\xCF \xF0\x92\xF0\x7F\xF0\xB4"), 4);
                func_801B2608("\xF1\xAA\xF2\x1C\xF0\xB9\xF0\x82\xF1\xAE\xF0\x78", 5);
            }
            if (D_801BAF0C == 0x200) {
                if (D_801BAF14) {
                    func_801B2608(KO_SAVE("Press Start button", "START \xF1\x8E\xF3\x0E\xF0\xB4 \xF1\x33\xF1\x6D\xF0\x69\xF0\x77"), 4);
                    func_801B2608(KO_SAVE("to begin new game．", "\xF1\xA7 \xF0\xBC\xF1\xDB\xF0\xB4 \xF0\x69\xF1\xDF\xF2\x3F\xF1\x36\xF0\x49"), 5);
                } else {
                    func_801B2608(KO_SAVE("Select New Game", "NEW GAME \xF1\xAA\xF2\x1C\xF0\x69"), 4);
                    func_801B2608(KO_SAVE("to start from beginning．", "\xF1\xA7\xF0\xA0\xF0\xDB \xF0\xBC\xF1\xDB\xF0\xB4 \xF0\x69\xF1\xDF\xF2\x3F\xF1\x36\xF0\x49"), 5);
                }
                if (g_GameClearFlag == SAVE_FLAG_NORMAL) {
                    D_801BAF0C = 0;
                }
            }
            if (D_801BAF0C == 0x300) {
                func_801B2608(KO_SAVE("Input ””RICHTER”” to play", "\xF0\x50\xF2\x4E\xF2\x1E\xF0\xA0 \xF2\x3D\xF0\x4C\xF0\x7D\xF0\xA3\xF1\x63\xF1\x81"), 4);
                func_801B2608(KO_SAVE("as Richter Belmont．", "””RICHTER””\xF0\xC6 \xF0\xDD\xF0\xA4\xF0\xA3\xF0\x69\xF0\x77．"), 5);
                D_801BAF0C = 0;
            }
            if ((g_pads[0].tapped & PAD_START) && D_801BAF14) {
                g_api.PlaySfx(SFX_UI_CONFIRM);
                func_801B2608(KO_SAVE("You won’t be able to save", "\xF0\xDE\xF0\x80\xF0\xB4 \xF3\x18 \xF0\xD2 \xF0\xD6\xF2\xC0\xF1\x36\xF0\x49"), 4);
                func_801B2608(KO_SAVE("your game． Is that OK？", "\xF0\xFB\xF0\xD0\xF0\xA3\xF2\x59\xF2\xC0\xF1\x36\xF1\x11？"), 5);
                func_801ADF94(0x81, 0);
                DrawNavigationTips(Tips_YesNo);
                g_GameEngineStep = Upd_Eng_65;
            } else if (g_pads[0].tapped & SEL_CONFIRM) {
                port = D_801D6B04 / 15;
                slot = D_801D6B04 % 15;
                if (g_SaveSummary[port].icon[slot] >= 0) {
                    g_api.PlaySfx(SFX_UI_CONFIRM);
                    g_GameEngineStep = Upd_Eng_0x100;
                } else if (g_SaveSummary[port].icon[slot] == -3) {
                    g_api.PlaySfx(SFX_UI_CONFIRM);
                    g_GameEngineStep = Upd_Eng_0x10;
                } else {
                    g_api.PlaySfx(SFX_UI_ERROR);
                }
            }
        }
        break;
    case Upd_Eng_NameChange:
        SelectMainMenuOption(MAIN_MENU_CURSOR_FILE_DELETE);
        func_801ACBE4(GFX_UNK_15, 0);
        func_801B2608(KO_SAVE("Checking Memory Card．", "\xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2 \xF2\x48\xF0\x7E \xF0\xA6．"), 4);
        func_801B2608(KO_SAVE("Do not remove Memory Card．", "\xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2\xF0\xC6 \xF2\xB4\xF0\x85 \xF0\x52\xF0\x69\xF0\x77．"), 5);
        func_801B3120();
        g_GameEngineStep++;
        /* fallthrough */
    case Upd_Eng_145:
        func_801ADF94(0x80, 0);
        D_800978C4 = 0;
        if (func_801B3164()) {
            D_800978C4 = 1;
            g_GameEngineStep++;
            CheckIfMemcardsCanBeUsed();
            if (g_SaveSummary[0].padding == -2 ||
                g_SaveSummary[1].padding == -2) {
                g_SelNextCrossPressEngStep = Upd_Eng_NameChange;
                g_SelEng220NextStep = Upd_Eng_NameChange + 2;
                g_GameEngineStep = Upd_Eng_0x200;
            }
        }
        break;
    case Upd_Eng_146:
        func_801ACBE4(GFX_UNK_15, 0);
        if (g_SaveSummary[0].padding < 0) {
            g_MemCardSelectorX = (g_MemCardSelectorX % 3) + 3;
        }
        if (g_SaveSummary[1].padding < 0) {
            g_MemCardSelectorX = g_MemCardSelectorX % 3;
        }
        func_801AEE74();
        if (D_801BAF10) {
            func_801B2608(KO_SAVE("No data found", "\xF0\x7D \xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2\xF0\x74\xF0\xBF"), 4);
            func_801B2608(KO_SAVE("on this Memory Card．", "\xF1\x3F\xF0\x7D\xF2\x1E\xF0\x40 \xF0\xD6\xF2\xC0\xF1\x36\xF0\x49．"), 5);
            func_801ADF94(0x82, 0);
            DrawNavigationTips(Tips_Confirm);
            g_GameEngineStep = Upd_Eng_0xA0;
        } else {
            DrawNavigationTips(Tips_Generic);
            func_801ADF94(2, 0);
            func_801B2608(KO_SAVE("Select the file", "\xF0\x7D\xF0\x4F\xF0\xB4 \xF0\xBD\xF2\x03 \xF0\x92\xF0\x7F\xF0\xB4"), 4);
            func_801B2608(KO_SAVE("you wish to rename．", "\xF1\xAA\xF2\x1C\xF0\xA3\xF0\x69\xF0\x77．"), 5);
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_0xA0:
        DrawNavigationTips(Tips_Confirm);
        func_801ADF94(0x82, 0);
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            func_801AE9A8();
            func_801AD490();
            g_GameEngineStep = Upd_Eng_MainMenuIdle;
        }
        break;
    case Upd_Eng_147:
        if (g_pads[0].tapped & SEL_CANCEL) {
            func_801AE9A8();
            func_801AD490();
            g_GameEngineStep = Upd_Eng_MainMenuIdle;
        } else {
            UpdateFileSelect();
            func_801ADF94(2, 0);
            if (g_pads[0].tapped & SEL_CONFIRM) {
                port = D_801D6B04 / 15;
                slot = D_801D6B04 % 15;
                if (g_SaveSummary[port].icon[slot] >= 0) {
                    g_api.PlaySfx(SFX_UI_CONFIRM);
                    g_GameEngineStep++;
                } else {
                    g_api.PlaySfx(SFX_UI_ERROR);
                }
            }
        }
        break;
    case Upd_Eng_148:
        strcpy(g_InputSaveName, "        ");
        func_801AEA8C(1);
        g_GameEngineStep++;
        /* fallthrough */
    case Upd_Eng_149:
        for (i = 0; i < 8; i++) {
            if (g_InputSaveName[i] != ' ') {
                break;
            }
        }
        if ((g_pads[0].tapped & PAD_SELECT) ||
            (g_pads[0].tapped & SEL_CANCEL && i == 8 &&
             g_InputCursorPos == 0)) {
            SelectMainMenuOption(MAIN_MENU_CURSOR_FILE_DELETE);
            g_GameEngineStep = Upd_Eng_146;
        } else {
            UpdateNameEntry();
            func_801AD78C();
            if (g_pads[0].tapped & PAD_START) {
                SelectMainMenuOption(MAIN_MENU_CURSOR_FILE_DELETE);
                var_s4 = 0;
                for (i = 0; i < 8; i++) {
                    if (g_InputSaveName[i] == ' ') {
                        var_s4++;
                    }
                }
                if (var_s4 == 8) {
                    g_GameEngineStep = Upd_Eng_146;
                } else {
                    func_801ACBE4(GFX_UNK_15, 0);
                    func_801B2608(KO_SAVE("Changing Name．", "\xF0\x7D\xF0\x4F \xF0\xA1\xF0\xF9 \xF0\xA6．"), 4);
                    func_801B2608(KO_SAVE("Do not remove Memory Card．", "\xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2\xF0\xC6 \xF2\xB4\xF0\x85 \xF0\x52\xF0\x69\xF0\x77．"), 5);
                    g_api.PlaySfx(SFX_UI_CONFIRM);
                    g_GameEngineStep++;
                }
            }
        }
        break;
    case Upd_Eng_150:
        func_801ACBE4(GFX_UNK_6, 8);
        func_801ADF94(0x82, 0);
        func_801B38B4(D_801D6B04, D_801D6B04);
        g_GameEngineStep++;
        break;
    case Upd_Eng_151:
        func_801ADF94(0x82, 0);
        D_800978C4 = 0;
        temp_s1 = func_801B3A94(g_InputSaveName);
        if (temp_s1 != 0) {
            D_800978C4 = 1;
        }
        if (temp_s1 == 1) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("Name changed．", "\xF0\x7D\xF0\x4F\xF0\x7D \xF0\xA1\xF0\xF9\xF2\x80\xF1\xC9\xF2\xC0\xF1\x36\xF0\x49．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep += 2;
        }
        if (temp_s1 == -1) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("Loading error．", "\xF0\xA0\xF0\xB2 \xF0\x74\xF1\x5D．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep++;
        }
        if (temp_s1 == -3) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("Save error．", "\xF1\xAE\xF0\x7D\xF0\x5D \xF0\x74\xF1\x5D．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_152:
        DrawNavigationTips(Tips_Confirm);
        func_801ADF94(0x82, 0);
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            func_801ACBE4(GFX_UNK_15, 8);
            g_GameEngineStep = Upd_Eng_NameChange;
        }
        break;
    case Upd_Eng_153:
        DrawNavigationTips(Tips_Confirm);
        func_801ADF94(0x82, 0);
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            g_GameEngineStep = Upd_Eng_146;
        }
        break;
    case Upd_Eng_FileCopy:
        SelectMainMenuOption(MAIN_MENU_CURSOR_NAME_CHANGE);
        func_801B2608(KO_SAVE("Checking Memory Card．", "\xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2 \xF2\x48\xF0\x7E \xF0\xA6．"), 4);
        func_801B2608(KO_SAVE("Do not remove Memory Card．", "\xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2\xF0\xC6 \xF2\xB4\xF0\x85 \xF0\x52\xF0\x69\xF0\x77．"), 5);
        func_801B3120();
        g_GameEngineStep++;
        /* fallthrough */
    case Upd_Eng_81:
        func_801ADF94(0x80, 0);
        D_800978C4 = 0;
        if (func_801B3164()) {
            D_800978C4 = 1;
            g_GameEngineStep++;
            CheckIfMemcardsCanBeUsed();
            if (g_SaveSummary[0].padding == -2 ||
                g_SaveSummary[1].padding == -2) {
                g_SelNextCrossPressEngStep = Upd_Eng_FileCopy;
                g_SelEng220NextStep = Upd_Eng_FileCopy + 2;
                g_GameEngineStep = Upd_Eng_0x200;
            }
        }
        break;
    case Upd_Eng_82:
        if (g_SaveSummary[0].padding < 0) {
            g_MemCardSelectorX = (g_MemCardSelectorX % 3) + 3;
        }
        if (g_SaveSummary[1].padding < 0) {
            g_MemCardSelectorX = g_MemCardSelectorX % 3;
        }
        func_801AEE74();
        if (D_801BAF10) {
            func_801B2608(KO_SAVE("No data found", "\xF0\x7D \xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2\xF0\x74\xF0\xBF"), 4);
            func_801B2608(KO_SAVE("on this Memory Card．", "\xF1\x3F\xF0\x7D\xF2\x1E\xF0\x40 \xF0\xD6\xF2\xC0\xF1\x36\xF0\x49．"), 5);
            func_801ADF94(0x82, 0);
            DrawNavigationTips(Tips_Confirm);
            g_GameEngineStep = Upd_Eng_0x60;
        } else {
            DrawNavigationTips(Tips_Generic);
            func_801ADF94(2, 0);
            func_801B2608(KO_SAVE("Select file", "\xF0\xCD\xF0\xAB\xF3\x18 \xF0\x92\xF0\x7F\xF0\xB4"), 4);
            func_801B2608(KO_SAVE("to be copied．", "\xF1\xAA\xF2\x1C\xF0\xA3\xF0\x69\xF0\x77．"), 5);
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_0x60:
        DrawNavigationTips(Tips_Confirm);
        func_801ADF94(0x82, 0);
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            func_801AE9A8();
            func_801AD490();
            g_GameEngineStep = Upd_Eng_MainMenuIdle;
        }
        break;
    case Upd_Eng_83:
        if (g_pads[0].tapped & SEL_CANCEL) {
            func_801AE9A8();
            func_801AD490();
            g_GameEngineStep = Upd_Eng_MainMenuIdle;
        } else {
            UpdateFileSelect();
            func_801ADF94(2, 0);
            if (g_pads[0].tapped & SEL_CONFIRM) {
                port = D_801D6B04 / 15;
                slot = D_801D6B04 % 15;
                if (g_SaveSummary[port].icon[slot] >= 0) {
                    func_801B2608(KO_SAVE("Where do you want", "\xF0\xCD\xF0\xAB\xF3\x18 \xF1\xD8\xF2\x02\xF0\xC6"), 4);
                    func_801B2608(KO_SAVE("to copy to？", "\xF1\xAA\xF2\x1C\xF0\xA3\xF0\x69\xF0\x77．"), 5);
                    D_801BC3EC = D_801D6B04;
                    g_api.PlaySfx(SFX_UI_CONFIRM);
                    g_GameEngineStep++;
                } else {
                    g_api.PlaySfx(SFX_UI_ERROR);
                }
            }
        }
        break;
    case Upd_Eng_84:
        UpdateFileSelect();
        func_801ADF94(2, 1);
        func_801AE6D0();
        if (g_pads[0].tapped & SEL_CANCEL) {
            func_801ACBE4(GFX_UNK_18, 8);
            func_801ACBE4(GFX_UNK_19, 8);
            func_801ACBE4(GFX_UNK_20, 8);
            func_801B2608(KO_SAVE("Select file", "\xF0\xCD\xF0\xAB\xF3\x18 \xF0\x92\xF0\x7F\xF0\xB4"), 4);
            func_801B2608(KO_SAVE("to be copied．", "\xF1\xAA\xF2\x1C\xF0\xA3\xF0\x69\xF0\x77．"), 5);
            g_GameEngineStep--;
        } else {
            if (g_pads[0].tapped & SEL_CONFIRM) {
                port = D_801D6B04 / 15;
                slot = D_801D6B04 % 15;
                if (D_801BC3EC != D_801D6B04) {
                    if (g_SaveSummary[port].icon[slot] >= 0) {
                        func_801B1F4C(5);
                        func_801B2608(KO_SAVE("OK to overwrite data？", "\xF1\x3F\xF0\x7D\xF2\x1E\xF0\xC6 \xF1\x3E\xF0\x71\xF0\xD5\xF1\x41 \xF2\x5D\xF2\xF6\xF2\xC0\xF1\x36\xF1\x11？"), 4);
                        g_api.PlaySfx(SFX_UI_CONFIRM);
                        g_GameEngineStep = Upd_Eng_89;
                    } else if (g_SaveSummary[port].icon[slot] != -2) {
                        func_801B2608(KO_SAVE("Copying data．", "\xF1\x3F\xF0\x7D\xF2\x1E \xF0\xCD\xF0\xAB \xF0\xA6．"), 4);
                        func_801B2608(KO_SAVE("Do not remove Memory Card．", "\xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2\xF0\xC6 \xF2\xB4\xF0\x85 \xF0\x52\xF0\x69\xF0\x77．"), 5);
                        g_api.PlaySfx(SFX_UI_CONFIRM);
                        g_GameEngineStep++;
                    }
                } else {
                    g_api.PlaySfx(SFX_UI_ERROR);
                }
            }
        }
        break;
    case Upd_Eng_85:
        func_801ACBE4(GFX_UNK_6, 8);
        func_801ADF94(0x82, 1);
        func_801AE6D0();
        func_801B38B4(D_801BC3EC, D_801D6B04);
        g_GameEngineStep++;
        break;
    case Upd_Eng_86:
        func_801ADF94(0x82, 1);
        func_801AE6D0();
        D_800978C4 = 0;
        temp_s1 = func_801B3A94(0);
        if (temp_s1 != 0) {
            D_800978C4 = 1;
        }
        if (temp_s1 == 1) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("File copied．", "\xF0\x92\xF0\x7F\xF0\x7D \xF0\xCD\xF0\xAB\xF2\x80\xF1\xC9\xF2\xC0\xF1\x36\xF0\x49．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep += 2;
        }
        if (temp_s1 == -1) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("Loading error．", "\xF0\xA0\xF0\xB2 \xF0\x74\xF1\x5D．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep++;
        }
        if (temp_s1 == -3) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("Save error．", "\xF1\xAE\xF0\x7D\xF0\x5D \xF0\x74\xF1\x5D．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_87:
        DrawNavigationTips(Tips_Confirm);
        func_801ADF94(0x82, 1);
        func_801AE6D0();
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            func_801ACBE4(GFX_UNK_15, 8);
            g_GameEngineStep = Upd_Eng_FileCopy;
        }
        break;
    case Upd_Eng_88:
        DrawNavigationTips(Tips_Confirm);
        func_801ADF94(0x82, 1);
        func_801AE6D0();
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            func_801ACBE4(GFX_UNK_18, 8);
            func_801ACBE4(GFX_UNK_19, 8);
            func_801ACBE4(GFX_UNK_20, 8);
            g_GameEngineStep = Upd_Eng_82;
        }
        break;
    case Upd_Eng_89:
        DrawNavigationTips(Tips_YesNo);
        func_801ADF94(0x82, 1);
        func_801AE6D0();
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            func_801B2608(KO_SAVE("Copying data．", "\xF1\x3F\xF0\x7D\xF2\x1E \xF0\xCD\xF0\xAB \xF0\xA6．"), 4);
            func_801B2608(KO_SAVE("Do not remove Memory Card．", "\xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2\xF0\xC6 \xF2\xB4\xF0\x85 \xF0\x52\xF0\x69\xF0\x77．"), 5);
            g_GameEngineStep = Upd_Eng_85;
        } else {
            if (g_pads[0].tapped & SEL_CANCEL) {
                func_801B1F4C(5);
                func_801B25D4("どこにコピーしますか？", 4);
                DrawNavigationTips(Tips_Generic);
                g_GameEngineStep = Upd_Eng_84;
            }
            func_801AD1D0();
        }
        break;
    case Upd_Eng_FileDelete:
        SelectMainMenuOption(MAIN_MENU_CURSOR_FILE_COPY);
        func_801B2608(KO_SAVE("Checking Memory Card．", "\xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2 \xF2\x48\xF0\x7E \xF0\xA6．"), 4);
        func_801B2608(KO_SAVE("Do not remove Memory Card．", "\xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2\xF0\xC6 \xF2\xB4\xF0\x85 \xF0\x52\xF0\x69\xF0\x77．"), 5);
        func_801B3120();
        g_GameEngineStep++;
        /* fallthrough */
    case Upd_Eng_113:
        func_801ADF94(0x80, 0);
        D_800978C4 = 0;
        if (func_801B3164()) {
            D_800978C4 = 1;
            g_GameEngineStep++;
            CheckIfMemcardsCanBeUsed();
            if (g_SaveSummary[0].padding == -2 ||
                g_SaveSummary[1].padding == -2) {
                g_SelNextCrossPressEngStep = Upd_Eng_FileDelete;
                g_SelEng220NextStep = Upd_Eng_FileDelete + 2;
                g_GameEngineStep = Upd_Eng_0x200;
            }
        }
        break;
    case Upd_Eng_114:
        if (g_SaveSummary[0].padding < 0) {
            g_MemCardSelectorX = (g_MemCardSelectorX % 3) + 3;
        }
        if (g_SaveSummary[1].padding < 0) {
            g_MemCardSelectorX = g_MemCardSelectorX % 3;
        }
        func_801AEE74();
        if (D_801BAF10) {
            func_801B2608(KO_SAVE("No data found", "\xF0\x7D \xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2\xF0\x74\xF0\xBF"), 4);
            func_801B2608(KO_SAVE("on this Memory Card．", "\xF1\x3F\xF0\x7D\xF2\x1E\xF0\x40 \xF0\xD6\xF2\xC0\xF1\x36\xF0\x49．"), 5);
            func_801ADF94(0x82, 0);
            DrawNavigationTips(Tips_Confirm);
            g_GameEngineStep = Upd_Eng_0x80;
        } else {
            DrawNavigationTips(Tips_Generic);
            func_801ADF94(2, 0);
            func_801B2608(KO_SAVE("Select file", "\xF0\x85\xF0\x79 \xF0\x92\xF0\x7F\xF0\xB4"), 4);
            func_801B2608(KO_SAVE("to be erased．", "\xF1\xAA\xF2\x1C\xF0\xA3\xF0\x69\xF0\x77．"), 5);
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_0x80:
        DrawNavigationTips(Tips_Confirm);
        func_801ADF94(0x82, 0);
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            func_801AE9A8();
            func_801AD490();
            g_GameEngineStep = Upd_Eng_MainMenuIdle;
        }
        break;
    case Upd_Eng_115:
        if (g_pads[0].tapped & SEL_CANCEL) {
            func_801AE9A8();
            func_801AD490();
            g_GameEngineStep = Upd_Eng_MainMenuIdle;
        } else {
            UpdateFileSelect();
            func_801ADF94(2, 0);
            DrawNavigationTips(Tips_Generic);
            if (g_pads[0].tapped & SEL_CONFIRM) {
                port = D_801D6B04 / 15;
                slot = D_801D6B04 % 15;
                if (g_SaveSummary[port].icon[slot] >= 0) {
                    func_801B1F4C(5);
                    func_801B2608(KO_SAVE("Is it OK to erase file？", "\xF0\x92\xF0\x7F\xF0\xB4 \xF0\x85\xF0\xB3\xF1\x41 \xF2\x5D\xF2\xF6\xF2\xC0\xF1\x36\xF1\x11？"), 4);
                    g_api.PlaySfx(SFX_UI_CONFIRM);
                    g_GameEngineStep++;
                } else {
                    g_api.PlaySfx(SFX_UI_ERROR);
                }
            }
        }
        break;
    case Upd_Eng_116:
        DrawNavigationTips(Tips_YesNo);
        func_801ADF94(0x82, 0);
        if (g_pads[0].tapped & SEL_CANCEL) {
            func_801B2608(KO_SAVE("Select file", "\xF0\x85\xF0\x79 \xF0\x92\xF0\x7F\xF0\xB4"), 4);
            func_801B2608(KO_SAVE("to be erased．", "\xF1\xAA\xF2\x1C\xF0\xA3\xF0\x69\xF0\x77．"), 5);
            g_GameEngineStep--;
        } else if (g_pads[0].tapped & SEL_CONFIRM) {
            func_801ACBE4(GFX_UNK_15, 8);
            g_api.PlaySfx(SFX_UI_CONFIRM);
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_117:
        func_801ACBE4(GFX_UNK_6, 8);
        func_801ADF94(0x82, 0);
        func_801B3E14(D_801D6B04);
        g_GameEngineStep++;
        break;
    case Upd_Eng_118:
        func_801ADF94(0x82, 0);
        D_800978C4 = 0;
        temp_s1 = func_801B3E2C();
        if (temp_s1 != 0) {
            D_800978C4 = 1;
        }
        if (temp_s1 == 1) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("File erased．", "\xF0\x92\xF0\x7F\xF0\x7D \xF0\x85\xF0\xB3\xF2\xE5\xF2\xC0\xF1\x36\xF0\x49．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep += 2;
        }
        if (temp_s1 == -1) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("Delete error．", "\xF1\xA5\xF1\xE7 \xF0\x74\xF1\x5D．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_119:
        DrawNavigationTips(Tips_Confirm);
        func_801ADF94(0x82, 0);
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            func_801ACBE4(GFX_UNK_15, 8);
            g_GameEngineStep = Upd_Eng_FileDelete;
        }
        break;
    case Upd_Eng_120:
        DrawNavigationTips(Tips_Confirm);
        func_801ADF94(0x82, 0);
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            g_GameEngineStep = Upd_Eng_114;
        }
        break;
    case Upd_Eng_0x100:
        func_801ACBE4(GFX_UNK_6, 8);
        func_801ADF94(0x81, 0);
        func_801B2608(KO_SAVE("Loading Memory Card．", "\xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2 \xF0\xA0\xF0\xB2 \xF0\xA6．"), 4);
        func_801B2608(KO_SAVE("Do not remove Memory Card．", "\xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2\xF0\xC6 \xF2\xB4\xF0\x85 \xF0\x52\xF0\x69\xF0\x77．"), 5);
        func_801ACBE4(GFX_UNK_15, 0);
        func_801B367C(D_801D6B04);
        g_GameEngineStep++;
        break;
    case Upd_Eng_257:
        func_801ADF94(0x81, 0);
        D_800978C4 = 0;
        temp_s1 = TryLoadFromMemcard();
        if (temp_s1 != 0) {
            D_800978C4 = 1;
        }
        if (temp_s1 == 1) {
            D_8003C730 = 1;
            D_80097924 = D_801D6B04 / 15;
            D_8006C378 = g_SaveSummary[D_80097924].slot[D_801D6B04 % 15];
            g_api.PlaySfx(SET_UNK_80);
            g_GameEngineStep = Upd_Eng_260;
        }
        if (temp_s1 == -1) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("Loading error．", "\xF0\xA0\xF0\xB2 \xF0\x74\xF1\x5D．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep++;
        }
        if (temp_s1 == -2) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("Version error．", "\xF1\x8E\xF1\xE4 \xF0\x74\xF1\x5D．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_258:
        func_801ADF94(0x81, 0);
        if (g_pads[0].tapped) {
            func_801ACBE4(GFX_UNK_15, 8);
            g_GameEngineStep = Upd_Eng_FileSelect;
        }
        break;
    case Upd_Eng_260:
        func_801ADF94(0x81, 0);
        if (func_801ACEC0()) {
            g_GameStep++;
        }
        break;
    case Upd_Eng_0x200:
        DrawNavigationTips(Tips_NoYes);
        func_801ADF94(0x80, 0);
        if (g_SaveSummary[0].padding != -2) {
            g_GameEngineStep = Upd_Eng_0x210;
        } else {
            func_801B2608(KO_SAVE("Format Memory Card", "１\xF0\xCC \xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2\xF0\xC6"), 4);
            func_801B2608(KO_SAVE("in slot １？", "\xF0\xB7\xF0\x46\xF0\x99\xF0\xA3\xF2\x59\xF2\xC0\xF1\x36\xF1\x11？"), 5);
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_513:
        func_801ADF94(0x80, 0);
        if (g_pads[0].tapped & SEL_CANCEL) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            func_801ACBE4(GFX_UNK_6, 8);
            MemCardSetPort(0);
            g_GameEngineStep++;
        } else {
            if (g_pads[0].tapped & SEL_CONFIRM) {
                g_GameEngineStep = Upd_Eng_0x210;
            }
            func_801AD218();
        }
        break;
    case Upd_Eng_514:
        func_801ADF94(0x80, 0);
        D_800978C4 = 0;
        temp_s1 = MemCardInitAndFormat();
        if (temp_s1 != 0) {
            D_800978C4 = 1;
        }
        if (temp_s1 == 1) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("Formatting completed．", "\xF0\xB7\xF0\x46\xF0\x99\xF0\x40 \xF2\x68\xF1\x21\xF2\xC0\xF1\x36\xF0\x49．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep += 2;
        }
        if (temp_s1 == -1) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("Formatting error．", "\xF0\xB7\xF0\x46\xF0\x99 \xF0\x74\xF1\x5D．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_515:
        DrawNavigationTips(Tips_Confirm);
        func_801ADF94(0x80, 0);
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            func_801ACBE4(GFX_UNK_15, 8);
            g_GameEngineStep = g_SelNextCrossPressEngStep;
        }
        break;
    case Upd_Eng_516:
        DrawNavigationTips(Tips_Confirm);
        func_801ADF94(0x80, 0);
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            g_GameEngineStep = g_SelNextCrossPressEngStep;
        }
        break;
    case Upd_Eng_0x210:
        func_801ADF94(0x80, 0);
        func_801AD218();
        if (g_SaveSummary[1].padding != -2) {
            g_GameEngineStep = Upd_Eng_0x220;
        } else {
            func_801B2608(KO_SAVE("Format Memory Card", "１\xF0\xCC \xF0\x54\xF1\x83\xF0\x50 \xF2\x06\xF0\xB2\xF0\xC6"), 4);
            func_801B2608(KO_SAVE("in slot １？", "\xF0\xB7\xF0\x46\xF0\x99\xF0\xA3\xF2\x59\xF2\xC0\xF1\x36\xF1\x11？"), 5);
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_529:
        func_801ADF94(0x80, 0);
        if (g_pads[0].tapped & SEL_CANCEL) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            func_801ACBE4(GFX_UNK_6, 8);
            MemCardSetPort(1);
            g_GameEngineStep++;
        } else {
            if (g_pads[0].tapped & SEL_CONFIRM) {
                g_GameEngineStep = Upd_Eng_0x220;
            }
            func_801AD218();
        }
        break;
    case Upd_Eng_530:
        func_801ADF94(0x80, 0);
        D_800978C4 = 0;
        temp_s1 = MemCardInitAndFormat();
        if (temp_s1 != 0) {
            D_800978C4 = 1;
        }
        if (temp_s1 == 1) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("Formatting completed．", "\xF0\xB7\xF0\x46\xF0\x99\xF0\x40 \xF2\x68\xF1\x21\xF2\xC0\xF1\x36\xF0\x49．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep += 2;
        }
        if (temp_s1 == -1) {
            func_801B1F4C(5);
            func_801B2608(KO_SAVE("Formatting error．", "\xF0\xB7\xF0\x46\xF0\x99 \xF0\x74\xF1\x5D．"), 4);
            func_801ACBE4(GFX_UNK_15, 0);
            g_GameEngineStep++;
        }
        break;
    case Upd_Eng_531:
        DrawNavigationTips(Tips_Confirm);
        func_801ADF94(0x80, 0);
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            func_801ACBE4(GFX_UNK_15, 8);
            g_GameEngineStep = g_SelNextCrossPressEngStep;
        }
        break;
    case Upd_Eng_532:
        DrawNavigationTips(Tips_Confirm);
        func_801ADF94(0x80, 0);
        if (g_pads[0].tapped & SEL_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            g_GameEngineStep = g_SelNextCrossPressEngStep;
        }
        break;
    case Upd_Eng_0x220:
        func_801ADF94(0x80, 0);
        if (D_801BAF10) {
            DrawNavigationTips(Tips_YesNo);
        }
        g_GameEngineStep = g_SelEng220NextStep;
        break;
    }
    g_GameTimer++;
    if (g_GameTimer == 10) {
        g_GameTimer = 0;
        D_801BAF08++;
        D_801BAF08 = D_801BAF08 % 3;
    }
}

void func_801B17C8(void) {
    s32 var_s0;

    switch (g_MenuStep) {
    case 0:
        if (D_80097924 == -1 || D_8006C378 == -1) {
            g_GameStep++;
        } else {
            D_800978C4 = 0;
            g_MenuStep++;
        }
        break;

    case 1:
        func_801B3A54(D_80097924, D_8006C378);
        g_MenuStep++;
        break;

    case 2:
        D_800978C4 = 0;
        var_s0 = func_801B3A94(1);
        if (var_s0 != 0) {
            D_800978C4 = 1;
            g_GameStep++;
        }
        break;
    }
}
