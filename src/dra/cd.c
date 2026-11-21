// SPDX-License-Identifier: AGPL-3.0-or-later
#include "dra.h"
#include "dra_bss.h"
#include "lba.h"
#include "sfx.h"

#if USE_CD_SPEED_DOUBLE == 0
#define CDL_MODE_SPEED CdlModeSpeedNormal
#else
#define CDL_MODE_SPEED CdlModeSpeedDouble
#endif

#define _CD_BLOCK 11 // 2048 (0x800) bytes per sector
#define CD_BLOCK_LEN (1 << (_CD_BLOCK))
#define TO_CD_BLOCK(x) ((x) << (_CD_BLOCK))

s16 D_800AC958[] = {
    0x200, 0x220, 0x200, 0x220, 0x240, 0x260, 0x240, 0x260, 0x280, 0x2A0, 0x280,
    0x2A0, 0x2C0, 0x2E0, 0x2C0, 0x2E0, 0x300, 0x320, 0x300, 0x320, 0x340, 0x360,
    0x340, 0x360, 0x380, 0x3A0, 0x380, 0x3A0, 0x3C0, 0x3E0, 0x3C0, 0x3E0,
};
s16 D_800AC998[] = {
    0x200, 0x220, 0x200, 0x220, 0x240, 0x260, 0x240, 0x260, 0x280, 0x2A0, 0x280,
    0x2A0, 0x2C0, 0x2E0, 0x2C0, 0x2E0, 0x300, 0x320, 0x300, 0x320, 0x340, 0x360,
    0x340, 0x360, 0x380, 0x3A0, 0x380, 0x3A0, 0x3C0, 0x3E0, 0x3C0, 0x3E0,
};

#if defined(VERSION_US)
CdFile g_LbaBinTitle = {KO_LBA(0x63D6), 0, 0x80000, 0, 0, 0xFF, 0};
CdFile g_LbaBinProlo = {KO_LBA(0x62D6), 6, 0x80000, 0, 0, 0xFF, 0};
CdFile g_LbaBinGame = {0x0000, 1, 0x42000, 0, 0, 0x13, 0};
CdFile g_LbaStageChr = {0x7E5D, 2, 0x40000, 0, 0, 0xFF, 0};
CdFile g_LbaSdAlkVh = {0x610D, 19, 0x01020, 1, 1, 0xFF, 7};
CdFile g_LbaSdAlkVb = {0x6110, 17, 0x0E160, 1, 1, 0xFF, 0};
CdFile g_LbaSdRihVh = {0x612D, 19, 0x01020, 1, 1, 0x16, 21};
CdFile g_LbaSdRihVb = {0x6130, 17, 0x0E190, 1, 1, 0xFF, 0};
CdFile g_LbaSdMarVh = {0x614D, 19, 0x00E20, 2, 2, 0xFF, 23};
#if defined(MOD_FIELD_JP_VOICE)
#define MOD_SD_MAR_VB_LEN SD_MAR_VB_LEN /* the Japanese bank grafted in (gen_ps1jp_banks.py): longer than retail's 0xCEA0 */
#else
#define MOD_SD_MAR_VB_LEN 0x0CEA0
#endif
CdFile g_LbaSdMarVb = {0x614F, 17, MOD_SD_MAR_VB_LEN, 2, 2, 0xFF, 0};
CdFile g_LbaSdJ010_1_Vh = {0x6082, 19, 0x02A20, 0, 0, 0x0A, 9};
CdFile g_LbaSdJ010_1_Vb = {0x6088, 17, 0x41CB0, 0, 0, 0xFF, 0};
CdFile g_LbaSdJ010_2_Vh = {0x6082, 19, 0x41CB0, 4, 4, 0x0C, 11};
CdFile g_LbaSdJ010_2_Vb = {0x6088, 17, 0x1A3A0, 4, 4, 0x0C, 0};
CdFile g_LbaStageSeq = {0x0000, 18, 0x00000, 0, 6, 0xFF, 0};
CdFile g_LbaStageVh = {0x0000, 19, 0x00000, 3, 3, 0xFF, 14};
CdFile g_LbaStageVb = {0x0000, 17, 0x00000, 3, 3, 0xFF, 0};
CdFile g_LbaSdJ010_3_Vh = {0x6082, 19, 0x02A20, 3, 3, 0xFF, 16};
CdFile g_LbaSdJ010_3_Vb = {0x6088, 17, 0x41CB0, 3, 3, 0xFF, 0};
CdFile g_LbaBinWeapon0 = {KO_LBA(0x6582), 12, 0x04000, 0, 0, 0xFF, 0};
CdFile g_LbaBinWeapon1 = {KO_LBA(0x68BC), 13, 0x04000, 0, 0, 0xFF, 1};
CdFile g_LbaBinMonster = {KO_LBA(0x6BF6), 14, 0x05800, 0, 0, 0xFF, 0};
CdFile g_LbaBinDemokey = {KO_LBA(0x7415), 4, 0x02000, 0, 0, 0xFF, 0};

// SERVANT/FT_xxx.BIN off
s32 g_LbaServantChrOff[] = {
    KO_LBA(0x7248), KO_LBA(0x728B), KO_LBA(0x72CE), KO_LBA(0x7311), KO_LBA(0x7350), KO_LBA(0x7392), KO_LBA(0x73D5),
};

// SERVANT/SD_xxx.VH off
s32 g_LbaServantVhOff[] = {
    KO_LBA(0x7268), KO_LBA(0x72AB), KO_LBA(0x72EE), KO_LBA(0x7331), KO_LBA(0x7370), KO_LBA(0x73B2), KO_LBA(0x73F5),
};

// SERVANT/SD_xxx.VH len
s32 g_LbaServantVhLen[] = {
    4128, 4128, 4128, 3616, 3104, 4128, 3616,
};

// SERVANT/SD_xxx.VB off
s32 g_LbaServantVbOff[] = {
    KO_LBA(0x726B), KO_LBA(0x72AE), KO_LBA(0x72F1), KO_LBA(0x7333), KO_LBA(0x7372), KO_LBA(0x73B5), KO_LBA(0x73F7),
};

// SERVANT/SD_xxx.VB len
s32 g_LbaServantVbLen[] = {
    63904, 63904, 63552, 59392, 64496, 63904, 60992,
};

CdFile g_LbaServantChr = {0x0000, 15, 0x06000, 0, 0, 0x1D, 0};
CdFile g_LbaServantVh = {0x0000, 19, 0x00000, 2, 2, 0xFF, 30};
CdFile g_LbaServantVb = {0x0000, 17, 0x00000, 2, 2, 0xFF, 0};
CdFile g_LbaBinRichter = {KO_LBA(0x64D6), 10, 0x39A58, 0, 0, KO_RICHTER_NEXT, 0};
#if defined(MOD_SATURN)
#ifndef OFF_BIN_ARC_F2
#define OFF_BIN_ARC_F2 0 // (the first compile of make disk, as below)
#endif
#ifndef OFF_BIN_KO
#define OFF_BIN_KO 0 // trial ARC_KO.BIN (A-gap); OFF has none
#endif
#endif
#if defined(MOD_SATURN) && OFF_BIN_ARC_F2 != 0
// Saturn port: BIN/ARC_F2.BIN = Alucard's sprites (retail ARC_F.BIN) + the
// System screen's "Ultimate settings" code after them
// (mods/sotn-ultimate/src/mod_sysopt,
// mods/sotn-ultimate/tools/build_sysopt.sh), a new file at the disc's end (no
// file moves; the retail ARC_F.BIN stays where it was, unused), loaded up to
// 0x80170000
CdFile g_LbaBinAlucard = {OFF_BIN_ARC_F2, 10, MOD_ARC_F_LEN, 0, 0, KO_ALUCARD_NEXT, 0};
#else
CdFile g_LbaBinAlucard = {KO_LBA(0x616A), 10, 0x31BEC, 0, 0, KO_ALUCARD_NEXT, 0};
#endif
#if defined(MOD_KO)
CdFile g_LbaBinKoGlyphs = {OFF_BIN_KO, 20, KO_ARC_SIZE, 0, 0, 0x04, 0};
#endif
CdFile g_LbaOpnWsStr = {0x3631, 0, 0x00000, 0, 0, 0xFF, 0};
CdFile g_LbaStagePrg = {0x0000, 8, 0x00000, 0, 0, 0xFF, 0};
CdFile g_LbaBinGo = {KO_LBA(0x654A), 9, 0x1C000, 0, 0, 0xFF, 0};
CdFile g_LbaBinEnd = {KO_LBA(0x7489), 9, 0x16000, 0, 0, 0xFF, 0};
CdFile D_800ACC64 = {0x0000, 3, 0x00000, 0, 0, 0xFF, 0};

#elif defined(VERSION_HD)
CdFile g_LbaBinTitle = {0x6424, 0, 0x80000, 0, 0, 0xFF, 0};
CdFile g_LbaBinProlo = {0x6324, 6, 0x80000, 0, 0, 0xFF, 0};
CdFile g_LbaBinGame = {0x0000, 1, 0x42000, 0, 0, 0x13, 0};
CdFile g_LbaStageChr = {0x7E97, 2, 0x40000, 0, 0, 0xFF, 0};
CdFile g_LbaSdAlkVh = {0x615B, 19, 0x01020, 1, 1, 0xFF, 7};
CdFile g_LbaSdAlkVb = {0x615E, 17, 0x0E1D0, 1, 1, 0xFF, 0};
CdFile g_LbaSdRihVh = {0x617B, 19, 0x01020, 1, 1, 0x16, 21};
CdFile g_LbaSdRihVb = {0x617E, 17, 0x0CC20, 1, 1, 0xFF, 0};
CdFile g_LbaSdMarVh = {0x6198, 19, 0x00E20, 2, 2, 0xFF, 23};
CdFile g_LbaSdMarVb = {0x619A, 17, 0x0E290, 2, 2, 0xFF, 0};
CdFile g_LbaSdJ010_1_Vh = {0x60D0, 19, 0x02A20, 0, 0, 0x0A, 9};
CdFile g_LbaSdJ010_1_Vb = {0x60D6, 17, 0x41CB0, 0, 0, 0xFF, 0};
CdFile g_LbaSdJ010_2_Vh = {0x60D0, 19, 0x41CB0, 4, 4, 0x0C, 11};
CdFile g_LbaSdJ010_2_Vb = {0x60D6, 17, 0x1A510, 4, 4, 0x0C, 0};
CdFile g_LbaStageSeq = {0x0000, 18, 0x00000, 0, 6, 0xFF, 0};
CdFile g_LbaStageVh = {0x0000, 19, 0x00000, 3, 3, 0xFF, 14};
CdFile g_LbaStageVb = {0x0000, 17, 0x00000, 3, 3, 0xFF, 0};
CdFile g_LbaSdJ010_3_Vh = {0x60D0, 19, 0x02A20, 3, 3, 0xFF, 16};
CdFile g_LbaSdJ010_3_Vb = {0x60D6, 17, 0x41CB0, 3, 3, 0xFF, 0};
CdFile g_LbaBinWeapon0 = {0x65D0, 12, 0x04000, 0, 0, 0xFF, 0};
CdFile g_LbaBinWeapon1 = {0x690A, 13, 0x04000, 0, 0, 0xFF, 1};
CdFile g_LbaBinMonster = {0x6C44, 14, 0x05800, 0, 0, 0xFF, 0};
CdFile g_LbaBinDemokey = {0x745D, 4, 0x02000, 0, 0, 0xFF, 0};

// SERVANT/FT_xxx.BIN off
s32 g_LbaServantChrOff[] = {
    0x7296, 0x72D9, 0x731C, 0x735F, 0x739F, 0x73DA, 0x741D,
};

// SERVANT/SD_xxx.VH off
s32 g_LbaServantVhOff[] = {
    0x72B6, 0x72F9, 0x733C, 0x737F, 0x73BF, 0x73FA, 0x743D,
};

// SERVANT/SD_xxx.VH len
s32 g_LbaServantVhLen[] = {
    4128, 4128, 4128, 3616, 3104, 4128, 3616,
};

// SERVANT/SD_xxx.VB off
s32 g_LbaServantVbOff[] = {
    0x72B9, 0x72FC, 0x733F, 0x7381, 0x73C1, 0x73FD, 0x743F,
};

// SERVANT/SD_xxx.VB len
s32 g_LbaServantVbLen[] = {
    63904, 63904, 65200, 59488, 51024, 63904, 60992,
};

CdFile g_LbaServantChr = {0x0000, 15, 0x06000, 0, 0, 0x1D, 0};
CdFile g_LbaServantVh = {0x0000, 19, 0x00000, 2, 2, 0xFF, 30};
CdFile g_LbaServantVb = {0x0000, 17, 0x00000, 2, 2, 0xFF, 0};
CdFile g_LbaBinRichter = {0x6524, 10, 0x3999C, 0, 0, KO_RICHTER_NEXT, 0};
CdFile g_LbaBinAlucard = {0x61B8, 10, 0x31BEC, 0, 0, KO_ALUCARD_NEXT, 0};
CdFile g_LbaOpnWsStr = {0x3628, 0, 0x00000, 0, 0, 0xFF, 0};
CdFile g_LbaStagePrg = {0x0000, 8, 0x00000, 0, 0, 0xFF, 0};
CdFile g_LbaBinGo = {0x6598, 9, 0x1C000, 0, 0, 0xFF, 0};
CdFile g_LbaBinEnd = {0x74D1, 9, 0x16000, 0, 0, 0xFF, 0};
CdFile D_800ACC64 = {0x0000, 3, 0x00000, 0, 0, 0xFF, 0};

#endif

CdFile* D_800ACC74[] = {
    &g_LbaBinTitle,    &g_LbaBinTitle,    &g_LbaBinGame,     &g_LbaStageChr,
    &g_LbaSdAlkVh,     &g_LbaBinRichter,  &g_LbaBinProlo,    &g_LbaSdAlkVb,
    &g_LbaSdJ010_1_Vh, &g_LbaSdJ010_1_Vb, &g_LbaSdJ010_2_Vh, &g_LbaSdJ010_2_Vb,
    &g_LbaStageSeq,    &g_LbaStageVh,     &g_LbaStageVb,     &g_LbaSdJ010_3_Vh,
    &g_LbaSdJ010_3_Vb, &g_LbaBinWeapon0,  &g_LbaBinWeapon1,  &g_LbaBinAlucard,
    &g_LbaSdRihVh,     &g_LbaSdRihVb,     &g_LbaSdMarVh,     &g_LbaSdMarVb,
    &g_LbaOpnWsStr,    &D_800ACC64,       &g_LbaBinGo,       &g_LbaServantChr,
    &g_LbaBinMonster,  &g_LbaServantVh,   &g_LbaServantVb,   &g_LbaBinEnd,
    &g_LbaBinDemokey,
#if defined(MOD_KO)
    &g_LbaBinKoGlyphs,
#endif
};

CdFileSeq D_800ACCF8[] = {
    {OFF_SEQ_LIB_SEQ, SEQ_LIB_SEQ_LEN, MU_SEQ_LIBRARIAN},
    {OFF_SEQ_DAI_SEQ, SEQ_DAI_SEQ_LEN, MU_SEQ_CONFESSIONAL_BELLS},
};

const char* D_800ACD10[] = {
    aPbav, aPbav_0, aPbav_1, aPbav_2, aPbav, NULL, aPqes_1,
};

void func_801080DC(void);
void CopyStageOvlCallback(void);
#if defined(MOD_KO)
void CopyKoGlyphsCallback(void);
#endif
void CopyRicOvlCallback(void);
void func_801078C4(void);
void CopyMonsterPreviewCallback(void);
void func_80107DB4(void);
void func_80107C6C(void);
void func_80107EF0(void);
#if defined(MOD_SATURN_DISC)
#include "../../mods/sotn-ultimate/src/disc/cd_mod_files.h"
#endif
void (*g_CdCallbacks[])(void) = {
    func_801080DC,        func_801080DC,        func_801080DC,
    func_801080DC,        CopyStageOvlCallback, func_801080DC,
    func_801080DC,        func_801080DC,        CopyStageOvlCallback,
    CopyStageOvlCallback, CopyRicOvlCallback,   func_801080DC,
    func_801078C4,        func_801078C4,        CopyMonsterPreviewCallback,
    func_801078C4,        func_80107DB4,        func_80107C6C,
    func_80107DB4,        func_80107EF0,
#if defined(MOD_KO)
    CopyKoGlyphsCallback,
#endif
};
s32 D_800ACD7C = 0;

extern u16 D_800A04CC[]; // palette?
extern u32* D_801EE000;
extern s32 g_VabAddrs[];

// BSS
static CdCallbacks g_CdCallback;
static CdlLOC g_CdLoc;
static CdThing g_Cd;
static s16 g_VabId;
static u8 D_80137F96;
STATIC_PAD_BSS(5);
static s32 D_80137F9C;
static s32 D_80137FA0;
STATIC_PAD_BSS(4);
static s16 D_80137FA8;
STATIC_PAD_BSS(2);
static s32 D_80137FAC;
static s32 D_80137FB0;

void func_801073C0(void) {
    CdReadyCallback(NULL);
    CdDataCallback(NULL);
}

s32 func_801073E8(void) {
    u8 res;
    s32 ret;
#if defined(MOD_DRA_STRIP) // D_80137F9C: written, never read (now a local)
    s32 D_80137F9C;
#endif

    if (CdSync(CdlNop, &res) == CdlNoIntr) {
        return D_80137F9C = 0;
    }

    ret = CdLastCom();
    if (ret >= CdlGetlocL && ret <= CdlGetlocP || !(res & CdlStatShellOpen)) {
        CdControlF(CdlNop, NULL);
        return D_80137F9C = 0;
    }
    return D_80137F9C = 1;
}

void func_80107460(void) {
    g_Cd.D_80137F7C = &D_801EC000[TO_CD_BLOCK(g_Cd.D_80137F6C)];
    CdGetSector(g_Cd.D_80137F7C, 0x200);
    g_Cd.D_80137F6C = (g_Cd.D_80137F6C + 1) & 7;
}

void CopyStageOvlCallback(void) {
    s32 i;
    s32 len;

    if (g_Cd.overlayBlockCount != 0) {
        len = CD_BLOCK_LEN;
    } else {
        len = g_Cd.overlayLastBlockSize;
    }
    if (g_CdCallback == CdCallback_4) {
        g_Cd.overlayCopyDst = TO_CD_BLOCK(g_Cd.D_80137F74) + DEMO_KEY_PTR;
    } else {
        g_Cd.overlayCopyDst = TO_CD_BLOCK(g_Cd.D_80137F74) + STAGE_PRG_PTR;
    }
    g_Cd.overlayCopySrc = TO_CD_BLOCK(g_Cd.D_80137F70) + D_801EC000;
#if USE_MICRO_OPTIMIZATIONS == 1
    MEMCPY(g_Cd.overlayCopyDst, g_Cd.overlayCopySrc, len);
#else
    for (i = 0; i < len; i++) {
        *g_Cd.overlayCopyDst = *g_Cd.overlayCopySrc;
        g_Cd.overlayCopySrc++;
        g_Cd.overlayCopyDst++;
    }
#endif
    g_Cd.D_80137F70 = (g_Cd.D_80137F70 + 1) & 7;
    g_Cd.D_80137F74++;
    g_Cd.overlayBlockCount--;
    if (g_Cd.overlayBlockCount < 0 ||
        (g_Cd.overlayBlockCount == 0 && g_Cd.overlayLastBlockSize == 0)) {
        g_Cd.D_80137F78 = 1;
        CdDataCallback(NULL);
    }
}

void CopyRicOvlCallback(void) {
    s32 i;
    s32 len;

    if (g_Cd.overlayBlockCount != 0) {
        len = CD_BLOCK_LEN;
    } else {
        len = g_Cd.overlayLastBlockSize;
    }
    g_Cd.overlayCopyDst = TO_CD_BLOCK(g_Cd.D_80137F74) + RIC_PRG_PTR;
    g_Cd.overlayCopySrc = TO_CD_BLOCK(g_Cd.D_80137F70) + D_801EC000;
#if USE_MICRO_OPTIMIZATIONS == 1
    MEMCPY(g_Cd.overlayCopyDst, g_Cd.overlayCopySrc, len);
#else
    for (i = 0; i < len; i++) {
        *g_Cd.overlayCopyDst = *g_Cd.overlayCopySrc;
        g_Cd.overlayCopySrc++;
        g_Cd.overlayCopyDst++;
    }
#endif
    g_Cd.D_80137F70 = (g_Cd.D_80137F70 + 1) & 7;
    g_Cd.D_80137F74++;
    g_Cd.overlayBlockCount--;
    if (g_Cd.overlayBlockCount < 0 ||
        g_Cd.overlayBlockCount == 0 && g_Cd.overlayLastBlockSize == 0) {
        g_Cd.D_80137F78 = 1;
        CdDataCallback(NULL);
    }
}

#if defined(MOD_KO)
// A-gap KO loader (+ JP bundle + 8px block ride the
// same Alucard chain into 0x801F0000~/0x801F2000~). Plain copy into fixed
// RAM (like the stage overlay copy, but fixed dest). Armed with the Alucard
// chain below. The dest block derives from the completed-block counter
// (no BSS pointer needed). Blocks 0-13: glyph area; block 14: JP bundle;
// blocks 15-16: 8px block.
void CopyKoGlyphsCallback(void) {
    s32 i;
    u8* dst;
    u8* src;
    s32 blk;
    blk = g_Cd.D_80137F74;
    dst = (u8*)(KO_GLYPH_BLOCK_ADDR + (blk << 11));
    if (blk >= KO_ARC_SECTORS) {
        if (blk == KO_ARC_SECTORS) {
            dst = (u8*)KO_JP_BUNDLE_BASE;
        } else {
            dst = (u8*)(KO_8P_BASE + ((blk - KO_ARC_SECTORS - 1) << 11));
        }
    }
    src = TO_CD_BLOCK(g_Cd.D_80137F70) + D_801EC000;
    for (i = 0; i < CD_BLOCK_LEN; i++) {
        *dst++ = *src++;
    }
    g_Cd.D_80137F70 = (g_Cd.D_80137F70 + 1) & 7;
    g_Cd.D_80137F74++;
    g_Cd.overlayBlockCount--;
    if (g_Cd.overlayBlockCount < 0 ||
        (g_Cd.overlayBlockCount == 0 && g_Cd.overlayLastBlockSize == 0)) {
        g_Cd.D_80137F78 = 1;
        CdDataCallback(NULL);
    }
}
#endif

void CopySupportOvlCallback(void) {
    s32 i;
    s32 len;

    if (g_Cd.overlayBlockCount != 0) {
        len = CD_BLOCK_LEN;
    } else {
        len = g_Cd.overlayLastBlockSize;
    }

    if (g_CdCallback == CdCallback_Familiar) {
        g_Cd.overlayCopyDst = FAMILIAR_PTR;
    } else if (D_80137F96 == 0) {
        g_Cd.overlayCopyDst = WEAPON0_PTR;
    } else {
        g_Cd.overlayCopyDst = WEAPON1_PTR;
    }
    g_Cd.overlayCopyDst += TO_CD_BLOCK(g_Cd.D_80137F74);
    g_Cd.overlayCopySrc = TO_CD_BLOCK(g_Cd.D_80137F70) + D_801EC000;
#if USE_MICRO_OPTIMIZATIONS == 1
    MEMCPY(g_Cd.overlayCopyDst, g_Cd.overlayCopySrc, len);
#else
    for (i = 0; i < len; i++) {
        *g_Cd.overlayCopyDst = *g_Cd.overlayCopySrc;
        g_Cd.overlayCopySrc++;
        g_Cd.overlayCopyDst++;
    }
#endif
    g_Cd.D_80137F70 = (g_Cd.D_80137F70 + 1) & 7;
    g_Cd.D_80137F74++;
    g_Cd.overlayBlockCount--;
    if (g_Cd.overlayBlockCount < 0 ||
        g_Cd.overlayBlockCount == 0 && g_Cd.overlayLastBlockSize == 0) {
        g_Cd.D_80137F78 = 1;
        CdDataCallback(NULL);
    }
}

void func_801078C4(void) {
    s32 i;
    s32 len;
    s32 var_a3;

    if (g_Cd.overlayBlockCount != 0) {
        len = CD_BLOCK_LEN;
    } else {
        len = g_Cd.overlayLastBlockSize;
    }
    g_Cd.overlayCopyDst = TO_CD_BLOCK(g_Cd.D_80137F74) + g_Pix[0];
    g_Cd.overlayCopySrc = TO_CD_BLOCK(g_Cd.D_80137F70) + D_801EC000;
#if USE_MICRO_OPTIMIZATIONS == 1
    MEMCPY(g_Cd.overlayCopyDst, g_Cd.overlayCopySrc, len);
#else
    for (i = 0; i < len; i++) {
        *g_Cd.overlayCopyDst = *g_Cd.overlayCopySrc;
        g_Cd.overlayCopySrc++;
        g_Cd.overlayCopyDst++;
    }
#endif
    g_Cd.D_80137F70 = (g_Cd.D_80137F70 + 1) & 7;
    g_Cd.D_80137F74++;
    g_Cd.overlayBlockCount--;
    if (g_Cd.overlayBlockCount < 0 ||
        g_Cd.overlayBlockCount == 0 && g_Cd.overlayLastBlockSize == 0) {
        if (g_CdCallback == CdCallback_Familiar) {
            // Copy familiar graphics in the VRAM
            g_Cd.overlayBlockCount = 20;
            g_Cd.overlayLastBlockSize = 0;
        } else {
            // Copy weapon graphics in the VRAM
            g_Cd.overlayBlockCount = 6;
            g_Cd.overlayLastBlockSize = 0;
        }
        g_Cd.D_80137F74 = 0;
        CdDataCallback(CopySupportOvlCallback);
        if (g_CdCallback == CdCallback_12) {
            LoadTPage(g_Pix[0], 0, 0, 0x240, 0x100, 0x100, 0x80);
        } else if (g_CdCallback == CdCallback_13) {
            LoadTPage(g_Pix[0], 0, 0, 0x240, 0x180, 0x100, 0x80);
        } else {
            LoadTPage(g_Pix[0], 0, 0, 0x2C0, 0x100, 0x100, 0x80);
            LoadTPage(g_Pix[2], 0, 0, 0x2C0, 0x180, 0x80, 0x80);
        }
    }
}

void CopyMonsterPreviewCallback(void) {
    s32 i;
    s32 len;

    if (g_Cd.overlayBlockCount != 0) {
        len = CD_BLOCK_LEN;
    } else {
        len = g_Cd.overlayLastBlockSize;
    }
    g_Cd.overlayCopyDst = TO_CD_BLOCK(g_Cd.D_80137F74) + g_Pix[0];
    g_Cd.overlayCopySrc = TO_CD_BLOCK(g_Cd.D_80137F70) + D_801EC000;
#if USE_MICRO_OPTIMIZATIONS == 1
    MEMCPY(g_Cd.overlayCopyDst, g_Cd.overlayCopySrc, len);
#else
    for (i = 0; i < len; i++) {
        *g_Cd.overlayCopyDst = *g_Cd.overlayCopySrc;
        g_Cd.overlayCopySrc++;
        g_Cd.overlayCopyDst++;
    }
#endif
    g_Cd.D_80137F70 = (g_Cd.D_80137F70 + 1) & 7;
    g_Cd.D_80137F74++;
    g_Cd.overlayBlockCount--;
    if (g_Cd.overlayBlockCount < 0 ||
        (g_Cd.overlayBlockCount == 0 && g_Cd.overlayLastBlockSize == 0)) {
        LoadTPage(g_Pix[0], 2, 0, 0x20, 0x100, 0x60, 0x70);
        g_Cd.D_80137F78 = 1;
        CdDataCallback(NULL);
    }
}

void func_80107C6C(void) {
    s32 len;

    if (g_Cd.D_80137F74 != 0 && (s16)SsVabTransCompleted(SS_IMEDIATE) != 1) {
        func_801073C0();
        g_Cd.D_80137F78 = -1;
        return;
    }
    g_Cd.overlayCopySrc = (g_Cd.D_80137F70 << 0xB) + D_801EC000;
    if (g_Cd.overlayBlockCount != 0) {
        len = CD_BLOCK_LEN;
    } else {
        len = g_Cd.overlayLastBlockSize;
    }
#if defined(MOD_DRA_STRIP) // D_80137FA8: read only here (now a local)
    {
        s16 D_80137FA8;
#endif
        D_80137FA8 = SsVabTransBodyPartly(g_Cd.overlayCopySrc, len, g_VabId);
        if (D_80137FA8 == -1) {
            CdDataCallback(NULL);
            g_Cd.D_80137F78 = -3;
        }
#if defined(MOD_DRA_STRIP)
    }
#endif
    g_Cd.D_80137F70 = (g_Cd.D_80137F70 + 1) & 7;
    g_Cd.D_80137F74++;
    g_Cd.overlayBlockCount--;
    if (g_Cd.overlayBlockCount < 0 ||
        (g_Cd.overlayBlockCount == 0 && g_Cd.overlayLastBlockSize == 0)) {
        CdDataCallback(NULL);
        g_Cd.D_80137F78 = 1;
    }
}

void func_80107DB4(void) {
    s32 i;
    s32 len;

    if (g_Cd.overlayBlockCount != 0) {
        len = CD_BLOCK_LEN;
    } else {
        len = g_Cd.overlayLastBlockSize;
    }
    g_Cd.overlayCopyDst = TO_CD_BLOCK(g_Cd.D_80137F74) + g_Cd.addr;
    g_Cd.overlayCopySrc = TO_CD_BLOCK(g_Cd.D_80137F70) + D_801EC000;
#if USE_MICRO_OPTIMIZATIONS == 1
    MEMCPY(g_Cd.overlayCopyDst, g_Cd.overlayCopySrc, len);
#else
    for (i = 0; i < len; i++) {
        *g_Cd.overlayCopyDst = *g_Cd.overlayCopySrc;
        g_Cd.overlayCopySrc++;
        g_Cd.overlayCopyDst++;
    }
#endif
    g_Cd.D_80137F70 = (g_Cd.D_80137F70 + 1) & 7;
    g_Cd.D_80137F74++;
    g_Cd.overlayBlockCount--;
    if (g_Cd.overlayBlockCount < 0 ||
        (g_Cd.overlayBlockCount == 0 && g_Cd.overlayLastBlockSize == 0)) {
        g_Cd.D_80137F78 = 1;
        CdDataCallback(NULL);
    }
}

void func_80107EF0(void) {
    s32 i;
    s32 len;
    short res;

    if (g_Cd.overlayBlockCount != 0) {
        len = CD_BLOCK_LEN;
    } else {
        len = g_Cd.overlayLastBlockSize;
    }
    g_Cd.overlayCopyDst = TO_CD_BLOCK(g_Cd.D_80137F74) + g_Cd.addr;
    g_Cd.overlayCopySrc = TO_CD_BLOCK(g_Cd.D_80137F70) + D_801EC000;
#if USE_MICRO_OPTIMIZATIONS == 1
    MEMCPY(g_Cd.overlayCopyDst, g_Cd.overlayCopySrc, len);
#else
    for (i = 0; i < len; i++) {
        *g_Cd.overlayCopyDst = *g_Cd.overlayCopySrc;
        g_Cd.overlayCopySrc++;
        g_Cd.overlayCopyDst++;
    }
#endif
    g_Cd.D_80137F70 = (g_Cd.D_80137F70 + 1) & 7;
    g_Cd.D_80137F74++;
    g_Cd.overlayBlockCount--;
    if (g_Cd.overlayBlockCount < 0 ||
        (g_Cd.overlayBlockCount == 0 && g_Cd.overlayLastBlockSize == 0)) {
        res = SsVabOpenHeadSticky(g_Cd.addr, g_VabId, g_VabAddrs[g_VabId]);
        if (res < 0) {
            g_Cd.D_80137F78 = -2;
            CdDataCallback(NULL);
        } else {
            CdFile* cdFile = D_800ACC74[D_80137F96];
            g_Cd.overlayBlockCount = cdFile->size / CD_BLOCK_LEN;
            g_Cd.overlayLastBlockSize =
                cdFile->size - cdFile->size / CD_BLOCK_LEN * CD_BLOCK_LEN;
            g_Cd.D_80137F74 = 0;
            CdDataCallback(func_80107C6C);
        }
    }
}

void func_801080DC(void) {
    int new_var2;
    s32 new_var;
    CdThing* new_var3;
    RECT* r;

    new_var2 = 6;
    if (g_Cd.D_80137F70 == 3 || g_Cd.D_80137F70 == 7) {
        switch (g_CdCallback) {
        case CdCallback_1:
            g_Cd.dstRect.x = D_800AC958[g_Cd.D_80137F74];
            g_Cd.dstRect.y = ((g_Cd.D_80137F74 << new_var2) & 0x80) + 0x100;
            g_Cd.dstRect.w = 0x20;
            g_Cd.dstRect.h = 0x80;
            if (g_Cd.D_80137F74 == 0x1A) {
                g_Cd.dstRect.y = 0x181;
                g_Cd.dstRect.h = 0x7F;
                if (!g_Cd.D_80137F74 && !g_Cd.D_80137F74) {
                }
            }
            new_var3 = &g_Cd;
            if (new_var3->D_80137F74 == 0x20) {
                g_Cd.dstRect.x = 0;
                g_Cd.dstRect.y = 0xF0;
                g_Cd.dstRect.w = 0x100;
                g_Cd.dstRect.h = 0x10;
            }
            break;
        case CdCallback_0:
        case CdCallback_StageChr:
        case CdCallback_6:
            g_Cd.dstRect.x = D_800AC998[g_Cd.D_80137F74 & 0x1F];
            new_var = g_Cd.D_80137F74;
            g_Cd.dstRect.y = (new_var << 6) & 0x80;
            if (g_Cd.D_80137F74 >= 0x20) {
                g_Cd.dstRect.y += 0x100;
            }
            g_Cd.dstRect.w = 0x20;
            g_Cd.dstRect.h = 0x80;
            break;
        case CdCallback_3:
            g_Cd.dstRect.x = 0x100;
            g_Cd.dstRect.y = 0x100;
            g_Cd.dstRect.w = 0x40;
            g_Cd.dstRect.h = 0x40;
            break;
        }

        if (g_Cd.D_80137F70 == 7) {
            g_Cd.overlayCopySrc = &D_801EE000;
            LoadImage(&g_Cd.dstRect, g_Cd.overlayCopySrc);
            g_Cd.D_80137F74++;
        }
        if (g_Cd.D_80137F70 == 3) {
            g_Cd.overlayCopyDst = D_801EC000;
            LoadImage(&g_Cd.dstRect, g_Cd.overlayCopyDst);
            g_Cd.D_80137F74++;
        }
        if (g_Cd.D_80137F74 >= g_Cd.D_80137F68) {
            g_Cd.D_80137F78 = 1;
            if (g_CdCallback == CdCallback_1) {
                g_Cd.dstRect.x = 0x380;
                g_Cd.dstRect.y = 0x180;
                g_Cd.dstRect.w = 0x10;
                g_Cd.dstRect.h = 1;
                LoadImage(&g_Cd.dstRect, D_800A04CC);
            }
            CdDataCallback(NULL);
        }
    }
    g_Cd.D_80137F70 = (g_Cd.D_80137F70 + 1) & 7;
}

void func_8010838C(s32 cdStep) {
    func_801073C0();
    g_CdStep = cdStep;
}

void func_801083BC(void) {
    g_CdStep = CdStep_LoadInit;
    g_LoadFile = CdFile_25;
    D_8013AE9C = 10;
    D_800ACC64.loc = 0;
}

#if !defined(MOD_DRA_STRIP) // no caller in the US build
bool func_801083F0(void) {
    g_CdStep = CdStep_LoadInit;
    g_LoadFile = CdFile_25;

    if (D_8013AE9C == 0) {
        return false;
    } else {
        D_8013AE9C--;
        D_800ACC64.loc += 4;
    }

    return true;
}
#endif

void func_80107460(void);

void PlaySfx(s32);
void SsVabClose(short);

#if defined(MOD_SATURN) && OFF_BIN_RIC2 != 0
// The Extra settings cheats (mods/sotn-ultimate/src/menu/sysopt.c, Alucard's
// file) patch DRA's code in place: HandleDamage's entry and the two stair
// branches in func_80109990 (named by its retail address; here it is at
// 0x80108490). A Richter / Maria game's file has no such code, so the built
// words go back when it is loaded (build_sysopt.sh checks them against the
// built DRA).
// FlushCache is the main exe's kernel stub (SLUS 0x800192CC: li $t2,0xA0 / jr $t2 / li $t1,0x44); the DRA link has no
// symbol for it, so it is called by address
#define MOD_FLUSH_CACHE() ((void (*)(void))0x800192CC)()
void func_80109990(void);
static void ModCheatUnpatch(void) {
    u32* hd = (u32*)HandleDamage;
    u16* s1 = (u16*)((u8*)func_80109990 + 0x2D2);
    u16* s2 = (u16*)((u8*)func_80109990 + 0x35E);

    if (hd[0] != 0x27BDFFD8 || hd[1] != 0xAFB20018 || *s1 != 0x1482 ||
        *s2 != 0x1482) {
        hd[0] = 0x27BDFFD8;
        hd[1] = 0xAFB20018;
        *s1 = 0x1482;
        *s2 = 0x1482;
        MOD_FLUSH_CACHE();
    }
}
#endif

void UpdateCd(void) {
#if defined(VERSION_US)
    const bool UseEuWarning = true;
#elif defined(VERSION_HD)
    const bool UseEuWarning = false;
#endif

    unsigned char result[8];
    unsigned char setModeArg[24];
    CdFile* cdFile;
    s32 temp_v1_8;
    s32 temp_v1_9;
    s32 i;
    s8 seqIdx;
    u32* pSrc;
    u32* pDst;
    u16* clutAddr;
    s32* pLoadFile;
    CdMgr* cd;

    if (D_8003C728 != 0) {
        return;
    }
    pLoadFile = &g_LoadFile;
    g_IsUsingCd = g_CdStep;
    if (g_LoadFile == CdFile_StagePrg) {
        cdFile = &g_LbaStagePrg;
        if (UseEuWarning && g_StageId == STAGE_EU_WARNING) {
            g_LbaStagePrg.loc = OFF_WARNING_TIM;
            g_LbaStagePrg.size = LEN_WARNING_TIM;
            g_LbaStagePrg.nextCdFileType = CdFile_NoNext;
        } else {
            g_LbaStagePrg.loc = g_StagesLba[g_StageId].ovlOff;
            g_LbaStagePrg.size = g_StagesLba[g_StageId].ovlLen;
#if defined(MOD_SATURN) && OFF_BIN_BO6RIC2 != 0
            // Saturn port: boss Richter in the Saturn coat. BIN/BO6RIC2.BIN
            // is BO6.BIN (same size and code) with RIC2's sprite sheet,
            // frames and palettes (mods/sotn-ultimate/tools/build_bo6ric2.py)
            if (g_StageId == STAGE_BO6 && MOD_BOSS_RIC_SATURN()) {
                g_LbaStagePrg.loc = OFF_BIN_BO6RIC2;
            }
#endif
#if defined(MOD_SATURN) && OFF_BIN_SELRIC2 != 0
            // ... and in the endings: BIN/SELRIC2.BIN is SEL.BIN with the
            // coat on its Richter (mods/sotn-ultimate/tools/build_selric2.py),
            // for an Alucard game with that boss or a Richter game in the
            // costume
            if (g_StageId == STAGE_SEL &&
                (MOD_BOSS_RIC_SATURN() || MOD_RIC_COSTUME2())) {
                g_LbaStagePrg.loc = OFF_BIN_SELRIC2;
            }
#endif
#if defined(MOD_SATURN) && OFF_BIN_CENP != 0
            // Saturn port: the PSP Maria fight (the select screen's "Maria
            // boss: PSP"). BIN/CENP.BIN is CEN with it in the Saturn fight's
            // place (mods/sotn-ultimate/tools/build_cenp.sh), padded to the
            // whole stage area (0x60000; the PSP dub's CEN bank follows, see
            // StageSfx). Only Alucard games set the flag (SEL); CENP's fight
            // checks the player too
            if (g_StageId == STAGE_CEN &&
                (MOD_STATUS_FLAGS & MOD_FLAG_MARIA_BOSS_PSP)) {
                g_LbaStagePrg.loc = OFF_BIN_CENP;
                g_LbaStagePrg.size = 0x60000;
            }
#endif
            seqIdx = g_StagesLba[g_StageId].seqIdx;
            if (seqIdx >= 0) { // A stage has an optional sequence file
                g_LbaStagePrg.nextCdFileType = CdFile_Seq;
                g_LbaStageSeq.loc = *(&D_800ACCF8[seqIdx].loc);
                g_LbaStageSeq.size = *(&D_800ACCF8[seqIdx].size);
            } else {
                g_LbaStagePrg.nextCdFileType = CdFile_NoNext;
            }
        }
    } else {
        cdFile = D_800ACC74[g_LoadFile & 0x7FFF];
        if (g_LoadFile == CdFile_StageChr) {
            cdFile->loc = g_StagesLba[g_LoadOvlIdx].gfxOff;
        }
        if (g_LoadFile == CdFile_StageSfx) {
            g_LbaStageVh.loc = g_StagesLba[g_LoadOvlIdx].vhOff;
            g_LbaStageVh.size = g_StagesLba[g_LoadOvlIdx].vhLen;
            g_LbaStageVb.size = g_StagesLba[g_LoadOvlIdx].vbLen;
#if defined(MOD_SATURN) && OFF_BIN_SFX_PSP != 0
            // PSP dub: the stage's bank with the PSP's voices (boss and
            // enemy voices; BIN/SFX_PSP.BIN, laid out as SD_Z*.VH + VB,
            // mods/sotn-ultimate/tools/gen_psp_sfx.py). lba_stage.c keeps its
            // sector + 1 in the (unused) name field; CEN has none (below)
            if (MOD_FIELD_PSP_BANKS() && g_StagesLba[g_LoadOvlIdx].name) {
                g_LbaStageVh.loc =
                    OFF_BIN_SFX_PSP - 1 + (s32)g_StagesLba[g_LoadOvlIdx].name;
#if OFF_BIN_JP_SD != 0
                // the JP dub: the bank with its Japanese clips, or retail
                {
                    s32 jp = ModJpSfx((s32)g_StagesLba[g_LoadOvlIdx].name);
                    if (jp) {
                        g_LbaStageVh.loc =
                            jp < 0 ? g_StagesLba[g_LoadOvlIdx].vhOff : jp;
                    }
                }
#endif
            }
#endif
#if defined(MOD_SATURN) && OFF_BIN_CENP != 0
            // the PS1 dub: CEN's bank for the Saturn boss Maria / the PSP
            // one, CENP.BIN's tail after the PSP dub's
            // (mods/sotn-ultimate/tools/ build_cenp.sh)
            if (g_LoadOvlIdx == STAGE_CEN && !MOD_PSP_DUB_ON()) {
                g_LbaStageVh.loc =
                    OFF_BIN_CENP +
                    (MOD_STATUS_FLAGS & MOD_FLAG_MARIA_BOSS_PSP ? 0x99000
                                                                : 0x7C800) /
                        0x800;
            }
            // the PSP boss Maria with the PSP dub: CEN's bank with her PSP
            // English clips (mods/sotn-ultimate/tools/gen_cenp_sounds.py),
            // CENP.BIN's tail
            if (g_LoadOvlIdx == STAGE_CEN && MOD_PSP_DUB_ON() &&
                (MOD_STATUS_FLAGS & MOD_FLAG_MARIA_BOSS_PSP)) {
                g_LbaStageVh.loc = OFF_BIN_CENP + 0x60000 / 0x800;
#if OFF_BIN_JP_SD != 0
                // the JP dub: her Japanese clips (same slots)
                if (MOD_JP_DUB_ON()) {
                    g_LbaStageVh.loc = OFF_BIN_JP_SD + MOD_JP_SD_CENP;
                }
#endif
            }
#endif
#if defined(MOD_SATURN) && OFF_BIN_NO3RIDE != 0
            // Richter's / Maria's NO3: the bank with their horse ride's neigh
            // over Death's samples (BIN/NO3RIDE.BIN, mods/sotn-ultimate/tools/
            // gen_no3_sounds.py: two banks of SD_ZKNO3's size, VH + VB, the
            // second from the PSP dub's NO3 bank; the JP dub has none of its
            // own for NO3)
            if (g_LoadOvlIdx == STAGE_NO3 &&
                g_PlayableCharacter != PLAYER_ALUCARD) {
                g_LbaStageVh.loc = OFF_BIN_NO3RIDE;
                if (MOD_FIELD_PSP_BANKS()) {
                    g_LbaStageVh.loc += MOD_NO3RIDE_SECTORS;
                }
            }
#endif
        }
        if (*pLoadFile == CdFile_GameChr) {
            if (g_StageId == STAGE_ST0 ||
                g_PlayableCharacter != PLAYER_ALUCARD) {
                g_LbaBinGame.loc = OFF_BIN_FGAME2;
                g_LbaBinGame.nextCdFileType = CdFile_RichterPrg;
#if defined(MOD_KO)
                // A3: the ARC_KO chain (glyph block, JP name bundle, 8px block) rides every non-Alucard load too:
                // RichterPrg -> KoGlyphs (static, KO_RICHTER_NEXT) -> 0x14 (Richter's old next)
                g_LbaBinKoGlyphs.nextCdFileType = 0x14;
#endif
            } else {
                g_LbaBinGame.loc = OFF_BIN_FGAME;
                g_LbaBinGame.nextCdFileType = CdFile_AlucardPrg;
#if defined(MOD_KO)
                g_LbaBinKoGlyphs.nextCdFileType = 0x04; // AlucardPrg -> KoGlyphs -> 0x04
#endif
            }
        }
#if defined(MOD_SATURN) && OFF_BIN_RIC2 != 0
        // Saturn port: Richter's alternate costume is a second copy of his
        // overlay with another sprite sheet (BIN/RIC2.BIN, same size and
        // code; mods/sotn-ultimate/tools/build_ric2.py)
        if (*pLoadFile == CdFile_RichterPrg) {
#if OFF_BIN_MARIA != 0 && OFF_BIN_MARIA_SD != 0
            // Saturn port: her Saturn voice and sounds. The overlay's load
            // goes on to VAB/SD_RIH (vab 1) and VAB/SD_MAR (vab 2, the
            // prologue's Maria); for her those four CdFiles (consecutive,
            // g_LbaSdRihVh..g_LbaSdMarVb) read BIN/MARIA_SD.BIN, her two
            // banks laid out as those files are
            // (mods/sotn-ultimate/tools/gen_mar_sounds.py)
            CdFile* sd = &g_LbaSdRihVh;
#if OFF_BIN_RIH_PSP != 0
            // PSP dub: Richter's PSP grunts (BIN/RIH_PSP.BIN, his vab 1 with
            // the PSP clips + the retail SD_MAR, the same layout;
            // mods/sotn-ultimate/tools/gen_psp_grunts.py). Maria's banks
            // replace it below (the JP dub: BIN/JP_SD.BIN's copy with the
            // Japanese clips, and the prologue Maria's in its SD_MAR)
            s32 sdOff = MOD_FIELD_JP_BANKS() && OFF_BIN_JP_SD
                            ? OFF_BIN_JP_SD + MOD_JP_SD_RIH - 0x612D
                        : MOD_FIELD_PSP_BANKS() ? OFF_BIN_RIH_PSP - 0x612D
                                                : 0;
#else
            s32 sdOff = 0;
#endif
            s32 sdLen = SD_MAR_VB_LEN;
#endif
            ModCheatUnpatch(); // (no cheats in a Richter / Maria game)
            g_LbaBinRichter.loc =
                MOD_RIC_COSTUME2() ? OFF_BIN_RIC2 : OFF_BIN_RIC;
#if OFF_BIN_MARIA != 0
            // Saturn port: the Saturn's Maria is a fork of Richter's
            // overlay, larger (BIN/MARIA.BIN;
            // mods/sotn-ultimate/tools/build_mar.sh)
            g_LbaBinRichter.size = 0x39A58;
            if (g_PlayableCharacter == PLAYER_MARIA) {
                g_LbaBinRichter.loc = OFF_BIN_MARIA;
                g_LbaBinRichter.size = LEN_BIN_MARIA;
#if OFF_BIN_MARIA_SD != 0
                // her Japanese Saturn voice for the JP dub, the PS1 dub's
                // bank (BIN/MARIA_SD_PS1.BIN: a copy of the PSP one, as her
                // select panel has no PS1 dub), else the PSP English lines
                // (BIN/MARIA_SD_PSP.BIN); all the same layout
                sdOff =
                    (MOD_JP_DUB_ON() || !OFF_BIN_MARIA_SD_PSP ? OFF_BIN_MARIA_SD
                     : !MOD_PSP_DUB_ON() && OFF_BIN_MARIA_SD_PS1
                         ? OFF_BIN_MARIA_SD_PS1
                         : OFF_BIN_MARIA_SD_PSP) -
                    0x612D;
                sdLen = vab2Len; // her vab 2 is larger than SD_MAR's
#endif
#if OFF_BIN_MARIA2 != 0 && OFF_BIN_MARIA2_SD != 0
                // Saturn port: the PSP Maria (the select screen's Maria
                // "PSP"): src/maria built for the PS1 (BIN/MARIA2.BIN;
                // mods/sotn-ultimate/tools/build_mar2.sh), her PSP voice in
                // BIN/MARIA2_SD.BIN (same layout and vab 2 size as
                // MARIA_SD.BIN)
                if (MOD_MARIA_PSP()) {
                    g_LbaBinRichter.loc = OFF_BIN_MARIA2;
                    g_LbaBinRichter.size = LEN_BIN_MARIA2;
                    // the PS1 dub: her bank after the Saturn Maria's in
                    // MARIA_SD_PS1.BIN (a copy of MARIA2_SD.BIN;
                    // build_disc.sh)
                    sdOff = (MOD_JP_DUB_ON() && OFF_BIN_JP_SD
                                 ? OFF_BIN_JP_SD + MOD_JP_SD_MAR2
                             : !MOD_PSP_DUB_ON() && OFF_BIN_MARIA_SD_PS1
                                 ? OFF_BIN_MARIA_SD_PS1 + MOD_MAR_SD_SECTORS
                                 : OFF_BIN_MARIA2_SD) -
                            0x612D;
                }
#endif
            }
#if OFF_BIN_MARIA2 != 0
            // the HUD's subweapon icons: Richter's table, unless the PSP
            // Maria's overlay points it at hers
            // (mods/sotn-ultimate/src/maria_psp/mar2.c)
            g_ModSubwpnIcons = g_ricSubwpnIcons;
            g_ModMar2HudIcons = NULL;
#endif
#if OFF_BIN_MARIA != 0
            ModMarSfxPrio(
                g_PlayableCharacter == PLAYER_MARIA && !MOD_MARIA_PSP());
#endif
#if OFF_BIN_MARIA_SD != 0
            sd[0].loc = 0x612D + sdOff;
            sd[1].loc = 0x6130 + sdOff;
            sd[2].loc = 0x614D + sdOff;
            sd[3].loc = 0x614F + sdOff;
            sd[3].size = sdLen;
#endif
#endif
        }
#endif
#if defined(MOD_SATURN) && OFF_BIN_ALK_PSP != 0
        // PSP dub: Alucard's PSP grunts (BIN/ALK_PSP.BIN, laid out as
        // VAB/SD_ALK.VH/VB; mods/sotn-ultimate/tools/gen_psp_grunts.py). The
        // overlay's load goes on to those two CdFiles
        if (*pLoadFile == CdFile_AlucardPrg) {
            s32 alkOff = MOD_FIELD_JP_BANKS() && OFF_BIN_JP_SD
                             ? OFF_BIN_JP_SD + MOD_JP_SD_ALK - 0x610D
                         : MOD_FIELD_PSP_BANKS() ? OFF_BIN_ALK_PSP - 0x610D
                                                 : 0;
            ModMarSfxPrio(false); // retail (a Maria game earlier this session)
            g_LbaSdAlkVh.loc = 0x610D + alkOff;
            g_LbaSdAlkVb.loc = 0x6110 + alkOff;
            // the System screen's Dub reload may have left its link to the
            // stage bank (mods/sotn-ultimate/src/menu/sysopt.c
            // SysOptBanks): not here
            g_LbaSdAlkVh.nextCdFileType = CdFile_NoNext;
        }
#endif
        if (g_LoadFile == CdFile_ServantChr) {
            g_LbaServantChr.loc = g_LbaServantChrOff[g_LoadOvlIdx];
            g_LbaServantVh.loc = g_LbaServantVhOff[g_LoadOvlIdx];
            g_LbaServantVh.size = g_LbaServantVhLen[g_LoadOvlIdx];
            g_LbaServantVb.loc = g_LbaServantVbOff[g_LoadOvlIdx];
            g_LbaServantVb.size = g_LbaServantVbLen[g_LoadOvlIdx];
#if defined(MOD_SATURN) && OFF_BIN_SFX_PSP != 0
            // PSP dub: the familiar's PSP voices (BIN/SFX_PSP.BIN: VH, then
            // VB from the next sector; every familiar has one, gen_psp_sfx.py).
            // The Pixie and the Nosedevil (5, 6) are silent in the US PS1:
            // their PSP voices in the PS1 dub too
            if (MOD_FIELD_PSP_BANKS() || g_LoadOvlIdx >= 5) {
                g_LbaServantVh.loc =
                    OFF_BIN_SFX_PSP - 1 + g_ModPspServantSfx[g_LoadOvlIdx];
                g_LbaServantVb.loc =
                    g_LbaServantVh.loc +
                    ((g_LbaServantVhLen[g_LoadOvlIdx] + 0x7FF) >> 11);
#if OFF_BIN_JP_SD != 0
                // the JP dub: the bank with its Japanese clips, or retail
                {
                    s32 jp = ModJpSfx(g_ModPspServantSfx[g_LoadOvlIdx]);
                    if (jp < 0) {
                        g_LbaServantVh.loc = g_LbaServantVhOff[g_LoadOvlIdx];
                        g_LbaServantVb.loc = g_LbaServantVbOff[g_LoadOvlIdx];
                    } else if (jp) {
                        g_LbaServantVh.loc = jp;
                        g_LbaServantVb.loc =
                            jp +
                            ((g_LbaServantVhLen[g_LoadOvlIdx] + 0x7FF) >> 11);
                    }
                }
#endif
            }
#endif
        }
    }

    switch (g_CdStep) {
    case CdStep_None:
        break;

    case CdStep_LoadInit:
#if !defined(MOD_DRA_STRIP) // D_80137FAC: written, never read
        D_80137FAC = 0;
#endif
        if (g_LoadFile == CdFile_None) {
            g_CdStep = CdStep_None;
            break;
        }

        func_801073C0();
        if (CdSync(CdlNop, NULL) == CdlNoIntr) {
            break;
        }

        CdControl(CdlNop, NULL, result);
        if (result[0] & CdlModeRT) {
            break;
        }

        if (!CdControl(CdlPause, NULL, NULL)) {
            break;
        }

        if (CdSync(CdlNop, NULL) == CdlDiskError) {
            func_8010838C(CdStep_DiskErr);
            break;
        }
        g_CdStep++;
        break;

    case CdStep_SetSpeed:
        if (CdSync(CdlNop, NULL) == CdlNoIntr) {
            break;
        }
        setModeArg[0] = CdlModeSpeed;
        if (!CdControl(CdlSetmode, setModeArg, NULL)) {
            break;
        }

        if (CdSync(CdlNop, NULL) == CdlDiskError) {
            func_8010838C(CdStep_DiskErr);
            break;
        }
        g_CdStep++;
        break;

    case CdStep_SetPos:
        if (CdSync(CdlNop, NULL) == CdlNoIntr) {
            break;
        }
        cd = &g_CdCallback;
        cd->cb = cdFile->cb;
        pLoadFile = &cdFile->size;
        g_Cd.D_80137F68 = *pLoadFile / 0x2000;
        g_VabId = cdFile->vabId;
        g_Cd.overlayBlockCount = *pLoadFile / 0x800;
        // how many bytes long will be the last block to read?
        // eg. if the file is 0x820 long, then the value will be 0x20
        g_Cd.overlayLastBlockSize = *pLoadFile % 0x800;
        D_80137F96 = cdFile->unkF;
        g_Cd.addr = D_800ACD10[cdFile->unkD];
        g_Cd.D_80137F6C = 0;
        g_Cd.D_80137F70 = 0;
        g_Cd.D_80137F74 = 0;
        g_Cd.D_80137F78 = 0;
        if (g_LoadFile & 0x8000) {
            CdIntToPos(cdFile->loc, &cd->loc);
            if (CdControl(CdlSeekL, &cd->loc, NULL)) {
                g_CdStep = CdStep_Complete;
            }
        } else {
            if (cd->cb == CdCallback_16 || cd->cb == CdCallback_Vh) {
                SsVabClose(g_VabId);
            }
            if (g_CdCallback != CdCallback_16 &&
                    g_CdCallback != CdCallback_Vh ||
                (s16)SsVabTransCompleted(SS_IMEDIATE) == 1) {
                CdIntToPos(cdFile->loc, &g_CdLoc);
                if (g_CdCallback == CdCallback_12) {
                    CdIntToPos(
                        g_EquippedWeaponIds[0] * 14 + cdFile->loc, &g_CdLoc);
                }
                if (g_CdCallback == CdCallback_13) {
                    CdIntToPos(
                        g_EquippedWeaponIds[1] * 14 + cdFile->loc, &g_CdLoc);
                }
#if defined(MOD_SATURN)
#ifndef OFF_BIN_WEAPON2
                // (the first compile of make disk, before it regenerates
                // lba_us.h with the file in it)
#define OFF_BIN_WEAPON2 0
#endif
                // Saturn port: weapons 60-63 are in BIN/WEAPON2.BIN, a
                // slot (0x7000 bytes, 14 sectors) per weapon and hand: the
                // retail WEAPON0/1.BIN hold 0-58 and can't grow (the files
                // after them are at fixed positions)
                if ((g_CdCallback == CdCallback_12 ||
                     g_CdCallback == CdCallback_13) &&
                    g_EquippedWeaponIds[g_CdCallback == CdCallback_13] >=
                        MOD_FIRST_WEAPON2) {
                    CdIntToPos(OFF_BIN_WEAPON2 +
                                   ((g_EquippedWeaponIds[g_CdCallback ==
                                                         CdCallback_13] -
                                     MOD_FIRST_WEAPON2) *
                                        2 +
                                    (g_CdCallback == CdCallback_13)) *
                                       14,
                               &g_CdLoc);
                }
#endif
                if (g_CdCallback == CdCallback_Monster) {
#if defined(MOD_SATURN)
                    CdIntToPos(
                        g_LoadOvlIdx * 11 +
                            (g_LoadOvlIdx >= MOD_FIRST_MONSTER2
                                 ? OFF_BIN_MONSTER2 - MOD_FIRST_MONSTER2 * 11
                                 : cdFile->loc),
                        &g_CdLoc);
#else
                    CdIntToPos(g_LoadOvlIdx * 11 + cdFile->loc, &g_CdLoc);
#endif
                }
                if (g_CdCallback == CdCallback_4) {
                    CdIntToPos(g_LoadOvlIdx * 4 + cdFile->loc, &g_CdLoc);
                }
                if (CdControl(CdlSetloc, &g_CdLoc, NULL)) {
                    if (CdSync(CdlNop, NULL) != CdlDiskError) {
                        g_CdStep++;
                    } else {
                        func_8010838C(CdStep_DiskErr);
                    }
                }
            }
        }
        break;

    case CdStep_Seek:
        if (CdSync(CdlNop, NULL) == CdlNoIntr) {
            break;
        }
        CdReadyCallback(func_80107460);
        CdDataCallback(g_CdCallbacks[g_CdCallback]);
        if (CdControl(CdlReadN, &g_CdLoc, NULL) == 0) {
            func_801073C0();
        } else if (CdSync(CdlNop, NULL) == CdlDiskError) {
            func_801073C0();
            func_8010838C(CdStep_DiskErr);
        } else {
            g_CdStep++;
        }
        break;

    case CdStep_5:
        if (CdSync(CdlNop, NULL) == CdlDiskError) {
            func_8010838C(CdStep_DiskErr);
            break;
        }
        if (g_Cd.D_80137F78 == -1) {
            func_8010838C(CdStep_DmaErr);
        }
        if (g_Cd.D_80137F78 == -3) {
            func_8010838C(CdStep_DmaErr);
        }
        if (g_Cd.D_80137F78 == -2) {
            func_8010838C(CdStep_SdHeaderErr);
        }
        if (g_Cd.D_80137F78 == 1) {
            func_801073C0();
            if (CdControl(CdlPause, NULL, NULL)) {
                g_CdStep++;
            }
        }
        break;

    case CdStep_6:
        if (CdSync(CdlNop, NULL) == CdlDiskError) {
            func_8010838C(CdStep_DiskErr);
            break;
        }
        switch (g_CdCallback) {
        case CdCallback_StagePrg:
            pSrc = STAGE_PRG_PTR;
            i = 0;
            pDst = &g_api.o;
            do {
                i++;
                *pDst++ = *pSrc++;
            } while (i < (s32)(sizeof(Overlay) / sizeof(void*)));
            break;
        case CdCallback_6:
            clutAddr = g_Clut[0];
            StoreImage(&g_Vram.D_800ACDB8, clutAddr);
            clutAddr = g_Clut[2];
            StoreImage(&g_Vram.D_800ACDB8, clutAddr);
            DrawSync(0);
            LoadImage(&g_Vram.D_800ACDB0, clutAddr);
            break;
        case CdCallback_0:
            LoadImage(&g_Vram.D_800ACD98, &D_800A04CC);
            break;
        case CdCallback_1:
            StoreImage(&g_Vram.D_800ACDA8, g_Clut[1]);
            break;
        case CdCallback_StageChr:
            StoreImage(&g_Vram.D_800ACDB8, g_Clut[0]);
            break;
#if defined(MOD_SATURN)
        case CdCallback_Ric: // (Alucard's overlay uses it too)
            if (g_LoadFile == CdFile_RichterPrg) {
                ModRicCostumeApply();
            }
            break;
#endif
        case CdCallback_16:
            D_80137FB0 = (short)SsVabOpenHeadSticky(
                g_Cd.addr, g_VabId, g_VabAddrs[g_VabId]);
            break;
        case CdCallback_17:
        case CdCallback_Vh:
            while ((s16)SsVabTransCompleted(SS_IMEDIATE) != 1) {
            }
            break;
        case CdCallback_Seq:
            temp_v1_9 = g_StagesLba[g_StageId].seqIdx;
            func_80131EBC(aPqes_1, D_800ACCF8[temp_v1_9].unk8);
            break;
        }

        if (D_800ACD7C != 0) {
            D_800ACD7C = 0;
            g_CdStep = CdStep_LoadInit;
        } else if (cdFile->nextCdFileType != CdFile_NoNext) {
            g_LoadFile = cdFile->nextCdFileType;
            g_CdStep = CdStep_LoadInit;
        } else if (CdControl(CdlPause, NULL, NULL)) {
            g_LoadFile = CdFile_None;
            g_CdStep = CdStep_None;
            D_80137FA0 = 0;
        }
        break;

    case CdStep_Complete:
        if (CdSync(CdlNop, NULL) == CdlDiskError) {
            func_8010838C(CdStep_DiskErr);
        } else if (CdSync(CdlNop, NULL) == CdlComplete) {
            g_LoadFile = CdFile_None;
            g_CdStep = CdStep_None;
            D_80137FA0 = 0;
        }
        break;

    case CdStep_DmaErr:
        if (g_Timer & 4) {
            FntPrint("DMA error\n");
        } else {
            FntPrint("\n");
        }
        g_CdStep = CdStep_Retry;
        break;

    case CdStep_SdHeaderErr:
        if (g_Timer & 4) {
            FntPrint("SD HEADER error\n");
        } else {
            FntPrint("\n");
        }
        g_CdStep = CdStep_Retry;
        break;

    case CdStep_DiskErr:
        if (g_Timer & 4) {
            FntPrint("disk error\n");
        } else {
            FntPrint("\n");
        }
        g_CdStep = CdStep_Retry;
        break;

    case CdStep_Retry:
        if (g_Timer & 4) {
            FntPrint("retry\n");
        } else {
            FntPrint("\n");
        }
        do {
        } while (CdInit() == 0);
        MuteSound();
        g_CdStep = CdStep_LoadInit;
        D_800ACD7C = 1;
        D_80137FA0++;
        break;

    case CdStep_RetryXa:
        if (g_Timer & 4) {
            FntPrint("retry xa\n");
        } else {
            FntPrint("\n");
        }
        do {
        } while (CdInit() == 0);
        MuteSound();
        PlaySfx(currentMusicId);
        g_CdStep = CdStep_None;
        D_800ACD7C = 1;
        break;

    case CdStep_CdShellOpenErr:
        // The CdlStatShellOpen flag is cleared only when CdlNOP is issued
        CdControlF(CdlNop, NULL);
        g_CdStep = CdStep_F1;
        if (g_Timer & 4) {
            FntPrint("cd shell open error\n");
        }
        break;

    case CdStep_F1:
        temp_v1_8 = CdSync(CdlNop, result) == CdlComplete;
        if (temp_v1_8 != 0) {
            if (!(result[0] & CdlStatShellOpen)) {
                g_CdStep = CdStep_F2;
            } else {
                g_CdStep = CdStep_CdShellOpenErr;
            }
        } else if (temp_v1_8 == 5) {
            g_CdStep = CdStep_CdShellOpenErr;
        }
        if (g_Timer & 4) {
            FntPrint("cd shell open error\n");
        }
        break;

    case CdStep_F2:
        CdControlF(CdlGetTN, NULL);
        g_CdStep = CdStep_F3;
        break;

    case CdStep_F3:
        temp_v1_9 = CdSync(CdlNop, result) == CdlComplete;
        if (temp_v1_9 != 0) {
            if ((result[0] & (CdlStatShellOpen | CdlStatStandby)) == 2) {
                g_CdStep = CdStep_Retry;
                if (g_LoadFile == CdFile_None && currentMusicId != 0 &&
                    stopMusicFlag == 0) {
                    g_CdStep = CdStep_RetryXa;
                }
            } else {
                g_CdStep = CdStep_CdShellOpenErr;
            }
        } else if (temp_v1_9 == 5) {
            g_CdStep = CdStep_CdShellOpenErr;
        }
        if (g_Timer & 4) {
            FntPrint("checking cd\n");
        } else {
            FntPrint("\n");
        }
        break;

    default:
        FntPrint("error step:%02x\n", g_CdStep);
        break;
    }

    if ((!(g_CdStep & 0xF0)) && func_801073E8() != 0) {
        func_8010838C(CdStep_CdShellOpenErr);
    }
}
