// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate: included by src/dra/cd.c (at file scope, in place)
#ifndef OFF_BIN_RIC2
// (the first compile of make disk, before it regenerates lba_us.h with the
// file in it): the costume branch is left out
#define OFF_BIN_RIC2 0
#endif
#ifndef OFF_BIN_BO6RIC2
#define OFF_BIN_BO6RIC2 0 // same first-compile case (BIN/BO6RIC2.BIN)
#endif
#ifndef OFF_BIN_SELRIC2
#define OFF_BIN_SELRIC2 0 // same first-compile case (BIN/SELRIC2.BIN)
#endif
#ifndef OFF_BIN_MARIA
#define OFF_BIN_MARIA 0 // same first-compile case (BIN/MARIA.BIN)
#define LEN_BIN_MARIA 0
#endif
#ifndef OFF_BIN_MARIA_SD
#define OFF_BIN_MARIA_SD 0 // same first-compile case (BIN/MARIA_SD.BIN)
#endif
#ifndef OFF_BIN_MARIA_SD_PSP
#define OFF_BIN_MARIA_SD_PSP 0 // same first-compile case
#endif
// one Maria voice file's sectors (MARIA_SD*.BIN, MARIA2_SD*.BIN: SD_RIH.VH /
// VB, SD_MAR.VH / VB as mods/sotn-ultimate/tools/gen_mar_sounds.py lays them
// out)
#define MOD_MAR_SD_SECTORS 0x42
#ifndef OFF_BIN_MARIA_SD_PS1
#define OFF_BIN_MARIA_SD_PS1 0 // same first-compile case
#endif
#ifndef OFF_BIN_ALK_PSP
#define OFF_BIN_ALK_PSP 0 // same first-compile case (BIN/ALK_PSP.BIN)
#endif
#ifndef OFF_BIN_RIH_PSP
#define OFF_BIN_RIH_PSP 0 // same first-compile case (BIN/RIH_PSP.BIN)
#endif
#ifndef OFF_BIN_SFX_PSP
#define OFF_BIN_SFX_PSP 0 // same first-compile case (BIN/SFX_PSP.BIN)
#endif
#ifndef OFF_BIN_NO3RIDE
#define OFF_BIN_NO3RIDE 0 // same first-compile case (BIN/NO3RIDE.BIN)
#endif
// Richter's / the Marias' pause menu code (src/menu/charmenu.c, 4 sectors):
// after ARC_F2.BIN's MOD_ARC_F_LEN bytes, which are all g_LbaBinAlucard reads
// (a new file in BIN/ would move the retail ones: the directory is full)
#define OFF_BIN_CHARMENU (OFF_BIN_ARC_F2 + MOD_ARC_F_LEN / 0x800)
// a bank in BIN/NO3RIDE.BIN: SD_ZKNO3.VH (4 sectors) + VB (53)
#define MOD_NO3RIDE_SECTORS 57
#ifndef OFF_BIN_MONSTER2
#define OFF_BIN_MONSTER2 0 // same first-compile case (BIN/MONSTER2.BIN)
#endif
#ifndef OFF_BIN_JP_SD
#define OFF_BIN_JP_SD 0 // same first-compile case (BIN/JP_SD.BIN)
#endif
// The JP dub (include/game.h MOD_JP_DUB_ON): its banks in BIN/JP_SD.BIN, at
// these sectors (mods/sotn-ultimate/tools/gen_jp_sd.py checks them), each laid
// out as the file it stands in for: ALK_PSP.BIN, RIH_PSP.BIN, MARIA2_SD.BIN,
// CENP.BIN's bank; then the stage / familiar banks (g_ModDub.jp.sfx, ModJpSfx)
#define MOD_JP_SD_ALK 0
#define MOD_JP_SD_RIH 32
#define MOD_JP_SD_MAR2 92
#define MOD_JP_SD_CENP 158
#define MOD_JP_SD_SFX 215
#if OFF_BIN_JP_SD != 0
// The JP dub's version of the PSP dub's stage / familiar bank psp (its
// SFX_PSP.BIN sector + 1): its sector, -1 for the retail bank, 0 for the
// PSP one (the Japanese clips are the same, or no JP dub)
static s32 ModJpSfx(s32 psp) {
    u16(*p)[2];

    if (!MOD_JP_DUB_ON() || g_ModDub.jp.magic != MOD_JP_MAGIC) {
        return 0;
    }
    for (p = g_ModDub.jp.sfx; (*p)[0]; p++) {
        if ((*p)[0] == psp) {
            return (*p)[1] == 0xFFFF ? -1 : OFF_BIN_JP_SD - 1 + (*p)[1];
        }
    }
    return 0;
}
#endif
#if defined(MOD_SATURN)
// Saturn port: the Saturn Maria's sounds take Richter's and Alucard's voice
// ids (gen_mar_sounds.py), so her shots / hits / beasts had her voice's
// priority and took its channel pair (sound.c func_8013572C: equal or lower
// is replaced), and her slide (30 / 40) lost to any stage sound. While she is
// loaded: her voice 90, her sound effects under it (as Alucard's weapons are
// under his voice), her slide 85; retail again for Richter, the PSP Maria
// and Alucard (DRA keeps the table for the session). Her hits play the
// common hit sounds (src/st/collision.h; 30-40, the Saturn's): at 65, or her
// shots cut them off. Seiryuu's sound (0x889) is a loop for her: channels
// 22/23, which her dragon releases (src/ric). Death 0x6FF / grab 0x6F6 are
// 90 in retail too; she never plays 0x705 (collision.h).
// The PSP Maria (column psp; user report 2026-11-13) had the same clash: her
// 16 voice clips sit in Richter's voice slots (src/maria_psp/mar2.c
// g_Mar2SfxSlot), three of them in 0x706-0x708 (30-40, cut by any sound),
// and her hits (30-40) tied with her moves' sounds (10-40): her voice 90 on
// its own channels (ModMarSfxPrio), the hits and the rest retail
static const struct {
    u16 id;
    u8 retail;
    u8 maria;
    u8 psp;
} g_ModMarSfxPrio[] = {
    // id, retail, Saturn Maria, PSP Maria
    {0x6F9, 80, 90, 90}, {0x6FA, 80, 90, 90},
    {0x6FB, 80, 90, 90},                      // grunts
    {0x6FC, 80, 90, 90}, {0x702, 81, 90, 90}, // kick, hurt
    {0x703, 81, 90, 90}, {0x704, 81, 90, 90}, // hurt
    {0x678, 40, 65, 40}, {0x62D, 30, 65, 30},
    {0x62E, 32, 65, 32}, // her hits
    {0x6FD, 80, 60, 90}, {0x6FE, 80, 60, 90},
    {0x700, 80, 60, 90},                      // shot, fire, ball
    {0x6F4, 80, 60, 80}, {0x6F5, 80, 60, 80}, // lunge, beasts end
    {0x6F7, 50, 60, 50}, {0x887, 90, 70, 90},
    {0x888, 90, 70, 90},                      // flap, Genbu, Byakko
    {0x889, 90, 70, 90}, {0x88A, 90, 70, 90}, // Seiryuu, its roar
    {0x707, 40, 85, 90}, {0x617, 30, 85, 30}, // slide skid, scrape
    {0x701, 80, 80, 90}, {0x705, 80, 80, 90}, // PSP voice slots
    {0x706, 30, 30, 90}, {0x708, 40, 40, 90}, // (Richter's sounds)
};
// 0: retail (Richter, Alucard), 1: the Saturn Maria, 2: the PSP Maria
static void ModMarSfxPrio(s32 maria) {
    s32 i;

    for (i = 0; i < LEN(g_ModMarSfxPrio); i++) {
        g_SfxData[g_ModMarSfxPrio[i].id - SFX_START].unk6 =
            maria == 2 ? g_ModMarSfxPrio[i].psp
            : maria    ? g_ModMarSfxPrio[i].maria
                       : g_ModMarSfxPrio[i].retail;
    }
    g_SfxData[0x889 - SFX_START].mode =
        maria == 1 ? SFX_MODE_CHANNELS_22_23 : SFX_MODE_CHANNELS_12_19;
    // the PSP Maria's voice clips (vab 1 slots 0x6F9-0x708) on channels 20/21:
    // on the shared channel her lines muted every hit and torch sound while
    // she spoke (only one effect plays there during music). A new line cuts
    // the last, like a voice; 20/21 otherwise carry only the rain loop, the
    // prologue's Death ambience and the Succubus reveal
    for (i = 0x6F9; i <= 0x708; i++) {
        g_SfxData[i - SFX_START].mode =
            maria == 2 ? SFX_MODE_CHANNELS_20_21 : SFX_MODE_CHANNELS_12_19;
    }
}
#endif
// the Saturn-only bestiary pictures (bits 146.., MOD_LIB_RETAIL_ENEMIES in
// src/st/lib/mod_bestiary.h) are in BIN/MONSTER2.BIN, record bit - 146
#define MOD_FIRST_MONSTER2 146
#if OFF_BIN_SFX_PSP != 0
#include "../../../../src/dra/mod_psp_sfx.h"
// PSP dub: the familiars' banks with the PSP's voices, by servant: VH
// sector in BIN/SFX_PSP.BIN + 1, 0 for the retail bank
static const u16 g_ModPspServantSfx[] = MOD_PSP_SFX_SERVANTS;
#endif
// Saturn port: Richter's alternate costume. BIN/RIC2.BIN ends its sprite
// sheet (just before spritebank0) with {magic, pointer}; the pointer leads to
// {count, {palette id, 16 colours}...} and {count, {VRAM rect, pixels}...}
// (the whip-brandish arms in F_GAME2's VRAM). RIC.BIN has sprite pixels there,
// so the magic never matches for the normal costume. Runs after every load of
// the overlay, which always follows F_GAME2's (CdFile_GameChr chains it), so
// nothing reloads the retail colours or arms afterwards.
#define MOD_RIC2_HDR ((u32*)(0x8013C000 + 0x170AC - 8))
static void ModRicCostumeApply(void) {
    u32* p;
    s32 n;
    s32 i;
    u16 id;
    RECT rect;

    if (MOD_RIC2_HDR[0] != MOD_RIC_COSTUME2_MAGIC) {
        return;
    }
    p = (u32*)MOD_RIC2_HDR[1];
    for (n = *p++; n > 0; n--) {
        id = *p++;
        for (i = 0; i < COLORS_PER_PAL; i++) {
            g_Clut[1][(id - 0x100) * COLORS_PER_PAL + i] = ((u16*)p)[i];
        }
        rect.x = (g_ClutIds[id] & 0x3F) << 4;
        rect.y = g_ClutIds[id] >> 6;
        rect.w = COLORS_PER_PAL;
        rect.h = 1;
        LoadImage(&rect, (u_long*)p);
        p += COLORS_PER_PAL / 2;
    }
    for (n = *p++; n > 0; n--) {
        rect.x = ((u16*)p)[0];
        rect.y = ((u16*)p)[1];
        rect.w = ((u16*)p)[2];
        rect.h = ((u16*)p)[3];
        p += 2;
        LoadImage(&rect, (u_long*)p);
        p += (rect.w * rect.h + 1) / 2;
    }
}
