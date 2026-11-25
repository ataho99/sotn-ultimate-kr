// SPDX-License-Identifier: AGPL-3.0-or-later
// 100 KO: familiar voice subtitles, 8px, under the familiar's body and following it. The drawing moved to the new resident area
// (mods/sotn-ultimate/src/easy/newarea_sub.c, header entry +20 = SubCmd; R1Tick calls SubFollow every frame): the servant overlays only say
// "show this voice id" when the XA stream starts and "hide" when it ends (ProcessSfxState, play_sfx.h: step 3 xa_play_ck true = SHOW, step 4
// end = HIDE, steps 0 / 5 / 6 / 8 = HIDE). The 094_17 12px bottom-of-screen code (1,492 B in tt_002) is gone from the overlays; its design
// is in git history (ko_sub.h 094_17). The faerie (tt_002) and the demon (tt_003) define KO_SUB_FAERIE / KO_SUB_DEMON; the others keep the
// empty macros of play_sfx.h. No area (an older disc, a room loaded before the area) = no subtitle, never a wait or a branch.
#ifndef KO_SUB_H
#define KO_SUB_H
#define KO_SUB_ENABLE 1 /* 094_16 = 0 (A-gap fix only, no subtitles); 100 = the 8px area version */
#if KO_SUB_ENABLE && defined(MOD_KO) && (defined(KO_SUB_FAERIE) || defined(KO_SUB_DEMON))
#define KO_SUBTITLE 1
#define KO_AREA_BASE 0x801F3000 /* MOD_NEWAREA_BASE (mods/sotn-ultimate/include/game_mod.h) */
#define KO_AREA_MAGIC 0x4B4F5231 /* 'KOR1' */
// the area's header word +20: SubCmd(cmd, id): 1 = show the voice id, 2 = hide
static void KoSubCmd(s32 cmd, s32 id) {
    u32* h = (u32*)KO_AREA_BASE;

    if (h[0] == KO_AREA_MAGIC && h[5] != 0) {
        ((void (*)(s32, s32))h[5])(cmd, id);
    }
}
#define KO_SUB_SHOW(id) KoSubCmd(1, id)
#define KO_SUB_HIDE() KoSubCmd(2, 0)
#endif
#endif
