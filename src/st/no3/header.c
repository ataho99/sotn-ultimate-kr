// SPDX-License-Identifier: AGPL-3.0-or-later
#include "no3.h"
#include "../pfn_entity_update.h"

extern RoomHeader rooms[];
extern SpriteParts* spriteBanks[];
extern u_long* cluts[];
extern RoomDef rooms_layers[];
extern GfxBank* gfxBanks[];
#if defined(MOD_SATURN_CASTLE)
#include "../../../mods/sotn-ultimate/src/castle/no3/header_mod_shaft.h"
#else
#define MOD_ROOMS rooms
#define MOD_ROOMS_LAYERS rooms_layers
#define MOD_SPRITE_BANKS spriteBanks
#define MOD_CLUTS cluts
#define MOD_GFX_BANKS gfxBanks
#define MOD_INIT_ROOM_ENTITIES InitRoomEntities
#endif

AbbreviatedOverlay g_Overlay = {
    .Update = Update,
    .HitDetection = HitDetection,
    .UpdateRoomPosition = UpdateRoomPosition,
    .InitRoomEntities = MOD_INIT_ROOM_ENTITIES,
    .rooms = MOD_ROOMS,
    .spriteBanks = MOD_SPRITE_BANKS,
    .cluts = MOD_CLUTS,
    .objLayoutHorizontal = &OBJ_LAYOUT_HORIZONTAL,
    .tileLayers = MOD_ROOMS_LAYERS,
    .gfxBanks = MOD_GFX_BANKS,
    .UpdateStageEntities = UpdateStageEntities,
};
