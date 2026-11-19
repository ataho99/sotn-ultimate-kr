// SPDX-License-Identifier: AGPL-3.0-or-later
// Trial10b KO blob (MOD_KO trial only): linked at KO_BLOB_ADDR (A-gap, right
// after the 776 glyphs), loaded as the tail of ARC_KO.BIN. MenuDrawStr's F0xx
// branch calls MenuDrawKoHook (absolute, 12px advance).
//
// Never linked into DRA (built by build_ko_trial.sh's trial10 section only);
// no .bss/.data allowed (the script fails the build otherwise). Scratch and
// the slot ring state are fixed A-gap addresses (write-before-read).
//
// Trial10b fixes: each char gets its own VRAM slot (ring of 64, 12px pitch)
// so uploads no longer clobber each other; u0/tpage are derived from the
// actual slot address (the old 0x1F0*4/u0 truncation sampled blank VRAM).
// Trial10r (fix A): the RAM unpack strip is per-slot too (ring of 16,
// KO_SCRATCH_ADDR + (idx & 15) * 0x60). One shared strip was re-expanded
// before the queued LoadTPage ran, so every slot sampled the last glyph.
// 10q DIAG removed; screen code back to K103 + this fix.
// KO_FLAG_ADDR (default 1): 0 bypasses the hook (blank 12px gap, caller
// still advances x) for on/off comparison from the emulator.
#include "dra.h"

#if defined(MOD_KO)

// Trial10 status-box strings (v11): ONE packed 0xFF00-terminated array (old cc1
// pads separate arrays, so a single literal keeps dra.h KO_STR_9x offsets
// exact). Order + bytes asserted by build_ko_trial.sh.
// 92 장비 @+0 / 93 기술 @+6 / 94 마도기 @+12 / 95 설정 @+20 / 96 사역마 @+26
// (measured row bytes 6/6/8/6/7+NUL; matches dra.h KO_STR_9x)
const char KO_BOXALL[] =
    "\xF0\x80\xF0\x5E\xFF\x00"
    "\xF0\x46\xF0\x67\xFF\x00"
    "\xF0\x52\xF1\x41\xF0\x46\xFF\x00"
    "\xF1\xAB\xF0\x81\xFF\x00"
    "\xF0\xAB\xF1\xCC\xF0\x52\xFF";
// Sheet tables (ko_sheet #80): upload list (29B with NUL) + 7 index
// strings (23B). Appended after KO_BOXALL; layout asserted by
// build_ko_trial.sh (ro == 89B = 34 + 2 pad + 52 + 1 NUL).
// #249: hand labels appended (upload 37B + 9 index strings 30B = 67B;
// ro == 104B = 34 + 2 + 67 + 1 NUL). New glyphs 오/른/손/왼 = cells 14-17.
const char KO_SHEET_ALL[] =
    "\xF0\x80\xF0\x5E\xF0\x46\xF0\x67\xF0\x52\xF1\x41\xF1\xAB"
    "\xF0\x81\xF0\xAB\xF1\xCC\xF0\xA9\xF0\xB0\xF0\xCA\xF0\x71"
    "\xF0\x77\xF0\xC5\xF0\x66\xF2\xD2"
    "\x00\x00\x01\xFF\x02\x03\xFF\x04\x05\x02\xFF\x06\x07\xFF"
    "\x08\x09\x04\xFF\x0A\x0B\xFF\x0C\x0D\xFF\x10\x11\x12\xFF"
    "\x13\x12\xFF";
extern u32 g_Timer;
extern GpuBuffer* g_CurrentBuffer;
extern GpuUsage g_GpuUsage;
extern Vram g_Vram;
extern void SetDrawMode(DR_MODE* p, int dfe, int dtd, int tpage, RECT* tw);
extern void AddPrim(void* ot, void* p);

static void MenuDrawKoChar(u16 ch16, s32 x, s32 y, MenuContext* ctx);

// Entry: must stay the first function (offset 0 = KO_BLOB_ADDR).
void MenuDrawKoHook(u16 ch16, s32 x, s32 y, void* ctx) {
    if (*(volatile u8*)KO_FLAG_ADDR == 0) {
        return;
    }
    MenuDrawKoChar(ch16, x, y, (MenuContext*)ctx);
}

// KO double-byte glyph blit for the 8x8 MenuDraw path. Glyph pixels come
// from func_80106A28 (same source as ShowText): 12px wide, drawn as 8px +
// 4px halves sampling one per-char VRAM slot (u0/tpage derived from the
// slot address). The slot ring resets when g_Timer changes (each frame).
// 12px advance (done by the caller).
static void MenuDrawKoChar(u16 ch16, s32 x, s32 y, MenuContext* ctx) {
    const s32 Width = 12;
    const s32 Height = 16;
    u8* glyph;
    s32 i;
    u8* dst;
    u8* scratch;
    u32 now = g_Timer;
    u8 idx;
    s32 sx, sy, tpage, u0, v0;
    OT_TYPE* ot;
    DR_MODE* drawMode;

    if (*(volatile u32*)KO_SLOT_TMR != now) {
        *(volatile u32*)KO_SLOT_TMR = now;
        *(volatile u8*)KO_SLOT_IDX = 0;
    }
    idx = *(volatile u8*)KO_SLOT_IDX;
    *(volatile u8*)KO_SLOT_IDX = (idx + 1) & 63;
    // Same idx drives the VRAM slot (sx below) and the RAM strip. At most
    // the 5-row box (12 chars) draws per frame, so live indices never share
    // a strip within a frame (16 strips).
    scratch = (u8*)(KO_SCRATCH_ADDR + (idx & (KO_SCRATCH_N - 1)) * 0x60);
    // Trial10m: single band BELOW both 240-row frames (frame1 ends at y=496).
    // The old bands (sy 336/400/416) sat on framebuffer text: uploads
    // scribbled the menu and SPRTs sampled menu rows (stripes, HEART smudge).
    sx = 64 + idx * Width;
    sy = 496;
    tpage = (sx >> 6) | 0x10;
    // Trial10p: sx is a halfword rect-x but u0 is texels (4bpp: 4px/hw).
    // (sx&63) alone samples (sx&63)/4 hw left of the upload (dots); K78
    // B-renderer precedent: u = rect_x_hw * 4.
    u0 = (sx & 63) * 4;
    v0 = sy - 256;

    glyph = (u8*)func_80106A28(ch16, 0);
    for (i = 0; i < Height; i++) {
        dst = &scratch[i * Width / 2];
        *dst++ = *glyph++;
        *dst++ = *glyph++;
        *dst++ = *glyph++;
        *dst++ = *glyph++;
        *dst++ = *glyph++;
        *dst++ = *glyph++;
    }
    LoadTPage((u_long*)scratch, 0, 0, sx, sy, Width, Height);
    // OT is LIFO within an entry: the DR_MODE must be added AFTER the two
    // SPRTs so it executes first (10k sampled the stale tpage: stripes).
    // func_800F53D4 inside MenuDrawImg is gated off during MenuDrawStr
    // (D_80137614 == 0), so this explicit mode is the only tpage install.
    MenuDrawImg(ctx, x, y, 8, Height, u0, v0, 0x196, tpage, 1, 0);
    MenuDrawImg(ctx, x + 8, y, 4, Height, u0 + 8, v0, 0x196, tpage, 1, 0);
    ot = g_CurrentBuffer->ot;
    drawMode = &g_CurrentBuffer->drawModes[g_GpuUsage.drawModes];
    SetDrawMode(drawMode, 0, 0, tpage, &g_Vram.D_800ACD80);
    AddPrim(&ot[ctx->otIdx + 2], drawMode);
    g_GpuUsage.drawModes++;
}

#endif
