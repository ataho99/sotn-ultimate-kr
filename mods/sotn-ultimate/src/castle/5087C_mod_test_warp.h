// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate: included by src/dra/5087C.c (at file scope, in place)
// Test aid (include/game.h): the Library card walks the game's dialogue
// scenes in story order, to hear the dubs point to point. Each stop lands
// just before its scene with the story state the scene needs (its "seen"
// flag cleared). The good route first (Richter freed with the Holy Glasses,
// inverted castle, Dracula), then optionally the Richter-killed ending.
// test_checklist.md "Dialogue warp chain" lists what to do at each stop.
void AddToInventory(u16 id, EquipKind kind);
typedef struct {
    u8 stageId;
    u8 teleport; // D_800A245C index (the arrival, as in retail)
    u8 clear;    // a castle flag the stop clears (0 = none; CEN_OPEN is 0
                 // but is only ever set)
} ModDlgStop;
#if defined(MOD_TEST_AREA_NAMES)
// the areas with a name banner, entered the way a first visit does
static const ModDlgStop g_ModDlgStops[] = {
    // (one per ending: the test chain's last use, 2026-11-13)
    {STAGE_RBO6, 0x08}, // Dracula, 1847+ rooms: good ending 3, Maria follows
    {STAGE_RBO6, 0x08}, // Dracula, fewer rooms: good ending 5, Maria stays
    {STAGE_BO6, 0x65},  // Castle Keep, no glasses, met Maria: bad ending 2
    {STAGE_BO6, 0x65},  // Castle Keep, no glasses, never met her: bad end 1
};
// their banners' "seen" flags (src/st/*: STAGE_FLAG; the mod's 0xF0 /
// 0xF8)
static const u16 g_ModNameFlags[] = {
    NO0_STAGE_FLAG, NO1_STAGE_FLAG, NO2_STAGE_FLAG, CAT_STAGE_FLAG,
    CHI_STAGE_FLAG, DAI_STAGE_FLAG, LIB_STAGE_FLAG, NZ0_STAGE_FLAG,
    TOP_STAGE_FLAG, NZ1_STAGE_FLAG, ARE_STAGE_FLAG, NO4_STAGE_FLAG,
    DRE_STAGE_FLAG, 0xF0,           0xF8,
};
#else
static const ModDlgStop g_ModDlgStops[] = {
    // 1 Marble Gallery clock room: Maria
    {STAGE_MAR, 0x64, 0},
    // 2 Alchemy Lab: Maria again
    {STAGE_NZ0, MOD_TP_DLG_NZ0, MET_MARIA_IN_NZ0},
    // 3 Long Library: Master Librarian
    {STAGE_LIB, 0x27, MET_LIBRARIAN},
    // 4 Caverns: ferryman, 1st
    {STAGE_NO4, MOD_TP_DLG_FERRY1, MET_FERRYMAN_1},
    // 5 Caverns: ferryman, 2nd
    {STAGE_NO4, MOD_TP_DLG_FERRY2, MET_FERRYMAN_2},
    // 6 Royal Chapel: Hippogryph, Maria
    {STAGE_BO5, 0x68, MET_MARIA_AFTER_HIPPOGRYPH},
    // 7 Colosseum: Richter, M+W
    {STAGE_BO2, 0x72, RICHTER_CS_AFTER_M_AND_W},
    // 8 Royal Chapel: Maria on Richter
    {STAGE_DAI, MOD_TP_DLG_DAI, MET_MARIA_IN_DAI},
    // 9 Nightmare: Lisa / Succubus
    {STAGE_DRE, 0x33, SUCCUBUS_CS},
    // 10 Castle Center: Maria, glasses
    {STAGE_CEN, 0x05, MET_MARIA_IN_CEN},
    // 11 Castle Keep: Richter, orb, Shaft
    {STAGE_BO6, 0x65, SHAFT_ORB_DEFEATED},
    // 12 Cave (inverted): Death
    {STAGE_RBO2, 0x5B, DEATH_FIGHT_CS},
    // 13 Reverse Center: Shaft
    {STAGE_RCEN, 0x05, SHAFT_FIGHT_CS_UNK1},
    // 14 Dracula + good ending
    {STAGE_RBO6, 0x08, 0},
    // 15 Castle Keep, no glasses: bad end
    {STAGE_BO6, 0x65, SHAFT_ORB_DEFEATED},
};
#endif
// the last stop warped to, 0xFF = none yet (initialised: DRA's BSS layout
// must not change)
static u8 g_ModDlgStop = 0xFF;

static void ModDlgHead(u32 item) {
    u32* eq = g_Status.equipment;
    u32 old = eq[HEAD_SLOT];

    // (the same item: taken from the inventory and given back, no change)
    if (item != ITEM_EMPTY_HEAD) {
        if (!g_Status.equipBodyCount[item]) {
            return;
        }
        g_Status.equipBodyCount[item]--;
    }
    eq[HEAD_SLOT] = item;
    if (old != ITEM_EMPTY_HEAD) {
        AddToInventory(old, EQUIP_HEAD);
    }
    make_all();
}

// picks the next stop (L2 held: the same one, R2 held: the previous one),
// sets its story state and returns its arrival teleport; g_StageId becomes
// the destination
static s32 ModDlgWarp(void) {
#if defined(MOD_TEST_AREA_NAMES)
    s32 n = LEN(g_ModDlgStops);
#else
    const s32 n = LEN(g_ModDlgStops);
#endif
    s32 stop = g_ModDlgStop;
    u8* cf = g_CastleFlags;

    if (stop < n && (g_pads[0].pressed & PAD_L2)) {
    } else if (stop < n && (g_pads[0].pressed & PAD_R2)) {
        stop = stop ? stop - 1 : n - 1;
    } else {
        stop = stop + 1 >= n ? 0 : stop + 1;
    }
    g_ModDlgStop = stop;

#if defined(MOD_TEST_AREA_NAMES)
    for (n = 0; n < LEN(g_ModNameFlags); n++) {
        cf[g_ModNameFlags[n]] = 0;
    }
    // the endings: Dracula's picks 3 / 5 by the room count (RBO6: below
    // 0x737, 5), the keep's 2 / 1 by MET_MARIA_IN_CEN (BO6). RAM only: the
    // ending marks the card's save from the card's own copy
    if (stop < 2) {
        cf[INVERTED_CASTLE_UNLOCKED] = 1;
        if (stop == 0) {
            if (g_RoomCount < 0x737) {
                g_RoomCount = 0x737;
            }
        } else if (g_RoomCount >= 0x737) {
            g_RoomCount = 0x736;
        }
    } else {
        cf[SHAFT_ORB_DEFEATED] = 0;
        cf[INVERTED_CASTLE_UNLOCKED] = 0;
        cf[MET_MARIA_IN_CEN] = stop == 2;
        ModDlgHead(ITEM_EMPTY_HEAD);
    }
#else
    // the inverted castle: open for stops 12-14, shut for 10, 11 and 15
    if (stop >= 9) {
        cf[INVERTED_CASTLE_UNLOCKED] = stop >= 11 && stop <= 13;
    }
    if (g_ModDlgStops[stop].clear) {
        cf[g_ModDlgStops[stop].clear] = 0;
    }
    if (stop == 4) {
        // the east lake's ferryman only comes with the Merman Statue
        g_Status.relics[RELIC_MERMAN_STATUE] |=
            RELIC_FLAG_FOUND | RELIC_FLAG_ACTIVE;
    } else if (stop == 9) {
        cf[CEN_OPEN] = 1;
    } else if (stop == 10 || stop == 14) {
        // stop 11 with the Holy Glasses on (orb -> Shaft -> Richter freed
        // -> Maria + Richter in the keep), stop 15 without (Richter dies)
        ModDlgHead(stop == 10 ? ITEM_HOLY_GLASSES : ITEM_EMPTY_HEAD);
    } else if (stop == 12) {
        cf[SHAFT_FIGHT_CS_UNK2] = 0;
    }
#endif
    g_StageId = g_ModDlgStops[stop].stageId;
    return g_ModDlgStops[stop].teleport;
}
