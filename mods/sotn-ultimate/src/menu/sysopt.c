// SPDX-License-Identifier: AGPL-3.0-or-later
// Saturn port mod: the pause menu System screen's "Extra settings" row
// (Alucard games). X opens a popup to its right, in the retail Sound box's
// window (MENU_DG_CFG_SOUND, resized while it is ours), with the Player
// Select's four Alucard options (Boss Richter, Boss Maria, Balance, Dub):
// Up / Down pick one, Left / Right (or X) change it (the flags byte,
// include/game.h MOD_STATUS_FLAGS), Triangle / Circle close it. A boss
// option is greyed and locked while that boss is alive. While the box is up
// (opening, open, closing) it owns the pad: the System list sees no input.
//
// DRA has no room for this: it follows Alucard's sprites in BIN/ARC_F2.BIN
// (retail ARC_F.BIN + this code at file offset 0x31BF0 = MOD_SYSOPT_ADDR
// 0x8016DBF0; a new file at the disc's end, loaded by src/dra/cd.c
// g_LbaBinAlucard in Alucard games only, up to the familiar overlay at
// 0x80170000). Built against the built DRA's addresses by
// mods/sotn-ultimate/tools/build_sysopt.sh (at most 0x34000 - 0x31BF0 = 9232
// bytes).
//
// DRA calls in through three stubs in its data (src/config_us.h), which
// check MOD_SYSOPT_MAGIC first: in Richter / Maria games their player file
// sits here instead, and the stubs call the retail functions:
//   ModSysOptBox   for MenuSystemDraw's cursor box (func_800F5E68),
//   ModSysOptInput for MENU_STEP_SYSTEM's MenuHandleCursorInput,
//   ModSysOptSound for MenuDraw's MenuSoundConfigDraw (the popup's window).
// The row's info bar text is this code's header (menu.c D_800A2D48[7]).
#include <game.h>
#include <sfx.h>
#include <lba.h>
#include <disk.h>

#ifndef OFF_BIN_ALK_PSP
#define OFF_BIN_ALK_PSP 0 // (the first compile, as in src/dra/cd.c)
#endif
#ifndef OFF_BIN_JP_SD
#define OFF_BIN_JP_SD 0
#endif

// DRA (src/dra/dra.h, menu.h, menu.c, cd.c)
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
#define MENU_DG_SETTINGS 7
#define MENU_DG_CFG_SOUND 11
#define MENU_STEP_SYSTEM 0x101
#define MOD_STEP_BOX 0x10F // (no MenuHandle case: menu.h ends at 0x107)
extern MenuContext g_Menus[];
extern MenuContextInit g_MenuInit[];
extern CdFile g_LbaSdAlkVh;
extern CdFile g_LbaSdAlkVb;
extern s32 D_8013784C; // menu.c: 1 = grey text
void func_800F5E68(MenuContext* ctx, s32 cursorIdx, s32 x, s32 y, s32 w, s32 h,
                   s32 yGap, s32 bColorMode);
void MenuDrawStr(const char* str, s32 x, s32 y, MenuContext* ctx);
void MenuHandleCursorInput(s32* nav, u8 nOptions, u32 type);
void MenuSoundConfigDraw(MenuContext* ctx);
void SsUtAllKeyOff(short mode); // (main SDK)

void ModSysOptBox(MenuContext* ctx, s32 cursorIdx, s32 x, s32 y, s32 w, s32 h,
                  s32 yGap, s32 bColorMode);
void ModSysOptInput(s32* nav, u8 nOptions, u32 type);
void ModSysOptSound(MenuContext* ctx);
void ModSysOptTick(void);

// the header the stubs read (first in .text, at MOD_SYSOPT_ADDR): the
// magic, the three entries, then the info bar text (MOD_SYSOPT_BAR_TEXT;
// the info bar reads an ASCII comma as a Shift-JIS lead byte: the
// full-width comma 81 43 instead)
#if defined(MOD_SYSOPT_KO)
#include "sysopt_ko_str.h"
// KO build: the info bar text is Korean (12px, ShowText's F0xx codes; gen_ko_cfg.py xset:bar)
__asm__(".text\n"
        ".word " MOD_SYSOPT_MAGIC_STR "\n"
        ".word ModSysOptBox\n"
        ".word ModSysOptInput\n"
        ".word ModSysOptSound\n"
        ".word ModSysOptTick\n"
        ".ascii \"" KO_XSET_BAR_ASM "\"\n"
        ".byte 0\n"
        ".org 52\n" /* 094_12: the Cheats row's info bar text at header + 52 (MOD_SYSOPT_BAR_TEXT2); .org fails if the first overruns */
        ".ascii \"" KO_XSET_BAR2_ASM "\"\n"
        ".byte 0\n"
        ".align 2\n");
#else
__asm__(".text\n"
        ".word " MOD_SYSOPT_MAGIC_STR "\n"
        ".word ModSysOptBox\n"
        ".word ModSysOptInput\n"
        ".word ModSysOptSound\n"
        ".word ModSysOptTick\n"
        ".asciz \"Boss versions\\201\\103 game balance and voices\"\n"
        ".align 2\n");
#endif

#define ROW 6     // the row, after the retail six (16 px apart: y 0x90); ROW + 1 = the Cheats row (094_12)
#define NOPT MOD_SYS_NOPT    // the Player Select options, rows 0 to 3 of the popup
#define NCHEAT MOD_SYS_NCHEAT // the cheats, rows NOPT and after
#define ROWS (NOPT + NCHEAT)
// 094_12: two popups, no scrolling: the options (rows 0..NOPT-1) and the cheats
// (rows NOPT..ROWS-1), g_PopMode 0 / 1
#if defined(MOD_PTBR)
// v1.5 PTBR here was LANG = ROWS = 4 and POP_ROWS = 5 with window 156 108 200 80; merged ROWS = 14, LANG = NOPT = 4, same POP_ROWS numbers, see M2 report
#define LANG NOPT
#define POP_ROWS (NOPT + 1)
#else
#define POP_ROWS NOPT
#endif
#define POP_X 172 // the popup window (the Sound box is 172, 112, 57, 32)
#if defined(MOD_PTBR)
#define POPN (g_PopMode ? NCHEAT : (NOPT + 1))
#else
#define POPN (g_PopMode ? NCHEAT : NOPT)  // rows of the open popup
#endif
#define POPBASE (g_PopMode ? NOPT : 0)    // its first row
#define POP_H(n) ((n) * 16)
#define POP_Y(n) (188 - POP_H(n)) // ends at 188: the info bar starts at 193
#define POP_W 168 // the right margin as wide as the left one
// v1.5 non-PTBR window was POP_X 172 POP_Y 124 POP_W 168 POP_H 64; merged POP_Y(NOPT) = 124 and POP_H(NOPT) = 64 match, PTBR width differs, see M2 report
#define VALUE_DX MOD_SYS_VALUE_DX

#if !defined(MOD_SYSOPT_KO) /* 095_39: the English labels are only drawn by the non-KO path; the KO build has its own (g_SysOptLabelKo) */
static const char g_SysOptName[7][8] = {
    _S("PS1"), _S("Saturn"), _S("PSP"), _S("JP"),
    _S("STN"), // 0.9.4: the Dub row's "Saturn" overflows its box -> "STN" (index 4)
    _S("OFF"), _S("ON")};
static const char* const g_SysOptLabel[ROWS] = {
    _S("Boss Richter"), _S("Boss Maria"), _S("Balance"), _S("Dub"),
    // the cheats (all off in a new game; see ModSysOptTick)
    _S("Inf. MP"), _S("Inf. Hearts"), _S("Fast MP regen"), _S("Fast stairs"),
    _S("Full heal"), _S("Invincible"), _S("Inf. HP"), _S("Inf. Gold"),
    _S("Inf. jump"), _S("Moon jump")};
#endif
#if defined(MOD_PTBR)
#include "mod_ptbr_sysopt.h"
#define SYSOPT_NAME()                                                          \
    (MOD_PTBR_TEXT_PT() ? g_ModPtbrSysOptName : _S("Extra settings"))
#define SYSOPT_LABEL(r)                                                        \
    (MOD_PTBR_TEXT_PT() ? g_ModPtbrSysOptLabel[r] : g_SysOptLabel[r])
#else
#define SYSOPT_NAME() _S("Extra settings")
#define SYSOPT_LABEL(r) g_SysOptLabel[r]
#endif
// each option: the flag bits it owns, and per choice (the Player Select's
// order, src/st/sel/2C048.c) their value and its name
// (SysOptRow: game_mod.h)
static const SysOptRow g_SysOptRows[NOPT] = {
    {MOD_FLAG_BOSS_RIC_SATURN, 2, {0, MOD_FLAG_BOSS_RIC_SATURN}, {0, 1}},
    {MOD_FLAG_MARIA_BOSS_PSP, 2, {0, MOD_FLAG_MARIA_BOSS_PSP}, {1, 2}},
    {MOD_FLAG_PS1_BALANCE, 2, {MOD_FLAG_PS1_BALANCE, 0}, {0, 1}},
    {MOD_FLAG_PS1_DUB | MOD_FLAG_JP_DUB,
     3,
     {0, MOD_FLAG_JP_DUB, MOD_FLAG_PS1_DUB},
     {2, 4, 0}}, // PSP / STN (was Saturn) / PS1
};
#define DUB 3

// ---- the cheats: popup rows NOPT.. (bit i of g_CheatMask = row NOPT + i on).
// All off in a new game: the mask is this file's data, loaded zero with each
// game load. ModSysOptTick runs every frame of play (DRA's HandlePlay, through
// the same kind of stub as the menu's three entries) and is a no-op when
// this file is not the one loaded (Richter / Maria games).
// (the cheats' bit numbers CHEAT_*: game_mod.h)
#define CHEAT_ON(c) ((g_CheatMask >> (c)) & 1)
static s32 g_CheatMask = 0;

// absolute symbols from build_sysopt.sh (this build's DRA): HandleDamage's
// entry and its first two words as built, the upper halfwords of the two
// stair-slowdown branches in func_80109990 (bne 0x1482, made b 0x1000)
extern char ModHdEntry;
extern char ModHdOrig0;
extern char ModHdOrig1;
extern char ModStairsA;
extern char ModStairsB;
void FlushCache(void); // (main SDK: code changed in place)

static void CodeWord(u32* p, u32 v) {
    if (*p != v) {
        *p = v;
        FlushCache();
    }
}

// ---- 099b: the easy tick, the cheats' tick and the Korean popup's draw live in the new resident area (MOD_NEWAREA_BASE, src/easy/*.c; loaded by the ARC_KO chain
// right after this file). NewFn(slot) = the entry the area's 32 B header holds in that slot, or 0 when the area is not there (yet) / its marker was overwritten.
// The state they need stays here (g_EasyState, g_CheatMask, the popup's statics) and is passed in, so it resets with every game load as before.
static ModEasyState g_EasyState = {0, -1, 0, 0};
static u32 NewFn(s32 slot) {
    volatile u32* h = (volatile u32*)MOD_NEWAREA_BASE;
    volatile u32* mk = (volatile u32*)MOD_NEWAREA_MARKER_ADDR;

    if (h[0] != MOD_NEWAREA_MAGIC) {
        return 0; // not loaded (yet)
    }
    if (*mk == 0) {
        *mk = MOD_NEWAREA_MARKER; // first call since the load
    } else if (*mk != MOD_NEWAREA_MARKER) {
        *(volatile u32*)MOD_NEWAREA_ALERT_ADDR = 0xDEAD0001; // the area's top was overwritten: stop using it
        return 0;
    }
    return h[slot];
}

void ModSysOptTick(void) {
    void (*easy)(ModEasyState*) = (void (*)(ModEasyState*))NewFn(MOD_NEWAREA_SLOT_TICK);
    void (*cheat)(s32) = (void (*)(s32))NewFn(MOD_NEWAREA_SLOT_CHEAT);

    if (easy) {
        easy(&g_EasyState);
    }
    g_ModDub.cheats = 0; // (Richter's / the Marias' cheats, DRA HandlePlay: none in his game)
    if (cheat) {
        cheat(g_CheatMask);
    } else {
        // the area is not there: the invincible patch must still come off (a game load can leave HandleDamage patched)
        u32* hd = (u32*)&ModHdEntry;
        CodeWord(hd, (u32)&ModHdOrig0);
        CodeWord(hd + 1, (u32)&ModHdOrig1);
    }
}

// the popup's cursor (loaded with the file: 0 after each game load)
static s32 g_PopRow = 0;
// the first row shown = the popup's first row (POPBASE; no scrolling in 094_12)
static s32 g_PopTop = 0;
// the popup open (or last opened): 0 options (Extra settings), 1 cheats
static s32 g_PopMode = 0;
// a Dub change made since the current stage's bank was loaded: 1 + the dub
// bits it was loaded with (0: none). Kept until the bank is reloaded
static s32 g_StageDubOld = 0;
#define DUB_BITS (MOD_FLAG_PS1_DUB | MOD_FLAG_JP_DUB)

// the choice shown: the one whose bits match (else the first)
static s32 SysOptCur(const SysOptRow* row) {
    s32 i;

    for (i = row->n - 1; i > 0; i--) {
        if ((MOD_STATUS_FLAGS & row->mask) == row->val[i]) {
            break;
        }
    }
    return i;
}

// the locked options (bit per option): a boss option waits while that boss
// is alive (boss Richter in BO6 / BO6RIC2: enemy 0x133 ENEMY_RICHTER; boss
// Maria in CEN / CENP: the mod's record 0x198 on both fights' body): the
// overlay loaded is the other version's. They change again after the fight
// (a new choice takes effect the next time that boss's area loads)
static s32 SysOptLocks(void) {
    Entity* e;
    s32 locks = 0;

    for (e = &g_Entities[STAGE_ENTITY_START];
         e < &g_Entities[TOTAL_ENTITY_COUNT]; e++) {
        if (e->pfnUpdate && e->hitPoints > 0) {
            if (e->enemyId == 0x133) {
                locks |= 1;
            }
            if (e->enemyId == MOD_PSP_MARIA_ENEMY) {
                locks |= 2;
            }
        }
    }
    // Dub: locked in Castle Center (CEN): its bank goes with the CEN / CENP
    // overlay loaded and Boss Maria's choice (cd.c's StageSfx pre-hook, for
    // STAGE_CEN only: RCEN has no such bank), so the stage bank is not
    // reloaded there; the Dub changes after the next area load
    if (g_StageId == STAGE_CEN) {
        locks |= 1 << DUB;
    }
    // Boss Maria / Boss Richter: the version goes with the overlay loaded
    // (cd.c: CEN or CENP.BIN for her, BO6 or BO6RIC2.BIN for him), so a
    // change inside that area would only show after the next load: locked
    // there, not only during the fight
    if (g_StageId == STAGE_CEN) {
        locks |= 2;
    }
    if (g_StageId == STAGE_BO6) {
        locks |= 1;
    }
    return locks;
}

// The popup is the Sound box's window with our size (MenuContext.unk1C:
// 2 hidden, 3 opening, 0 open, 1 closing) while the menu is in
// MOD_STEP_BOX (from X on the row until the closing animation's end)
//
// closed at once, the Sound box's own window again (also when the System
// screen is left with the box still up: Start, or any other way)
static void PopReset(MenuContext* pop) {
    MenuContextInit* init = &g_MenuInit[MENU_DG_CFG_SOUND];

    pop->unk1C = 2;
    pop->cursorX = init->cursorX;
    pop->cursorY = init->cursorY;
    pop->cursorW = init->cursorW;
    pop->cursorH = init->cursorH;
}

// Alucard's grunt bank (vab 1: VAB/SD_ALK, BIN/ALK_PSP.BIN or the JP dub's
// copy in BIN/JP_SD.BIN, all laid out alike) is loaded with his player file
// only (src/dra/cd.c's CdFile_AlucardPrg branch, which sets
// g_LbaSdAlkVh/Vb.loc: so .loc is the bank loaded). After a Dub change, load
// the new one as that chain does (CdFile_4: the VH, then the VB), once the CD
// is idle. The current stage's bank (vab 3: boss / enemy voices) follows
// right after in the same CD chain (g_LbaSdAlkVh.nextCdFileType =
// CdFile_StageSfx, g_LoadOvlIdx = g_StageId: UpdateCd's pre-hook picks the
// PSP / JP / retail bank as on a stage load), all voices keyed off first
// (the pause already keyed off and saved the held 20-23 pairs and the
// scripts; they key on again through the new VAB header on unpause). The
// link is cleared below once the chain moved on, and by cd.c's
// CdFile_AlucardPrg branch at every game load. The menu has stopped the
// music; its exit waits for the whole chain (g_IsUsingCd); nothing plays
// from vab 1 / 3 while it is open. Castle Center keeps its bank until the
// next area load (its bank goes with the CEN / CENP overlay loaded, which
// Boss Maria may have changed since)
static void SysOptBanks(void) {
    s32 off = MOD_FIELD_JP_BANKS() && OFF_BIN_JP_SD ? OFF_BIN_JP_SD - 0x610D
              : MOD_FIELD_PSP_BANKS()               ? OFF_BIN_ALK_PSP - 0x610D
                                                    : 0;
    s32 stage =
        g_StageDubOld && (g_StageDubOld - 1) != (MOD_STATUS_FLAGS & DUB_BITS) &&
        g_StageId != STAGE_CEN;

    if (g_LbaSdAlkVh.nextCdFileType != CdFile_NoNext &&
        g_LoadFile != CdFile_4) {
        g_LbaSdAlkVh.nextCdFileType = CdFile_NoNext; // (the chain moved on)
    }
    if (!OFF_BIN_ALK_PSP || g_CdStep != CdStep_None || g_IsUsingCd) {
        return;
    }
    if (g_LbaSdAlkVh.loc != 0x610D + off) {
        g_LbaSdAlkVh.loc = 0x610D + off;
        g_LbaSdAlkVb.loc = 0x6110 + off;
        g_LoadFile = CdFile_4;
        if (stage) {
            g_LbaSdAlkVh.nextCdFileType = CdFile_StageSfx;
        }
    } else if (stage) {
        g_LoadFile = CdFile_StageSfx;
    } else {
        g_StageDubOld = 0; // (changed back)
        return;
    }
    if (stage) {
        SsUtAllKeyOff(0);
        g_LoadOvlIdx = g_StageId;
    }
    g_StageDubOld = 0;
    g_CdStep = CdStep_LoadInit;
}

#if defined(MOD_PTBR)
static void SysOptLang(void) {
    if (MOD_PTBR_BLOB[1] != MOD_PTBR_MAGIC) {
        g_api.PlaySfx(SFX_UI_ERROR);
        return;
    }
    MOD_PTBR_BLOB[2] ^= 1;
    g_api.PlaySfx(SFX_UI_MOVE);
    (*(void (**)(s32))MOD_PTBR_CODE)(1);
}
#endif

// The box's input, while g_MenuStep is MOD_STEP_BOX: only when it is fully
// open (Up / Down pick, Left / Right / X change, Triangle / Circle close)
/* KO build: confirm = Circle (JP style; DRA's PAD_MENU_SELECT = Circle|Square,
 * PAD_MENU_BACK = Cross); the mod box used the North-American X = confirm / Circle = close
 * (found with a control test). */
#if defined(MOD_SYSOPT_JPPAD) /* build_ko_trial.sh: SYSOPT_DEFS=-DMOD_SYSOPT_JPPAD */
#define SYSOPT_SELECT (PAD_CIRCLE | PAD_SQUARE)
#define SYSOPT_BACK (PAD_CROSS | PAD_TRIANGLE)
#else
#define SYSOPT_SELECT PAD_CROSS
#define SYSOPT_BACK (PAD_TRIANGLE | PAD_CIRCLE)
#endif

static void PopInput(MenuContext* pop) {
    const SysOptRow* row;
    s32 v, t;

    v = g_PopRow - POPBASE;
    MenuHandleCursorInput(&v, g_PopMode ? NCHEAT + MOD_SYS_NACT : POPN, 0); // Up / Down, wraps inside this popup (the cheat popup scrolls over the toggles and the actions)
    g_PopRow = v + POPBASE;
    t = g_PopTop - POPBASE;
    if (v < t) {
        t = v;
    }
    if (v >= t + POPN) {
        t = v - POPN + 1;
    }
    g_PopTop = POPBASE + t;
    if (g_PopMode && g_PopRow >= NOPT) {
        if (g_PopRow - NOPT >= NCHEAT) {
            // an instant action (the new area's CheatAct): X runs it once
            if (g_pads[0].tapped & SYSOPT_SELECT) {
                void (*act)(s32) = (void (*)(s32))NewFn(MOD_NEWAREA_SLOT_ACT);

                if (act) {
                    act(g_PopRow - NOPT - NCHEAT);
                    g_api.PlaySfx(SFX_UI_MOVE);
                }
            }
        } else if (g_pads[0].tapped & (PAD_LEFT | PAD_RIGHT | SYSOPT_SELECT)) {
            // a cheat: Left / Right / X flips it
            g_CheatMask ^= 1 << (g_PopRow - NOPT);
            g_api.PlaySfx(SFX_UI_MOVE);
        }
    }
    row = &g_SysOptRows[g_PopRow < NOPT ? g_PopRow : 0];
    // X steps on as Right does
    if (g_pads[0].tapped & SYSOPT_SELECT) {
        g_pads[0].repeat |= PAD_RIGHT;
    }
#if defined(MOD_PTBR)
    if ((g_pads[0].repeat & (PAD_LEFT | PAD_RIGHT)) && (g_PopRow < NOPT || g_PopRow == LANG)) {
        if (g_PopRow == LANG) {
            SysOptLang();
        } else
#else
    if (g_PopRow < NOPT && (g_pads[0].repeat & (PAD_LEFT | PAD_RIGHT))) {
#endif
            if ((SysOptLocks() >> g_PopRow) & 1) {
            g_api.PlaySfx(SFX_UI_ERROR);
        } else {
            v = SysOptCur(row);
            MenuHandleCursorInput(&v, row->n, 5); // wraps, SFX_UI_MOVE
#if !defined(MOD_FIELD_JP_VOICE)
            // (the field banks are the same for every Dub: the
            // stage and familiar banks are not reloaded, game_mod.h)
            if (g_PopRow == DUB && !g_StageDubOld) {
                // the stage's bank was loaded with these
                g_StageDubOld = 1 + (MOD_STATUS_FLAGS & DUB_BITS);
            }
#endif
            MOD_STATUS_FLAGS = (MOD_STATUS_FLAGS & ~row->mask) | row->val[v];
#if !defined(MOD_FIELD_JP_VOICE)
            if (g_PopRow == DUB) {
                // the familiar's bank has the dub's voices too: the menu's
                // exit reloads it as when another one is chosen
                // (CdFile_ServantChr; it is summoned again)
                g_ServantLoaded = 0;
            }
#endif
            // the PS1 balance has no third hand: its item back to the
            // inventory
            if (MOD_PS1_BALANCE() && MOD_HOLDER_ITEM) {
                g_api.AddToInventory(MOD_HOLDER_ITEM, EQUIP_HAND);
                MOD_HOLDER_ITEM = 0;
            }
        }
    }
    // Triangle / Circle close it as the Sound box closes (MenuHide)
    if (g_pads[0].tapped & SYSOPT_BACK) {
        g_api.PlaySfx(SFX_UI_CONFIRM);
        pop->unk1C = 1;
        pop->unk1D = 0;
    }
}

#if defined(MOD_SYSOPT_KO)
// KO build: the popup's rows are Korean 8px text (the equipment names' renderer in DRA's resident
// tail: 8x8 plate cells, mods/sotn-ultimate/src/menu/eq_core.c; strings generated by gen_ko_cfg.py
// from the xset: rows of 번역_설정화면.tsv). One call draws up to 16 names (a row's label and value);
// locked rows are drawn apart, grey (D_8013784C), as the English text was
#define KO_EQUIP_SEL 0x8009F500 // src/dra/dra.h (build_sysopt.sh checks it)
// 099b: the 8px popup tables and PopDrawKo are in the new resident area (src/easy/newarea_pop.c)
#endif

// MenuSystemDraw's cursor box: runs every frame the System window is drawn
// (the System list, the box, and the menu's closing fade). While the box is
// up the menu is in MOD_STEP_BOX, a step MenuHandle has no case for (as
// MENU_STEP_SYSTEM_SOUND is for the Sound box): the System list takes no
// input, and the info bar is not drawn again every frame (MENU_STEP_SYSTEM
// re-renders it with the BIOS font each frame: with the box's text on top
// that ran past one frame, 30 fps), so the bar keeps its text as during
// Sound. The box's own input is handled here
void ModSysOptBox(MenuContext* ctx, s32 cursorIdx, s32 x, s32 y, s32 w, s32 h,
                  s32 yGap, s32 bColorMode) {
    MenuContext* pop = &g_Menus[MENU_DG_CFG_SOUND];

    if (g_MenuStep == MOD_STEP_BOX) {
        if (pop->cursorW != POP_W || pop->unk1C == 2) {
            PopReset(pop); // closed: back to the System list
            g_MenuStep = MENU_STEP_SYSTEM;
        } else if (pop->unk1C == 0) {
            PopInput(pop);
        }
    } else {
        if (pop->cursorW == POP_W) {
            PopReset(pop); // the menu left the box some other way (Start)
        }
        // the choice is final (the box closed, or the System screen or the
        // menu is closing, Start included): the banks follow
        SysOptBanks();
    }
    func_800F5E68(ctx, cursorIdx, x, y, w, h, yGap, bColorMode);
#if !defined(MOD_SYSOPT_KO) /* KO: the row's name is the 7th entry of menu.c's 8px KO draw (ko_str38), one call with the six retail rows */
    MenuDrawStr(SYSOPT_NAME(), 0x20, 0x30 + ROW * 0x10, ctx);
    MenuDrawStr(_S("Cheats"), 0x20, 0x30 + (ROW + 1) * 0x10, ctx);
#endif
}

void ModSysOptSound(MenuContext* ctx) {
    const SysOptRow* row;
    s32 r, locks;
    s32 x = ctx->cursorX + 4;
    s32 y = ctx->cursorY + 4;

    if (ctx->cursorW != POP_W) {
        MenuSoundConfigDraw(ctx);
        return;
    }
    // only while the box's step is the menu's (else an orphan: reset)
    if (g_MenuStep != MOD_STEP_BOX) {
        PopReset(ctx);
        return;
    }
    locks = SysOptLocks();
#if defined(MOD_SYSOPT_KO)
    {
        void (*pop)(MenuContext*, s32, s32, ModPopArgs*) = (void (*)(MenuContext*, s32, s32, ModPopArgs*))NewFn(MOD_NEWAREA_SLOT_POP);

        if (pop) {
            ModPopArgs pa;

            pa.top = g_PopTop;
            pa.popn = POPN;
            pa.locks = locks;
            pa.cheatMask = g_CheatMask;
            pa.rows = g_SysOptRows;
            pop(ctx, x, y, &pa);
        }
    }
#else
    for (r = g_PopTop; r < g_PopTop + POPN; r++, y += 16) {
        D_8013784C = r < NOPT ? (locks >> r) & 1 : 0;
        MenuDrawStr(SYSOPT_LABEL(r), x, y, ctx);
        if (r < NOPT) {
            row = &g_SysOptRows[r];
            MenuDrawStr(
                g_SysOptName[row->name[SysOptCur(row)]], x + VALUE_DX, y, ctx);
        } else {
            MenuDrawStr(g_SysOptName[CHEAT_ON(r - NOPT) ? 6 : 5],
                        x + VALUE_DX, y, ctx);
        }
    }
#endif
    D_8013784C = 0;
#if defined(MOD_PTBR)
    MenuDrawStr(
        MOD_PTBR_TEXT_PT() ? g_ModPtbrLangLabel : _S("Language"), x, y, ctx);
    MenuDrawStr(g_ModPtbrLangName[MOD_PTBR_BLOB[2] != 0], x + VALUE_DX, y, ctx);
#endif
    func_800F5E68(ctx, g_PopRow - g_PopTop, ctx->cursorX + 2,
                  ctx->cursorY + 2, POP_W - 4, 12, 4, 1);
}

// MENU_STEP_SYSTEM's cursor input: the list's 7 rows; X on "Extra
// settings" opens the box (as MenuShow does for Sound) and enters
// MOD_STEP_BOX. Its tap is consumed (MENU_STEP_SYSTEM would buzz)
void ModSysOptInput(s32* nav, u8 nOptions, u32 type) {
    MenuContext* pop = &g_Menus[MENU_DG_CFG_SOUND];

    if (*nav > ROW + 1) {
        *nav = ROW + 1; // (a save from the build with four rows here)
    }
    MenuHandleCursorInput(nav, ROW + 2, type);
    if ((*nav == ROW || *nav == ROW + 1) && (g_pads[0].tapped & SYSOPT_SELECT)) {
        g_PopMode = *nav - ROW;
        g_PopRow = POPBASE;
        g_PopTop = POPBASE;
        pop->cursorX = POP_X;
        pop->cursorY = POP_Y(POPN);
        pop->cursorW = POP_W;
        pop->cursorH = POP_H(POPN);
        pop->unk1C = 3;
        pop->unk1D = 0;
        g_MenuStep = MOD_STEP_BOX;
        g_api.PlaySfx(SFX_UI_CONFIRM);
        g_pads[0].tapped = 0;
        g_pads[0].repeat = 0;
    }
}
