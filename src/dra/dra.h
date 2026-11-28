// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef DRA_H
#define DRA_H

#include "disk.h"
#include "game.h"
#include "weapon.h"
#include "items.h"

#define DAMAGE_FLAG_NORMAL 0x0000
#define DAMAGE_FLAG_CRITICAL 0x4000
#define DAMAGE_FLAG_ABSORB 0x8000
#define DAMAGE_FLAG_IMMUNE 0xC000

#define SFX_START (0x600)
#ifdef VERSION_PSP
#define SFX_LAST (0x90B)
#else
#define SFX_LAST (0x8E0)
#endif
#define MAX_SND_COUNT (0x100)

#define SEQ_TABLE_S_MAX 0x10
#define SEQ_TABLE_T_MAX 1

#define VSYNC_UNK_LEN 1024

#define NUM_CH 4
#define NUM_CH_2 4 // not sure if this is related to NUM_CH

typedef enum {
    MENU_DG_MAIN,
    MENU_DG_BG,
    MENU_DG_EQUIP_OVERVIEW,
    MENU_DG_EQUIP_SELECTOR,
    MENU_DG_INFO_BAR,
    MENU_DG_RELICS,
    MENU_DG_SPELLS,
    MENU_DG_SETTINGS,
    MENU_DG_CLOAK_COLOR,
    MENU_DG_CFG_BUTTONS,
    MENU_DG_CLOAK_LINING,
    MENU_DG_CFG_SOUND,
    MENU_DG_WINDOW_COLORS,
    MENU_DG_TIME_ATTACK,
    MENU_DG_EQUIP_SORT,
    MENU_DG_FAMILIARS,
#if defined(VERSION_PSP)
    MENU_PSP_EXTRA_1,
    MENU_PSP_EXTRA_2,
    MENU_PSP_EXTRA_3,
#endif
    NUM_MENU,
} MenuDialogue;

typedef enum {
    DEBUG_NORMAL,
    DEBUG_TEXTURE_VIEWER,
    DEBUG_TILESET_VIEWER,
    DEBUG_PALETTE_VIEWER,
    DEBUG_END,
} DebugMode;

typedef enum {
    DEBUG_COLOR_CHANNEL_RED,
    DEBUG_COLOR_CHANNEL_GREEN,
    DEBUG_COLOR_CHANNEL_BLUE,
} DebugColorChannel;

typedef enum {
    SimFileType_System,
    SimFileType_StagePrg,
    SimFileType_Vh,
    SimFileType_Vb,
    SimFileType_Seq,
    SimFileType_StageChr,
    SimFileType_Unused6,
    SimFileType_Weapon0Prg,
    SimFileType_Weapon1Prg,
    SimFileType_Weapon0Chr,
    SimFileType_Weapon1Chr,
    SimFileType_FamiliarPrg,
    SimFileType_FamiliarChr,
    SimFileType_Monster,
} SimFileType;

typedef enum {
    SIM_STAGE_PRG,
    SIM_1,
    SIM_STAGE_CHR,
    SIM_3,
    SIM_VH,
    SIM_VB,
    SIM_6,
    SIM_SEQ,
    SIM_WEAPON_PRG,
    SIM_WEAPON0_CHR,
    SIM_WEAPON1_CHR,
    SIM_11,
    SIM_12,
    SIM_13,
    SIM_14,
    SIM_15,
    SIM_16,
    SIM_17,
    SIM_FAMILIAR_PRG,
    SIM_FAMILIAR_CHR,
    SIM_MONSTER,
    SIM_21,
} SimKind;

typedef struct {
    const char* path; // file name
    void* addr;       // where to load the file to
    size_t size;      // file size
    SimKind kind;
} SimFile;

enum AluEntities {
    E_NONE,                         // EntityNull
    E_FACTORY,                      // EntityFactory
    E_SMOKE_PUFF,                   // EntitySmokePuff
    E_GRAVITY_BOOT_BEAM,            // EntityGravityBootBeam
    E_KNIFE,                        // EntitySubwpnKnife
    E_UNK_5,                        // func_8011E4BC
    E_UNK_6,                        // EntityDiveKickAttack
    E_UNK_7,                        // EntityGiantSpinningCross
    E_CRASH_CROSS_ROTATING,         // EntitySubwpnCrashCross
    E_SUBWPN_CRASH_CROSS_PARTICLES, // EntitySubwpnCrashCrossParticles
    E_SUBWPN_AXE,                   // EntitySubwpnThrownAxe
    E_BLINK_WHITE,                  // EntityPlayerBlinkWhite
    E_SUBWPN_VIBHUTI,               // EntitySubwpnThrownVibhuti
    E_UNK_13,                       // func_8011E0E4
    E_UNK_14,                       // func_8011EDA0
    E_UNK_15,                       // EntityUnarmedAttack
    E_UNK_16,                       // func_8011EDA8
    E_SUBWPN_AGUNEA,                // EntitySubwpnAgunea
    E_SUBWPN_AGUNEA_HIT_ENEMY,      // EntityAguneaHitEnemy
    E_UNK_19,                       // EntityNumberMovesToHpMeter
    E_SUBWPN_REBOUND_STONE,         // EntitySubwpnReboundStone
    E_UNK_21,                       // EntityLevelUpAnimation
    E_SUBWPN_HOLY_WATER,            // EntitySubwpnHolyWater
    E_SUBWPN_HOLY_WATER_FLAME,      // EntitySubwpnHolyWaterFlame
    E_UNK_24,                       // EntityUnkId24
    E_HELLFIRE,                     // EntityHellfire
    E_HELLFIRE_SMALL_FIREBALL,      // EntityHellfireNormalFireball
    E_HELLFIRE_BIG_FIREBALL,        // EntityHellfireBigFireball
    E_UNK_28,                       // EntityExpandingCircle
    E_UNK_29,                       // func_80127CC8
    E_HIT_BY_LIGHTNING,             // EntityHitByLightning
    E_PLAYER_OUTLINE,               // EntityPlayerOutline
    E_UNK_32,                       // EntityPlayerDissolves
    E_HIT_BY_ICE,                   // EntityHitByIce
    E_MIST,                         // EntityMist
    E_WING_SMASH_TRAIL,             // EntityWingSmashTrail
    E_UNK_36,                       // func_8011B480
    E_UNK_37,                       // EntityGuardText
    E_UNK_38,                       // EntityTransparentWhiteCircle
    E_UNK_39,                       // EntityPlayerPinkEffect
    E_HOLYWATER_BREAK_GLASS,        // EntitySubwpnHolyWaterBreakGlass
    E_SUBWPN_STOPWATCH,             // EntityStopWatch
    E_SUBWPN_STOPWATCH_CIRCLE,      // EntityStopWatchExpandingCircle
    E_SUBWPN_BIBLE,                 // EntitySubwpnBible
    E_SUBWPN_BIBLE_TRAIL,           // EntitySubwpnBibleTrail
    E_BAT_FIREBALL,                 // EntityBatFireball
    E_UNK_46,                       // func_80123B40
    E_UNK_47,                       // func_80119F70
    E_UNK_48,                       // UnknownEntId48
    E_UNK_49,                       // UnknownEntId49
    E_UNK_50,                       // func_80123A60
    E_SMALL_RISING_HEART,           // EntitySmallRisingHeart
    E_BAT_ECHO,                     // EntityBatEcho
    E_UNK_53,                       // func_8011B530
    E_UNK_54,                       // func_8011F074
    E_UNK_55,                       // func_80130264
    E_UNK_56,                       // func_8012F894
    E_UNK_57,                       // func_80130618
    E_UNK_58,                       // func_801309B4
    E_UNK_59,                       // func_80130E94
    E_UNK_60,                       // func_8013136C
    E_UNK_61,                       // func_80129864
    E_UNK_62,                       // EntityNull
    E_SUMMON_SPIRIT,                // EntitySummonSpirit
    E_UNK_64,                       // func_80123F78
    E_TELEPORT,                     // EntityTeleport
    E_SLEEP_ZZZ,                    // EntityPlayerSleepZ
    E_UNK_67,                       // EntityNull
    NUM_ENTITIES,

    E_UNK_238 = 238, // ???
    E_UNK_254 = 254, // ???

};

enum AluBlueprints {
    BP_0,
    BP_1,
    BP_GRAVITY_BOOT_BEAM,
    BP_KNIFE,
    BP_4,
    BP_5,
    BP_CRASH_CROSS,
    BP_CRASH_CROSS_CROSSES_ONLY,
    BP_CRASH_CROSS_BLUE_PARTICLES,
    BP_AXE,
    BP_REBOUND_STONE_HIT,
    BP_11,
    BP_12,
    BP_13,
    BP_14,
    BP_15,
    BP_16,
    BP_17,
    BP_18,
    BP_19,
    BP_20,
    BP_21,
    BP_22,
    BP_23,
    BP_REBOUND_STONE,
    BP_LEVEL_UP_TEXT,
    BP_VIBHUTI,
    BP_HOLYWATER, // Alucard's holy water subweapon is different from Ricther's
    BP_HOLYWATER_GREEN_FIRE,
    BP_29,
    BP_30,
    BP_31,
    BP_32,
    BP_HELLFIRE,
    BP_HELLFIRE_SMALL_FIREBALL,
    BP_HELLFIRE_BIG_FIREBALL,
    BP_36,
    BP_37,
    BP_38,
    BP_39,
    BP_RIPPLE_OUTLINE,
    BP_41,
    BP_42,
    BP_43,
    BP_BLINK_WHITE,
    BP_45,
    BP_HIT_BY_ICE,
    BP_47,
    BP_48,
    BP_49,
    BP_50,
    BP_51,
    BP_52,
    BP_53,
    BP_54,
    BP_55,
    BP_56,
    BP_57,
    BP_58,
    BP_59,
    BP_60,
    BP_61,
    BP_62,
    BP_63,
    BP_64,
    BP_WING_SMASH_TRAIL,
    BP_66,
    BP_67,
    BP_68,
    BP_69,
    BP_70,
    BP_71,
    BP_72,
    BP_73,
    BP_74,
    BP_STOPWATCH_ACTIVATE,
    BP_76,
    BP_77,
    BP_78,
    BP_79,
    BP_80,
    BP_BAT_FIREBALL,
    BP_82,
    BP_83,
    BP_84,
    BP_85,
    BP_86,
    BP_87,
    BP_88,
    BP_89,
    BP_90,
    BP_91,
    BP_92,
    BP_93,
    BP_94,
    BP_95,
    BP_96,
    BP_97,
    BP_98,
    BP_SMALL_RISING_HEART,
    BP_100,
    BP_101,
    BP_102,
    BP_BAT_ECHO,
    BP_104,
    BP_105,
    BP_106,
    BP_107,
    BP_108,
    BP_109,
    BP_110,
    BP_111,
    BP_112,
    BP_113,
    BP_114,
    BP_115,
    BP_116,
    BP_SUMMON_SPIRIT,
    BP_118,
    BP_119,
    BP_120,
    BP_TELEPORT,
    BP_SLEEP_ZZZ,
    NUM_BLUEPRINTS,
};
#if defined(MOD_SATURN)
// Saturn port: the Saturn's blueprint table (ALUCARD.PRG 0x060CCB78) goes on
// to 128; 123-126 (and 128) spawn Saturn-only entities. 7879C.c appends
// 123-127 after the retail ones (macros, not enum entries: the asset tool
// matches the enum against assets/dra/us/blueprints.yaml)
#define MOD_NUM_BLUEPRINTS (NUM_BLUEPRINTS + 5)
#define BP_MOD_SPEAR_SPARK 127 // Alucard spear's sparks (weapon slot 3)
#endif

// NOTE: B_MAKE from DRA is different than RIC or MARIA!
// Parsing is done in EntityFactory
// entityId: what entity to spawn based on the Entity Set
// amount: How many entities to spawn in total
// nPerCycle: how many entities to spawn at once without waiting for tCycle
// isNonCritical: 'true' for particles, 'false' for gameplay related entities
//   false: keep searching for a free entity slot every frame to make the entity
//   true: when there are no entities available then just forgets about it
// incParamsKind: the technique used to set the self->params to the new entity
//   false: it is set from 0 to 'nPerCycle'
//   true: it is set from 0 to 'amount'
// timerCycle: wait frames per cycle until 'amount' of entities are made
// kind: refer to `BlueprintKind` for a list of options
// origin: position where the entity will spawn from
// timerDelay: how many frames to wait before starting to make the first entity
#define B_MAKE(entityId, amount, nPerCycle, isNonCritical, incParamsKind,      \
               timerCycle, kind, origin, timerDelay)                           \
    {(entityId),                                                               \
     (amount),                                                                 \
     ((nPerCycle) & 0x3F) | ((!!(incParamsKind)) << 6) |                       \
         ((!!(isNonCritical)) << 7),                                           \
     (timerCycle),                                                             \
     ((kind) & 15) | (((origin) & 15) << 4),                                   \
     timerDelay}
enum BlueprintKind {
    B_KIND_0,
    B_KIND_1,
    B_KIND_2,
    B_KIND_3,
    B_KIND_4,
    B_KIND_5,
    B_KIND_6,
    B_KIND_7,
    B_KIND_8,
    B_KIND_9,
    NUM_BLUEPRINT_KIND,
};
enum BlueprintOrigin {
    // Spawned entities have a life-cycle on their own and
    B_ORIGIN_DEFAULT,

    B_ORIGIN_1,
    B_ORIGIN_2,
    B_ORIGIN_3,
    B_ORIGIN_4,
    B_ORIGIN_5,
    B_ORIGIN_6,
    B_ORIGIN_7,
    B_ORIGIN_8,
    B_ORIGIN_9,
    B_ORIGIN_10,
    B_ORIGIN_11,
    B_ORIGIN_12,
    B_ORIGIN_13,
    B_ORIGIN_14,
};

typedef enum {
    STATUS_AILMENT_POISON,
    STATUS_AILMENT_CURSE,
    STATUS_AILMENT_PETRIFIED,
    STATUS_AILMENT_DARK_METAMORPHOSIS,
    STATUS_AILMENT_UNK04,
    STATUS_AILMENT_UNK05, // used by EntityPlayerPinkEffect
} StatusAilments;

// Used in HandleTransformationMP, maybe elsewhere
typedef enum { FORM_BAT, FORM_MIST, FORM_WOLF } TransformationForm;

// Used for many functions which do things like testing and decrementing a value
typedef enum { CHECK_ONLY, REDUCE } CallMode;

// Info necessary to load a file from the Cd in UpdateCd
typedef struct {
    s32 loc;        // lba offset, might be a s32
    CdCallbacks cb; // sets g_CdCallback
    s32 size;       // file size
    u8 vabId;       // index for g_VabAddrs, between 0 and 5?
    u8 unkD;        // index for D_800ACD10, between 0 and 6?
    u8 nextCdFileType;
    u8 unkF;
} CdFile;

// Used for SEQ_LIB.SEQ and SEQ_DAI.SEQ
typedef struct {
    s32 loc;
    s32 size;
    u32 unk8;
} CdFileSeq;

typedef struct {
    CdCallbacks cb;
    CdlLOC loc;
} CdMgr;

typedef struct {
    RECT dstRect;
    s32 D_80137F68;
    s32 D_80137F6C;
    s32 D_80137F70;
    s32 D_80137F74;
    s32 D_80137F78;
    s32 D_80137F7C;
    u8* overlayCopySrc;
    u8* overlayCopyDst;
    s8* addr;
    s32 overlayBlockCount;
    s32 overlayLastBlockSize;
} CdThing;

#define NOW_LOADING_PRIM_COUNT 16
typedef struct {
    /* 0x00 */ s32 step;
    /* 0x04 */ s32 primIndex;
    /* 0x08 */ s16 x;
    /* 0x0A */ s16 y;
    /* 0x0C */ s16 unkC; // unused
    /* 0x0E */ s16 verticalWave;
    /* 0x10 */ s16 horizontalWave;
    /* 0x12 */ u16 unk12; // unused
    /* 0x14 */ s16 speed;
    /* 0x16 */ s16 waveTable[NOW_LOADING_PRIM_COUNT];
} NowLoadingModel; // size=0x36

typedef struct {
    s32 x;
    s32 y;
    s32 w;
    s32 h;
    s32 u;
    s32 v;
    s32 tpage;
    s32 clut;
} RicSubwpnIconParams;
#if defined(MOD_SATURN_MARIA) && !defined(VERSION_PSP)
// Saturn port: the Richter HUD's subweapon icon table (src/dra/5F60C.c);
// the PSP Maria's overlay points it at her own icons, cd.c resets it
extern RicSubwpnIconParams g_ricSubwpnIcons[];
extern RicSubwpnIconParams* g_ModSubwpnIcons;
extern Primitive* (*g_ModMar2HudIcons)(Primitive* prim);
#endif

typedef struct {
    u32 displayHP;
    s32 primIndex1;
    s32 primIndex2;
    s32 unk0C;
    u32 unk10;
    s32 unk14;
    u32 unk18;
    u32 unk1C;
    u32 unk20;
    u32 unk24;
    s32 unk28;
    s32 g_HealingMailTimer;
} PlayerHud;

// g_ButtonCombo is an array of 16, here is what each index is for
// Names should be updated as each one is decompiled.
typedef enum {
    COMBO_GRAVITY_BOOTS,
    COMBO_QCF, // Quarter circle forward (down, down+forward, forward)
    COMBO_BF,  // Back forward
    COMBO_HELLFIRE,
    COMBO_TETRA_SPIRIT,
    COMBO_UNK5,
    COMBO_SOUL_STEAL,
    COMBO_UNK7,
    COMBO_UNK8,
    COMBO_SWORD_BROTHERS,
    COMBO_UNK10,
    COMBO_UNK11,
    COMBO_UNK12,
    COMBO_UNK13,
    COMBO_SUMMON_SPIRIT,
    COMBO_DARK_METAMORPH,
    COMBO_NUM,
} ButtonComboIdx;

typedef struct SeqData {
    u8 volume;
    u8 reverb_depth;
    u8 unk2;
} SeqData;

typedef struct Cmd14 {
    u8 cdLoc[8];
    s32 unk8;
    s16 unkc;
    u8 unke;
} Cmd14;

// Used in dra/7E4BC, dra/bss, rbo5/unk_4907C, bo4/unk_49A6C
typedef struct {
    f32 posX;
    f32 posY;
    s16 angle1;
    s16 angle2;
    s16 size;
    s16 xOffset;
    s16 yOffset;
    s16 pad;
} mistStruct; // size = 0x14

// Used in dra/menu, dra_psp/menu
typedef struct EquipMenuHelper {
    EquipKind equipTypeFilter;
    s32 index;
    s32 isAccessory;
} EquipMenuHelper;

// Used in dra/menu, dra_psp/menu
// Struct for table of values to intitialize MenuContext structs
typedef struct {
    /* 0x00 */ s16 cursorX;
    /* 0x02 */ s16 cursorY;
    /* 0x04 */ u16 cursorW;
    /* 0x06 */ u16 cursorH;
    /* 0x08 */ s32 otIdx;
} MenuContextInit; // size = 0x0C

// Used in dra/dra, dra/7E4BC, dra/d_DBD4
typedef struct {
    u16 palette;
    u8 unk2;
} Unkstruct_800AE180;

// Used in dra/7E4BC, dra/d_DBD4
typedef struct {
    u8 timers[8];
    u8 blueprints[8];
    u8 blueprintParams[8];
    u32 unk18;
} Unkstruct_800ADEF0; // size:0x1C

// Used in dra/d_10798, dra/8D3E8, dra/dra.h
// This appears to be a super miniature Entity or something
// All it has is a state, a timer, and a facing direction.
typedef struct {
    s32 state;
    s32 timer;
    s32 facingLeft;
} helper_8012F178;

// Used for the button combos to signal successfully completing the sequence
#define COMBO_COMPLETE 0xFF

// File list:
extern u8 aPqes[];         // TODO: extract file
extern u8 aPqes_0[];       // TODO: extract file
extern u8 aPqes_1[];       // TODO: extract file
extern u8 aPbav[0x3000];   // TODO: extract file
extern u8 aPbav_0[0x2000]; // TODO: extract file
extern u8 aPbav_1[0x2000]; // TODO: extract file
extern u8 aPbav_2[0x2000]; // TODO: extract file

extern s16 g_ButtonMask[];
extern u16 D_800A04CC[];
extern u32 D_800A04F8;
extern s32 D_800A2438;

extern RoomTeleport D_800A245C[];
extern u32 D_800A2D24;
extern const char* D_800A83AC[];

extern u16 D_800A37D8[2];
extern u16 g_JosephsCloakColors[4];
extern GfxBank** g_GfxSharedBank[];
extern s16** D_800A3B70[18];
extern u_long* D_800A3BB8[];
extern Lba g_StagesLba[80];
#if defined(MOD_SATURN)
u32 ModStageMusic(void);
#endif

extern SubweaponDef g_SubwpnDefs[13];
// These are different on PSP since they have text that needs translating.
#if defined(VERSION_PSP)
extern char** g_SaveAreaNamesSecondPart;
extern char** g_SaveAreaNames;
extern RelicDesc* g_RelicDefs;
extern SpellDef* g_SpellDefs;
extern char** g_MenuStr;
extern EnemyDef* g_EnemyDefs;
extern Accessory* g_AccessoryDefs;
extern Equipment* g_EquipDefs;
#else
extern RelicDesc g_RelicDefs[30];
extern SpellDef g_SpellDefs[28];
#if defined(MOD_KO)
// Korean trial: one appended System row ("Korean")
extern const char* g_MenuStr[111];
#else
extern const char* g_MenuStr[110];
#endif
#if defined(MOD_KO)
// Static chain: AlucardPrg -> KoGlyphs -> 0x04 (Alucard's old next,
// kept as KO struct's next). Zero DRA text (init data only, OFF keeps 0x04).
#define KO_ALUCARD_NEXT CdFile_KoGlyphs
// A3: non-Alucard chain RichterPrg -> KoGlyphs -> 0x14 (cd.c sets g_LbaBinKoGlyphs.nextCdFileType per load)
#define KO_RICHTER_NEXT CdFile_KoGlyphs
#else
#define KO_ALUCARD_NEXT 0x04
#define KO_RICHTER_NEXT 0x14
#endif
#if defined(MOD_KO)
// Disc: BIN/ARC_KO.BIN at the disc end shifts every post-VAB literal
// sector +1 (layout dry-run + image TOC verified; sizes and root movies stay).
// KO_LBA gates those literals (OFF keeps retail values). Data only.
#define KO_LBA(b) ((b) + 1)
#else
#define KO_LBA(b) (b)
#endif
#if defined(MOD_KO)
// Korean trial: 776 Hangul glyphs, 16x15 1bpp MSB,
// 30 bytes each (same layout as Krom2RawAdd returns). Not in DRA's rodata:
// build_ko_trial.sh packs BIN/ARC_KO.BIN (glyphs + KO blob, trial-only disc
// file at the disc end) and cd.c's CdFile_KoGlyphs chain loads it to
// KO_GLYPH_BLOCK_ADDR (A-gap). The 66590.c wrapper and the blob renderer
// (MenuDrawKoHook at KO_BLOB_ADDR) both read the block.
#define KO_GLYPH_COUNT 958 /* v14h = v14g + 6 Hangul cells F3F8..F3FD (꿀낄뎌떤앉픔, gen_ko_glyph_h.py); v14g = v14f + 1 Hangul cell F3F7 (켬, Extra settings; gen_ko_glyph_add.py GLYPH_PREV_TAG=v14f GLYPH_NO_REUSE=1); v14f = v14e + 1 Hangul cell F3F6 (gen_ko_glyph_f.py); v14e = v14d + 91 Hangul cells F3CB.. (gen_ko_glyph_e.py; 10 cells kept free below KO_POP_TEXT 0x5640 = 960 cells); v14d = v14c + 40 ASCII cells F373.. (gen_ko_glyph_lat.py d; check_ko_glyph.py fails if a cell was skipped); v14c (+16 Latin cells F360..F36F, +롯쓸됐 F370..F372); v14a: MGalmuri12 12x15 1bpp 23 B cells; 800 cells = 18400 B fit the original 23280 B block */
#define KO_GLYPH_CELL_BYTES 23
#define KO_GLYPH_12COL 1 /* MGalmuri12 "an na" 12-col 23 B direct path + a (kind0) */
/* cutscene 1-byte promotion table: 112 x u16 (byte 0x80+i -> glyph code), in the
 * ARC_KO zero gap after the engtab (0x8009EFBC..0x8009F500, loaded with the ARC). Read directly by
 * the cutscene render hooks in src/st/*; gen_ko_promo.py writes ko_promo_tbl.bin. */
#define KO_PROMO_TBL_ADDR 0x8009F000
#define KO_PROMO_TBL_SLOTS 112
#define KO_GLYPH_BASE 0xF040
#define KO_GLYPH_BLOCK_ADDR 0x80099000
// Blob: MenuDrawKoHook entry, fixed 768B slot (text + zero pad).
#define KO_BLOB_ADDR 0x8009EAF0
#define KO_BLOB_SLOT 896
// Status-box strings: 0xFF00-terminated KO bytes baked into ARC_KO.BIN
// right after the blob slot (ko_blob.c KO_STR_9x, order asserted by build).
#define KO_STR_BASE 0x8009EE70
#define KO_STR_92 ((const char*)0x8009EE70) // 장비 (6B)
#define KO_STR_93 ((const char*)0x8009EE76) // 10p term (6B)
#define KO_STR_94 ((const char*)0x8009EE7C) // 10p term (8B)
#define KO_STR_95 ((const char*)0x8009EE84) // 10p term (6B)
#define KO_STR_96 ((const char*)0x8009EE8A) // 사역마 (8B)
// ko_sheet: 12x16 sheet tables in the ARC_KO strings slot, appended
// after the 34B box strings (offsets asserted by build_ko_trial.sh).
// Upload list 37B + 9 index strings 30B = 67B; FLAG at +98 stays clear.
// Measured: KO_BOXALL 34B + 2B linker pad, sheet starts at +36
// (ro == 104B = 34 + 2 + 67 + 1 trailing NUL). Strings end 0x8009EED8,
// FLAG 0x8009EEF4 stays clear (16B gap).
// RIGHT/LEFT hand-slot labels (오른손/왼손) appended; SEL/OV/EPOCH
// dirty state moves +8 (EED8/DC/E0).
#define KO_SHEET_UPLOAD ((const char*)0x8009EE94)
#define KO_SHEET_EQUIP ((const char*)0x8009EEB9)
#define KO_SHEET_SKILL ((const char*)0x8009EEBC)
#define KO_SHEET_MAGIC ((const char*)0x8009EEBF)
#define KO_SHEET_SYSTEM ((const char*)0x8009EEC3)
#define KO_SHEET_FAMILIAR ((const char*)0x8009EEC6)
/* Big labels moved to the ARC zero gap behind the engtab (gen_ko_sheet2.py, ko_sheet2.bin
 * at ARC offset 0x6100 = 0x8009F100): UPLOAD2 now carries 12 glyphs (cells 0x10..0x1B),
 * ATK/DEF got 력, and HEAD/BODY/CLOAK/OTHER are new 12px labels (retail slot names Head/Body/
 * Cloak/Other were 8px English). The old strings at 0x8009EECA/EECD stay unused. */
#define KO_SHEET_ATK ((const char*)0x8009F11C)
#define KO_SHEET_DEF ((const char*)0x8009F120)
#define KO_SHEET_HEAD ((const char*)0x8009F124)
#define KO_SHEET_BODY ((const char*)0x8009F128)
#define KO_SHEET_CLOAK ((const char*)0x8009F12C)
#define KO_SHEET_OTHER ((const char*)0x8009F130)
#define KO_SHEET_HOLDER ((const char*)0x8009F134)
#define KO_SHEET2_ADDR 0x8009F100
/* menu titles g_MenuStr[97]/[98] (gen_ko_sheet2.py fixed 8 B slots, asserted there) + the upload list
 * of their glyphs that are not on the sheet yet: sheet row 1 of tpage 7 (indexes 0x30..), uploaded with
 * func_800F98AC(KO_SHEET_UPLOAD3, 0x101) at menu open (PSP uses the same row for its fourth list) */
#define KO_SHEET_FAMLIST ((const char*)0x8009F138)
#define KO_SHEET_SPECIAL ((const char*)0x8009F140)
#define KO_SHEET_UPLOAD3 ((const char*)0x8009F148)
#define KO_POP_TEXT 0x8009E640 /* Resident KO_PopupText(str, leftAlign) (eq_popt.c, <= 0x4B0 B) in the glyph block slot behind the last possible cell (960 x 23 B = 0x5640): the stage overlays only call it */
#define KO_POP_GLYPH 0x8009F280 /* Resident KO_PopGlyph(code, n) (eq_pop.c, <= 0x80 B) for the pickup popup Hangul; ARC zero gap behind the advance table */
#define KO_FIELD_BASE 0x8009F300 /* Stage message-box labels (gen_ko_fieldmsg.py ko_field.bin, 4 x 0x80) in the ARC zero gap behind KO_POP_GLYPH; 0x8009F300..F4FF is unused (KO_SCRATCH is dead) */
#define KO_FONT8_ADV_ADDR 0x8009F200 /* 96 B, per sheet cell index: pen advance of the re-baked ASCII cells (ink width + 2), 0 = other cell (advance 8, no offset); gen_ko_font8.py ko_font8_adv.bin */
#define KO_SHEET_RIGHT ((const char*)0x8009EED0)
#define KO_SHEET_LEFT ((const char*)0x8009EED4)
#define KO_SHEET_UPLOAD2 ((const char*)0x8009F100) /* Glyphs 14-17 re-uploaded to tpage 7 row 0 (cells 0x10-0x13), call y=0x100 */
// ko_equipreloc: equipment-name tail. 3 fixed entries (plain jal
// from DRA, same cost as ordinary calls) + core. Loaded as the tail of
// ARC_KO.BIN (cd.c CdFile_KoGlyphs chain). DRA keeps no KO code/BSS.
// Dirty state lives at A-gap absolute (zero gap 9EED8~9EEF4, like FLAG).
#define KO_EQUIP_BASE 0x8009F500
#define KO_EQUIP_SIZE 0xB00 // 2816B; entries + core + load end <= 0x800A0000
#define KO_EQUIP_SEL 0x8009F500 // 0x40 slot (entry must be first function)
#define KO_EQUIP_OV 0x8009F540 // 0x40 slot
#define KO_EQUIP_ENTER 0x8009F580 // 0x80 slot
#define KO_EQUIP_CORE 0x8009F600 // 0xA00 slot (core end + load <= 0x800A0000)
#define KO_EQUIP_LOAD 0x8009FF40 // 0xC0 slot (menu-open bundle loader ends 0x8009FFE0)
#define KO_EQUIP_NAMES 0x8009FECC // kept (unused by ko_equip_jp; build asserts)
#define KO_JP_BUNDLE_BASE 0x801F0000 // KR hanpae name-block address
#define KO_JP_BUNDLE_LBA 0x4928F // 2-pass wired (OFF_BIN_KO_JP, disc end)
#define KO_JP_BUNDLE_SECTORS 1 // 1669B
#define KO_8P_BASE 0x801F2000 // 8px block RAM base (boot-loaded)
#define KO_8P_SECTORS 2 // 2832B
#define KO_ARC_SECTORS 14 // glyph-area sectors inside ARC_KO.BIN (blocks 0-13)
#define KO_EQ_SELSIG_ADDR 0x8009EED8
#define KO_EQ_OVSIG_ADDR 0x8009EEDC
#define KO_EQ_EPOCH_ADDR 0x8009EEE0
// Scratch: 16 ring buffers of one 12x16/2 strip (96B each,
// KO_SCRATCH_ADDR + (idx & 15) * 0x60 = EF00~F4FF). EF00~EFF7 reuses the
// removed 10q DIAG area; F000~F4FF is past ARC_KO.BIN in the A-gap remainder
// (write-before-read, never loaded from disc). FLAG/TMR/IDX below stay;
// EF00~F4FF is scratch; F500~A0000 is the ko_equip tail (DRA starts at
// 0x800A0000).
#define KO_SCRATCH_N 16
#define KO_SCRATCH_ADDR 0x8009EF00
// Runtime switch (default 1=on, 0=bypass) + per-frame slot ring
// state (timer/id), in the ARC_KO pad (zeros on disc, flag set by build).
#define KO_FLAG_ADDR 0x8009EEF4
#define KO_SLOT_TMR 0x8009EEF8
#define KO_SLOT_IDX 0x8009EEFC
// ARC_KO.BIN content bytes (17 full sectors =
// glyphs + blob slot + strings slot + gap + ko_equip tail + JP bundle 1 +
// 8px block 2, zero-padded). Multiple of 2048 so the loader callback copies
// a fixed CD_BLOCK_LEN per block.
#define KO_ARC_SIZE 57344 // glyph area 28672 + bundle 2048 + 8px 4096 + the new resident area's 11 sectors 22528 (28 sectors; block 17 lands at 0x801F3000, then every 0x800 up to block 27 at 0x801F8000; 3877: was 8 sectors / 51200, the field voice subtitles outgrew 16384)
#endif
#if defined(MOD_SATURN)
extern EnemyDef g_EnemyDefs[426];
#else
extern EnemyDef g_EnemyDefs[400];
#endif
extern Accessory g_AccessoryDefs[90];
#if defined(MOD_SATURN)
extern Equipment g_EquipDefs[MOD_NUM_EQUIP_DEFS];
#else
extern Equipment g_EquipDefs[217];
#endif
#endif
extern s32 g_ExpNext[];
extern s16 D_800AC958[];
extern CdFile* D_800ACC74[];
extern s32 g_CurrentStream;
extern s32 D_800ACE44;
extern s16 g_SensorsCeilingBat[];
extern s16 g_SensorsFloorBat[];
extern s16 g_SensorsWallBat[];
extern s16 g_SensorsCeilingDefault[];
extern s16 g_SensorsFloorDefault[];
extern s16 g_SensorsWallDefault[];
extern s16 g_SensorsCeilingCrouch[];
extern s16 g_SensorsWallCrouch[];

extern Point16 g_SensorsCeiling[];
extern Point16 g_SensorsFloor[];
extern Point16 g_SensorsWall[];

#if !defined(VERSION_US)
extern s32 D_800ACEDC_hd;
#define TRANSFORM_LOCKOUT_TIMER D_800ACEDC_hd
#else
#define TRANSFORM_LOCKOUT_TIMER g_Player.unk20
#endif
extern s16 D_800ACF7C[4];
extern s16 g_SfxPainGrunts[8]; // Alucard's random pain sfx table
extern s16 D_800ACF94[16];
typedef union {
    u8 rawBytes[4];
    AnimationFrame af;
} unionD_800ACFB4;
extern unionD_800ACFB4 D_800ACFB4[];
extern s16* D_800CF324[];
extern s16* D_800CFE48[];
extern PfnEntityUpdate D_800AD0C4[];
extern s16 D_800AD54C[6];
extern s32 D_800AD558[6];
extern s16 D_800AD570[6];
extern AnimationFrame D_800AD57C[25];
extern u8 D_800AD5E0[14];
extern u8 D_800AD5F0[10];
extern AnimationFrame D_800AD5FC[];
extern s32 D_800AD630[];
extern s16 D_800AD670[42][10];
extern s16 D_800AD9B8[24][5];
extern AnimationFrame D_800ADBD4[];
extern AnimationFrame D_800ADC10[];
extern AnimationFrame D_800ADC44[];
extern u16 D_800ADC7C[2];
extern Point16* D_800ADCC8[];
extern s16 D_800ADDE8[][6];
extern u8 D_800AE120[];
extern RECT D_800AE130;
extern RECT D_800AE138;
extern u8 D_800AE140[][8];
extern Unkstruct_800AE180 D_800AE180[];
extern s16 D_800AE190[];
extern s16 D_800AE1B0[];
extern s16 D_800AE1F0[];
extern u32 D_800AE230[8];
extern u32 D_800AE250[8];
extern s32 D_800AE270;
extern s32 D_800AE274;
extern s32 D_800AE278;
extern s32 D_800AE27C;
extern s32 D_800AE280;
extern s32 D_800AE284;
extern s32 D_800AE288;
extern s32 D_800AE28C;
extern s32 D_800AE290;
extern AnimationFrame D_800AE294[];
extern AnimationFrame D_800AFC50[];
extern AnimationFrame D_800AFDA4[];
extern RECT c_backbufferClear;
extern AnimationFrame D_800AFFB8[];
extern s8 D_800B0130[];
extern AnimationFrame* D_800B01B8[];
extern u8 D_800B0608[];
extern s32 D_800B0688[];
extern u32 D_800B06C8[24];
extern u32 D_800B0728[24];
extern RECT D_800B0788;
extern RECT D_800B0790;
extern AnimationFrame D_800B0798[];
extern AnimationFrame D_800B07C8[];
extern s32 D_800B0830[];
extern s32 D_800B083C[];
extern u8 D_800B0848[];
extern s16 D_800B0858[];
extern s16 D_800B0860[];
extern s16 D_800B0884[];
extern s16 D_800B08A8[];

extern s16 g_CdVolumeTable[];
extern struct SeqData g_SeqInfo[];
extern struct XaMusicConfig g_XaMusicConfigs[];
extern s32 g_DebugEnabled;
extern s32 D_800BD1C4;
extern s32 g_VabAddrs[6];
extern s8* g_SfxScripts[172];
extern u8* D_800C52F8[];
extern const char D_800DB524[];
extern const char a0104x04x;
extern const char a2304x04x;
extern const char aBlue;
extern const char aDr03x;
extern const char aEff03x;
extern const char aEnv03x;
extern const char aG403x;
extern const char aGreen;
extern const char aGt303x;
extern const char aGt403x;
extern const char aHalfOff;
extern const char aHalfOn;
extern const char aLine03x;
extern const char aRed;
extern const char aRgb02x02x02x;
extern const char aSp03x;
extern const char aSp1603x;
extern const char aTile03x;
extern Unkstruct_800BF554 g_SfxData[];

#if defined(MOD_SATURN)
// Saturn port: the third hand (mods/sotn-ultimate/src/items/mod_holder.h, in
// 7879C.c)
#define MOD_E_HOLDER_FOOD 68 // g_DraEntityTbl: EntityModHolderFood
bool ModHolderAllows(s32 id);
s32 ModHolderItem(void);
s32 ModHolderOn(void); // MOD_HOLDER_ON() as a call (menu.c size)
s32 ModHolderUse(void);
void EntityModHolderFood(Entity* self);
#endif

extern char* aLightTimer02x;
extern SVECTOR D_800E2024;
extern SVECTOR D_800E202C;
extern helper_8012F178 D_800B08CC[6];
extern s32 D_800B0914;
extern s32 D_800B0918;
extern s32 D_800B091C;
extern s32 D_800B0920;

void func_801072DC(Primitive* prim);
void InitializePads(void);
void ReadPads(void);
void ClearBackbuffer(void);
void CheckCollision(s32 x, s32 y, Collider* res, s32 unk);
void DemoInit(s32 arg0);
void DemoUpdate(void);
void SetGPUBuffRGBZero(void);
void SetGPUBuffRGB(u8 arg0);
void func_800E34DC(s32 arg0);
void SetGameState(GameState gameState);
void func_800E4970(void);
void func_800E8DF0(void);
s32 _peek_event(void);
s32 _card_event_x(void);
void _clear_event_x(void);
void MemcardInit(void);
void MemcardInfoInit(void);
s32 func_800EA5E4(u32);
void func_800EA538(s32);
void func_800EAD7C(void);
void ResetPendingGfxLoad(void);
void LoadEquipIcon(s32 equipIcon, s32 palette, s32 index);
bool func_800EB720(void);
void HideAllBackgroundLayers(void);
void DestroyAllPrimitives(void);
s32 func_800EDAE4(void);
DR_ENV* func_800EDB08(Primitive* prim);
void DemoGameInit(s32 arg0);
void FreePrimitives(s32 index);
void DemoOpenFile(s32);
void DemoInit(s32 arg0);
s32 SetNextRoomToLoad(u32 chunkX, u32 chunkY);
s32 func_800F0CD8(s32 arg0);
s32 func_800F16D0(void);
void resetMapTileOnCastleBlueprint(s32, s32);
void paintMapTileOnCastleBlueprint(s32, s32, s32);
void RevealSecretPassageOnMap(s32, s32, s32);
void func_800F2120(void);
void CalcDefense(void);
bool IsAlucart(void);
void make_all(void);
bool ScissorSprite(SPRT* arg0, MenuContext* arg1);
void MenuDrawImg(MenuContext* ctx, s32 x, s32 y, s32 w, u32 h, s32 u, s32 v,
                 s32 idx, s32 unk2, bool disableTexShade, s32 unk4);
void MenuDrawSprite(
    MenuContext* context, s32 x, s32 y, s32 width, s32 height, s32 u, s32 v,
    s32 clut, s32 tpage, s32 arg9, s32 colorIntensity, s32 argB);
void MenuDrawRect(MenuContext* context, s32 posX, s32 posY, s32 width,
                  s32 height, s32 r, s32 g, s32 b);
s32 func_800F62E8(s32 arg0);
void InitStatsAndGear(bool isDeathTakingItems);

void InitFade(void);
s32 func_801025F4(void);
void SetFadeWidth(s32 fadeWidth);
void SetFadeMode(FadeModes fadeMode);
void UpdateFade(bool skipFollowup);
void HideMap(void);

s32 TimeAttackController(TimeAttackEvents eventId, TimeAttackActions action);
s32 func_800FD664(s32 arg0);
s32 func_800FD6C4(EquipKind equipTypeFilter);
u8* GetEquipOrder(EquipKind kind);
u8* GetEquipCount(EquipKind kind);
const char* GetEquipmentName(EquipKind kind, s32 equipId);
u32 CheckEquipmentItemCount(u32 itemId, u32 equipType);
void GetSpellDef(SpellDef* spell, s32 id);
void LearnSpell(s32 spellId);
bool IsRelicActive(RelicIds relicId);
#include "../../mods/sotn-ultimate/src/familiars/dra_mod_familiars.h"
s32 HandleGravityBootsMP(CallMode mode);
s32 func_800FE3C4(SubweaponDef* subwpn, s32 subweaponId, bool useHearts);
void GetEquipProperties(s32 handId, Equipment* res, s32 equipId);
s32 HandleDamage(DamageParam*, s32, s32 amount, s32);
s32 HandleTransformationMP(TransformationForm, CallMode);
void ClearStatBuff(s32 arg0);
s32 func_801025F4(void);
void ShakeCamera(cameraShakeTypes);
Entity* GetFreeEntity(s16 start, s16 end);
Entity* GetFreeEntityReverse(s16 start, s16 end);
void DestroyEntitiesFromIndex(s16 startIndex);
void SetTexturedPrimRect(
    Primitive* poly, s32 x, s32 y, s32 width, s32 height, s32 u, s32 v);
void func_801073C0(void);
void func_801092E8(s32);
void SetPrimRect(Primitive* poly, s32 x, s32 y, s32 width, s32 height);
void SetPlayerStep(s16 step);
u32 UpdateAnim(s8* frameProps, AnimationFrame** anims);
void func_8010DFF0(s32 resetAnims, s32 arg1);
void ForceAfterImageOn(void);
void EnableAfterImage(void);
s32 func_8010E334(s32 xStart, s32 xEnd);
void func_8010E470(s32, s32);
void func_8010E570(s32);
void func_8010E83C(bool);
void DoGravityJump(void);
bool WolfFormFinished(void);
bool StatusPause(s32 arg0);
s32 func_801104D0();
bool CheckGravityBootsInput(void);
bool CheckQuarterCircleForwardInput();
bool CheckBackForwardInput();
bool CheckDarkMetamorphosisInput();
bool CheckSummonSpiritInput();
void PlayAnimation(s8*, AnimationFrame** frames);
bool CheckHellfireInput();
bool CheckTetraSpiritInput();
bool CheckSoulStealInput();
bool CheckSwordBrothersInput();
void func_80111928(void);
void func_80111CC0(void);
bool func_80111D24(void);
void func_80118894(Entity*);
Entity* func_80118970(void);
void func_80118C28(s32 arg0);
void GetServantStats(Entity* entity, s32 spellId, s32 arg2, FamiliarStats* out);
bool func_8010FDF8(s32 branchFlags);

#ifndef CREATE_FACTORY_FAKE_ARGS
Entity* CreateEntFactoryFromEntity(Entity* entity, u32, s32);
#endif

// Forward declarations for all the entity updating functions
void EntityNull(Entity* self);
void EntityFactory(Entity* self);
void EntitySmokePuff(Entity* self);
void EntityGravityBootBeam(Entity* self);
void EntitySubwpnKnife(Entity* self);
void func_8011E4BC(Entity* self);
void EntityDiveKickAttack(Entity* self);
void EntityGiantSpinningCross(Entity* self);
void EntitySubwpnCrashCross(Entity* self);
void EntitySubwpnCrashCrossParticles(Entity* self);
void EntitySubwpnThrownAxe(Entity* self);
void EntityPlayerBlinkWhite(Entity* self);
void EntitySubwpnThrownVibhuti(Entity* self);
void func_8011E0E4(Entity* self);
void func_8011EDA0();
void EntityUnarmedAttack(Entity* self);
void func_8011EDA8(Entity* self);
void EntitySubwpnAgunea(Entity* self);
void EntityAguneaHitEnemy(Entity* self);
void EntityNumberMovesToHpMeter(Entity* self);
void EntitySubwpnReboundStone(Entity* self);
void EntityLevelUpAnimation(Entity* self);
void EntitySubwpnHolyWater(Entity* self);
void EntitySubwpnHolyWaterFlame(Entity* self);
void EntityUnkId24(Entity* self);
void EntityHellfire(Entity* self);
void EntityHellfireNormalFireball(Entity* self);
void EntityHellfireBigFireball(Entity* self);
void EntityExpandingCircle(Entity* self);
void func_80127CC8(Entity* self);
void EntityHitByLightning(Entity* self);
void EntityPlayerOutline(Entity* self);
void EntityPlayerDissolves(Entity* self);
void EntityHitByIce(Entity* self);
void EntityMist(Entity* self);
void EntityWingSmashTrail(Entity* self);
void func_8011B480(Entity* self);
void EntityGuardText(Entity* self);
void EntityTransparentWhiteCircle(Entity* self);
void EntityPlayerPinkEffect(Entity* self);
void EntitySubwpnHolyWaterBreakGlass(Entity* self);
void EntityStopWatch(Entity* self);
void EntityStopWatchExpandingCircle(Entity* self);
void EntitySubwpnBible(Entity* self);
void EntitySubwpnBibleTrail(Entity* self);
void EntityBatFireball(Entity* self);
void func_80123B40(Entity* self);
void func_80119F70(Entity* self);
void UnknownEntId48(Entity* self);
void UnknownEntId49(Entity* self);
void func_80123A60(Entity* self);
void EntitySmallRisingHeart(Entity* self);
void EntityBatEcho(Entity* self);
void func_8011B530(Entity* self);
void func_8011F074(Entity* self);
void func_80130264(Entity* self);
void func_8012F894(Entity* self);
void func_80130618(Entity* self);
void func_801309B4(Entity* self);
void func_80130E94(Entity* self);
void func_8013136C(Entity* self);
void func_80129864(Entity* self);
void EntitySummonSpirit(Entity* self);
void func_80123F78(Entity* self);
void EntityTeleport(Entity* self);
void EntityPlayerSleepZ(Entity* self);

// commented as a requirement for UpdateCd to match
// void func_80131EBC(const char* str, s16 arg1);
void SetCdPos(s32 value);
void MuteCd(void);
void UnMuteCd(void);
s32 func_80131F28(void);
u16 func_80131F38(void);
bool func_80131F68(void);
s16 func_80131F94(void);
void SetMaxVolume(void);
void InitSoundVars3(void);
void InitSoundVars2(void);
void InitSoundVars1(void);
static void AddCdSoundCommand(s16);
u8 DoCdCommand(u_char com, u_char* param, u_char* result);
void SoundWait(void);
void MuteSound(void);
void func_80132A04(s16 voice, s16 vabId, s16 prog, s16 tone, s16 note,
                   s16 volume, s16 distance);
void StopSeq(void);
bool CdSoundCommandQueueEmpty(void);
bool func_80133950(void);
void func_80133FCC(void);
void SetReleaseRate2(void);
void func_801361F8(void);

#ifdef FIX_UB
// Without these prototypes, the callers assume an int return type and read
// garbage from the unused upper bits of the return register.
s16 AllocPrimitives(u8 primType, s32 count);
s16 AllocPrimitivesReverse(u8 type, s32 count);
s16 GetStatusAilmentTimer(StatusAilments statusAilment, s16 timer);
s16 func_80110394(void);
int MemcardDetectSave(s32 nPort, u8* expectedSaveName, s32 block);
#endif

#if defined(MOD_SATURN) && defined(VERSION_US) && !defined(MOD_ULTIMATE_DEBUG)
// Saturn port: US never calls FntFlush (only HD and the mod's debug build
// do), so DRA's FntPrint debug lines never reach the screen. Dropped for
// room (MOD_DRA_PAD): the PSP dub toggle's voice table (sound.c)
#define FntPrint(...) ((void)0)
#endif

#if defined(MOD_SATURN) && defined(VERSION_US)
// Saturn port: DRA code the US disc build can never run is dropped to free
// room for the mod (MOD_DRA_PAD). Each removal says why it is dead. No
// overlay calls DRA's code directly
// (only through g_api, a data table), so code may move.
#define MOD_DRA_STRIP
// main.c sets g_UseDisk = 1 whenever DRA.BIN came from the CD (0 and 2 mean
// the dev-kit "sim:" PC loader) and DRA never writes it, so every PC-file
// branch folds away.
#define g_UseDisk 1
#endif

#endif
