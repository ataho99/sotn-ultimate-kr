// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate: included by src/st/sel/2C048.c (at file scope, in place)
// Saturn port: the sound mode's main menu button (MOD_MENU_SOUND), a 5th
// one in the retail buttons' style
// (mods/sotn-ultimate/tools/gen_sel_soundmode.py: their template, a capital S
// block and "ound mode" cut from their words). The staircase is tightened so
// five fit with the retail right margin (x 351): x step 0x34 (retail 0x40), y
// step 0x20 (retail 0x28), from (0x10, 0x1C). The button lives at VRAM x 960 y
// 96 (4bpp tpage 0xF, u 0 v 96): F_SEL fills x 512-895 and the character select
// (x 896-959 at y 96+) never reaches it; uploaded with the menu's UI
// (InitMainMenuUI).
#define MOD_RO __attribute__((section(".rodata")))
#include "../../../../src/st/sel/mod_sel_soundmode_gfx.h"
// 095_36: the "Drama mode" (6th) line is a RUNTIME switch, one byte in SEL's rodata that the assembling tool sets when the
// user's drama CD material is in: marker "DRMFLAG" + the flag byte (0 = the five-line menu of SOTN Ultimate v1.5, 1 = six
// lines with Drama mode). The Drama code and picture are always in; the flag only picks the menu's line count and the
// button positions. Read through a volatile pointer so the compiler cannot fold the constant.
#ifndef MOD_DRAMA_DEFAULT
#define MOD_DRAMA_DEFAULT 0
#endif
const u8 g_ModDramaFlag[8] MOD_RO = {'D', 'R', 'M', 'F', 'L', 'A', 'G', MOD_DRAMA_DEFAULT};
#define MOD_DRAMA_ON() (*(volatile const u8*)&g_ModDramaFlag[7] != 0)
// positions, index = the prim number i (0 = the hidden first prim, 1..6 = the lines; the old macro's formula at i = 0 kept): flag 0 = the Ultimate v1.5 five-line staircase (x step 0x34 from 0x10,
// y step 0x20 from 0x1C); flag 1 = the six-line staircase (x step 0x29 from 0x10, y step 0x20 from 0x14, 094_18 / 095 plan A)
static const s16 g_ModBtnX[2][7] MOD_RO = {{-0x24, 0x10, 0x44, 0x78, 0xAC, 0xE0, 0}, {-0x19, 0x10, 0x39, 0x62, 0x8B, 0xB4, 0xDD}};
static const s16 g_ModBtnY[2][7] MOD_RO = {{-4, 0x1C, 0x3C, 0x5C, 0x7C, 0x9C, 0}, {-0xC, 0x14, 0x34, 0x54, 0x74, 0x94, 0xB4}};
#define MOD_MENU_BTN_X(i) ((s32)g_ModBtnX[MOD_DRAMA_ON()][i])
#define MOD_MENU_BTN_Y(i) ((s32)g_ModBtnY[MOD_DRAMA_ON()][i])
#define MOD_SOUND_BTN_TPAGE 0xF // 4bpp, x 960 y 0
#define MOD_SOUND_BTN_V 96
static void ModSoundBtnUpload(void) {
    RECT rect;

    rect.x = MOD_SOUND_BTN_TPAGE * 64;
    rect.y = MOD_SOUND_BTN_V;
    rect.w = MOD_SOUND_BTN_W / 4;
    rect.h = MOD_SOUND_BTN_H;
    LoadImage(&rect, (u_long*)g_ModSoundBtnPix);
}
// 094_18: the 6th button, "Drama cd" (MOD_MENU_DRAMA), VRAM x 960 y 256 = tpage 0x1F u 0 v 0 (unchanged and
// unreferenced on the title / menu / file select / name entry / sound mode screens, 094_17 dumps)
#include "../../../../src/st/sel/mod_sel_dramabtn_gfx.h"
#define MOD_DRAMA_BTN_TPAGE 0x1F
static void ModDramaBtnUpload(void) {
    RECT rect;

    rect.x = 960;
    rect.y = 256;
    rect.w = MOD_DRAMA_BTN_W / 4;
    rect.h = MOD_DRAMA_BTN_H;
    LoadImage(&rect, (u_long*)g_ModDramaBtnPix);
}
