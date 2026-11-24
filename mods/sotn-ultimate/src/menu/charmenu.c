// SPDX-License-Identifier: AGPL-3.0-or-later
// Saturn port mod: the pause menu for Richter and the Marias
// (include/game_mod.h MOD_CHAR_MENU):
// the drawing that DRA has no room for.
//
// DRA (src/dra/menu.c, mods/sotn-ultimate/src/menu/menu_mod_charmenu.h)
// runs Alucard's menu, trimmed, and calls in here through the header below.
// This code (12 sectors after ARC_F2.BIN's loaded part, at most 24 KB) is read
// each time the menu opens, once the music has stopped (MENU_STEP_OPENING),
// into g_Pix[1] to [3]: image staging buffers (8 KB each) that nothing uses
// while the menu is open in these games. The menu's text is rendered from
// g_Pix[0] on, reset each frame (see Commands below: at most three rows a
// frame); g_Pix[2] is also the save room messages' and Alucard's weapon loads';
// the CD's overlay copies and the compressed graphics use them all, but only
// outside the menu: DRA clears the magic before the exit reloads the stage
// graphics. It borrows the demo-key load (src/dra/cd.c: g_LoadOvlIdx
// MOD_CHARMENU_OVL sends it here instead of DEMO_KEY_PTR).
// Built by mods/sotn-ultimate/tools/build_charmenu.sh against DRA's
// symbols, at g_Pix + 0x2000. No .bss (the file is the whole state).
//
//   main box   two rows, Commands and System, centred where his five were;
//   status     Alucard's positions, with what applies to them: their
//              portrait (the Player Select's, see PorUpload), name, HP,
//              HEART, MP (the Saturn Maria only: her specials use it),
//              STATUS, ATT / DEF, GOLD, ROOMS, KILLS, TIME. Left out: level,
//              EXP, NEXT (only Alucard's code levels up, src/dra/692E8.c)
//              and STR..LCK (from his equipment).
#include <game.h>
#include <disk.h>

#if defined(MOD_PTBR)
#include "mod_ptbr_charmenu.h"
#include "mod_ptbr_sysopt.h"
#define PT(en, pt) (MOD_PTBR_TEXT_PT() ? (pt) : (en))
#else
#define PT(en, pt) (en)
#endif

// DRA (src/dra/menu.c)
void MenuDrawStr(const char* str, s32 x, s32 y, MenuContext* ctx);
void MenuDrawInt(s32 number, s32 x, s32 y, MenuContext* ctx);
void MenuDrawChar(s32 ch, s32 x, s32 y, MenuContext* ctx);
void MenuDrawTime(s32 number, s32 x, s32 y, MenuContext* ctx, s32 digits);
void func_800F66BC(const char* str, s32 x, s32 y, MenuContext* ctx, s32 arg4);
extern const char* g_MenuStr[];
extern const char* D_800A2D68[]; // the 12x16 font's ATT / DEF

void CmMain(MenuContext* ctx, s32 x, s32 y);
void CmStats(MenuContext* ctx);
void CmSysDraw(MenuContext* ctx);
void CmSysStep(void);
void CmBtnDraw(MenuContext* ctx);
void CmBtnStep(void);
s32 CmPopDraw(MenuContext* ctx);
void CmCmdDraw(MenuContext* ctx);
void CmCmdStep(void);

// the header DRA reads (first in .text, at g_Pix[1]): the magic, then the
// entries (include/game_mod.h ModCharMenuBlob)
__asm__(".text\n"
        ".word " MOD_CHARMENU_MAGIC_STR "\n"
        ".word CmMain\n"
        ".word CmStats\n"
        ".word CmSysDraw\n"
        ".word CmSysStep\n"
        ".word CmBtnDraw\n"
        ".word CmBtnStep\n"
        ".word CmPopDraw\n"
        ".word CmCmdDraw\n"
        ".word CmCmdStep\n"
        ".align 2\n");

// (separate strings: the encoded names need more than their letters)
static const char g_NameRic[] = _S("RICHTER");
static const char g_NameMar[] = _S("MARIA");
static const char g_Commands[] = _S("Commands");

// KO build (-DMOD_CHARMENU_KO, build_ko_trial.sh): the Korean strings of
// charmenu_ko_str.h (gen_ko_cm.py from 번역_리히터마리아메뉴.tsv). 8px labels go
// through DRA's 8px name renderer (the call the Extra settings popup of
// Alucard's menu makes, sysopt.c KO_EQUIP_SEL); the info bar font's strings
// (descriptions, the command names) go to func_800F99B8 as they are.
// STR8(en, ko, x, y, ctx): draw the label; KX(en_x, ko_x): its x; INFO(en, ko):
// the info bar string
#if defined(MOD_CHARMENU_KO)
#include "charmenu_ko_str.h"
#define KO_EQUIP_SEL 0x8009F500 // src/dra/dra.h (build_charmenu.sh checks it)
#define KO_EQUIP_OV 0x8009F540  // (the second renderer: same)
#define KO_EQUIP_ENTER 0x8009F580 // src/dra/dra.h: forget the uploaded cells
typedef void (*KoDrawFn)(const char**, const s32*, const s32*, s32,
                         MenuContext*);
static void Ko8(const char* ko, s32 x, s32 y, MenuContext* ctx, s32 fn) {
    const char* n[1];
    s32 xs[1];
    s32 ys[1];

    n[0] = ko;
    xs[0] = x;
    ys[0] = y;
    ((KoDrawFn)fn)(n, xs, ys, 1, ctx);
}
#define KO_SHEET_UPLOAD ((const char*)0x8009EE94)
#define KO_SHEET_UPLOAD2 ((const char*)0x8009F100)
#define KO_SHEET_UPLOAD3 ((const char*)0x8009F148)
void func_800F98AC(const char* str, u32 y);
#define KENTER()                                                               \
    (func_800F98AC(KO_SHEET_UPLOAD, 0), func_800F98AC(KO_SHEET_UPLOAD2, 0x100),   \
     func_800F98AC(KO_SHEET_UPLOAD3, 0x101), ((void (*)(void))KO_EQUIP_ENTER)())
#define STR8(en, ko, x, y, ctx) Ko8(ko, x, y, ctx, KO_EQUIP_SEL)
#define STR8O(en, ko, x, y, ctx) Ko8(ko, x, y, ctx, KO_EQUIP_OV)
#define Q8DECL                                                                 \
    const char* q8n[8];                                                        \
    s32 q8x[8];                                                                \
    s32 q8y[8];                                                                \
    s32 q8c = 0;
#define Q8(en, ko, x, y)                                                       \
    (q8n[q8c] = (ko), q8x[q8c] = (x), q8y[q8c] = (y), q8c++)
#define KN8(ctx)                                                               \
    (q8c ? ((KoDrawFn)KO_EQUIP_SEL)(q8n, q8x, q8y, q8c, ctx) : (void)0)
#define KN8O(ctx)                                                              \
    (q8c ? ((KoDrawFn)KO_EQUIP_OV)(q8n, q8x, q8y, q8c, ctx) : (void)0)
#define KX(en_x, ko_x) (ko_x)
#define INFO(en, ko) (ko)
#else
#define KENTER() ((void)0)
#define STR8(en, ko, x, y, ctx) MenuDrawStr(en, x, y, ctx)
#define STR8O(en, ko, x, y, ctx) MenuDrawStr(en, x, y, ctx)
#define KN8O(ctx) ((void)0)
#define Q8DECL
#define Q8(en, ko, x, y) MenuDrawStr(en, x, y, ctx)
#define KN8(ctx) ((void)0)
#define KX(en_x, ko_x) (en_x)
#define INFO(en, ko) (en)
#endif

#define SAT_MARIA() (g_PlayableCharacter == PLAYER_MARIA && !MOD_MARIA_PSP())

extern s16 g_CurSfxId20_21; // src/dra/sound.c
extern s16 g_CurSfxId22_23;

// the main box's rows (func_800F8754 for Alucard). Also: the menu's exit
// keys the sound channels 20-23 on again (SET_KEY_ON_20_21 / 22_23, to go
// on with a sound the pause cut) while these ids are set; their overlays
// never clear them (Alucard's do), so their last voice sound played again
// at every unpause, standing still too. Nothing of theirs is left to go on
// with there (the pause released it)
void CmMain(MenuContext* ctx, s32 x, s32 y) {
    Q8DECL
    g_CurSfxId20_21 = 0;
    g_CurSfxId22_23 = 0;
    Q8(PT(g_Commands, g_Pt_commands), kc_cmds, x + KX(6, 12), y + 4);
    Q8(g_MenuStr[95], kc_system, x + KX(14, 12), y + 20);
    KN8O(ctx); /* 095_14: on the second renderer (coming back from the Commands, the first one showed garbage) */
}

// ATT: Richter's whip (src/ric/pl_whip.c), 32, or 64 while the flame whip
// (his crash with no subweapon) is lit: his timer PL_T_POISON, the index of
// ALU_T_POISON. The Marias: their basic attack (the Saturn Maria's shot,
// mar_attacks.h mar_def_shot: 15; the PSP Maria's owl, src/maria
// pl_blueprints.c EntityMariaOwl: 12).
static s32 Attack(void) {
    if (g_PlayableCharacter == PLAYER_RICHTER) {
        return g_Player.timers[ALU_T_POISON] ? 64 : 32;
    }
    return MOD_MARIA_PSP() ? 12 : 15;
}

static void Pair(const char* label, s32 a, s32 b, s32 y, MenuContext* ctx) {
    MenuDrawStr(label, 96, y, ctx);
    MenuDrawInt(a, 168, y, ctx);
    MenuDrawChar(CH('/'), 176, y, ctx);
    MenuDrawInt(b, 208, y, ctx);
}

static void Label(s32 str, s32 n, s32 x, s32 dx, s32 y, MenuContext* ctx) {
    MenuDrawStr(g_MenuStr[str], x, y, ctx);
    MenuDrawInt(n, x + dx, y, ctx);
}

// Their portrait: the Player Select's (the Saturn's, 80x88 8bpp; both
// Marias share hers), packed per row (gen_title_portraits.py pack_row:
// k < 128 then k + 1 bytes, else k - 125 copies of the next byte). When the
// menu opens it goes to VRAM (0,256), its CLUT under it: the lower-left of
// VRAM is free while the menu is up (its buffers are at (0,0) and
// (128,256); seen in a VRAM dump, 2026-11-14), and in play it is screen
// buffer, drawn over every frame. Drawn where Alucard's is, at the
// Saturn's size (80 px; the Player Select's 91 would reach "HP").
#include "../../../../src/st/sel/mod_charmenu_portraits.h"
#define POR_W 80
#define POR_H 88
#define POR_VY 256
#define POR_CLUT_Y (POR_VY + POR_H)
bool ScissorSprite(SPRT* sp, MenuContext* ctx);
void func_800F53D4(s32 tpage, s32 otIdx);

static s32 g_PorUp = 0;
static u32 g_PorRow[POR_W / 4] = {1}; // (words: LoadImage's DMA)

static void PorUpload(void) {
    s32 maria = g_PlayableCharacter == PLAYER_MARIA;
    const u8* s = maria ? g_CmPorMaria : g_CmPorRichter;
    u8* row = (u8*)g_PorRow;
    RECT r;
    s32 x, n, k;

    r.x = 0;
    r.w = POR_W / 2;
    r.h = 1;
    for (r.y = POR_VY; r.y < POR_VY + POR_H; r.y++) {
        for (x = 0; x < POR_W;) {
            k = *s++;
            if (k < 128) {
                for (n = k + 1; n; n--) {
                    row[x++] = *s++;
                }
            } else {
                for (n = k - 125; n; n--) {
                    row[x++] = *s;
                }
                s++;
            }
        }
        LoadImage(&r, (u_long*)g_PorRow);
        DrawSync(0); // (the row buffer is used again)
    }
    r.y = POR_CLUT_Y;
    r.w = 128;
    LoadImage(&r, (u_long*)(maria ? g_CmClutMaria : g_CmClutRichter));
    DrawSync(0);
}

// at Alucard's portrait's place (MenuDrawAlucardPortrait: 16,36 in the
// background window, one OT slot under its text)
static void PorDraw(MenuContext* ctx) {
    s32 otIdx = ctx->otIdx + 1;
    SPRT* sp = &g_CurrentBuffer->sprite[g_GpuUsage.sp];

    if (!g_PorUp) {
        g_PorUp = 1;
        PorUpload();
    }
    SetSprt(sp);
    SetSemiTrans(sp, 0);
    SetShadeTex(sp, 1);
    sp->x0 = 12;
    sp->y0 = 36;
    sp->w = POR_W;
    sp->h = POR_H;
    sp->u0 = 0;
    sp->v0 = 0;
    if (!ScissorSprite(sp, ctx)) {
        sp->clut = getClut(0, POR_CLUT_Y);
        AddPrim(&g_CurrentBuffer->ot[otIdx], sp);
        g_GpuUsage.sp++;
        func_800F53D4(getTPage(1, 0, 0, POR_VY), otIdx);
    }
}

// MenuDrawStats(1) in these games
void CmStats(MenuContext* ctx) {
    s32 i, y;

    PorDraw(ctx);

    STR8(g_PlayableCharacter == PLAYER_MARIA ? g_NameMar : g_NameRic,
         g_PlayableCharacter == PLAYER_MARIA ? kc_mar : kc_ric, 128, 40, ctx);
    y = 56;
    Pair(g_MenuStr[12], g_Status.hp, g_Status.hpMax, y, ctx);
    if (SAT_MARIA()) {
        y += 12;
        Pair(g_MenuStr[13], g_Status.mp, g_Status.mpMax, y, ctx);
    }
    Pair(g_MenuStr[14], g_Status.hearts, g_Status.heartsMax, y + 12, ctx);

    MenuDrawStr(g_MenuStr[15], 248, 56, ctx);
    i = 37;
    if (g_Player.status & PLAYER_STATUS_CURSE) {
        i = 40;
    }
    if (g_Player.status & PLAYER_STATUS_POISON) {
        i = 38;
    }
    if (g_Player.status & PLAYER_STATUS_STONE) {
        i = 39;
    }
    MenuDrawStr(g_MenuStr[i], 260, 68, ctx);

    // ATT / DEF where Alucard's are, one ATT value (no hands)
    func_800F66BC(D_800A2D68[0], 248, 88, ctx, 1);
    MenuDrawInt(Attack(), 248 + 76, 93, ctx);
    func_800F66BC(D_800A2D68[1], 248, 108, ctx, 1);
    MenuDrawInt(g_Status.defenseEquip, 248 + 76, 114, ctx);

    Label(7, g_Status.gold, 32, 112, 200, ctx);
    Label(10, g_RoomCount, 240, 88, 150, ctx);
    Label(11, g_Status.killCount, 240, 88, 164, ctx);
    Label(9, g_Status.timerHours, 208, 56, 192, ctx);
    MenuDrawChar(CH(':'), 272, 192, ctx);
    MenuDrawTime(g_Status.timerMinutes, 288, 192, ctx, 2);
    MenuDrawChar(CH(':'), 296, 192, ctx);
    MenuDrawTime(g_Status.timerSeconds, 312, 192, ctx, 2);
}

// ---------------------------------------------------------------------------
// System: Button set-up, Window color, Sound, Extra settings (no cloak
// rows: Alucard's cape; no time attack: Alucard's bosses). DRA's
// MENU_STEP_SYSTEM / _BUTTONS and MenuSystemDraw / MenuButtonConfigDraw call in
// here for them.
//
// Button set-up: each character's own buttons, saved with the game in one
// word (include/game_mod.h MOD_PAD_WORD): action f = Alucard's row order,
// whose default button is g_ButtonMask[f]; 3 bits each, button ^ f (0 = the
// default buttons). src/dra/pads.c applies it in gameplay.
// ---------------------------------------------------------------------------
#define MENU_DG_MAIN 0
#define MENU_DG_BG 1
#define MENU_DG_INFO_BAR 4
#define MENU_DG_SETTINGS 7
#define MENU_DG_CFG_BUTTONS 9
#define MENU_DG_CFG_SOUND 11
#define MENU_DG_WINDOW_COLORS 12
#define MENU_STEP_OPENED 0x10
#define MENU_STEP_SYSTEM 0x101
#define MENU_STEP_SYSTEM_BUTTONS 0x102
#define MENU_STEP_SYSTEM_WINDOW_COLOR 0x105
#define MENU_STEP_SYSTEM_SOUND 0x106
#if defined(MOD_CHARMENU_KO)
#define PAD_MENU_SELECT (PAD_CIRCLE | PAD_SQUARE)
#define PAD_MENU_BACK (PAD_CROSS | PAD_TRIANGLE)
#define PAD_POP_CLOSE (PAD_CROSS | PAD_TRIANGLE)
#else
#define PAD_MENU_SELECT (PAD_CROSS)
#define PAD_MENU_BACK (PAD_TRIANGLE)
#define PAD_POP_CLOSE (PAD_TRIANGLE | PAD_CIRCLE)
#endif
#define PAD_MENU_BACK_ALT (PAD_MENU_BACK | PAD_MENU_SELECT)
#define SFX_UI_CONFIRM 0x633
#define SFX_UI_ERROR 0x686
#define SFX_UI_MOVE 0x67B

void MenuShow(s32 menuDialogue);
void MenuHide(s32 menuDialogue);
void MenuHandleCursorInput(s32* nav, u8 nOptions, u32 type);
void func_800F5E68(MenuContext* ctx, s32 cursorIdx, s32 x, s32 y, s32 w, s32 h,
                   s32 yGap, s32 bColorMode);
void func_800F9808(u32 y);
void func_800F99B8(const char* str, u32 y, bool arg2);
void PlaySfx(s16 sfxId);
extern MenuContext g_Menus[];
extern const char* D_800A2D48[]; // the info bar texts
extern u8 g_ChButtons[];
extern u8 g_ChButtons2[];

#define ShowText(str) func_800F99B8(str, 2, false)

typedef struct {
    u8 n;    // rows
    u8 f[5]; // their actions (the default button's index)
    const char* label[5];
} PadRows;

static const char g_LAttack[] = _S("Attack");
static const char g_LJump[] = _S("Jump");
static const char g_LItemCrash[] = _S("Item crash");
static const char g_LCrash[] = _S("Crash");
static const char g_LDashR[] = _S("Dash right");
static const char g_LDashL[] = _S("Dash left");
static const char g_LShot[] = _S("Shot");
static const char g_LKick[] = _S("Kick");
static const char g_LBackdash[] = _S("Backdash");

// MOD_PAD_CHAR order: Richter, the PSP Maria, the Saturn Maria
static const PadRows g_PadRows[3] = {
    // src/ric: Square whip (Up + it: the subweapon), Cross, Triangle crash
    {3, {0, 2, 3}, {g_LAttack, g_LJump, g_LItemCrash}},
    // src/maria: the same three, R1 / L1 dash (pl_collision.c MarSetRun)
    {5, {0, 2, 3, 6, 5}, {g_LAttack, g_LJump, g_LCrash, g_LDashR, g_LDashL}},
    // maria_saturn (ric_mod_maria.h): Square shot, Circle kick, Cross,
    // Triangle backdash
    {4, {0, 1, 2, 3}, {g_LShot, g_LKick, g_LJump, g_LBackdash}},
};
#if defined(MOD_CHARMENU_KO)
static const char* const g_PadKo[3][5] = {
    {kc_l_attack, kc_l_jump, kc_l_itemcrash},
    {kc_l_attack, kc_l_jump, kc_l_crash, kc_l_dashr, kc_l_dashl},
    {kc_l_shot, kc_l_kick, kc_l_jump, kc_l_backdash},
};
#endif

// the info bar text for each System row (retail D_800A2D48[row + 1])
static const u8 g_SysInfo[3] = {1, 4, 5};

static const char g_AssignAll[] = "Assign the function to each button．";

// the buttons being edited (initialised: no .bss)
static s32 g_Perm[8] = {-1};

static const PadRows* Rows(void) { return &g_PadRows[MOD_PAD_CHAR()]; }

static void PermLoad(void) {
    u32 v = MOD_PAD_WORD(MOD_PAD_CHAR());
    s32 f;

    for (f = 0; f < 8; f++) {
        g_Perm[f] = ((v >> (f * 3)) & 7) ^ f;
    }
}

// false while two rows share a button; else the other actions take the
// buttons left (in order: the defaults give 0) and it is saved
static s32 PermStore(void) {
    const PadRows* r = Rows();
    u32 used = 0;
    u32 v = 0;
    s32 i, f, b;

    for (i = 0; i < r->n; i++) {
        b = 1 << g_Perm[r->f[i]];
        if (used & b) {
            return 0;
        }
        used |= b;
    }
    b = 0;
    for (f = 0; f < 8; f++) {
        for (i = 0; i < r->n; i++) {
            if (r->f[i] == f) {
                break;
            }
        }
        if (i == r->n) { // not their action: the next button left
            while (used & (1 << b)) {
                b++;
            }
            g_Perm[f] = b++;
        }
        v |= (g_Perm[f] ^ f) << (f * 3);
    }
    MOD_PAD_WORD(MOD_PAD_CHAR()) = v;
    return 1;
}

// MenuButtonConfigDraw (its positions)
void CmBtnDraw(MenuContext* ctx) {
    const PadRows* r = Rows();
    s32 i, b, y;
    Q8DECL

    for (i = 0; i < r->n; i++) {
        y = 0x30 + i * 16;
        Q8(PT(r->label[i], g_PtPad[MOD_PAD_CHAR()][i]),
           g_PadKo[MOD_PAD_CHAR()][i], 0x98, y);
        b = g_Perm[r->f[i]];
        MenuDrawChar(g_ChButtons[b], 0xC0 + 0x30 + b * 12, y, ctx);
        if (b >= 4) {
            MenuDrawChar(g_ChButtons2[b - 4], 0xC0 + 8 + 0x30 + b * 12, y, ctx);
        }
    }
    KN8(ctx);
#if defined(MOD_PTBR)
    func_800F5E68(
        ctx, g_MenuNavigation.cursorButtons, 0x98 - 2, 46, 0x59, 12, 4, 1);
#else
    func_800F5E68(
        ctx, g_MenuNavigation.cursorButtons, 0x98 - 2, 46, 0x54, 12, 4, 1);
#endif
}

// MENU_STEP_SYSTEM_BUTTONS
void CmBtnStep(void) {
    const PadRows* r = Rows();

    MenuHandleCursorInput(&g_MenuNavigation.cursorButtons, r->n, 0);
    MenuHandleCursorInput(&g_Perm[r->f[g_MenuNavigation.cursorButtons]], 8, 5);
    if (!(g_pads[0].tapped & PAD_MENU_BACK_ALT)) {
        return;
    }
    if (PermStore()) {
        KENTER();
        PlaySfx(SFX_UI_CONFIRM);
        MenuHide(MENU_DG_CFG_BUTTONS);
        g_MenuStep = MENU_STEP_SYSTEM;
    } else {
        func_800F9808(2);
        ShowText(INFO(g_AssignAll, kc_assign));
        PlaySfx(SFX_UI_ERROR);
    }
}

// ---------------------------------------------------------------------------
// Extra settings: the Dub (Richter PSP / PS1 / JP, the Marias PSP / JP, as
// on their Player Select panel), in a popup like Alucard's
// (src/menu/sysopt.c, which is loaded in Alucard games only): the Sound
// box's window with our size while the menu is in MOD_STEP_BOX (a step
// MenuHandle has no case for). Once it closes, their voice banks (vab 1 / 2,
// src/dra/cd.c ModCharSdLocs) and the stage's (vab 3) reload for the new
// dub; the menu's exit waits for the CD. Castle Center keeps its bank
// (locked there, as Alucard's).
// ---------------------------------------------------------------------------
#define MOD_STEP_BOX 0x10F
#define SYS_ROWS 4 // Button set-up, Window color, Sound, Extra settings
#define SYS_EXTRA 3
// (16 px left of Alucard's popup and 16 wider: the Balance row's
// "Ultimate" ends 64 px after VALUE_DX; user report 2026-11-17)
#define POP_X 156
#define POP_Y 92 // its row level with "Extra settings" (y 0x60)
#define POP_W 184
#define POP_H (4 + PopRows() * 16)
#define VALUE_DX 112
#define DUB_BITS (MOD_FLAG_PS1_DUB | MOD_FLAG_JP_DUB)
#define IS_MARIA() (g_PlayableCharacter == PLAYER_MARIA)
#define CDFILE_SD_RIH_VH 20 // g_LbaSdRihVh (src/dra/cd.c D_800ACC74)

typedef struct {
    s32 loc;
    s32 cb;
    s32 size;
    u8 vabId;
    u8 unkD;
    u8 nextCdFileType;
    u8 unkF;
} CdFile;
typedef struct {
    s16 cursorX;
    s16 cursorY;
    u16 cursorW;
    u16 cursorH;
    s32 otIdx;
} MenuContextInit;
extern MenuContextInit g_MenuInit[];
extern CdFile g_LbaSdMarVh;
extern s32 D_8013784C; // menu.c: 1 = grey text
void ModCharSdLocs(void);
void SsUtAllKeyOff(short mode);

static const char g_LExtra[] = _S("Extra settings");
static const char g_LDub[] = _S("Dub");
static const char g_NPsp[] = _S("PSP");
static const char g_NPs1[] = _S("PS1");
static const char g_NJp[] = _S("JP");
static const char* const g_DubName[3] = {g_NPsp, g_NPs1, g_NJp};
#if defined(MOD_CHARMENU_KO)
static const char* const g_DubKo[3] = {kc_n_psp, kc_n_ps1, kc_n_jp};
#endif
static const u8 g_DubVal[3] = {0, MOD_FLAG_PS1_DUB, MOD_FLAG_JP_DUB};
static const char g_ExtraInfo[] = "Character voices";

// the dub bits their banks were loaded with (-1: not asked this menu)
static s32 g_DubLoaded = -1;
// the popup's cursor; Richter's costume when it opened (-1: not yet)
static s32 g_PopRow = -1;
static s32 g_CostumeLoaded = -1;
static const char g_LCostume[] = _S("Costume");
static const char g_LVersion[] = _S("Version");
static const char g_NSaturn[] = _S("Saturn");
static const char g_LBalance[] = _S("Balance");
static const char g_NClassic[] = _S("Classic");
static const char g_NUltimate[] = _S("Ultimate");
#if !defined(MOD_CHARMENU_KO)
static const char g_LInfHp[] = _S("Inf. HP");
static const char g_LInfHearts[] = _S("Inf. Hearts");
static const char g_LInfMp[] = _S("Inf. MP");
static const char g_NOn[] = _S("ON");
static const char g_NOff[] = _S("OFF");
#endif

// the popup's rows: Richter Costume (PS1 / Saturn), the Marias Version
// (Saturn / PSP), as on their Player Select panels; the Balance (Classic /
// Ultimate); then the Dub
#define POP_BAL 1
#define POP_DUB 2
// then their cheats (Richter: HP, hearts; the Saturn Maria: + MP; the PSP Maria has no MP), as Alucard's
// names: Inf. HP / Inf. Hearts / Inf. MP; Left / Right / the select button flips one
static s32 NCheat(void) { return SAT_MARIA() ? 3 : 2; }
#if defined(MOD_PTBR)
#define POP_LANG 3
#define POP_CH0 4
static s32 PopRows(void) { return 4 + NCheat(); }
static void PopLang(void) {
    if (MOD_PTBR_BLOB[1] != MOD_PTBR_MAGIC) {
        PlaySfx(SFX_UI_ERROR);
        return;
    }
    MOD_PTBR_BLOB[2] ^= 1;
    PlaySfx(SFX_UI_MOVE);
    (*(void (**)(s32))MOD_PTBR_CODE)(1);
}
#else
#define POP_CH0 3
static s32 PopRows(void) { return 3 + NCheat(); }
#endif
static s32 PopIsDub(void) { return g_PopRow == POP_DUB; }

// the Balance switched (user, 2026-11-17; anywhere: a grate that comes back
// may shut the player in, switching back opens it): the game converted.
// Classic = retail Richter mode (game_mod.h MOD_RIC_ULTIMATE): 50 HP less
// (the max-ups kept) and the Succubus, Minotaur & Werewolf and Death marked
// done (1, as ModCastleAccess does in a Classic game) unless beaten;
// Ultimate: 50 HP more, those marks cleared, and the room's grates taken out
// now (ModCastleAccess re-scans a room it has not seen)
// the rooms a switch to Classic could shut the player in (user,
// 2026-11-17): past the Outer Wall's grate, the Library's grate room, the
// Royal Chapel's spike corridor and Maria's room past it, past the Abandoned
// Mine's demon wall, under the clock room down to the Castle Center (all of
// it), the Caverns' east lake (its ferryman needs the Merman Statue) and
// past it; the reverse castle's mirrors. {stage, left, top}; left 0xFF =
// the whole stage. (The Colosseum's grate: both sides reachable, no lock)
static const u8 g_ClassicLock[][3] = {
    {STAGE_NO1, 59, 27},  {STAGE_LIB, 44, 18},  {STAGE_DAI, 10, 15},
    {STAGE_DAI, 8, 15},   {STAGE_CHI, 33, 40},  {STAGE_CHI, 37, 40},
    {STAGE_CHI, 36, 41},  {STAGE_NO0, 31, 27},  {STAGE_NO0, 32, 27},
    {STAGE_NO0, 33, 27},  {STAGE_NO0, 32, 28},  {STAGE_CEN, 0xFF, 0},
    {STAGE_NO4, 40, 41},  {STAGE_NO4, 53, 41},  {STAGE_NO4, 55, 41},
    {STAGE_RNO1, 4, 36},  {STAGE_RLIB, 15, 42}, {STAGE_RDAI, 49, 48},
    {STAGE_RDAI, 54, 48}, {STAGE_RCHI, 27, 23}, {STAGE_RCHI, 26, 22},
    {STAGE_RCHI, 27, 22}, {STAGE_RNO0, 32, 36}, {STAGE_RNO0, 31, 36},
    {STAGE_RNO0, 30, 36}, {STAGE_RNO0, 31, 35}, {STAGE_RCEN, 0xFF, 0},
    {STAGE_RNO4, 11, 21}, {STAGE_RNO4, 9, 22},  {STAGE_RNO4, 8, 22},
};
// the player stands where a grate the castle access took out would come
// back (user, 2026-11-19: no Classic there, its collision drops them
// through). The list's tiles: fg + row * hSize * 16 + col, 16 px each
static s32 OnGrate(void) {
    s32 w = g_Tilemap.hSize * 16;
    s32 x = PLAYER.posX.i.hi + g_Tilemap.scrollX.i.hi;
    s32 y = PLAYER.posY.i.hi + g_Tilemap.scrollY.i.hi;
    s32 i, k;

    if (g_ModDub.grateStage != (MOD_GRATE_MAGIC | g_StageId)) {
        return 0;
    }
    for (i = 0; i < g_ModDub.grateCount; i++) {
        k = g_ModDub.grate[i].p - g_Tilemap.fg;
        if (k < 0 || k >= w * g_Tilemap.vSize * 16) {
            continue;
        }
        if ((k / w) * 16 + 16 > y - 24 && (k / w) * 16 < y + 24 &&
            (k % w) * 16 + 16 > x - 8 && (k % w) * 16 < x + 8) {
            return 1;
        }
    }
    return 0;
}

// Ultimate -> Classic not allowed here
static s32 ClassicLocked(void) {
    s32 i;

    if (MOD_PS1_BALANCE()) {
        return 0; // (Classic -> Ultimate only opens things)
    }
    if (OnGrate()) {
        return 1;
    }
    for (i = 0; i < LEN(g_ClassicLock); i++) {
        if (g_ClassicLock[i][0] == g_StageId &&
            (g_ClassicLock[i][1] == 0xFF ||
             (g_ClassicLock[i][1] == g_Tilemap.left &&
              g_ClassicLock[i][2] == g_Tilemap.top))) {
            return 1;
        }
    }
    return 0;
}

static const u8 g_ClassicBoss[3] = {
    TIMEATTACK_EVENT_SUCCUBUS_DEFEAT, TIMEATTACK_EVENT_MINOTAUR_WEREWOLF_DEFEAT,
    TIMEATTACK_EVENT_DEATH_DEFEAT};
static void PopBalance(void) {
    s32* r;
    s32 i;

    if (ClassicLocked()) {
        PlaySfx(SFX_UI_ERROR);
        return;
    }
    PlaySfx(SFX_UI_MOVE);
    MOD_STATUS_FLAGS ^= MOD_FLAG_PS1_BALANCE;
    for (i = 0; i < LEN(g_ClassicBoss); i++) {
        r = &g_Settings.timeAttackRecords[g_ClassicBoss[i]];
        if (MOD_PS1_BALANCE()) {
            if (!*r) {
                *r = 1;
            }
        } else if (*r == 1) {
            *r = 0;
        }
    }
    if (!MOD_PS1_BALANCE()) {
        g_ModDub.accessRoom = 0;
    } else {
        // what the castle access set goes (the castle as retail Richter
        // mode's: none of it is his to set there; user, 2026-11-17)
        g_Status.relics[RELIC_MERMAN_STATUE] = 0;
        g_CastleFlags[CEN_OPEN] = 0;
        g_CastleFlags[CHI_DEMON_SWITCH] = 0;
        g_CastleFlags[RCHI_DEMON_SWITCH] = 0;
        g_CastleFlags[CAT_SPIKE_ROOM_LIT] = 0;
        g_CastleFlags[MET_FERRYMAN_1] = 0;
        g_CastleFlags[MET_FERRYMAN_2] = 0;
        // the grates the stage's room data lost come back (only this
        // stage's: another stage reloads whole)
        if (g_ModDub.grateStage == (MOD_GRATE_MAGIC | g_StageId)) {
            for (i = 0; i < g_ModDub.grateCount; i++) {
                *g_ModDub.grate[i].p = g_ModDub.grate[i].v;
            }
        }
        g_ModDub.grateCount = 0;
        g_ModDub.grateStage = 0;
    }
    if (IS_MARIA() && !MOD_MARIA_PSP()) {
        return; // (the Saturn Maria's HP is the Saturn's in both)
    }
    if (MOD_PS1_BALANCE()) {
        if (g_Status.hpMax > 50) {
            g_Status.hpMax -= 50;
        }
        if (g_Status.hp > g_Status.hpMax) {
            g_Status.hp = g_Status.hpMax;
        }
    } else {
        g_Status.hpMax += 50;
        g_Status.hp += 50;
    }
}
// a costume / version change: any time but while a scene has control or
// the prologue's boss bar is up (D_8003C744, only Richter's prologue shows
// one). Boss fights allow it (user, 2026-11-14: the swap only rebuilds the
// player's entities 1..63; tested mid-Doppelganger)
static s32 SwapLocked(void) { return D_8003C744 || g_CutsceneHasControl; }
// the first row's choice: Richter 1 = the Saturn costume, Maria 1 = PSP
static s32 CostumeCur(void) {
    return IS_MARIA() ? MOD_MARIA_PSP() != 0 : MOD_RIC_COSTUME2() != 0;
}

// closed at once: the Sound box's own window again
static void PopReset(MenuContext* pop) {
    MenuContextInit* init = &g_MenuInit[MENU_DG_CFG_SOUND];

    pop->unk1C = 2;
    pop->cursorX = init->cursorX;
    pop->cursorY = init->cursorY;
    pop->cursorW = init->cursorW;
    pop->cursorH = init->cursorH;
}

// the choice shown (the Marias: 0 PSP, 1 JP; a PS1 flag shows as PSP, as
// on their panel)
static s32 DubCur(void) {
    s32 b = MOD_STATUS_FLAGS & DUB_BITS;

    if (b & MOD_FLAG_JP_DUB) {
        return IS_MARIA() ? 1 : 2;
    }
    if (b & MOD_FLAG_PS1_DUB) {
        return IS_MARIA() ? 0 : 1;
    }
    return 0;
}

// fully open: Up / Down pick, Left / Right / X change, Triangle / Circle
// close
static void PopInput(MenuContext* pop) {
    s32 v;

    MenuHandleCursorInput(&g_PopRow, PopRows(), 0);
    if (g_pads[0].tapped & PAD_MENU_SELECT) {
        g_pads[0].repeat |= PAD_RIGHT;
    }
    if ((g_pads[0].repeat & (PAD_LEFT | PAD_RIGHT)) && g_PopRow >= POP_CH0) {
        g_ModDub.cheats ^= 1 << (g_PopRow - POP_CH0);
        PlaySfx(SFX_UI_MOVE);
    } else
#if defined(MOD_PTBR)
        if ((g_pads[0].repeat & (PAD_LEFT | PAD_RIGHT)) && g_PopRow == POP_LANG) {
        PopLang();
    } else
#endif
        if ((g_pads[0].repeat & (PAD_LEFT | PAD_RIGHT)) &&
            g_PopRow == POP_BAL) {
        PopBalance();
    } else if ((g_pads[0].repeat & (PAD_LEFT | PAD_RIGHT)) && !PopIsDub()) {
        // the costume / version: the files load on the menu's exit
        // (src/menu/menu_mod_charmenu.h ModCharSwap converts Maria's state)
        if (SwapLocked()) {
            PlaySfx(SFX_UI_ERROR);
        } else {
            PlaySfx(SFX_UI_MOVE);
            if (IS_MARIA()) {
                MOD_STATUS_FLAGS ^= MOD_FLAG_MARIA_PSP;
            } else {
                g_Status.D_80097C40 = CostumeCur() ? 0 : MOD_RIC_COSTUME2_MAGIC;
            }
            g_ModDub.charSwap = CostumeCur() != g_CostumeLoaded;
        }
    } else if (g_pads[0].repeat & (PAD_LEFT | PAD_RIGHT)) {
        if (g_StageId == STAGE_CEN) {
            PlaySfx(SFX_UI_ERROR);
        } else {
            v = DubCur();
            MenuHandleCursorInput(&v, IS_MARIA() ? 2 : 3, 5); // wraps
            MOD_STATUS_FLAGS = (MOD_STATUS_FLAGS & ~DUB_BITS) |
                               g_DubVal[IS_MARIA() && v ? 2 : v];
        }
    }
    if (g_pads[0].tapped & PAD_POP_CLOSE) {
        PlaySfx(SFX_UI_CONFIRM);
        pop->unk1C = 1;
        pop->unk1D = 0;
    }
}

// after a Dub change: their two banks, then the stage's (one CD chain, as
// at a game load: g_LbaSdRihVh.. g_LbaSdMarVb, linked on to CdFile_StageSfx
// with g_LoadOvlIdx = the stage; cd.c's pre-hook picks the dub's bank and
// ModCharSdLocs clears the link at the next load). Voices keyed off first
static void CharBanks(void) {
    s32 b = MOD_STATUS_FLAGS & DUB_BITS;

    if (g_DubLoaded < 0 || g_DubLoaded == b) {
        return;
    }
    if (g_CdStep != CdStep_None || g_IsUsingCd) {
        return;
    }
    ModCharSdLocs();
    g_LbaSdMarVh.nextCdFileType = CdFile_StageSfx;
    SsUtAllKeyOff(0);
    g_LoadOvlIdx = g_StageId;
    g_LoadFile = CDFILE_SD_RIH_VH;
    g_CdStep = CdStep_LoadInit;
    g_DubLoaded = b;
}

// MenuSystemDraw (every frame the System window is drawn: the list, the
// popup's life, and after it closes the voice banks)
void CmSysDraw(MenuContext* ctx) {
    MenuContext* pop = &g_Menus[MENU_DG_CFG_SOUND];
    Q8DECL

    if (g_MenuStep == MOD_STEP_BOX) {
        if (pop->cursorW != POP_W || pop->unk1C == 2) {
            PopReset(pop); // closed: back to the System list
            KENTER();
            g_MenuStep = MENU_STEP_SYSTEM;
        } else if (pop->unk1C == 0) {
            PopInput(pop);
        }
    } else {
        if (pop->cursorW == POP_W) {
            PopReset(pop); // the menu left the popup some other way (Start)
        }
        CharBanks(); // the choice is final: the banks follow
    }
    func_800F5E68(ctx, g_MenuNavigation.cursorSettings, 30, 46, 128, 12, 4,
                  g_MenuStep == MENU_STEP_SYSTEM);
    Q8(g_MenuStr[16], kc_sysbtn, 0x20, 0x30); // Button settings
    Q8(g_MenuStr[35], kc_syswin, 0x20, 0x40); // Window color
    Q8(g_MenuStr[32], kc_syssnd, 0x20, 0x50); // Sound
    Q8(PT(g_LExtra, g_ModPtbrSysOptName), kc_extra, 0x20, 0x60);
    KN8O(ctx);
}

// MENU_STEP_SYSTEM
void CmSysStep(void) {
    const PadRows* r;
    s32* nav = &g_MenuNavigation.cursorSettings;
    MenuContext* pop = &g_Menus[MENU_DG_CFG_SOUND];

    if (*nav >= SYS_ROWS) {
        *nav = 0;
    }
    MenuHandleCursorInput(nav, SYS_ROWS, 0);
    func_800F9808(2);
    ShowText(*nav == SYS_EXTRA ? INFO(PT(g_ExtraInfo, g_Pt_voices), kc_voices)
                               : D_800A2D48[g_SysInfo[*nav]]);
    if (g_pads[0].tapped & PAD_MENU_BACK) {
        KENTER();
        MenuShow(MENU_DG_MAIN);
        MenuShow(MENU_DG_BG);
        MenuHide(MENU_DG_INFO_BAR);
        MenuHide(MENU_DG_SETTINGS);
        g_MenuStep = MENU_STEP_OPENED;
        return;
    }
    if (!(g_pads[0].tapped & PAD_MENU_SELECT)) {
        return;
    }
    KENTER();
    switch (*nav) {
    case 0:
        r = Rows();
        PermLoad();
        if (g_MenuNavigation.cursorButtons >= r->n) {
            g_MenuNavigation.cursorButtons = 0;
        }
        g_Menus[MENU_DG_CFG_BUTTONS].cursorH =
            g_Menus[MENU_DG_CFG_BUTTONS].unk1.h = r->n * 16;
        MenuShow(MENU_DG_CFG_BUTTONS);
        g_MenuStep = MENU_STEP_SYSTEM_BUTTONS;
        break;
    case 1:
        MenuShow(MENU_DG_WINDOW_COLORS);
        g_MenuStep = MENU_STEP_SYSTEM_WINDOW_COLOR;
        break;
    case 2:
        MenuShow(MENU_DG_CFG_SOUND);
        g_MenuStep = MENU_STEP_SYSTEM_SOUND;
        break;
    default:
        // Extra settings: the popup, in the Sound box's window with our
        // size (as Alucard's, src/menu/sysopt.c ModSysOptInput)
        if (g_DubLoaded < 0) {
            g_DubLoaded = MOD_STATUS_FLAGS & DUB_BITS; // (the banks loaded)
        }
        if (g_CostumeLoaded < 0) {
            g_CostumeLoaded = CostumeCur(); // (his files loaded)
        }
        if (g_PopRow < 0 || g_PopRow >= PopRows()) {
            g_PopRow = 0;
        }
        pop->cursorX = POP_X;
        pop->cursorY = POP_Y - (PopRows() - 3) * 8; // (grows up: the info bar is under it)
        pop->cursorW = POP_W;
        pop->cursorH = POP_H;
        pop->unk1C = 3;
        pop->unk1D = 0;
        g_MenuStep = MOD_STEP_BOX;
        g_pads[0].tapped = 0;
        g_pads[0].repeat = 0;
        break;
    }
    PlaySfx(SFX_UI_CONFIRM);
}

// MenuSoundConfigDraw, for the popup: false when the window is the Sound
// box's own (DRA draws it)
s32 CmPopDraw(MenuContext* ctx) {
    s32 x = ctx->cursorX + 4;
    s32 y = ctx->cursorY + 4;
    s32 v;

    if (ctx->cursorW != POP_W) {
        return 0;
    }
    if (g_MenuStep != MOD_STEP_BOX) { // (an orphan: reset)
        PopReset(ctx);
        return 1;
    }
#if defined(MOD_CHARMENU_KO)
    {
        const char* nm[12];
        s32 xs[12];
        s32 ys[12];
        s32 lk[6];
        s32 pass, r, n;
        const char* kv[12];

        lk[3] = lk[4] = lk[5] = 0; // (the cheats are never locked)

        lk[0] = SwapLocked() != 0;
        lk[1] = ClassicLocked() != 0;
        lk[2] = g_StageId == STAGE_CEN;
        v = DubCur();
        kv[0] = IS_MARIA() ? kc_version : kc_costume;
        kv[1] = IS_MARIA() ? (CostumeCur() ? kc_n_psp : kc_saturn)
                           : (CostumeCur() ? kc_saturn : kc_n_ps1);
        kv[2] = kc_balance;
        kv[3] = MOD_PS1_BALANCE() ? kc_classic : kc_ultimate;
        kv[4] = kc_dub;
        kv[5] = g_DubKo[IS_MARIA() && v ? 2 : v];
        for (r = 0; r < NCheat(); r++) {
            kv[6 + r * 2] = r == 0 ? kc_ch_hp : (r == 1 ? kc_ch_heart : kc_ch_mp);
            kv[7 + r * 2] = ((g_ModDub.cheats >> r) & 1) ? kc_on : kc_off;
        }
        for (pass = 0; pass < 2; pass++) {
            n = 0;
            for (r = 0; r < 3 + NCheat(); r++) {
                if (lk[r] != pass) {
                    continue;
                }
                nm[n] = kv[r * 2];
                xs[n] = x;
                ys[n] = y + r * 16;
                n++;
                nm[n] = kv[r * 2 + 1];
                xs[n] = x + VALUE_DX;
                ys[n] = y + r * 16;
                n++;
            }
            if (n) {
                D_8013784C = pass;
                ((KoDrawFn)KO_EQUIP_SEL)(nm, xs, ys, n, ctx);
            }
        }
        D_8013784C = 0;
        y += 32;
    }
#else
    D_8013784C = SwapLocked(); // (grey: locked)
    if (IS_MARIA()) {
        STR8(PT(g_LVersion, g_Pt_version), kc_version, x, y, ctx);
        STR8(CostumeCur() ? g_NPsp : g_NSaturn,
             CostumeCur() ? kc_n_psp : kc_saturn, x + VALUE_DX, y, ctx);
    } else {
        STR8(PT(g_LCostume, g_Pt_costume), kc_costume, x, y, ctx);
        STR8(CostumeCur() ? g_NSaturn : g_NPs1,
             CostumeCur() ? kc_saturn : kc_n_ps1, x + VALUE_DX, y, ctx);
    }
    y += 16;
    D_8013784C = ClassicLocked(); // (grey: no Classic in this room)
    STR8(PT(g_LBalance, g_ModPtbrSysOptLabel[2]), kc_balance, x, y, ctx);
    STR8(MOD_PS1_BALANCE() ? PT(g_NClassic, g_Pt_classic) : g_NUltimate,
         MOD_PS1_BALANCE() ? kc_classic : kc_ultimate, x + VALUE_DX, y, ctx);
    y += 16;
    D_8013784C = g_StageId == STAGE_CEN;
    STR8(PT(g_LDub, g_ModPtbrSysOptLabel[3]), kc_dub, x, y, ctx);
    v = DubCur();
    STR8(g_DubName[IS_MARIA() && v ? 2 : v], g_DubKo[IS_MARIA() && v ? 2 : v],
         x + VALUE_DX, y, ctx);
#endif
    D_8013784C = 0;
#if defined(MOD_PTBR)
    y += 16;
    MenuDrawStr(PT(_S("Language"), g_ModPtbrLangLabel), x, y, ctx);
    MenuDrawStr(g_ModPtbrLangName[MOD_PTBR_BLOB[2] & 1], x + VALUE_DX, y, ctx);
#endif
#if !defined(MOD_CHARMENU_KO)
    for (v = 0; v < NCheat(); v++) {
        y = ctx->cursorY + 4 + (POP_CH0 + v) * 16;
        MenuDrawStr(v == 0 ? g_LInfHp : (v == 1 ? g_LInfHearts : g_LInfMp), x,
                    y, ctx);
        MenuDrawStr(((g_ModDub.cheats >> v) & 1) ? g_NOn : g_NOff,
                    x + VALUE_DX, y, ctx);
    }
#endif
    func_800F5E68(
        ctx, g_PopRow, ctx->cursorX + 2, ctx->cursorY + 2, POP_W - 4, 12, 4, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// Commands: Alucard's spell list (MenuSpellsDraw / MENU_STEP_SPELL, its
// window MENU_DG_SPELLS and positions) with their moves: the name in the
// info bar's font (rendered into a row slot, as his spell names), the input
// in the 8x8 font with their own buttons (Button set-up), and the cost.
// Their item crashes are all listed (Richter's flame whip, his crash with
// no subweapon, is left out). More rows than the window's 8 scroll.
//
// (Their texts: no ':' ',' or apostrophe, which the info bar's font draws
// as a star.)
//
// Text memory: the menu's text renderer writes from g_Pix[0] on, reset
// each frame, 2304 bytes a row (func_800F99B8; a clear, func_800F9808, as
// much): three rows a frame stay in g_Pix[0]. A fourth would reach g_Pix[1],
// this code. So: the info bar (a clear and a row) when the cursor moves,
// else one name a frame. Row r's name lives in slot 0x80 + (r & 7), so a
// scroll renders only the row that comes in.
// ---------------------------------------------------------------------------
#define MENU_DG_SPELLS 6
#define MENU_STEP_SPELL_INIT 0x30 // (it stays there: one DRA hook)
#define CMD_ROWS 8
#define NB 0xFF // no button
#define K_MP 1
#define K_HEART 2

void MenuDrawImg(MenuContext* ctx, s32 x, s32 y, s32 w, u32 h, s32 u, s32 v,
                 s32 idx, s32 unk2, bool disableTexShade, s32 unk4);
void MenuDrawRect(MenuContext* context, s32 posX, s32 posY, s32 width,
                  s32 height, s32 r, s32 g, s32 b);
u8 func_800F548C(u8 y);

typedef struct {
    const char* combo; // 8x8 font; a char followed by "ため" (C0 D2) is
                       // red: held (Alucard's Tetra Spirit)
    const char* name;  // ASCII, the info bar's font
    const char* info;  // NULL: an item crash
    u8 btn, btn2;      // the finishing button(s): action f, NB none
    u8 kind, cost;
} CmdRow;

static const char g_CNone[] = _S("");
static const char g_CBlade[] = _S("↑↓↘");
static const char g_CHigh[] = _S("↓↑");
static const char g_CDown[] = _S("↓");
static const char g_CSlide[] = _S("(slide)");
static const char g_CHold[] = _S("(hold)");
static const char g_CRun[] = _S("(run)");
static const char g_NBlade[] = "Blade dash";
static const char g_NHigh[] = "High jump";
static const char g_NSlide[] = "Slide";
static const char g_NSlideKick[] = "Slide kick";
static const char g_IHigh[] = "Leap high into the air";
static const char g_ISlide[] = "Slide along the ground";
static const char g_ICrash[] = "Item crash with its subweapon";

// src/ric: pl_main.c CheckBladeDashInput / CheckHighJumpInput,
// pl_collision.c (Down + Cross), pl_steps.c (Cross early in the slide);
// crash costs: assets/ric/subweapons.yaml (each subweapon's crashid)
static const CmdRow g_CmdRic[] = {
    {g_CBlade, g_NBlade, "Dash forward with the whip", 0, NB, 0},
    {g_CHigh, g_NHigh, g_IHigh, 2, NB, 0},
    {g_CDown, g_NSlide, g_ISlide, 2, NB, 0},
    {g_CSlide, g_NSlideKick, "Jump early in a slide to kick", 2, NB, 0, 0},
    {g_CNone, "Dagger crash", NULL, 3, NB, K_HEART, 10},
    {g_CNone, "Axe crash", NULL, 3, NB, K_HEART, 10},
    {g_CNone, "Hydro storm", NULL, 3, NB, K_HEART, 15},
    {g_CNone, "Cross crash", NULL, 3, NB, K_HEART, 20},
    {g_CNone, "Bible crash", NULL, 3, NB, K_HEART, 15},
    {g_CNone, "Stopwatch crash", NULL, 3, NB, K_HEART, 20},
    {g_CNone, "Rebound crash", NULL, 3, NB, K_HEART, 10},
    {g_CNone, "Vibhuti crash", NULL, 3, NB, K_HEART, 10},
    {g_CNone, "Agunea crash", NULL, 3, NB, K_HEART, 20},
};

// src/maria (pl_main.c, pl_collision.c); crash costs:
// assets/maria/subweapons.yaml. Her thrown subweapon is kept as Richter's
// id outside her code (src/maria_psp/mar2.c Mar2MariaToRic)
static const CmdRow g_CmdMarPsp[] = {
    {g_CNone, "Dash right", "Dash to the right", 6, NB, 0},
    {g_CNone, "Dash left", "Dash to the left", 5, NB, 0},
    {g_CBlade, g_NBlade, "A dashing attack", 0, NB, 0},
    {g_CHigh, g_NHigh, g_IHigh, 2, NB, 0},
    {g_CDown, g_NSlide, g_ISlide, 2, NB, 0},
    {g_CNone, "Cardinal crash", NULL, 3, NB, K_HEART, 25},
    {g_CNone, "Cat crash", NULL, 3, NB, K_HEART, 15},
    {g_CNone, "Turtle crash", NULL, 3, NB, K_HEART, 20},
    {g_CNone, "Dragon crash", NULL, 3, NB, K_HEART, 50},
    {g_CNone, "Doll crash", NULL, 3, NB, K_HEART, 8},
};

// src/maria_saturn: mar_specials.h (inputs, then Shot or Kick; MP
// mar_sp_cost), mar_attacks.h (charge, dive kick, slide kick), the high
// jump's 4 MP uppercut; Triangle backdash (ric_mod_maria.h)
static const CmdRow g_CmdMarSat[] = {
    {_S("↓↘→"), "Guardian knuckle", "Summons the Guardian Knuckle", 0, 1, K_MP,
     40},
    {_S("←↖↑↗→"), "Genbu", "Summons the Black Tortoise", 0, 1, K_MP, 80},
    {_S("↑↓↘→"), "Byakko", "Summons the White Tiger", 0, 1, K_MP, 100},
    {_S("←→↘↓↙←→"), "Seiryuu", "Summons the Azure Dragon", 0, 1, K_MP, 120},
    {_S("↑ため↗→↘↓"), "Suzaku", "Summons the Vermilion Bird", 0, 1, K_MP, 60},
    {_S("↑↗→↘↓↙←↖↑ため↓"), "Four beasts", "Hold the last Up then Down", 0, 1,
     K_MP, 150},
    {g_CHold, "Charge shot", "Charge longer for a stronger shot", 0, NB, K_MP,
     10},
    {g_CHigh, "Uppercut", "A rising uppercut", 2, NB, K_MP, 4},
    {g_CDown, "Dive kick", "In the air after both jumps", 2, NB, 0},
    {g_CRun, g_NSlideKick, "Kick while running", 1, NB, 0},
    {g_CNone, "Backdash", "Leap backward", 3, NB, 0},
};

typedef struct {
    const CmdRow* rows;
    u8 n;
    u8 x; // the input's column (the Saturn Maria's are longer)
} CmdList;
// MOD_PAD_CHAR order: Richter, the PSP Maria, the Saturn Maria
static const CmdList g_CmdLists[3] = {
    {g_CmdRic, LEN(g_CmdRic), 176},
    {g_CmdMarPsp, LEN(g_CmdMarPsp), 176},
    {g_CmdMarSat, LEN(g_CmdMarSat), 152},
};

// (initialised: no .bss; the file is read again each time the menu opens)
static s32 g_CmdOpen = 0;
static s32 g_CmdTop = 0;
static s32 g_CmdInfo = -1;
static s32 g_CmdSlot[CMD_ROWS] = {-1, -1, -1, -1, -1, -1, -1, -1};

static const CmdList* Cmds(void) { return &g_CmdLists[MOD_PAD_CHAR()]; }

#if defined(MOD_CHARMENU_KO)
// the Korean names / descriptions, row for row as the lists above (NULL: an
// item crash, its description is the general one)
static const char* const g_KoCmdName[3][13] = {
    {kc_c_blade, kc_c_high, kc_c_slide, kc_c_skick, kc_n_dagger, kc_n_axe,
     kc_n_hydro, kc_n_cross, kc_n_bible, kc_n_stop, kc_n_rebound, kc_n_vib,
     kc_n_agunea},
    {kc_cn_dashr, kc_cn_dashl, kc_c_blade, kc_c_high, kc_c_slide, kc_n_cardinal,
     kc_n_cat, kc_n_turtle, kc_n_dragon, kc_n_doll},
    {kc_n_gknuckle, kc_n_genbu, kc_n_byakko, kc_n_seiryuu, kc_n_suzaku,
     kc_n_four, kc_n_charge, kc_n_upper, kc_n_dive, kc_c_skick, kc_cn_backdash},
};
static const char* const g_KoCmdInfo[3][13] = {
    {kc_d_blade, kc_d_high, kc_d_slide, kc_d_skick},
    {kc_d_dashr, kc_d_dashl, kc_d_dashatk, kc_d_high, kc_d_slide},
    {kc_d_gknuckle, kc_d_genbu, kc_d_byakko, kc_d_seiryuu, kc_d_suzaku,
     kc_d_four, kc_d_charge, kc_d_upper, kc_d_dive, kc_d_kickrun,
     kc_d_backdash},
};
#endif

// action f's button as they set it up; the x after it
static s32 CmdBtn(s32 f, s32 x, s32 y, MenuContext* ctx) {
    u32 v = MOD_PAD_WORD(MOD_PAD_CHAR());
    s32 b = ((v >> (f * 3)) & 7) ^ f;

    MenuDrawChar(g_ChButtons[b], x, y, ctx);
    x += 8;
    if (b >= 4) {
        MenuDrawChar(g_ChButtons2[b - 4], x, y, ctx);
        x += 8;
    }
    return x;
}

// the 8x8 string's width (the red marks take no room)
static s32 CmdLen(const u8* s) {
    s32 n = 0;

    for (; *s != 0xFF; s++) {
        if (s[0] == 0xC0 && s[1] == 0xD2) {
            s++;
            continue;
        }
        n++;
    }
    return n;
}

#if defined(MOD_PTBR)
static const char* CmdCombo(const char* s) {
    if (!MOD_PTBR_TEXT_PT()) {
        return s;
    }
    return s == g_CSlide  ? g_Pt_slide
           : s == g_CHold ? g_Pt_hold
           : s == g_CRun  ? g_Pt_run
                          : s;
}
#define CMD_NAME(i) PT(l->rows[i].name, g_PtCmdName[MOD_PAD_CHAR()][i])
#define CMD_INFO(i)                                                            \
    (MOD_PTBR_TEXT_PT() ? g_PtCmdInfo[MOD_PAD_CHAR()][i] : l->rows[i].info)
#define CMD_CRASH() PT(g_ICrash, g_Pt_crash)
#define CMD_OR_2 PT(CH('r'), CH('u'))
#else
#define CmdCombo(s) (s)
#define CMD_NAME(i) (l->rows[i].name)
#define CMD_INFO(i) (l->rows[i].info)
#define CMD_CRASH() g_ICrash
#define CMD_OR_2 CH('r')
#if defined(MOD_CHARMENU_KO)
#undef CMD_NAME
#undef CMD_INFO
#undef CMD_CRASH
#define CMD_NAME(i) (g_KoCmdName[MOD_PAD_CHAR()][i])
#define CMD_INFO(i) (g_KoCmdInfo[MOD_PAD_CHAR()][i])
#define CMD_CRASH() kc_d_icrash
#endif
#endif

// MenuSpellsDraw for them (DRA's window dispatcher calls in here)
void CmCmdDraw(MenuContext* ctx) {
    const CmdList* l = Cmds();
    const CmdRow* r;
    s32 i, v, x, y, c;

    STR8(PT(g_Commands, g_Pt_commands), kc_cmds, 104, 40, ctx);
    for (v = 0; v < CMD_ROWS && g_CmdTop + v < l->n; v++) {
        i = g_CmdTop + v;
        r = &l->rows[i];
        y = v * 16 + 0x40;
        if (g_CmdSlot[i & 7] == i) {
            MenuDrawImg(ctx, 0x1C, y - 4, 0xF0, 0x10, 0,
                        func_800F548C(0x80 + (i & 7)), 0x1A1, 6, true, 0);
        }
        x = l->x;
        MenuDrawStr(CmdCombo(r->combo), x, y, ctx);
        x += CmdLen((const u8*)CmdCombo(r->combo)) * 8;
        if (x != l->x) {
            MenuDrawChar(CH('+'), x, y, ctx);
            x += 8;
        }
        x = CmdBtn(r->btn, x, y, ctx);
        if (r->btn2 != NB) {
            MenuDrawChar(CH('o'), x, y, ctx);
            MenuDrawChar(CMD_OR_2, x + 8, y, ctx);
            CmdBtn(r->btn2, x + 16, y, ctx);
        }
        if (r->kind == K_MP) {
            MenuDrawStr(g_MenuStr[13], 284, y, ctx);
        } else if (r->kind == K_HEART) {
            MenuDrawStr(g_MenuStr[14], 264, y, ctx);
        }
        if (r->kind) {
            MenuDrawInt(r->cost, 316, y, ctx);
        }
    }
    // more above / below
    if (g_CmdTop > 0) {
        MenuDrawStr(_S("↑"), 336, 0x40, ctx);
    }
    if (g_CmdTop + CMD_ROWS < l->n) {
        MenuDrawStr(_S("↓"), 336, 0x40 + 7 * 16, ctx);
    }
    // the cursor glows yellow (as Alucard's)
    if (g_Timer & 0x20) {
        c = 0x60 + (g_Timer & 0x1F);
    } else {
        c = 0x7F - (g_Timer & 0x1F);
    }
    MenuDrawRect(
        ctx, 26, (g_MenuNavigation.cursorSpells - g_CmdTop) * 16 + 0x3B, 300,
        17, c, c, 0);
}

// MENU_STEP_SPELL_INIT, every frame while their Commands are open
void CmCmdStep(void) {
    const CmdList* l = Cmds();
    s32* nav = &g_MenuNavigation.cursorSpells;
    const char* info;
    s32 i;

    if (!g_CmdOpen) {
        KENTER();
        g_CmdOpen = 1;
        if (*nav >= l->n) {
            *nav = 0;
        }
        g_CmdTop = 0;
        g_CmdInfo = -1;
        for (i = 0; i < CMD_ROWS; i++) {
            g_CmdSlot[i] = -1;
        }
        MenuShow(MENU_DG_INFO_BAR);
        MenuShow(MENU_DG_SPELLS);
    } else {
        MenuHandleCursorInput(nav, l->n, 0);
    }
    if (*nav < g_CmdTop) {
        g_CmdTop = *nav;
    }
    if (*nav >= g_CmdTop + CMD_ROWS) {
        g_CmdTop = *nav - (CMD_ROWS - 1);
    }
    if (g_CmdInfo != *nav) {
        g_CmdInfo = *nav;
        info = CMD_INFO(*nav);
        func_800F9808(2);
        ShowText(info ? info : CMD_CRASH());
    } else {
        // one name a frame (see above)
        for (i = g_CmdTop; i < g_CmdTop + CMD_ROWS && i < l->n; i++) {
            if (g_CmdSlot[i & 7] != i) {
                g_CmdSlot[i & 7] = i;
                func_800F99B8(CMD_NAME(i), 0x80 + (i & 7), false);
                break;
            }
        }
    }
    if (g_pads[0].tapped & PAD_MENU_BACK) {
        KENTER();
        MenuShow(MENU_DG_MAIN);
        MenuShow(MENU_DG_BG);
        MenuHide(MENU_DG_INFO_BAR);
        MenuHide(MENU_DG_SPELLS);
        g_MenuStep = MENU_STEP_OPENED;
        g_CmdOpen = 0;
    }
}
