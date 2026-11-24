// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate: included by src/dra/game_handlers.c (at file scope, in place)
// Every room of both castles for Richter and both Marias (user, 2026-11-14),
// Ultimate balance only (Classic: the retail castle; user, 2026-11-17).
// Each frame of play (not the prologue, where Richter plays before an Alucard
// game), for any character but Alucard, what Alucard opens with relics,
// familiars or forms is open:
// - the Merman Statue on (the Caverns' east ferryman, src/st/no4/e_ferryman.c)
// - the Abandoned Mine's demon-switch walls, both castles (they read the flag
//   when the room loads), the Catacombs spike-room light and the Castle
//   Center (the clock room's floor; src/st/cen keeps Maria and her scene out
//   for them, cen.h CEN_MARIA_DONE)
// - the mist grates (tile collision 0xF8, mist form only; Arena, Outer Wall,
//   Library and Royal Chapel in both castles) taken out of each room as it
//   loads, for a few frames after the room changes (the tiles may arrive a
//   frame after the bounds). Each grate tile takes the tile ModGrateFill
//   gives, so no hole is left (the Outer Walls' grille itself is a background
//   block sprite, removed in src/st/no1 and rno1 e_room_bg.c)
// - the Caverns ferryman silent (his scene skips its talk once
//   MET_FERRYMAN_1/2 are set, and still starts the ferry)
// The spikes break for them as with the Spike Breaker (src/st/e_spikes.h).
// where a grate tile is filled from, by stage (checked in the rooms, live):
// the Outer Walls' dark doorway beside it, the Library's and the Arena's
// corridor wall (its tiles repeat every 2); 0 = emptied (the Royal Chapels,
// whose bars sit between spikes and empty tiles)
static s32 ModGrateFill(void) {
    switch (g_StageId) {
    case STAGE_NO1:
        return -1;
    case STAGE_RNO1:
        return 1;
    case STAGE_LIB:
    case STAGE_ARE:
        return 2;
    case STAGE_RLIB:
    case STAGE_RARE:
        return -2;
    }
    return 0;
}

static const u8 g_ModClassicBoss[] = {
    TIMEATTACK_EVENT_SUCCUBUS_DEFEAT, TIMEATTACK_EVENT_MINOTAUR_WEREWOLF_DEFEAT,
    TIMEATTACK_EVENT_DEATH_DEFEAT};

static void ModCastleAccess(void) {
    u16* fg;
    u8* coll;
    s32 i;
    s32 n;
    s32 side;
    u32 room;

    if (g_PlayableCharacter == PLAYER_ALUCARD || g_StageId == STAGE_ST0) {
        return;
    }
    if (MOD_PS1_BALANCE()) {
        // Classic: retail Richter mode's marks, the Succubus, Minotaur &
        // Werewolf and Death done (a new game zeroed them; a beaten one's
        // record stays)
        for (i = 0; i < LEN(g_ModClassicBoss); i++) {
            if (!g_Settings.timeAttackRecords[g_ModClassicBoss[i]]) {
                g_Settings.timeAttackRecords[g_ModClassicBoss[i]] = 1;
            }
        }
        return;
    }
    g_Status.relics[RELIC_MERMAN_STATUE] |=
        RELIC_FLAG_FOUND | RELIC_FLAG_ACTIVE;
    g_CastleFlags[CEN_OPEN] = 1;
    g_CastleFlags[CHI_DEMON_SWITCH] = 1;
    g_CastleFlags[RCHI_DEMON_SWITCH] = 1;
    g_CastleFlags[CAT_SPIKE_ROOM_LIT] = 1;
    g_CastleFlags[MET_FERRYMAN_1] = 1;
    g_CastleFlags[MET_FERRYMAN_2] = 1;
    room =
        g_StageId << 16 | (g_Tilemap.left & 0xFF) << 8 | (g_Tilemap.top & 0xFF);
    // (in g_ModDub: no new DRA BSS, see include/game_mod.h)
    if (room != g_ModDub.accessRoom) {
        g_ModDub.accessRoom = room;
        g_ModDub.accessScan = 8;
    }
    if (g_ModDub.accessScan <= 0 || g_ModDub.accessScan > 8 || !g_Tilemap.fg ||
        !g_Tilemap.tileDef) {
        return;
    }
    g_ModDub.accessScan--;
    fg = g_Tilemap.fg;
    coll = g_Tilemap.tileDef->collision;
    n = g_Tilemap.hSize * g_Tilemap.vSize * 256;
    for (i = 3; i < n - 3; i++) {
        if (coll[fg[i]] == 0xF8) {
            // (kept for a switch to Classic: src/menu/charmenu.c)
            if (g_ModDub.grateStage != (MOD_GRATE_MAGIC | g_StageId)) {
                g_ModDub.grateStage = MOD_GRATE_MAGIC | g_StageId;
                g_ModDub.grateCount = 0;
            }
            if (g_ModDub.grateCount < MOD_GRATE_MAX) {
                g_ModDub.grate[g_ModDub.grateCount].p = &fg[i];
                g_ModDub.grate[g_ModDub.grateCount++].v = fg[i];
            }
            side = ModGrateFill();
            fg[i] = side ? fg[i + side] : 0;
        }
    }
}
