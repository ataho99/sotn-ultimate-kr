// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate (Korean build): the Extra settings popup's Korean 8px rows (099b: moved here from ARC_F2's sysopt.c), linked into the new resident area
// (build_newarea.sh). PopDrawKo is called by ARC_F2's ModSysOptSound with the popup's statics passed in (ModPopArgs, game_mod.h).
#include <game.h>
#include "../menu/sysopt_ko_str.h" // the popup's strings (gen_ko_cfg.py): ARC_F2 no longer refers to them, so its -O2 build drops them

#define KO_EQUIP_SEL 0x8009F500 // src/dra/dra.h (build_newarea.sh checks it)
#define NOPT MOD_SYS_NOPT
#define NCHEAT MOD_SYS_NCHEAT
#define ROWS (NOPT + NCHEAT)
#define VALUE_DX MOD_SYS_VALUE_DX
extern s32 D_8013784C; // menu.c: 1 = grey text

typedef void (*KoDrawFn)(const char**, const s32*, const s32*, s32, MenuContext*);
static const char* const g_SysOptLabelKo[ROWS] = {
    ko_xset_r0, ko_xset_r1, ko_xset_r2, ko_xset_r3, ko_xset_c0, ko_xset_c1,
    ko_xset_c2, ko_xset_c3, ko_xset_c4, ko_xset_c5, ko_xset_c6, ko_xset_c7,
    ko_xset_c8, ko_xset_c9};
// as the option names: PS1, Saturn (new: 새턴), PSP, JP (never shown), STN, OFF, ON
static const char* const g_SysOptNameKo[7] = {
    ko_xset_ps1, ko_xset_sat, ko_xset_psp, ko_xset_psp,
    ko_xset_stn, ko_xset_off, ko_xset_on};

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

void PopDrawKo(MenuContext* ctx, s32 x, s32 y, ModPopArgs* a) {
    const char* names[2 * NCHEAT];
    s32 xs[2 * NCHEAT];
    s32 ys[2 * NCHEAT];
    const SysOptRow* row;
    s32 r, n, pass, locked, ry;

    for (pass = 0; pass < 2; pass++) {
        n = 0;
        for (r = a->top; r < a->top + a->popn; r++) {
            locked = r < NOPT ? (a->locks >> r) & 1 : 0;
            if (locked != pass) {
                continue;
            }
            ry = y + (r - a->top) * 16;
            names[n] = g_SysOptLabelKo[r];
            xs[n] = x;
            ys[n] = ry;
            n++;
            if (r < NOPT) {
                row = &a->rows[r];
                names[n] = g_SysOptNameKo[row->name[SysOptCur(row)]];
            } else {
                names[n] = g_SysOptNameKo[((a->cheatMask >> (r - NOPT)) & 1) ? 6 : 5];
            }
            xs[n] = x + VALUE_DX;
            ys[n] = ry;
            n++;
        }
        if (n) {
            D_8013784C = pass;
            ((KoDrawFn)KO_EQUIP_SEL)(names, xs, ys, n, ctx);
        }
    }
    D_8013784C = 0;
}
