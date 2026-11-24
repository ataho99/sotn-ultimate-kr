// SPDX-License-Identifier: AGPL-3.0-or-later
#include <stage.h>
#include <sfx.h>

#ifndef SFX_BREAKABLE_HIT
#define SFX_BREAKABLE_HIT SFX_CANDLE_HIT
#endif

extern EInit g_EInitBreakable;

void CreateEntityFromCurrentEntity(u16 entityId, Entity* entity);
void ReplaceBreakableWithItemDrop(Entity*);
Entity* AllocEntity(Entity* start, Entity* end);

void EntityBreakable(Entity* entity) {
    u16 breakableType = entity->params >> 12;
    if (entity->step) {
        AnimateEntity(g_eBreakableAnimations[breakableType], entity);
#if defined(MOD_SATURN) && defined(MOD_SATURN_CANDLES)
        ModCandleGlowUpdate(entity);
#endif
        if (entity->hitParams) { // If the candle is destroyed
            Entity* entityDropItem;
#if defined(MOD_SATURN) && defined(MOD_SATURN_CANDLES)
            g_api.PlaySfx(g_ModBreakableSfx[breakableType]);
#elif defined(STAGE_IS_TEK)
            if (breakableType == 1) {
                g_api.PlaySfx(SFX_BREAKABLE_HIT_ALT);
            } else {
                g_api.PlaySfx(SFX_BREAKABLE_HIT);
            }
#else
            g_api.PlaySfx(SFX_BREAKABLE_HIT);
#endif
#if defined(MOD_SATURN) && defined(MOD_SATURN_CANDLES)
            // the entity becomes the drop, which doesn't own the glow
            ModCandleGlowFree(entity);
            // Saturn port: the stand stays on the wall (Saturn entity 0x12;
            // mods/sotn-ultimate/src/castle/mod_candle.h)
            if (g_ModCandleRemnantFrames[breakableType]) {
                entityDropItem =
                    AllocEntity(&g_Entities[224], &g_Entities[256]);
                if (entityDropItem != NULL) {
                    CreateEntityFromCurrentEntity(
                        E_MOD_CANDLE_REMNANT, entityDropItem);
#ifdef MOD_CANDLE_REMNANT_PFN
                    // a stage whose entity table can't grow (retail data)
                    entityDropItem->pfnUpdate = MOD_CANDLE_REMNANT_PFN;
#endif
                    entityDropItem->params = entity->params;
                }
            }
            // the Saturn spawns the explosion and the drop on the hitbox
            // (STAGE_16 0x060DC1A8): an item holder's drop at its foot
            // would start inside the floor, where EntityEquipItemDrop
            // destroys itself
            entity->posY.i.hi += g_ModBreakableHitboxOffY[breakableType];
            // the candle becomes the drop: drawn no more (it would show its
            // last frame for a moment at the raised position)
            entity->animCurFrame = 0;
            entityDropItem = AllocEntity(&g_Entities[224], &g_Entities[256]);
            if (entityDropItem != NULL) {
                CreateEntityFromCurrentEntity(E_EXPLOSION, entityDropItem);
                entityDropItem->params =
                    g_eBreakableExplosionTypes[breakableType];
            }
            // Saturn port: the item candle (Garden type 1, STAGE_15
            // 0x060DC2A0; Prison type 2) releases one of the stage's items (a
            // persistent drop, params & 0x1FF) instead of a random prize, and
            // doesn't come back
            if (breakableType == MOD_ITEM_CANDLE_TYPE) {
                entityDropItem =
                    AllocEntity(&g_Entities[160], &g_Entities[192]);
                if (entityDropItem != NULL) {
                    CreateEntityFromCurrentEntity(
                        E_PERSISTENT_ITEM_DROP, entityDropItem);
                    entityDropItem->params = entity->params & 0x1FF;
                }
                PreventEntityFromRespawning(entity);
                DestroyEntity(entity);
                return;
            }
#else
            entityDropItem = AllocEntity(&g_Entities[224], &g_Entities[256]);
            if (entityDropItem != NULL) {
                CreateEntityFromCurrentEntity(E_EXPLOSION, entityDropItem);
                entityDropItem->params =
                    g_eBreakableExplosionTypes[breakableType];
            }
#endif
            ReplaceBreakableWithItemDrop(entity);
        }
    } else {
        InitializeEntity(g_EInitBreakable);
#if defined(STAGE_IS_GDN) || defined(STAGE_IS_RGDN) ||                         \
    defined(STAGE_IS_PRS) || defined(STAGE_IS_RPRS)
        // Saturn port stages: the scenery under every enemy part (their
        // torches covered the Hell Garden's Venus Weed, which draws down to
        // 0x7E), above the decorations (0x60-0x6A); user, 2026-11-13
        entity->zPriority = g_unkGraphicsStruct.g_zEntityCenter - 0x20;
#else
        entity->zPriority = g_unkGraphicsStruct.g_zEntityCenter - 20;
#endif
        entity->blendMode = blend_modes[breakableType];
        entity->hitboxHeight = g_eBreakableHitboxes[breakableType];
        entity->animSet = g_eBreakableanimSets[breakableType];
#if defined(MOD_SATURN) && defined(MOD_SATURN_CANDLES)
        // the Saturn's hitbox sits on the flame, above the entity (its
        // candle configs' hitboxOffY)
        entity->hitboxOffY = g_ModBreakableHitboxOffY[breakableType];
        ModCandleGlowInit(entity);
#endif
#ifdef STAGE_IS_BO5
        entity->hitboxOffX = 8;
        entity->hitboxOffY = 8;
        if (!breakableType) {
            entity->hitboxWidth = 4;
            entity->hitboxOffX = 2;
            entity->hitboxOffY = 2;
        }
#endif
    }
}
