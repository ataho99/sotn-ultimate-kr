// SPDX-License-Identifier: AGPL-3.0-or-later
// ko_equipreloc: shared declarations for the 4-TU resident tail.
// One link, fixed entry addresses (linker script): sel/ov/enter + core.
// No .bss/.data/.rodata (state = A-gap absolute, tables computed/on-stack).
#include "dra.h"

#if defined(MOD_KO)

#define KO_EQ_SEL_CELLS 64 // groups 0,1,2 (24+24+16)
#define KO_EQ_OV_CELLS 41 // groups 3,4 (24+17)

// Layout (boot-loaded, base/sectors truth in dra.h for cd.c).
// Layout: u16 count, count x u16 codes (LE), count x 8B cells.
#define KO_8P_MAX 512
// Disc LBA: DUMMY.BIN filler (275 zero sectors) at 0x18; KO_8P.BIN spliced
// into its first 2 sectors (readback; no longer read at runtime).
#define KO_8P_LBA 0x18

// Upload-once dirty state: A-gap absolute (zero gap 9EED8~9EEF4, like FLAG).
// DRA keeps no BSS for this (check_dra_layout).
#define KO_EQ_SELSIG (*(volatile u32*)0x8009EED8)
#define KO_EQ_OVSIG (*(volatile u32*)0x8009EEDC)
#define KO_EQ_EPOCH (*(volatile u32*)0x8009EEE0)

extern void func_800F98AC(const char* str, u32 y);
extern void func_800F66BC(
    const char* str, s32 x, s32 y, MenuContext* ctx, s32 disableTexShade);
extern void MenuDrawImg(
    MenuContext* ctx, s32 x, s32 y, s32 w, u32 h, s32 u, s32 v, s32 tpage,
    s32 clut, s32 disableTexShade, s32 c);

s32 KO_8pFind(u16 code);
s32 KO_8pRow(s32 g);
void KO_8pLoad(void);
s32 KO_8pValid(void);
void KO_8pRC(s32 cell, s32* grp, s32* col, s32 isSel);
u32 KO_EqSig(const char** names, s32 count);
s32 KO_EqParseName(const char* str, s32* idx, s32 cap);
void KO_8pExpand(s32 cell, u8* dst);
void KO_8pUpload(s32* cells, s32 nCells, s32 isSel);
void KO_8pDrawIdx(const char* idx, const s32* spc, s32 x, s32 y, MenuContext* ctx);
void KO_EqDraw(const char** names, const s32* xs, const s32* ys, s32 count,
               MenuContext* ctx, s32 isSel);

#endif
