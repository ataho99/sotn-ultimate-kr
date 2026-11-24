// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate: included by src/dra/cd.c (at file scope, in place)
// Saturn port: Richter's / the Marias' voice banks for the character and
// the dub: the overlay's load (CdFile_RichterPrg) goes on to VAB/SD_RIH
// (vab 1) and VAB/SD_MAR (vab 2, the prologue's Maria). For her those four
// CdFiles (consecutive, g_LbaSdRihVh..g_LbaSdMarVb) read BIN/MARIA_SD.BIN,
// her two banks laid out as those files are
// (mods/sotn-ultimate/tools/gen_mar_sounds.py). Also called by their pause
// menu's Extra settings after a Dub change
// (mods/sotn-ultimate/src/menu/charmenu.c CharBanks), which links the chain
// on to the stage's bank: cleared here, at every load
void ModCharSdLocs(void) {
    CdFile* sd = &g_LbaSdRihVh;
#if OFF_BIN_RIH_PSP != 0
    // PSP dub: Richter's PSP grunts (BIN/RIH_PSP.BIN, his vab 1 with the
    // PSP clips + the retail SD_MAR, the same layout;
    // mods/sotn-ultimate/tools/gen_psp_grunts.py). Maria's banks replace it
    // below (the JP dub: BIN/JP_SD.BIN's copy with the Japanese clips, and
    // the prologue Maria's in its SD_MAR)
    // KO field voices (game_mod.h MOD_FIELD_JP_VOICE): the dub banks are not used then (both macros are 0) and the standard VAB/SD_MAR
    // (the Japanese field voices grafted in, SD_MAR_VB_LEN long) is read; RIH_PSP.BIN's copy of SD_MAR is the retail 0xCEA0 long and
    // would be transferred 0xE290 long (SsVabTransBodyPartly error, Now Loading never ends). Without field voices the macros are
    // MOD_JP_DUB_ON() / MOD_PSP_DUB_ON(), the upstream conditions.
    s32 sdOff = MOD_FIELD_JP_BANKS() && OFF_BIN_JP_SD
                    ? OFF_BIN_JP_SD + MOD_JP_SD_RIH - 0x612D
                : MOD_FIELD_PSP_BANKS() ? OFF_BIN_RIH_PSP - 0x612D
                                        : 0;
#else
    s32 sdOff = 0;
#endif
    s32 sdLen = SD_MAR_VB_LEN;

    if (g_PlayableCharacter == PLAYER_MARIA) {
        // her Japanese Saturn voice for the JP dub, the PS1 dub's bank
        // (BIN/MARIA_SD_PS1.BIN: a copy of the PSP one, as her select panel
        // has no PS1 dub), else the PSP English lines
        // (BIN/MARIA_SD_PSP.BIN); all the same layout
        sdOff = (MOD_JP_DUB_ON() || !OFF_BIN_MARIA_SD_PSP ? OFF_BIN_MARIA_SD
                 : !MOD_PSP_DUB_ON() && OFF_BIN_MARIA_SD_PS1
                     ? OFF_BIN_MARIA_SD_PS1
                     : OFF_BIN_MARIA_SD_PSP) -
                0x612D;
        sdLen = vab2Len; // her vab 2 is larger than SD_MAR's
#if OFF_BIN_MARIA2 != 0 && OFF_BIN_MARIA2_SD != 0
        // the PSP Maria: her PSP voice in BIN/MARIA2_SD.BIN (same layout
        // and vab 2 size as MARIA_SD.BIN); the PS1 dub: her bank after the
        // Saturn Maria's in MARIA_SD_PS1.BIN (a copy of MARIA2_SD.BIN;
        // build_disc.sh)
        if (MOD_MARIA_PSP()) {
            sdOff = (MOD_JP_DUB_ON() && OFF_BIN_JP_SD
                         ? OFF_BIN_JP_SD + MOD_JP_SD_MAR2
                     : !MOD_PSP_DUB_ON() && OFF_BIN_MARIA_SD_PS1
                         ? OFF_BIN_MARIA_SD_PS1 + MOD_MAR_SD_SECTORS
                         : OFF_BIN_MARIA2_SD) -
                    0x612D;
        }
#endif
    }
    sd[0].loc = 0x612D + sdOff;
    sd[1].loc = 0x6130 + sdOff;
    sd[2].loc = 0x614D + sdOff;
    sd[3].loc = 0x614F + sdOff;
    sd[3].size = sdLen;
    g_LbaSdMarVh.nextCdFileType = CdFile_NoNext;
}
