// SPDX-License-Identifier: AGPL-3.0-or-later
// Saturn port: the third hand ("Holder", ホルダー; the Saturn's equipment[2]),
// Saturn balance and Alucard only (include/game.h MOD_HOLDER_ON). Its item
// id is kept in g_Status.saveName[10] (MOD_HOLDER_ITEM). Included by
// 7879C.c; the equip menu row and the 8th button config row are in menu.c,
// the button check in 6FDF8.c func_8010FDF8.
// - ModHolderAllows: the Saturn's holder filter (GAME func_060737A0): food or
//   medicine that needs no weapon overlay.
// - ModHolderUse: Saturn func_060A67B8 / func_060A66FC, the food and
//   medicine part of the hand path (6E42C.c func_8010EDB8) with the holder.
// - EntityModHolderFood: the Saturn moved its food weapon into ALUCARD.PRG
//   (entity 0x48, func_060B25F4) so the holder needs no weapon slot; this is
//   the PS1 w_021 food (EntityWeaponAttack + func_ptr_80170004, the Meal
//   ticket's prize) moved into DRA the same way. w_021's food sheet is only
//   in VRAM while a hand holds food, so it is drawn from the food's equip
//   icon (LoadEquipIcon slots, tpage 0x1A).

void func_8010E42C(u32 substep);
void SetPlayerAnim(u8 anim);
void AddToInventory(u16 id, EquipKind kind);

#define MOD_HOLDER_ICON 0x17       // its menu row's icon slot (and the food)
#define MOD_HOLDER_PRIZE_ICON 0x1E // the Meal ticket's food
#define MOD_FOOD_PEANUTS 6         // food sprite numbers (equip unk14)
#define MOD_FOOD_TOADSTOOL 7
#define MOD_FOOD_MEAL_TICKET 41

bool ModHolderAllows(s32 id) {
    Equipment* e;

    if (!id) {
        return true;
    }
    if (id >= HAND_ITEMS_CAP || id == ITEM_LIFE_APPLE || id == ITEM_HAMMER ||
        id == ITEM_SIGNED_PENCIL_BOARD) {
        return false;
    }
    e = &g_EquipDefs[id];
    // food runs in EntityModHolderFood here (the Saturn's food records have
    // weaponId 0xFF; the PS1's still name w_021)
    if (e->itemCategory == ITEM_FOOD) {
        return true;
    }
    return e->itemCategory == ITEM_MEDICINE && e->weaponId == 0xFF;
}

s32 ModHolderOn(void) { return MOD_HOLDER_ON(); }

// the holder's item, empty if the stored id isn't one it takes
s32 ModHolderItem(void) {
    s32 id = MOD_HOLDER_ITEM;

    return ModHolderAllows(id) ? id : 0;
}

// The use and the food entity: src/items/holder_code.c, in the Alucard-only
// code after his sprites (tools/build_sysopt.sh); these call it through the
// table there (include/game.h MOD_HOLDER_TBL), whose magic is only there in
// Alucard games (it is never called otherwise: the third hand is his only)
#define MOD_HOLDER_CALLS() (MOD_HOLDER_TBL[0] == MOD_HOLDER_MAGIC)
s32 ModHolderUse(void) {
    if (!MOD_HOLDER_CALLS()) {
        return false;
    }
    return ((s32(*)(void))MOD_HOLDER_TBL[1])();
}
void EntityModHolderFood(Entity* self) {
    if (!MOD_HOLDER_CALLS()) {
        DestroyEntity(self);
        return;
    }
    ((void (*)(Entity*))MOD_HOLDER_TBL[2])(self);
}
