// SPDX-License-Identifier: AGPL-3.0-or-later
#include "lib.h"
#if defined(MOD_KO)
#include "gen/ko_shop_str.h" /* gen_ko_shop.py: the shop's label words, document names and descriptions */
#endif
/* the LIB overlay link has no DrawSync symbol (undefined_syms_auto only lists what the retail asm calls);
 * main is at a fixed address, call it directly (same address as symbols.us.txt DrawSync). */
#define DrawSync(m) (((s32 (*)(s32))0x8001290C)(m))

/* MOD_KO: enemy names come from g_api.enemyDefs (DRA g_EnemyDefs, gen_ko_names.py merged the
 * old ko_lib_names.h list into it, FF 00 terminated as func_us_801B1064 expects). ko_lib_names.h
 * stays as generator input only. */

// An inventory item consists of a category, which affects
// how the other fields are interpretted, an "unlock level",
// which is related to the number of things which have been
// accomplished in the castle (beating the game unlocks all),
// an item ID which corresponds to known item IDs, except for
// Jewel of Open, and the final item, which doesn't follow
// the pattern and doesn't appear in the shop
typedef struct {
    /* 0x0 */ u8 category;
    /* 0x1 */ u8 unlockLevel;
    /* 0x2 */ u16 itemId;
    /* 0x4 */ u32 price;
} InventoryItem;

typedef struct {
    /* 0x0 */ u16 category;
    /* 0x2 */ u16 itemId;
    /* 0x4 */ u32 price;
} ShopItem;

typedef enum {
    DRACULA_TACTICS,
    OLROX_TACTICS,
    DOPPLEGANGER10_TACTICS,
    GRANFALOON_TACTICS,
    MINOTAUR_WEREWOLF_TACTICS,
    SCYLLA_TACTICS,
    SLO_GAI_TACTICS,
    HIPPOGRYPH_TACTICS,
    BEELZEBUB_TACTICS,
    SUCCUBUS_TACTICS,
    KARASUMAN_TACTICS,
    TREVOR_GRANT_SYPHA_TACTICS,
    DEATH_TACTICS,
    CERBERUS_TACTICS,
    RICHTER_TACTICS,
    MEDUSA_TACTICS,
    THE_CREATURE_TACTICS,
    LESSER_DEMON_TACTICS,
    DOPPLEGANGER40_TACTICS,
    AKMODAN_II_TACTICS,
    DARKWING_BAT_TACTICS,
    GALAMOTH_TACTICS,
    MARIA_TACTICS = 0x1A,
    SHAFT_TACTICS,
    LORD_DRACULA_TACTICS,
} SHOP_TACTICS;

/// "documents" in the shop have their own
/// item index, separate from other items.
typedef enum {
    DOC_CASTLE_MAP,
    DOC_MAGIC_SCROLL_1,
    DOC_MAGIC_SCROLL_2,
    DOC_MAGIC_SCROLL_3,
    DOC_MAGIC_SCROLL_4,
    DOC_MAGIC_SCROLL_5,
    DOC_MAGIC_SCROLL_6,
    DOC_MAGIC_SCROLL_7,
    DOC_MAGIC_SCROLL_8,
} SHOP_DOCUMENTS;

/// the first 5 inventory categories are the same as
/// `EquipKind`. `EQUIP_RELIC` and `EQUIP_DOCUMENT` are
/// special and only used for inventory so far.
typedef enum {
    INVENTORY_HAND,
    INVENTORY_HEAD,
    INVENTORY_BODY,
    INVENTORY_CAPE,
    INVENTORY_ACCESSORY,
    INVENTORY_RELIC,
    INVENTORY_DOCUMENT,
} InventoryCategory;

static u16 D_us_80180E90[] = {
    0x0A9, 0x046, 0x040, 0x180, 0x06E, 0x01B, 0x04B, 0x0AF, 0x069, 0x01D, 0x051,
    0x0F6, 0x00D, 0x0B2, 0x0C1, 0x00B, 0x09D, 0x0FE, 0x09C, 0x031, 0x0F3, 0x010,
    0x028, 0x12F, 0x07C, 0x066, 0x130, 0x061, 0x055, 0x006, 0x0E9, 0x06C, 0x0B7,
    0x0FD, 0x016, 0x043, 0x126, 0x053, 0x05D, 0x0F7, 0x080, 0x058, 0x009, 0x06A,
    0x022, 0x0B4, 0x092, 0x06B, 0x063, 0x0FB, 0x105, 0x073, 0x072, 0x09E, 0x044,
    0x030, 0x081, 0x076, 0x11F, 0x0E3, 0x0D6, 0x014, 0x088, 0x0E4, 0x029, 0x12C,
    0x0D3, 0x042, 0x07A, 0x03D, 0x06F, 0x0E1, 0x0EF, 0x0CB, 0x0CE, 0x02D, 0x085,
    0x10F, 0x115, 0x017, 0x083, 0x0DD, 0x0BE, 0x0BC, 0x0BA, 0x04D, 0x118, 0x041,
    0x047, 0x16B, 0x15E, 0x032, 0x156, 0x071, 0x0A1, 0x106, 0x142, 0x127, 0x143,
    0x108, 0x11D, 0x078, 0x0A5, 0x00E, 0x0B6, 0x074, 0x131, 0x185, 0x025, 0x111,
    0x090, 0x182, 0x07E, 0x08D, 0x02B, 0x094, 0x0A7, 0x0C3, 0x08A, 0x0D8, 0x097,
    0x099, 0x026, 0x04F, 0x01F, 0x0EC, 0x10B, 0x188, 0x174, 0x16E, 0x172, 0x14E,
    0x148, 0x11C, 0x151, 0x144, 0x0E0, 0x0D9, 0x0E5, 0x133, 0x068, 0x0C6, 0x18C,
    0x164, 0x15F, 0x17B};

#if defined(MOD_SATURN_ENEMIES)
#include "../../../mods/sotn-ultimate/src/enemies/e_shop_mod_bestiary.h"
#else
#define LIB_BIT(i) (i)
#define LIB_ENEMY(i) D_us_80180E90[i]
#define LIB_TOP_MAX 0x42
#endif

#if defined(MOD_KO) && defined(MOD_SATURN)
// Saturn bestiary descriptions (KO): rows 0..16 of the 18 new entries (bits 146..162,
// enemies 0x190..0x1A8) are read from the dead ARC_KO slot at 0x8009EAF0 (896 B: 17 LE
// pointers followed by the literals, 856 B in all). The other two (bit 163 = 0x1A9 and
// Maria's PSP text) are static literals here. Literal format as the KO rows of
// D_us_80180FB4: Hangul as \xF0..\xF3 escapes, full-width period as a source literal.
#define KO_LIBDESC_TBL ((const char* const*)0x8009EAF0)
#define KO_LIBDESC_SLOT_MAX 16
static const char g_ModLibKoPlace1A9[] = "\xF1\xF7\xF0\xFF \xF0\xCA\xF2\x30\xF0\xC6 \xF3\x36\xF2\x01 \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24@\xF0\x78\xF2\xFD\xF0\x85 \xF0\xF9\xF0\x5E\xF0\xC6 \xF3\x5D\xF0\xBD \xF1\xDD\xF0\x49．";
static const char g_ModLibKoPlaceMariaPsp[] = "\xF1\xE4\xF1\xAB\xF0\x7C \xF1\x2C \xF0\x52\xF0\xD2\xF1\xD2 \xF2\x3E\xF1\x14@\xF0\x50\xF2\x4E\xF2\x1E\xF0\xC6 \xF2\xF3\xF0\xBF \xF1\x8D\xF0\x92\xF0\x7D\xF0\x71 \xF2\x43\xF2\x1E．";
#endif
static const char* D_us_80180FB4[] = {
    #if defined(MOD_KO)
        "１７９２\xF2\x70 \xF0\x50\xF2\x4E\xF2\x1E\xF0\x40 \xF0\xB0\xF0\x92\xF3\x19\xF0\x49（５\xF2\x70 \xF1\xE4）．",
    #else
    "Destroyed by Richter in １７９２．（５ years ago）",
    #endif
    #if defined(MOD_KO)
        "\xF0\x94\xF3\x17\xF1\xD9 \xF2\x6B\xF0\xA0@\xF2\x80\xF0\xCF\xF0\x6C\xF2\x6A\xF0\x49．",
    #else
    "Revives due to blood soaked@bones．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xF1\xF3\x1B\xF1\x05 \xF0\x58\xF0\x83．@\xF0\x6D\xF0\x7C \xF0\xA3\xF0\xD2\xF0\x7E\xF0\xDC\xF0\xA0 \xF0\xC2\xF1\x63\xF2\xD3\xF1\x8B\xF0\xBF\xF0\x49．",
    #else
    "Vampire bats．@Feared as servants of evil．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x92\xF0\xBE \xF1\x9C\xF0\x40 \xF0\xEE\xF1\xE4 \xF0\xB9\xF0\xFE．",
    #else
    "Indestructible spinning skull．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xA7 \xF0\x69\xF0\x88\xF0\x74 \xF0\x52\xF1\x90\xF0\xDC\xF0\xA0@\xF1\xA9\xF1\x82\xF0\xB4 \xF0\x5C\xF0\xD7\xF0\x95\xF0\x49．",
    #else
    "Fresh corpses．@Magically animated．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xE4\xF1\xAB\xF0\x7C \xF0\xBE\xF0\x9E．@\xF2\x04\xF0\xDD\xF1\xDE\xF0\xBF \xF1\x83\xF0\xC2 \xF0\xA9\xF0\xB0\xF0\x95\xF0\x49．",
    #else
    "Legendary ””creature””．@Attacks all invaders．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x52\xF1\x90\xF0\xDC\xF0\xA0 \xF2\xD3\xF1\xF1\xF0\x7D\xF0\xBF \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．",
    #else
    "Magically animated skeleton．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x63 \xF0\xDD\xF0\x44\xF0\xC6 \xF0\x85\xF0\xE4\xF0\xBF \xF0\x48\xF0\x4A．",
    #else
    "Wolves which guard castle @entrance．",
    #endif
    #if defined(MOD_KO)
        "\xF2\x6A\xF2\x39\xF0\x95 \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24 \xF0\x43\xF0\xAB",
    #else
    "Ferocious skeleton @swordsman．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x9C\xF2\x7E\xF0\x7D \xF3\x48\xF0\xBF \xF1\x7B\xF1\x7A．@\xF1\x7B\xF1\x7A \xF0\xA6 \xF0\x40\xF0\x80 \xF0\x8C\xF0\x49．",
    #else
    "Fireball−spitting Merman．@Largest of merman kind．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x2C \xF0\xC9 \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．@\xF0\xC1\xF0\xB4 \xF3\x25\xF1\x6F\xF0\x49．",
    #else
    "４−legged skeleton．@Drips poison．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xA3\xF1\x0B \xF0\x70\xF0\x68 \xF0\x9F\xF0\x7D\xF0\x90．",
    #else
    "Low level axe knight．",
    #endif
    #if defined(MOD_KO)
        "３００\xF2\x70 \xF1\xE4 \xF0\x92\xF2\x8D\xF1\x42 \xF0\x9C\xF0\xCA\xF2\x5E\xF0\xEE@\xF1\x98\xF0\xAB",
    #else
    "Eastern Church soldiers @dispatched ３００ years ago．",
    #endif
    #if defined(MOD_KO)
        "\xF2\x6B\xF0\xC6 \xF1\x3D\xF0\x85\xF0\xBF \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．",
    #else
    "Bone−hurling skeleton．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x05\xF0\x6A \xF1\x47\xF1\x6F \xF1\xD8\xF1\xDE \xF0\x8E\xF0\x7D\xF1\x9F．",
    #else
    "Possessed Ouija table．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x5C\xF0\x54\xF1\x58 \xF1\xBF\xF0\xBF \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．",
    #else
    "Boomerang wielding skeleton．",
    #endif
    #if defined(MOD_KO)
        "\xF2\xDD\xF0\x7D \xF1\x81\xF1\x41\xF1\x1D\xF1\xFA\xF1\x60 \xF1\x1D\xF2\x06\xF0\xA0\xF0\xDB \xF1\xB8\xF0\x9E",
    #else
    "Plant with razor−sharp @leaves．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x9C \xF3\x48\xF0\xBD \xF0\x9F\xF0\xBF \xF1\x3F\xF0\x55．",
    #else
    "Fire−breathing flying demon．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xA3\xF1\x0B \xF1\x9C\xF1\x15 \xF0\x7B\xF0\x4D．",
    #else
    "Low level flaming ghosts．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x85\xF2\xCD\xF0\x7C \xF2\x65\xF0\xC2\xF0\xF2\xF0\x69 \xF1\x3F\xF0\x55．",
    #else
    "Demonic puppet from Hell．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x52\xF1\xF7\xF0\xB4 \xF1\x46 \xF1\x3F\xF0\x55",
    #else
    "Demon with enchanted spear．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xC2 \xF1\x7B\xF0\x50 \xF1\xCC\xF2\x81\xF0\xDB@\xF1\xC5\xF0\xD2． \xF0\x71\xF2\x74 \xF2\xF2\xF0\x7D \xF1\x7B\xF0\x50？",
    #else
    "Nauseating two−headed @beast． Which is the head？",
    #endif
    #if defined(MOD_KO)
        "\xF0\x46\xF2\xA4\xF0\x95 \xF0\x7E\xF0\xAE\xF2\x46 \xF1\xA9\xF0\x9E．@\xF1\x95\xF3\x0A\xF1\xFA\xF1\x60 \xF0\x20\xF0\x49．",
    #else
    "Strange human−like creature．@Jumps like a flea．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x54\xF0\xC2\xF0\xAB \xF1\x7B\xF0\x50\xF2\x06\xF2\x8E\xF0\x74\xF0\x61 \xF0\xA2\xF0\x71\xF2\x6A\xF0\x49．",
    #else
    "From Medusa’s hair．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xBB\xF1\x1D \xF0\x43\xF0\x67\xF0\x7C \xF0\xB7\xF0\x6B\xF1\xDE",
    #else
    "Novice of twin−bladed @combat．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x65\xF2\xFA \xF1\x46@\xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．",
    #else
    "Skeletons equipped with @rifles．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x54\xF0\xC2\xF0\xAB\xF0\x7C \xF1\x7B\xF0\x50\xF2\x06\xF2\x8E．@\xF0\x62\xF0\x99\xF0\x69\xF2\x17\xF0\x49．",
    #else
    "From Medusa’s hair．@Can petrify．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xFD\xF0\x44 \xF1\x46 \xF0\xA6\xF0\x80 \xF0\x9F\xF0\x7D\xF0\x90．",
    #else
    "Heavily armored knight with @iron ball．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xA1\xF0\x7D \xF1\xB8\xF0\x9E． \xF1\xC1\xF2\xC6\xF0\x74@\xF0\x62\xF0\x99 \xF0\xEF\xF0\xFF\xF0\x40 \xF1\xDD\xF0\x49．",
    #else
    "Mutated plant．@Seeds petrify．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xA6\xF0\xC7\xF0\x80 \xF0\x70\xF0\x68 \xF0\x9F\xF0\x7D\xF0\x90．",
    #else
    "Well armored axe knight．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x9C\xF0\xFF \xF0\x73\xF0\xAC\xF0\x7C \xF1\x3F\xF0\x55．@\xF2\xC3\xF0\xDA\xF0\x46\xF0\xC6 \xF3\x4C\xF0\x71\xF0\x95\xF0\x49．",
    #else
    "Fire and ice demon．@Seems reluctant to fight．",
    #endif
    #if defined(MOD_KO)
        "\xF3\x1E \xF1\x46 \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24 \xF2\x9C\xF0\xD2．",
    #else
    "Skeleton archer equipped @with bow．",
    #endif
#if defined(MOD_KO)
#if defined(MOD_SATURN)
    "\xF0\xB2\xF1\x59\xF0\xFD \xF0\xB9\xF0\xFE\xF0\xA0 \xF1\x74\xF1\x46 \xF0\x46\xF1\x44",
#else
    "\xF0\x5A\xF1\xA8 \xF0\xB2\xF1\x59\xF0\xFD \xF0\xB9\xF0\xFE@\xF0\x4A\xF0\x93．",
#endif
#else
#if defined(MOD_SATURN)
    // the PSP port's text (mods/sotn-ultimate/tools/gen_menu_text.py lists the
    // menu changes)
    "Pillar built from bleached@skull of dragon．",
#else
    "Cannon built of white @dragon skulls．",
#endif
#endif
    #if defined(MOD_KO)
        "\xF0\xA1\xF0\x6A\xF0\xA3\xF0\xBF \xF1\x3F\xF0\x55．",
    #else
    "Shape−shifting demon．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x63\xF0\x7C \xF0\x9A\xF0\x74 \xF0\xA1\xF0\x7D\xF1\x42 \xF1\xCF\xF2\xB4\xF0\x57．",
    #else
    "Local barnyard owl Mutated @by castle’s influence．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xFA\xF2\x46\xF1\x42 \xF1\x98\xF0\xAB\xF0\x7C \xF1\x7B\xF0\x50． \xF0\xCD\xF0\xD2\xF0\xC6@\xF1\x2E\xF1\x6F\xF0\x49．",
    #else
    "Executed soldier’s head．@Hunting for vengeance．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x68\xF0\xE5\xF0\x4B\xF0\x74 \xF3\x27\xF0\x71\xF0\xAB\xF0\xBF \xF1\x8D．",
    #else
    "Snake living off Scylla．",
    #endif
    #if defined(MOD_KO)
        "\xF2\xD3\xF1\xF1\xF0\x7D\xF0\xBF \xF1\xD6\xF3\x4B\xF0\x7D \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．@\xF0\x9F\xF0\xC7\xF0\xE9\xF0\xB4 \xF1\x3D\xF0\xB6\xF0\x49．",
    #else
    "Animated ape skeleton．@Hurls wooden barrels．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x9F\xF0\x46\xF0\x9F\xF0\x8D \xF1\x46 \xF0\xC7\xF0\xAB．",
    #else
    "Armored warrior with @naginata．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xB7\xF0\xA5\xF1\xDE\xF1\xD5 \xF0\x52\xF1\x41\xF0\x61",
    #else
    "Book of beginner’s spells．",
    #endif
    #if defined(MOD_KO)
        "\xF2\x74\xF0\x50\xF0\xBC \xF0\x9F\xF0\xBF \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24 \xF1\xE4\xF0\xAB．",
    #else
    "Slow−flying skeleton warrior．",
    #endif
    #if defined(MOD_KO)
        "\xF2\xD9\xF0\x88 \xF0\xD6\xF0\xBF \xF0\xBD\xF0\xE9\xF1\x8B\xF0\xBF \xF0\x76\xF0\x97\xF0\x7C@\xF1\xB9\xF0\x88．",
    #else
    "Physical manifestations of @bodiless tormented souls．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x4A\xF0\x43 \xF1\x46 \xF0\x9F\xF0\x7D\xF0\x90．",
    #else
    "Knight armed with @Greatsword．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x6D\xF0\x52\xF0\x7C \xF1\xAE\xF3\x04\xF0\xA0 \xF0\xF6\xF0\x4A\xF0\xB9\xF0\xB6 \xF0\xC2\xF2\x63\xF0\x5E",
    #else
    "Toad magically enlarged by @demonic baptism．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xA6\xF0\xC7\xF0\x80 \xF0\x72\xF1\x3F\xF0\xB2@\xF1\xE4\xF0\xAB．",
    #else
    "Heavily armored undead @warrior．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xF4\xF1\xD1 \xF0\xF9\xF0\x5E．@\xF2\x44\xF0\x85\xF0\x74\xF0\x61 \xF0\xAF\xF0\xA3\xF0\x49．",
    #else
    "Armored guard．@Strong in perilous areas． ",
    #endif
    #if defined(MOD_KO)
        "\xF0\x4C\xF0\x7D\xF0\x94\xF0\x71 \xF1\xBF\xF0\xBF \xF1\x84@\xF0\xD6\xF0\xBF \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．",
    #else
    "Headless skeleton．@Wields a rapier．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x9C\xF2\x93\xF0\x74 \xF0\xAB\xF0\xBF \xF0\x42\xF0\x44\xF0\x50",
    #else
    "Cave−dwelling frog．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x73\xF0\xAC \xF1\xBF\xF0\xBF \xF0\xD7\xF1\xDE \xF0\x7B\xF0\x4D．",
    #else
    "Ice−wielding female spirit．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xBD\xF0\x4A \xF0\xC7\xF0\x46\xF0\x7C \xF0\x61．",
    #else
    "Book of ancient weapons．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xF6\xF0\x4A \xF0\xB9\xF0\xFE \xF0\x76\xF0\x97．",
    #else
    "Giant skull spirit．",
    #endif
    #if defined(MOD_KO)
        "\xF2\xD9\xF1\xB8\xF0\x63 \xF1\x11\xF0\x52\xF1\x05",
    #else
    "Flesh−eating raven．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xEC\xF1\xD9 \xF0\x85 \xF0\x77\xF1\x59\xF1\x42 \xF0\xEA\xF0\xC5@\xF1\x11\xF0\x52\xF1\x05 \xF0\x76\xF0\x97．",
    #else
    "Spirit of long−dead blue @raven．",
    #endif
    #if defined(MOD_KO)
        "\xF2\xDD\xF0\x7D \xF1\x81\xF1\x41\xF1\x1D\xF1\xFA\xF1\x60 \xF1\x1D\xF2\x06\xF0\xA0\xF0\xDB \xF1\xB8\xF0\x9E．",
    #else
    "Plant with razor−sharp @leaves．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xFD\xF2\x25 \xF1\x46 \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．",
    #else
    "Skeleton equipped with @spiked iron flail．",
    #endif
    #if defined(MOD_KO)
        "\xF3\x20\xF2\x96\xF1\x42 \xF1\x02\xF1\xD5 \xF0\xF6\xF1\xD8 \xF0\xE6 \xF2\x3D\xF0\x50 \xF1\x7A",
    #else
    "Flea Man riding on specially @trained war−goose．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xAB\xF0\x6D\xF0\x95 \xF0\x52\xF0\x43．@\xF3\x14\xF2\x1E\xF0\x40\xF0\x7D\xF0\x68\xF0\x90\xF0\xC6 \xF0\x7F\xF0\xDC\xF2\x17\xF0\x49．",
    #else
    "Evil demon−sword． Causes @poltergeist phenomena．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x9F\xF0\x46\xF0\x9F\xF0\x8D \xF1\x46 \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．",
    #else
    "Naginata−wielding skeleton．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x6A\xF0\x99\xF0\x7C \xF1\x8A\xF0\x49 \xF0\xBE\xF0\x9E．",
    #else
    "Mythological sea monster．",
    #endif
    #if defined(MOD_KO)
        "\xF2\x26\xF1\x82\xF0\x95 \xF0\xD7\xF0\xAB\xF1\x26\xF1\x17 \xF0\x76\xF0\x97",
    #else
    "Invisible huntress spirit．",
    #endif
    #if defined(MOD_KO)
        "\xF2\xD3\xF1\xF1\xF0\x7D\xF0\xBF \xF0\xB6\xF3\x26 \xF0\xFE\xF2\xF5．",
    #else
    "Animated mud golem．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xA6\xF0\xC7\xF0\x80 \xF0\x9F\xF0\x7D\xF0\x90．@\xF3\x20\xF2\x96\xF1\x42 \xF1\xCF\xF2\xB4\xF0\x57\xF1\xD2 \xF0\xA9\xF0\xB0\xF0\x95\xF0\x49．",
    #else
    "Heavily armored knight @Attacks with trained owl．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xAB\xF0\x6D\xF0\x95 \xF0\x52\xF0\x43．@\xF3\x14\xF2\x1E\xF0\x40\xF0\x7D\xF0\x68\xF0\x90\xF0\xC6 \xF0\x7F\xF0\xDC\xF2\x17\xF0\x49．",
    #else
    "Evil demon−sword． Causes @poltergeist phenomena．",
    #endif
    #if defined(MOD_KO)
        "\xF2\xF3\xF2\x81\xF0\x9F\xF1\x41 \xF0\x43\xF0\xB4 \xF3\x22\xF0\xC2\xF1\x6D\xF0\xBF \xF0\x65\xF0\xB2 \xF0\xA0\xF0\xB2",
    #else
    "Sword Lord wields sword @even when banished．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x41\xF1\x1C\xF1\xD2 \xF1\xB9\xF0\xB2\xF0\xA0@\xF0\xC7\xF0\x80\xF0\x95 \xF2\x3D\xF0\x50 \xF1\x7A．",
    #else
    "Flea Man armed with axe and@shield．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x76， \xF0\xAB\xF1\xDE， \xF0\xC1\xF0\xD2\xF0\x50\xF0\x7C \xF2\x3F\xF0\x63\xF0\xD2．",
    #else
    "Part horse， lion and eagle．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xF6\xF0\x4A \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．",
    #else
    "Giant skeleton．",
    #endif
    #if defined(MOD_KO)
        "\xF2\xE3\xF0\xA0 \xF1\x42 \xF0\x55\xF0\x68\xF2\x1E",
    #else
    "Monster made from gel．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xBB\xF1\x1D \xF1\xE4\xF2\x26\xF0\x7C@\xF1\x3A\xF0\x7E．",
    #else
    "Master of twin−bladed @combat．",
    #endif
    #if defined(MOD_KO)
        "\xF3\x11\xF0\x61 \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．",
    #else
    "Were panther skeleton．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xC7\xF2\x7D\xF0\x85\xF0\x46． \xF1\x7A\xF0\x66 \xF0\xB0\xF2\x26@\xF1\x3A\xF0\x7E．",
    #else
    "Grave Keeper．@Master of unarmed combat．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x9C\xF0\xB4 \xF2\xE7\xF0\x6C\xF0\xA3\xF0\xBF \xF1\x08\xF2\xF5\xF1\x6F",
    #else
    "Pyromaniacal gremlin．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xD7\xF0\x7E \xF1\x7B\xF0\x50\xF0\x74 \xF1\xA7 \xF2\xA3\xF0\x7C@\xF0\xBE\xF0\x9E．",
    #else
    "Creature with head of @woman and body of bird．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x65\xF1\x7B\xF0\x50 \xF0\xBE\xF0\x9E．",
    #else
    "Bull−headed monster．",
    #endif
    #if defined(MOD_KO)
        "\xF2\xC7\xF0\xF4\xF0\xA3\xF0\xBF \xF0\x4B\xF0\x7D\xF3\x02\xF0\x68\xF0\xA0\xF0\xA7．",
    #else
    "Shape−shifting lycanthrope．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x68\xF2\x0F\xF0\x4C\xF2\x24 \xF0\xAB\xF1\xE7\xF0\x40 \xF1\x46 \xF0\xB9\xF0\xFE \xF1\xE7\xF0\xC0",
    #else
    "Portable skull shrine carried @by skeleton priests．",
    #endif
    #if defined(MOD_KO)
        "\xF2\xC3\xF2\xD3\xF0\xB4 \xF0\x76\xF1\xD6\xF2\x4E \xF0\x41\xF0\x44\xF0\xA3\xF0\xBF@\xF0\x94\xF0\x74 \xF1\x03\xF0\x82\xF1\x6F \xF0\x72\xF1\x3F\xF0\xB2 \xF0\x9F\xF0\x7D\xF0\x90．",
    #else
    "Bloodthirsty undead knight @who eternally seeks combat．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xD2\xF0\xD2\xF1\x14\xF1\x1C\xF0\x7C \xF1\x78\xF2\x22 \xF0\x43\xF2\x75．",
    #else
    "Mysterious cloaked @swordsman．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x71\xF0\x7E \xF1\x7B\xF0\x50 \xF0\xD2\xF2\x47\xF1\xDE．",
    #else
    "Fish head guardian．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xDF\xF1\xD9 \xF0\x52\xF1\x90 \xF1\x3F\xF0\x55． \xF0\x9C\xF0\x9D\xF0\xC6@\xF0\x65\xF0\x9B\xF0\x95\xF0\x49．",
    #else
    "Short magic−wielding demon．@Summons demonic allies．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xC7\xF2\x7D\xF0\x40\xF0\xC6 \xF2\x45\xF0\x53\xF3\x0C@\xF1\xCE\xF3\x36 \xF0\x85\xF3\x47\xF0\xC6 \xF0\x6E\xF0\x49．",
    #else
    "Searches among graves and @knows much wise lore．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xAE\xF0\x7F\xF1\x60\xF0\x7C \xF0\x7B\xF0\x4D \xF0\x52\xF2\x6F．",
    #else
    "Ghost witches from Salem．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xF6\xF0\x7E \xF2\x46\xF1\xE7\xF0\x7C \xF3\x0B\xF0\x7D．@\xF0\xF6\xF0\x4A\xF0\x95 \xF0\xC0\xF0\x43 \xF0\xC2 \xF1\xDE\xF1\x67\xF0\xC6 \xF3\x3E\xF0\x49．",
    #else
    "Eldest Giant brother．@Wields two huge daggers．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xF6\xF0\x7E \xF2\x46\xF1\xE7．@\xF0\x44\xF1\x6D\xF2\x06 \xF0\x9F\xF0\x7D\xF0\xA7\xF0\xC6 \xF3\x3E\xF0\x49．",
    #else
    "Giant brother．@Wields a gurkha knife．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xF6\xF0\x7E \xF2\x46\xF1\xE7．@\xF2\x0A\xF0\x49\xF1\x55 \xF0\xAF\xF1\xFD \xF1\x78\xF2\x02\xF0\xC6 \xF3\x3E\xF0\x49．",
    #else
    "Giant brother．@Uses huge steel hammer．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xD6\xF0\x59 \xF1\x46 \xF0\xF4\xF1\xD1@\xF0\x9F\xF0\x7D\xF0\x90．",
    #else
    "Discus−wielding armored @knight．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x11\xF0\x52\xF1\x05\xF0\x7C \xF0\x52\xF2\xD0．",
    #else
    "Demon Lord of Ravens．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xB5\xF0\x70\xF0\xE0 \xF2\xE3\xF0\xA0 \xF0\xA2\xF0\x71\xF2\x6A \xF0\x55\xF0\x68\xF2\x1E",
    #else
    "Monster created from slimy @gel substance．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xCC\xF0\x42 \xF1\xBF\xF0\xBF \xF1\x3F\xF0\x55．@“\xF0\x85\xF2\xCD\xF0\x7C \xF0\x6D\xF0\xAB”\xF0\x4B \xF1\x9C\xF1\x6F\xF0\x49．",
    #else
    "Thunder−wielding demon．@Called ””Musician from Hell””．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x85\xF2\xCD\xF0\x7C \xF0\x92\xF0\xD2\xF2\x8D．",
    #else
    "Watchdog of Hell．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x71\xF0\x7E \xF0\xDA\xF0\xC2\xF1\x7B\xF0\x50．",
    #else
    "Master Fish−head．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x52\xF1\x90 \xF1\xBF\xF0\xBF \xF0\xF1\xF3\x1B\xF1\x05．@\xF0\x63 \xF0\x60\xF2\xFF\xF0\xB4 \xF0\x85\xF0\xA8\xF0\x95\xF0\x49．",
    #else
    "Magic−wielding vampire．@Rules upper part of castle．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xD7\xF1\xDE \xF1\x3F\xF0\x55．@\xF1\x1E\xF1\xDE\xF0\x7C \xF2\x67\xF0\x74 \xF2\x04\xF0\xDD\xF0\x95\xF0\x49．",
    #else
    "Female demon．@Invades men’s dreams．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xEC\xF1\xD9 \xF1\xDE\xF0\x7C \xF0\xAB\xF0\x4D \xF1\x46@\xF2\xA4\xF0\x62．",
    #else
    "Gravestones possessed by @evil spirits of dead．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x7E\xF3\x1B\xF0\xA0 \xF0\xE4\xF0\xDB \xF1\xB8\xF0\x9E．",
    #else
    "Watered with human blood．",
    #endif
    #if defined(MOD_KO)
        "\xF2\x78\xF2\xE0\xF0\x7D \xF0\xAB\xF1\xDE．",
    #else
    "The cowardly lion．",
    #endif
#if defined(MOD_KO)
#if defined(MOD_SATURN)
    "\xF2\x73\xF0\xC6 \xF1\x2E\xF0\x50\xF0\xBF \xF1\x18\xF1\x54\xF1\x6F \xF1\x1E\xF1\xDE．",
#else
    "\xF2\x73\xF0\xC6 \xF2\xF7\xF0\xBF \xF1\x18\xF1\x54\xF1\x6F \xF1\x1E\xF1\xDE．",
#endif
#else
#if defined(MOD_SATURN)
    "Impaled man looking for brains．",
#else
    "Impaled man looking for brain．",
#endif
#endif
    #if defined(MOD_KO)
        "\xF0\xD7\xF2\xEF\xF0\x7D \xF1\x83\xF0\x7E \xF0\x6D\xF1\x86\xF0\x7C@\xF1\xA9\xF1\x82\xF0\x88．",
    #else
    "Nightmarish creature．@Composed of many．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xD4\xF0\xC7\xF0\x4B\xF0\xBF \xF1\x97\xF1\x82\xF0\x7C \xF0\x55\xF0\x68\xF2\x1E．",
    #else
    "Monster nicknamed ””schmoo””．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x6B\xF0\x80 \xF0\xD6\xF0\xBF \xF1\xC7\xF1\xFD \xF0\x9F\xF0\xC7\xF1\x17．",
    #else
    "Heartless tin man．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xD2\xF1\x75\xF1\xD9 \xF0\xC1\xF1\xC1\xF0\xC6 \xF3\x16\xF0\xBF\xF0\x49．",
    #else
    "Holds countless poison seeds．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xDE\xF0\x46 \xF0\xB9\xF0\xFE \xF2\xF3\xF0\xBF \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．",
    #else
    "Skeleton eternally chasing @his own skull．",
    #endif
    #if defined(MOD_KO)
        "\xF2\x39\xF0\xE6 \xF1\x3D\xF0\x85\xF0\xBF \xF0\x6C\xF1\x7B@\xF0\x9F\xF0\x7D\xF0\x90",
    #else
    "Bomb−tossing armored @knight．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x59\xF0\xDC\xF0\xA0 \xF1\xE0\xF1\x63\xF1\x41 \xF2\x7D\xF0\x5E\xF0\xBF@\xF2\xE6\xF0\x5E．",
    #else
    "Zombies which attack even @when cut in half．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xB1\xF0\xD8\xF0\xA1\xF0\x7D \xF0\xFD\xF2\xFD．@\xF0\x52\xF1\x90\xF0\xAB\xF1\x74 \xF0\x5B \xF0\xD2 \xF1\xDD\xF0\x49．",
    #else
    "Mutated insect．@Visible only to magicians．",
    #endif
    #if defined(MOD_KO)
        "\xF2\x26\xF1\xFB \xF0\xC7\xF0\x46 \xF1\x3A\xF0\x7E．",
    #else
    "Expert at throwing weapons．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xC7\xF3\x3D\xF1\x41\xF0\x4E \xF0\x81\xF2\x48\xF0\x95 \xF2\x9C\xF0\xD2．",
    #else
    "Fiendishly accurate archer．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x59\xF1\xD9 \xF0\x7E\xF0\xAE， \xF0\x59\xF1\xD9 \xF0\x48\xF0\x4A．",
    #else
    "Half−man， half−wolf．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x70\xF0\x88 \xF1\xA9\xF0\x9E．@\xF2\x4B\xF0\xB8\xF1\x8F \xF1\x83\xF2\xC0\xF0\x7D \xF1\x42\xF0\x49．",
    #else
    "Liquid creature．@Takes black panther shape．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xF6\xF0\x4A \xF0\xF1\xF3\x1B \xF0\x58\xF0\x83．",
    #else
    "Giant vampire bat．",
    #endif
    #if defined(MOD_KO)
        "\xF2\x3D\xF0\x50 \xF1\x7A\xF0\x7D \xF0\x8D\xF0\xBF \xF0\x5A\xF3\x09",
    #else
    "White dragon ridden by Flea @Man．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x59\xF1\xD9 \xF0\x7E\xF0\xAE， \xF0\x59\xF1\xD9 \xF0\x65．",
    #else
    "Half−man， half−bull．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xC7\xF0\x61\xF0\xDB \xF3\x40@\xF0\xC7\xF0\x46\xF0\xC6 \xF3\x3E\xF0\x49．",
    #else
    "Wields fearsome beam @weapon．",
    #endif
    #if defined(MOD_KO)
        "\xF2\x3D\xF0\x50\xF1\x7A\xF0\x7D \xF1\xE8\xF1\xEB\xF0\xA3\xF0\xBF \xF0\x5E\xF3\x09．",
    #else
    "Wyrm controlled by Flea Man．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xF6\xF0\x4A \xF0\xB2\xF1\x59\xF0\xFD\xF0\x7C \xF0\xB9\xF0\xFE",
    #else
    "Animated skeleton of huge @dragon．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x9C\xF2\x7E\xF0\x7D \xF3\x48\xF0\xBF \xF0\x48\xF0\x4A．",
    #else
    "Fireball−spitting wolf．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xB1 \xF1\x3D\xF0\x85\xF0\xBF \xF0\xF4\xF1\xD1@\xF0\x9F\xF0\x7D\xF0\x90．",
    #else
    "Rock−tossing armored @knight．",
    #endif
    #if defined(MOD_KO)
        "\xF1\xFC\xF0\xAB \xF2\x9C\xF0\xD2．@\xF0\xBD\xF0\x90\xF1\xE9\xF0\x7C \xF0\x6C\xF1\x76\xF0\x4B\xF0\x50\xF0\x8C\xF0\xC6 \xF1\xEC\xF2\xCB\xF0\x49．",
    #else
    "Angel archer．@Slew Amalaric of the Goths．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xAB\xF0\x6D\xF0\x95 \xF0\x52\xF0\x43．@\xF3\x14\xF2\x1E\xF0\x40\xF0\x7D\xF0\x68\xF0\x90\xF0\xC6 \xF0\x7F\xF0\xDC\xF2\x17\xF0\x49．",
    #else
    "Evil demon−sword． Causes @poltergeist phenomena．",
    #endif
    #if defined(MOD_KO)
        "\xF2\xCC\xF0\x71\xF1\xCF\xF0\x4B \xF0\xA9\xF0\xB0\xF0\xA3\xF0\xBF \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．",
    #else
    "Skeleton with leaping attack．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xB3\xF1\x08 \xF0\xE6 \xF0\x68\xF2\x0F\xF0\x4C\xF2\x24．",
    #else
    "Skeleton mounted on Warg．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x0D \xF3\x1A\xF0\xA0 \xF0\x94@\xF1\xA2\xF0\xBF \xF0\xF1\xF3\x1B\xF2\xFD．",
    #else
    "Pre−intelligent bloodsucker @with long tongue for sucking．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xAB\xF0\x6D\xF0\x95 \xF0\xA1\xF0\x7D \xF0\xC8\xF0\x71",
    #else
    "Evil mutated octopus．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x9C\xF1\x15 \xF1\x3F\xF0\x55．",
    #else
    "Flame demon．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x80\xF0\xF4 \xF1\xE4\xF2\x26\xF1\xC5\xF0\xD2．",
    #else
    "Armor plated war−beast．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x85\xF2\xCD\xF0\x7C \xF2\xDE\xF3\x1D \xF1\x3F\xF0\x55．",
    #else
    "Gruesome Demon from Hell．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xBD\xF0\x4A \xF0\x7D\xF0\xE1\xF0\x90 \xF0\x57\xF0\x4B",
    #else
    "Ancient egyptian mummy．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xB6\xF0\x99\xF0\x95 \xF0\x5E\xF1\x27\xF0\x68 \xF1\xD8\xF0\xB2．@\xF1\x3F\xF0\x55\xF0\x7C \xF0\x94\xF0\xA0 \xF1\xDE\xF2\x8F\xF0\x49．",
    #else
    "Advanced Venus Weed．@Fed with demon blood．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xA1\xF0\x6A\xF0\xA3\xF0\xBF \xF1\x3F\xF0\x55．",
    #else
    "Shape−shifting demon．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xA5\xF1\x81 \xF0\xB1 \xF2\x80\xF0\xBF@\xF1\x8D\xF1\x7B\xF0\x50 \xF0\x52\xF2\x6F．",
    #else
    "Snake−headed demoness @with gaze of stone．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xAF\xF0\x95 \xF1\xE4\xF0\xAB\xF1\x47\xF0\x7C \xF2\xA3\xF0\xB4@\xF0\x7D\xF0\x71 \xF3\x27\xF0\xD7 \xF1\x74\xF1\x47\xF1\xC9\xF0\x49．",
    #else
    "Made from the body parts @of powerful warriors．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x08\xF1\x5A\xF0\x90\xF0\xC6 \xF0\x40\xF0\x80\xF0\x95 \xF2\xE6\xF0\x5E．",
    #else
    "Zombie impersonating Grant．",
    #endif
    #if defined(MOD_KO)
        "\xF1\x56\xF0\xA7\xF0\xC6 \xF0\x40\xF0\x80\xF0\x95 \xF2\xE6\xF0\x5E．",
    #else
    "Zombie impersonating Trevor．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x80\xF2\x6A\xF2\xA8\xF1\x5D\xF0\x46 \xF2\x64\xF0\x52 \xF1\x3F\xF0\x55．",
    #else
    "Mischievous little demon．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xAB\xF0\x7D\xF0\x92\xF0\xC6 \xF0\x40\xF0\x80\xF0\x95 \xF2\xE6\xF0\x5E．",
    #else
    "Zombie impersonating Sypha．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x92\xF0\x50\xF0\x7C \xF2\xD0 \xF1\x93\xF1\xE7\xF0\x5D\xF0\x5D．",
    #else
    "Beelzebub Lord of Flies．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x85\xF2\xCD\xF0\x7C \xF1\xFA\xF2\x46\xF1\xDE．",
    #else
    "Executioner of Hell．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x41\xF0\x4B\xF1\x83\xF0\x68 \xF0\x5C\xF0\xA3 \xF0\x73\xF0\xAC \xF0\x81\xF0\x4D．",
    #else
    "New−half ice spirit．@Servant of Galamoth．",
    #endif
    #if defined(MOD_KO)
        "\xF2\xE1\xF1\xD9 \xF0\xD7\xF1\xDE \xF0\x52\xF2\x6F",
    #else
    "Young female witch．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xF1\xF3\x1B\xF1\x05 \xF0\xAB\xF1\x26\xF0\x7C \xF1\x3A\xF0\x7E．",
    #else
    "Master Vampire hunter．",
    #endif
#if defined(MOD_KO)
#if defined(MOD_SATURN)
    "\xF2\x4D\xF1\x05 \xF1\xE8\xF1\x6B．@\xF0\xAB\xF1\x57 \xF0\xA5\xF1\x81 \xF1\x41\xF1\x78\xF3\x00\xF0\x49．",
#else
    "\xF0\xAB\xF1\x57 \xF0\xA5\xF1\x81 \xF1\x41\xF1\x78\xF2\x02\xF0\xBF@\xF2\x4D\xF1\x05 \xF1\xA7．",
#endif
#else
#if defined(MOD_SATURN)
    "Rare bird．@Runs when it sees people．",
#else
    "Rare bird．@Runs when sees people．",
#endif
#endif
    #if defined(MOD_KO)
        "\xF0\x52\xF0\xFB \xF0\x85\xF0\xA8 \xF1\x2E\xF0\x50\xF0\xBF \xF0\x52\xF1\x41\xF0\xAB",
    #else
    "Magical being with plan to @rule the netherworld．",
    #endif
    #if defined(MOD_KO)
        "\xF0\x60\xF1\x0B \xF0\x6C\xF1\x7B \xF0\xA0\xF0\xB2．",
    #else
    "High level Armor Lord．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xB2\xF0\x4B\xF2\x16\xF0\x4B\xF0\x7C \xF2\xFE\xF2\xA0．",
    #else
    "Dracula’s close friend．",
    #endif
    #if defined(MOD_KO)
        "\xF0\xB2\xF0\x4B\xF2\x16\xF0\x4B\xF0\x7C \xF0\x71\xF1\x43\xF0\x7C \xF0\x6A\xF1\x00．",
    #else
    "Dark Priest of Dracula．",
    #endif
    #if defined(MOD_KO)
        "\xF3\x4D\xF0\x4B\xF0\xE4\xF0\x6C \xF0\x76\xF0\x82． \xF0\x6F\xF2\x06\xF0\xB2\xF0\x7C \xF0\x6C\xF1\x8E\xF0\x85",
    #else
    "Lord of Wallachia．@Father of Alucard．",
    #endif
};
static u8 D_us_801811FC[] = {6, 3, 16, 4, -1, 0};
static u8 D_us_80181204[] = {10, 5, 10, 4, 10, 5, 1, 4, -1, 0};
static u8 D_us_80181210[] = {12, 14, 10, 15, 8, 16, 4, 17, -1, 0};
static u8 D_us_8018121C[] = {4,  16, 53, 19, 7,  18, 7,  19, 7,  18, 7,
                             19, 7,  18, 3,  19, 6,  16, 4,  17, -1, 0};
static u8 D_us_80181234[] = {7, 16, 6, 19, 6, 20, 7, 21, 8, 22, 4, 23, -1, 0};
static u8 D_us_80181244[] = {
    4,  22, 29, 21, 4,  20, 4, 19, 3,  16, 52, 17, 6,  16,
    10, 15, 7,  14, 10, 4,  9, 3,  52, 2,  11, 10, 7,  11,
    8,  12, 36, 13, 6,  12, 7, 11, 7,  10, 4,  2,  -1, 0};
static u8 D_us_80181270[] = {4, 22, 29, 21, 4, 20, 4, 19, 3, 16, 4, 17, -1, 0};
static u8 D_us_80181280[] = {6, 16, 10, 15, 7, 14, 10, 4, 9, 3, 4, 2, -1, 0};
static u8 D_us_80181290[] = {11, 10, 7, 11, 8, 12, 4, 13, -1, 0};
static u8 D_us_8018129C[] = {6, 12, 7, 11, 7, 10, 4, 2, -1, 0};
static u8 D_us_801812A8[] = {11, 2, 10, 6, 7, 7, 7, 8, 49, 9, 0, 0};
static u8 D_us_801812B4[] = {3, 8, 7, 7, 7, 6, 4, 2, 0, 0};
static u8 D_us_801812C0[] = {32, 4, 8, 3, 4, 2, -1, 0};
static u8 D_us_801812C8[] = {8, 2, 8, 24, 0, 0};
static u8 D_us_801812D0[] = {4, 41, 4, 42, -1, 0};
static u8* D_us_801812D8[] = {
    D_us_801811FC, D_us_80181204, D_us_80181210, D_us_8018121C, D_us_80181234,
    D_us_80181270, D_us_80181280, D_us_80181290, D_us_8018129C, D_us_801812A8,
    D_us_801812B4, D_us_801812C0, D_us_801812C8, D_us_801812D0};

#ifdef VERSION_PSP
extern s32 E_ID(ID_25);
extern s32 E_ID(ID_27);
extern s32 E_ID(ID_28);
extern s32 E_ID(ID_29);
extern s32 E_ID(ID_2A);
extern s32 E_ID(ID_2D);
extern s32 E_ID(ID_2E);
extern s32 E_ID(ID_2F);
extern s32 E_ID(ID_48);
extern s32 E_ID(ID_4F);
extern s16 D_psp_092A4A10[];
extern u16 D_psp_092A4A78[];
extern s16 D_psp_092A4A88[];
extern u16 D_psp_092A4AA0[];
extern s16 D_psp_092A49B8[];
extern u8** D_psp_092A5F48;
extern u8** D_psp_092A5F98;
#define BUTTON_CONFIRM g_ConfirmButton
#define BUTTON_CANCEL g_CancelButton
#elif defined(MOD_KO)
#define BUTTON_CONFIRM PAD_CIRCLE
#define BUTTON_CANCEL PAD_CROSS
#else
#define BUTTON_CONFIRM PAD_CROSS
#define BUTTON_CANCEL PAD_TRIANGLE
#endif

// bss
static u32 D_us_801D415C[64];
static u32 D_us_801D425C[64];
STATIC_PAD_BSS(8);
static ShopItem D_us_801D4364[75];
STATIC_PAD_BSS(8);

extern s32 g_SkipCutscene;
extern u32 g_CutsceneFlags;
extern s32 g_IsCutsceneDone;

// This is probably EntityLibrarian, but I don't know for sure
void func_us_801AFE0C(Entity* self) {
    Tilemap* tilemap = &g_Tilemap;
    Entity* player = &PLAYER;

    switch (self->step) {
    case 0:
#ifdef VERSION_PSP
        func_psp_0925D4D0();
#endif
        InitializeEntity(D_us_80180824);
        if (player->posX.i.hi < 0x100) {
            // sfxID for SEQ_LIB was originally 0x202 but is 0x302 on PSP
#ifdef VERSION_PSP
            g_api.PlaySfx(MU_SEQ_LIBRARIAN_PSP);
#else
            g_api.PlaySfx(MU_SEQ_LIBRARIAN);
#endif
        }
        if (g_CastleFlags[MET_LIBRARIAN]) {
            self->step = 8;
            break;
        }
#ifdef VERSION_PSP
        g_Player.padSim = PAD_LEFT | PAD_SIM_UNK20000;
        g_Player.demo_timer = 1;
#endif
        break;

    case 1:
        g_Entities[E_AFTERIMAGE_1].ext.afterImage.disableFlag = 1;
        g_PauseAllowed = false;
        g_unkGraphicsStruct.pauseEnemies = true;
        g_Player.padSim = PAD_LEFT;
        g_Player.demo_timer = 1;
        if (g_Player.status & PLAYER_STATUS_BAT_FORM) {
            g_Player.padSim = PAD_BAT;
        } else if (g_Player.status & PLAYER_STATUS_MIST_FORM) {
#ifdef VERSION_PSP
            g_Player.padSim = PAD_NONE;
#else
            g_Player.padSim = PAD_MIST;
#endif
        } else if (g_Player.status & PLAYER_STATUS_WOLF_FORM) {
            g_Player.padSim = PAD_WOLF;
        }
        g_Player.demo_timer = 1;
        SetStep(2);
        break;

    case 2:
        if (player->posX.i.hi > 0xE8) {
            if (g_Player.status & PLAYER_STATUS_TRANSFORM) {
                g_Player.padSim = PAD_NONE;
                if (g_Timer & 1) {
                    if (g_Player.status & PLAYER_STATUS_BAT_FORM) {
#ifdef VERSION_PSP
                        g_Player.padSim = PAD_BAT | PAD_SIM_UNK20000;
#else
                        g_Player.padSim = PAD_BAT;
#endif
                    } else if (g_Player.status & PLAYER_STATUS_MIST_FORM) {
#ifdef VERSION_PSP
                        g_Player.padSim = PAD_NONE | PAD_SIM_UNK20000;
#else
                        g_Player.padSim = PAD_MIST;
#endif
                    } else if (g_Player.status & PLAYER_STATUS_WOLF_FORM) {
#ifdef VERSION_PSP
                        g_Player.padSim = PAD_WOLF | PAD_SIM_UNK20000;
#else
                        g_Player.padSim = PAD_WOLF;
#endif
                    }
                }
            } else {
                g_Player.padSim = PAD_LEFT;
            }
        } else {
            g_CutsceneFlags |= 1;
            g_Player.padSim = PAD_NONE;
            player->posX.i.hi = 0xE8;
            self->step++;
        }
        g_Player.demo_timer = 1;
        break;

    case 3:
        if (g_CutsceneFlags & 0x40) {
            if (player->posX.i.hi > 0x74) {
                g_Entities[E_AFTERIMAGE_1].ext.afterImage.disableFlag = 1;
                g_Player.padSim = PAD_LEFT;
            } else {
                player->posX.i.hi = 0x74;
                g_Player.padSim = PAD_NONE;
                self->step++;
            }
        } else {
            player->posX.i.hi = 0xE8;
        }
        g_Player.demo_timer = 1;
        break;

    case 4:
        g_Player.padSim = PAD_NONE | PAD_SIM_UNK20000;
        g_Player.demo_timer = 1;
        self->step++;
        break;

    case 5:
        g_CastleFlags[MET_LIBRARIAN] = 1;
        g_api.TimeAttackController(
            TIMEATTACK_EVENT_MEET_MASTER_LIBRARIAN, TIMEATTACK_SET_RECORD);
        g_Player.demo_timer = 1;
        self->step++;
        /* fallthrough */
    case 6:
        if (g_CutsceneFlags & 0x100) {
            g_CutsceneFlags |= 0x2000;
            self->step = 0x10;
        } else {
            player->posX.i.hi = 0x74;
        }
        break;

    case 8:
        self->step++;
        /* fallthrough */
    case 9:
        if (player->posX.i.hi > 0xFF) {
            g_api.PlaySfx(CD_SOUND_COMMAND_7);
            DestroyEntity(self);
            return;
        }
        if (player->posX.i.hi < 0x75) {
            switch (self->step_s) {
            case 0:
                g_Entities[E_AFTERIMAGE_1].ext.afterImage.disableFlag = 1;
                g_PauseAllowed = false;
                g_unkGraphicsStruct.pauseEnemies = true;
                g_Player.padSim = PAD_NONE;
                g_Player.demo_timer = 16;
                self->step_s++;
                g_CutsceneFlags |= 1;
                break;

            case 1:
                if (g_Player.status & PLAYER_STATUS_TRANSFORM) {
                    g_Player.padSim = PAD_NONE;
                    if (g_Timer & 1) {
                        if (g_Player.status & PLAYER_STATUS_BAT_FORM) {
                            g_Player.padSim = PAD_BAT;
                        } else if (g_Player.status & PLAYER_STATUS_MIST_FORM) {
#ifdef VERSION_PSP
                            g_Player.padSim = PAD_NONE;
#else
                            g_Player.padSim = PAD_MIST;
#endif
                        } else if (g_Player.status & PLAYER_STATUS_WOLF_FORM) {
                            g_Player.padSim = PAD_WOLF;
                        }
                    }
                } else {
                    g_Player.padSim = PAD_LEFT;
                    self->step_s++;
                }
                g_Player.demo_timer = 1;
                break;

            case 2:
                g_Player.padSim = PAD_NONE;
                g_Player.demo_timer = 128;
                SetStep(10);
                break;
            }
            player->posX.i.hi = 0x74;
        }
        break;

    case 10:
        if (!g_Player.demo_timer && (g_Player.vram_flag & TOUCHING_GROUND)) {
            g_Player.padSim = PAD_NONE | PAD_SIM_UNK20000;
            g_Player.demo_timer = 1;
            self->step++;
        }
        player->posX.i.hi = 0x74;
        break;

    case 11:
        g_Player.padSim = PAD_NONE | PAD_SIM_UNK20000;
        g_Player.demo_timer = 1;
        if (g_CutsceneFlags & 0x100) {
            g_CutsceneFlags |= 0x2000;
            self->step = 0x10;
        }
        break;

    case 16:
#ifdef VERSION_PSP
        g_PauseAllowed = false;
#endif
        g_Player.demo_timer = 32;
        g_Player.padSim = PAD_RIGHT;
        stopMusicFlag = true;
        self->step++;
        break;

    case 17:
#ifdef VERSION_PSP
        g_PauseAllowed = false;
#endif
        if (!g_Player.demo_timer) {
#ifdef VERSION_PSP
            g_PauseAllowed = true;
#endif
            SetStep(9);
        }
        break;
    }
}

void EntityLibrarianChair(Entity* self) {
    Entity* newEnt;
    Entity* player = &PLAYER;
    Tilemap* tilemap = &g_Tilemap;

    if (self->step && (self->step < 11) && (g_SkipCutscene != 0) &&
        (g_IsCutsceneDone != 0)) {
        self->step = 11;
        self->animCurFrame = 2;
    }
    if (player->posX.i.hi < 0x38) {
        self->zPriority = 0xC0;
        if (g_Player.status & PLAYER_STATUS_TRANSFORM) {
            GetPlayerCollisionWith(self, 16, 12, 2);
            // If the chair is not in step 16, and the player is high enough,
            // and moving upward, trigger a hit.
        } else if (self->step < 16 &&
                   ((player->posY.i.hi + tilemap->scrollY.i.hi) < 201) &&
                   F(player->velocityY).i.hi < 0) {
            SetStep(16);
            if (PLAYER.step == Player_HighJump) {
                g_Player.gravBootTimer = 0x1C;
                if (PLAYER.step_s == 0) {
                    PLAYER.step_s = 1;
                    PLAYER.velocityY = FIX(-8);
                }
            }
            // Play chair jumping sound
            g_api.PlaySfx(SFX_QUICK_STUTTER_EXPLODE_B);
            self->ext.libraryChair.totalHits++;
            self->ext.libraryChair.consecutiveHits++;

            // At first hit, give Life Max Up.
            if (!(g_CastleFlags[LIBRARIAN_DROPS] & 1)) {
                newEnt = AllocEntity(&g_Entities[160], &g_Entities[192]);
                if (newEnt != NULL) {
                    CreateEntityFromCurrentEntity(E_PRIZE_DROP, newEnt);
                    newEnt->params = ITEMDROP_LIFE_VESSEL;
                    g_CastleFlags[LIBRARIAN_DROPS] |= 1;
                }
            }
            // Getting Axe Lord Armor requires hitting librarian 64 times.
            if (!(g_CastleFlags[LIBRARIAN_DROPS] & 2) &&
                (self->ext.libraryChair.totalHits >= 64)) {
                newEnt = AllocEntity(&g_Entities[160], &g_Entities[192]);
                if (newEnt != NULL) {
                    CreateEntityFromCurrentEntity(E_EQUIP_ITEM_DROP, newEnt);
                    newEnt->params = NUM_HAND_ITEMS + ITEM_AXE_LORD_ARMOR;
                    g_CastleFlags[LIBRARIAN_DROPS] |= 2;
                }
            }
            // Ring of Arcana requires 16 hits, without touching ground
            if (!(g_CastleFlags[LIBRARIAN_DROPS] & 4) &&
                (self->ext.libraryChair.consecutiveHits >= 16)) {
                newEnt = AllocEntity(&g_Entities[160], &g_Entities[192]);
                if (newEnt != NULL) {
                    CreateEntityFromCurrentEntity(E_EQUIP_ITEM_DROP, newEnt);
                    newEnt->params = NUM_HAND_ITEMS + ITEM_RING_OF_ARCANA;
                    g_CastleFlags[LIBRARIAN_DROPS] |= 4;
                }
            }
            // Dracula Tunic requires 24 hits, and inverted castle must be
            // unlocked.
            if (!(g_CastleFlags[LIBRARIAN_DROPS] & 8) &&
                (self->ext.libraryChair.consecutiveHits >= 24) &&
                (g_CastleFlags[INVERTED_CASTLE_UNLOCKED])) {
                newEnt = AllocEntity(&g_Entities[160], &g_Entities[192]);
                if (newEnt != NULL) {
                    CreateEntityFromCurrentEntity(E_EQUIP_ITEM_DROP, newEnt);
                    newEnt->params = NUM_HAND_ITEMS + ITEM_DRACULA_TUNIC;
                    g_CastleFlags[LIBRARIAN_DROPS] |= 8;
                }
            }
        }
    } else {
        self->zPriority = 0x80;
    }
    // If the player touches the ground, reset the frames airborne.
    if (g_Player.vram_flag & TOUCHING_GROUND) {
        self->ext.libraryChair.consecutiveHits = 0;
    }
    switch (self->step) {
    case 0:
        InitializeEntity(g_EInitCommon);
        self->zPriority = 0x80;
        self->animSet = ANIMSET_OVL(11);
        self->animCurFrame = 2;
        self->palette = 0x210;
        self->unk5A = 0x48;
        self->ext.libraryChair.debugAnimID = 0;
        self->ext.libraryChair.timer = 0x20;
        self->flags &= ~FLAG_DESTROY_IF_OUT_OF_CAMERA;
        if (g_PlayableCharacter != PLAYER_ALUCARD) {
            self->facingLeft = 1;
            self->posX.i.hi -= 8;
        }
        g_CutsceneFlags = 0;
        g_IsCutsceneDone = 0;
        g_SkipCutscene = 0;
        break;
    case 1:
        if (g_CutsceneFlags & 2) {
            if (--self->ext.libraryChair.timer) {
                SetStep(2);
            }
        } else {
            if (g_CutsceneFlags & 0x80) {
                SetStep(9);
            }
        }
        break;
    case 2:
        if (AnimateEntity(D_us_801811FC, self) == 0) {
            SetStep(3);
        }
        break;
    case 3:
        AnimateEntity(D_us_80181204, self);
        if (g_CutsceneFlags & 4) {
            SetStep(4);
        }
        break;
    case 4:
        AnimateEntity(D_us_80181210, self);
        if (g_CutsceneFlags & 8) {
            SetStep(5);
        }
        break;
    case 5:
        AnimateEntity(D_us_8018121C, self);
        if (g_CutsceneFlags & 0x10) {
            SetStep(6);
        }
        break;
    case 6:
        AnimateEntity(D_us_80181234, self);
        if (g_CutsceneFlags & 0x20) {
            SetStep(7);
        }
        break;
    case 7:
        if (AnimateEntity(D_us_80181244, self) == 0) {
            SetStep(11);
        }
        break;
    case 9:
        if (AnimateEntity(D_us_801811FC, self) == 0) {
            SetStep(10);
        }
        break;
    case 10:
        if (AnimateEntity(D_us_801812C0, self) == 0) {
            SetStep(11);
        }
        break;
    case 11:
        if (g_CutsceneFlags & 0x800) {
            SetStep(12);
            self->ext.libraryChair.timer = 0x30;
        }
        if (g_CutsceneFlags & 0x1000) {
            SetStep(13);
        }
        break;
    case 12:
        AnimateEntity(D_us_801812C8, self);
        if (!--self->ext.libraryChair.timer) {
            g_CutsceneFlags &= ~0x800;
            self->animCurFrame = 2;
            SetStep(11);
        }
        if (g_CutsceneFlags & 0x1000) {
            self->animCurFrame = 2;
            SetStep(13);
        }
        break;
    case 13:
        break;
    case 16:
        AnimateEntity(D_us_801812D0, self);
        if (g_Player.status & PLAYER_STATUS_TRANSFORM) {
            self->velocityY += FIX(3.0 / 8);
            if (self->velocityY > FIX(7)) {
                self->velocityY = FIX(7);
            }
            MoveEntity();
            GetPlayerCollisionWith(self, 16, 12, 0x12);
        } else {
            if (PLAYER.step == Player_HighJump && PLAYER.step_s == 0) {
                PLAYER.step_s = 1;
                PLAYER.velocityY = FIX(-8);
            }
            self->posY.i.hi = player->posY.i.hi - 0x1C;
            self->velocityY = player->velocityY;
        }
        if ((self->posY.i.hi + tilemap->scrollY.i.hi) >= 172) {
            self->posY.i.hi = 172 - tilemap->scrollY.i.hi;
            self->animCurFrame = 2;
            SetStep(1);
        }
        break;
    case 255:
        if (g_pads[0].tapped & PAD_UP) {
            if (self->ext.libraryChair.debugAnimID) {
                self->ext.libraryChair.debugAnimID--;
                self->pose = 0;
                self->poseTimer = 0;
            }
        } else if (g_pads[0].tapped & PAD_DOWN) {
            if (self->ext.libraryChair.debugAnimID != 0x10) {
                self->ext.libraryChair.debugAnimID++;
                self->pose = 0;
                self->poseTimer = 0;
            }
        }
        AnimateEntity(D_us_801812D8[self->ext.libraryChair.debugAnimID], self);
        break;
    }
}

#ifdef VERSION_PSP
extern const char** D_us_80181310;
#else
static const char* D_us_80181310[] = {
#if defined(MOD_KO)
    "\x07\xF0\x44\xF0\xDD",
#else
    "\003"
    "Buy item",
#endif
#if defined(MOD_KO)
    "\x07\xF1\xE4\xF0\x67",
#else
    "\003"
    "Tactics",
#endif
#if defined(MOD_KO)
    "\x06\xF0\xAD\xF1\x84\xF0\x4E",
#else
    "\001"
    "Enemy list",
#endif
#if defined(MOD_KO)
    "\x01\xF0\xAB\xF0\xDB\xF0\xB2\xF0\x8E\xF0\x68\xF0\x90",
#else
    "\001"
    "Sound test",
#endif
#if defined(MOD_KO)
    "\x06\xF0\x9F\xF0\x40\xF0\x46",
#else
    "\006"
    "Exit",
#endif
#if defined(MOD_KO)
    "\x04\xF0\xA5\xF0\x62\xF0\x53\xF0\xF2",
#else
    "\003"
    "Sell gem"
#endif
};
#endif

static u8 D_us_80181328[] = {4, 0, 5, 1, 4};
static u8 D_us_80181330[] = {5, 0, 5, 1, 2, 4};
static u8 D_us_80181338[] = {6, 0, 5, 1, 2, 3, 4};
static u8* D_us_80181340[] = {D_us_80181328, D_us_80181330, D_us_80181338};

static InventoryItem D_us_8018134C[] = {
    // clang-format off
    { INVENTORY_RELIC,     0xFF, 0x0000,              500 }, // special case: Jewel of Open
    { INVENTORY_HAND,      0,    ITEM_POTION,         800 },
    { INVENTORY_HAND,      3,    ITEM_HIGH_POTION,    2000 },
    { INVENTORY_HAND,      5,    ITEM_ELIXIR,         8000 },
    { INVENTORY_HAND,      3,    ITEM_MANNA_PRISM,    4000 },
    { INVENTORY_HAND,      0,    ITEM_ANTIVENOM,      200 },
    { INVENTORY_HAND,      0,    ITEM_UNCURSE,        200 },
    { INVENTORY_HAND,      0,    ITEM_HAMMER,         200 },
    { INVENTORY_HAND,      0,    ITEM_MAGIC_MISSILE,  300 },
    { INVENTORY_HAND,      0,    ITEM_BWAKA_KNIFE,    400 },
    { INVENTORY_HAND,      1,    ITEM_BOOMERANG,      500 },
    { INVENTORY_HAND,      3,    ITEM_JAVELIN,        800 },
    { INVENTORY_HAND,      4,    ITEM_FIRE_BOOMERANG, 1000 },
    { INVENTORY_HAND,      4,    ITEM_SHURIKEN,       2400 },
    { INVENTORY_HAND,      6,    ITEM_CROSS_SHURIKEN, 5000 },
    { INVENTORY_HAND,      7,    ITEM_BUFFALO_STAR,   8000 },
    { INVENTORY_HAND,      7,    ITEM_FLAME_STAR,     15000 },
    { INVENTORY_HAND,      0,    ITEM_LIBRARY_CARD,   500 },
    { INVENTORY_HAND,      8,    ITEM_MEAL_TICKET,    2000 },
    { INVENTORY_HAND,      0,    ITEM_SABER,          1500 },
    { INVENTORY_HAND,      0,    ITEM_MACE,           2000 },
    { INVENTORY_HAND,      0,    ITEM_DAMASCUS_SWORD, 4000 },
    { INVENTORY_HAND,      5,    ITEM_FIREBRAND,      10000 },
    { INVENTORY_HAND,      5,    ITEM_ICEBRAND,       10000 },
    { INVENTORY_HAND,      5,    ITEM_THUNDERBRAND,   10000 },
    { INVENTORY_HAND,      5,    ITEM_HARPER,         12000 },
    { INVENTORY_HAND,      0,    ITEM_LEATHER_SHIELD, 400 },
    { INVENTORY_HAND,      0,    ITEM_IRON_SHIELD,    3980 },
    { INVENTORY_HEAD,      0,    ITEM_VELVET_HAT,     400 },
    { INVENTORY_HEAD,      0,    ITEM_LEATHER_HAT,    1000 },
    { INVENTORY_HEAD,      0,    ITEM_CIRCLET,        4000 },
    { INVENTORY_HEAD,      5,    ITEM_SILVER_CROWN,   12000 },
    { INVENTORY_BODY,      0,    ITEM_IRON_CUIRASS,   1500 },
    { INVENTORY_BODY,      0,    ITEM_STEEL_CUIRASS,  4000 },
    { INVENTORY_BODY,      0,    ITEM_DIAMOND_PLATE,  12000 },
    { INVENTORY_CAPE,      0,    ITEM_REVERSE_CLOAK,  2000 },
    { INVENTORY_CAPE,      0,    ITEM_ELVEN_CLOAK,    3000 },
    { INVENTORY_CAPE,      0,    ITEM_JOSEPHS_CLOAK,  30000 },
    { INVENTORY_ACCESSORY, 0,    ITEM_MEDAL,          3000 },
    { INVENTORY_ACCESSORY, 1,    ITEM_RING_OF_PALES,  4000 },
    { INVENTORY_ACCESSORY, 4,    ITEM_GAUNTLET,       8000 },
    { INVENTORY_ACCESSORY, 8,    ITEM_DUPLICATOR,     500000 },
    { INVENTORY_DOCUMENT,  0x80, DOC_CASTLE_MAP,      103 },
    { INVENTORY_DOCUMENT,  0x81, DOC_MAGIC_SCROLL_1,  500 },
    { INVENTORY_DOCUMENT,  0x82, DOC_MAGIC_SCROLL_2,  1500 },
    { INVENTORY_DOCUMENT,  0x83, DOC_MAGIC_SCROLL_3,  5000 },
    { INVENTORY_DOCUMENT,  0x84, DOC_MAGIC_SCROLL_4,  8000 },
    { INVENTORY_DOCUMENT,  0x85, DOC_MAGIC_SCROLL_5,  15000 },
    { INVENTORY_DOCUMENT,  0,    0x000A,              1000000 },
    // clang-format on
};

/// relic index
u16 D_us_801814D4[] = {16, 0};

// sellable items
ShopItem D_us_801814D8[] = {
    // clang-format off
    { INVENTORY_ACCESSORY, ITEM_ZIRCON,     150 },
    { INVENTORY_ACCESSORY, ITEM_AQUAMARINE, 800 },
    { INVENTORY_ACCESSORY, ITEM_TURQUOISE,  1500 },
    { INVENTORY_ACCESSORY, ITEM_ONYX,       3000 },
    { INVENTORY_ACCESSORY, ITEM_GARNET,     5000 },
    { INVENTORY_ACCESSORY, ITEM_OPAL,       8000 },
    { INVENTORY_ACCESSORY, ITEM_DIAMOND,    20000 },
    // clang-format on
};

// magic scroll index to spell ID
static u16 D_us_80181510[] = {
    // clang-format off
    SPELL_DARK_METAMORPHOSIS,
    SPELL_SUMMON_SPIRIT,
    SPELL_HELLFIRE,
    SPELL_TETRA_SPIRIT,
    SPELL_SOUL_STEAL,
    // clang-format on
};

static u16 D_us_8018151C[] = {0, 1, 2, 5, 6};

#ifdef VERSION_PSP
extern u8* cutscene_data_offset_four;
#else
extern u8 D_us_80183F64;
#endif

void* func_us_801B0C40(u8* pix, const char* str, s32 x, s32 y, s32 size) {
    const u16 DOUBLE_SPACE = 0x8140;
    const u16 RIGHT_DOUBLE_QUOTATION_MARK = 0x8168;

    const int FontWidth = 12;
    const int FontHeight = 16;
    const int FontStride = FontWidth / 2;

    s32 letterWidth;
    s32 i, j;
    s32 pos;
    s32 s_8;
    s32 s_6;
    u16 ch;
    u8* ptr;
    u8* chPix;
    u8 sp3f;
#ifdef MOD_KO
    s32 ko_trim;
#endif

    s_6 = 0;
    ptr = pix;
#if defined(MOD_PTBR)
    for (i = 0; i < (size * FontHeight) * 3; i++) {
#else
    for (i = 0; i < (size * FontHeight) * 2; i++) {
#endif
        *ptr++ = 0;
    }
    while (true) {
        pos = 0;
        while (*str >= 8) {
            s_8 = 0;
#ifdef VERSION_PSP
            ch = g_api.func_psp_0913F960(str, &sp3f);
#else
            ch = *str;
#endif
            str++;
            if (ch == '@') {
                break;
            }
            if (ch >= 'a' && ch <= 'z') {
                ch = ch + 0x8220;
            } else if (ch >= 'A' && ch <= 'Z') {
                ch = ch + 0x821F;
            } else if (ch == ' ') {
                ch = DOUBLE_SPACE;
                s_8 = 2;
            } else {
#ifdef VERSION_PSP
                if (sp3f > 1) {
                    str += sp3f - 1;
                }
            }
            chPix = (u8*)g_api.func_psp_0913FA28(ch, 1);
#else
#ifdef MOD_KO
                /* Pair only real lead bytes (Hangul F0-F3, SJIS
                 * 81-9F/E0-EF). A Hangul lead always takes its 2nd byte,
                 * even 0x00 (codes F100/F200/F300 end in a zero byte);
                 * an SJIS lead never takes the NUL terminator. A
                 * half-width byte is widened to its SJIS full-width code
                 * instead of eating the next byte (a trailing '.' ate the
                 * terminator: run-on text, g_Pix overflow, freeze). */
                if ((ch & 0xFC) == 0xF0 ||
                    (((ch >= 0x81 && ch <= 0x9F) ||
                      (ch >= 0xE0 && ch <= 0xEF)) &&
                     *str)) {
                    ch = (u8)*str++ | (ch << 8);
                } else if (ch >= '0' && ch <= '9') {
                    ch = ch - '0' + 0x824F; // ０-９
                } else if (ch == '.') {
                    ch = 0x8144; // ．
                } else if (ch == ',') {
                    ch = 0x8143; // ，
                } else if (ch == '?') {
                    ch = 0x8148; // ？
                } else if (ch == '!') {
                    ch = 0x8149; // ！
                } else if (ch == '(') {
                    ch = 0x8169; // （
                } else if (ch == ')') {
                    ch = 0x816A; // ）
                } else {
                    ch = DOUBLE_SPACE; // unknown single byte: blank
                }
#else
                ch = *str++ | (ch << 8);
#endif
                if (ch == DOUBLE_SPACE) {
                    s_8 = 2;
                }
            }
            if (ch == RIGHT_DOUBLE_QUOTATION_MARK) {
#ifdef MOD_KO
                /* Skip only a doubled ”” (retail data); a single ”
                 * before Hangul (KO data) must not eat the next glyph */
                if ((u8)str[0] == 0x81 && (u8)str[1] == 0x68)
#endif
                    str += 2;
            }
#ifdef MOD_KO
            /* 0.9.2: full-width digits / . ( ) of the description (retail Japanese ROM glyphs, bold grey) use
             * the Korean engtab cells (KO_ENGTAB 0x8009EF00, ascii - 0x21) like the dialogue; 0 = no cell -> keep */
            {
                u32 koAsc = (ch >= 0x824F && ch <= 0x8258) ? (ch - 0x824F + '0')
                          : ch == 0x8144 ? '.' : ch == 0x8169 ? '(' : ch == 0x816A ? ')' : 0;
                u16 koCell = koAsc ? ((const u16*)0x8009EF00)[koAsc - 0x21] : 0;
                chPix = (u8*)g_api.func_80106A28(koCell ? koCell : ch, 1);
            }
#else
            chPix = (u8*)g_api.func_80106A28(ch, 1);
#endif
#endif
#ifdef MOD_KO
            ko_trim = 0;
#endif
            while (true) {
                if (ch == DOUBLE_SPACE) {
                    break;
                }
#ifdef MOD_KO
                /* A blank glyph never gets ink into column 0 */
                if (++ko_trim > 12) {
                    break;
                }
#endif
                for (i = 0; i < FontHeight; i++) {
                    if (chPix[i * FontStride]) {
                        break;
                    }
                }
                if (i != 16) {
                    break;
                }
                for (i = 0; i < FontHeight; i++) {
                    ptr = &chPix[i * FontStride];
                    for (j = 0; j < 5; j++) {
                        *(ptr + 0) = *(ptr + 1);
                        ptr += 1;
                    }
                    *ptr = 0;
                }
            }

            for (i = 0, letterWidth = 0; i < FontHeight; i++) {
                for (j = 0; j < FontStride; j++) {
                    if (chPix[i * FontStride + j] && (letterWidth < j)) {
                        letterWidth = j;
                    }
                }
            }
            for (i = 0; i < FontHeight; i++) {
                if (chPix[letterWidth + i * FontStride] & 0xF0) {
                    break;
                }
            }

            if (i != 16) {
                letterWidth++;
            }
            if (letterWidth < 6) {
                letterWidth++;
            }
            for (i = 0; i < FontHeight; i++) {
                ptr = &pix[pos + (i + (s_6 * FontHeight)) * size];
                *ptr++ = *chPix++;
                *ptr++ = *chPix++;
                *ptr++ = *chPix++;
                *ptr++ = *chPix++;
                *ptr++ = *chPix++;
                *ptr++ = *chPix++;
            }
            pos += letterWidth + s_8;
            if (pos >= size - 5) {
                break;
            }
        }
        LoadTPage(
            (u_long*)&pix[s_6 * FontHeight * size], 0, 0, x, y, size * 2, 0x10);
        if (!*str) {
            break;
        }
#ifdef MOD_KO
        /* pix is cleared/sized for 2 lines; never write a 3rd */
        if (s_6 >= 1) {
            break;
        }
#endif
        s_6++;
#if defined(MOD_PTBR)
        y += FontHeight - 1;
#else
        y += FontHeight;
#endif
        if (*str < 8) {
            str++;
        }
    }
    return &pix[(size << 4) << 1];
}

void func_us_801B0FBC(const char* str, u16 x, u16 y) {
    RECT rect;
    char ch;
#ifdef MOD_KO
    u16 ko_ch;
    u_long* ko_glyph;
#endif

loop:
    ch = *str++;
    if (ch) {
        if (ch == ' ') {
            x++;
            goto loop;
#ifdef VERSION_PSP
        } else if (ch == 0xCC) {
            ch = 0xBB;
#endif
        }
#ifdef MOD_KO
        if (((u8)ch & 0xFC) == 0xF0) {
            ko_ch = ((u16)(u8)ch << 8) | (u8)*str++;
            ko_glyph = (u_long*)g_api.func_80106A28(ko_ch, 1);
            if (ko_glyph) {
                LoadTPage(ko_glyph, 0, 0, x, y, 0xC, 0x10);
                /* func_80106A28 returns ONE static buffer and
                 * LoadTPage only queues the copy: wait before the next glyph
                 * overwrites it (shop menu showed the last glyph in every cell) */
                DrawSync(0);
                x += 3;
                goto loop;
            }
        }
#endif
        rect.x = ((ch & 0x0F) << 1) + 0x380;
        rect.y = ((ch & 0xF0) >> 1) + 0xF0;
        rect.w = 2;
        rect.h = 8;
        MoveImage(&rect, x, y);
        x += 2;
        goto loop;
    }
}

#if defined(MOD_KO)
/* Hangul in the 8x8 SPRT lists (buy/sell, bestiary).
 * Chosen: the 12px func_80106A28 path, not the 8px KO_8P block: the 8px block
 * only holds the equipment-menu character set (371 cells, enemy names miss)
 * and is uploaded into dra menu sheet rows with dirty-signature state the
 * stage overlay cannot share. Each Hangul pair is uploaded (LoadTPage +
 * DrawSync, same as KO_PopGlyph) to a per-frame VRAM slot in the y=496 band
 * below both frames (the dra menu hook band, idle while the shop runs) and
 * drawn as ONE 12x16 SPRT; advance 12px like the menu hook / popup.
 * Slots: 21 per 64-hw texture page, pages 1..3 (x 64..252 hw, left of the
 * 256..511 list render target), 63 per frame; ring state = the menu hook's
 * KO_SLOT_TMR/IDX (dra.h), reset when g_Timer changes. */
#define KO_LIB_SLOT_TMR (*(volatile u32*)0x8009EEF8) // dra.h KO_SLOT_TMR
#define KO_LIB_SLOT_IDX (*(volatile u8*)0x8009EEFC)  // dra.h KO_SLOT_IDX
#define KO_LIB_SLOTS 63
#define KO_LIB_SLOT_Y 496
#define KO_LIB_PAGE(k) (1 + (k) / 21)
#define KO_LIB_COL(k) ((k) % 21)
/* abr bits 0x60 mark a KO glyph prim (ignored: DRAW_DEFAULT is opaque); no
 * LIB prim uses tpage 0x71-0x73, so KoLibPrimClean finds exactly these */
#define KO_LIB_TPAGE(k) (0x70 | KO_LIB_PAGE(k))
#define KO_LIB_GLYPH_DY 3 // 15-row cell vs 8-row ASCII: tune in emulator

/* The glyph ink is palette index 8 (func_80106A28, D_800AC934[16]); the shop
 * text palette 0x196 has the white ASCII ink at index 6 and a red at 8, so
 * the Korean names drew red. Remap 8 -> 6 here only (the table is shared
 * with the menu, where 8 is white). Measured 094_09: names (152,48,24) at 8,
 * (192,200,200) = the English digits at 6. */
#define KO_LIB_INK_FROM 8
#define KO_LIB_INK_TO 6

static s32 KoLibGlyphSlot(u16 code) {
    s32 k;
    s32 i, n;
    u16* gl;
    u16 v, w, nib;

    if (KO_LIB_SLOT_TMR != g_Timer) {
        KO_LIB_SLOT_TMR = g_Timer;
        KO_LIB_SLOT_IDX = 0;
    }
    k = KO_LIB_SLOT_IDX;
    if (k >= KO_LIB_SLOTS) {
        return -1; // full this frame: glyph skipped, x still advances
    }
    KO_LIB_SLOT_IDX = k + 1;
    gl = (u16*)g_api.func_80106A28(code, 0);
    for (i = 0; i < 48; i++) { // 12x16 4bpp = 48 halfwords
        v = gl[i];
        w = 0;
        for (n = 0; n < 16; n += 4) {
            nib = (v >> n) & 0xF;
            if (nib == KO_LIB_INK_FROM) {
                nib = KO_LIB_INK_TO;
            }
            w |= nib << n;
        }
        gl[i] = w;
    }
    LoadTPage((u_long*)gl, 0, 0, KO_LIB_PAGE(k) * 64 + KO_LIB_COL(k) * 3,
              KO_LIB_SLOT_Y, 12, 16);
    DrawSync(0); // one shared glyph buffer (see func_us_801B0FBC)
    return k;
}

/* The shop chains are set up once as 8x8 SPRTs on tpage 0x1E and callers
 * only rewrite clut/u0/v0/x0/y0. A prim that held a Hangul glyph last frame
 * may be a digit/cursor prim this frame (names shift on scroll), so every
 * call first restores marked prims from its start onward. */
static void KoLibPrimClean(Primitive* prim) {
    s32 n;

    for (n = 0; prim != NULL && n < 128; n++, prim = prim->next) {
        if (prim->tpage >= 0x71 && prim->tpage <= 0x73) {
            prim->tpage = 0x1E;
            prim->u1 = prim->v1 = 8;
        }
    }
}
#endif

Primitive* func_us_801B1064(
    Primitive* prim, s16 x, s16 y, const char* str, u16 clut) {
    char buffer[64];
    u16 length;
    s32 i;
    char ch;
    char* chPtr;
#if defined(MOD_KO)
    s32 ko_slot;
#endif

    chPtr = buffer;
    length = 0;
    for (i = 0; i < 64; i++) {
        *chPtr++ = 0;
    }
#if defined(MOD_KO)
    KoLibPrimClean(prim);
#endif
    chPtr = buffer;
    while (true) {
        ch = *str++;
        if (ch == 0xFF) {
            ch = *str++;
            if (ch == 0) {
                break;
            }
        }
#if defined(MOD_KO)
        /* Copy a Hangul pair whole (its 2nd byte may be FF or 00)
         * and stop before buffer[63] so an unterminated name cannot smash
         * the stack */
        if (((u8)ch & 0xFC) == 0xF0) {
            if (length >= 62) {
                break;
            }
            *chPtr++ = ch;
            *chPtr++ = *str++;
            length += 2;
            continue;
        }
        if (length >= 63) {
            break;
        }
#endif
        *chPtr++ = ch;
        length++;
#ifdef VERSION_PSP
        ch = *str;
        if (ch == 0xFF) {
            str++;
            ch = *str++;
            if (ch == 0) {
                break;
            }
            if (ch != 0xFF) {
                *chPtr = ch;
            } else {
                str -= 2;
            }
        }
        chPtr++;
#endif
    }
    chPtr = buffer;
    if (!length) {
        return prim;
    }
    do {
        ch = *chPtr++;
#ifdef VERSION_PSP
        if (ch) {
            prim->clut = clut;
            prim->u0 = (s8)((ch & 0x0F) << 3);
            prim->v0 = (s8)((ch & 0xF0) >> 1);
            prim->drawMode = DRAW_DEFAULT;
            prim->x0 = x;
            prim->y0 = y;
            prim = prim->next;
        }
        ch = *chPtr++;
        if (ch) {
            prim->clut = clut;
            prim->u0 = (s8)((ch & 0x0F) << 3);
            prim->v0 = (s8)((ch & 0xF0) >> 1);
            prim->drawMode = DRAW_DEFAULT;
            prim->x0 = x;
            prim->y0 = y - 8;
            prim = prim->next;
        }
        if (ch >= 0xF0 && ch <= 0xFC) {
            x += 12;
        } else {
            x += 8;
        }
#else
#if defined(MOD_KO)
        if (((u8)ch & 0xFC) == 0xF0 && length > 1) {
            /* Hangul pair -> one 12x16 glyph SPRT */
            ko_slot = KoLibGlyphSlot(((u16)(u8)ch << 8) | (u8)*chPtr);
            chPtr++;
            --length;
            if (ko_slot >= 0) {
                prim->clut = clut;
                prim->tpage = KO_LIB_TPAGE(ko_slot);
                prim->u0 = KO_LIB_COL(ko_slot) * 12;
                prim->v0 = KO_LIB_SLOT_Y - 256;
                prim->u1 = 12;
                prim->v1 = 16;
                prim->drawMode = DRAW_DEFAULT;
                prim->x0 = x;
                prim->y0 = y - KO_LIB_GLYPH_DY;
                prim = prim->next;
            }
            x += 12;
        } else
#endif
        if (ch) {
            prim->clut = clut;
            prim->u0 = (s8)((ch & 0x0F) << 3);
            prim->v0 = (s8)((ch & 0xF0) >> 1);
            prim->drawMode = DRAW_DEFAULT;
            if (ch == CH('i') || ch == CH('l') || ch == CH('f') ||
                ch == CH('I')) {
                x--;
            } else if (ch == CH('\'')) {
                x -= 2;
            }
            prim->x0 = x;
            prim->y0 = y;
            prim = prim->next;
            if (ch == CH('i') || ch == CH('l')) {
                x += 7;
            } else if (ch == CH('\'')) {
                x += 6;
            } else {
                x += 8;
            }
        } else {
            x += 4;
        }
#endif

    } while (--length);

    return prim;
}

void func_us_801B11A0(s16 x, s16 y, u16 w, u16 h) {
    RECT rect;

    rect.x = x / 4;
    rect.y = y;
    rect.w = w / 4 + 1;
    rect.h = h + 1;
    ClearImage(&rect, 0, 0, 0);
}

static void func_us_801B1200(Primitive* prim, Primitive* otherPrim) {
    prim->x0 = otherPrim->x0;
    prim->y0 = otherPrim->y0;
    prim->x1 = otherPrim->x1;
    prim->y1 = otherPrim->y1;
    prim->drawMode = DRAW_DEFAULT;

    prim = prim->next;
    prim->x0 = otherPrim->x1;
    prim->y0 = otherPrim->y1;
    prim->x1 = otherPrim->x3;
    prim->y1 = otherPrim->y3;
    prim->drawMode = DRAW_DEFAULT;

    prim = prim->next;
    prim->x0 = otherPrim->x0;
    prim->y0 = otherPrim->y0;
    prim->x1 = otherPrim->x2;
    prim->y1 = otherPrim->y2;
    prim->drawMode = DRAW_DEFAULT;

    prim = prim->next;
    prim->x0 = otherPrim->x2;
    prim->y0 = otherPrim->y2;
    prim->x1 = otherPrim->x3;
    prim->y1 = otherPrim->y3;
    prim->drawMode = DRAW_DEFAULT;
}

void func_us_801B12D0(Entity* self, u16 arg1) {
    Primitive* prim2;
    Primitive* prim;
    s16 s_4;
    s16 posY;
    s16 posX;
    s32 i;
    s16 s_3;
    s16 temp_v1;
    s16 var_s4;

    prim = &g_PrimBuf[g_CurrentEntity->primIndex];
    prim = prim->next;
    prim2 = prim;
    for (i = 0; i < 4; i++) {
        prim = prim->next;
    }
    i = 0;
    s_3 = g_CurrentEntity->ext.et_801B15C0.unk86;
    s_4 = g_CurrentEntity->ext.et_801B15C0.unk80 * 0x10;
#ifdef VERSION_PSP
    if (arg1) {
        posY = (g_CurrentEntity->ext.et_801B15C0.unk7C * ((s_3 + 0x18) - s_4)) /
               24;
        posX = (g_CurrentEntity->ext.et_801B15C0.unk7C * -0x88) / 24;
#else
    if (arg1 & 1) {
        posY = (g_CurrentEntity->ext.et_801B15C0.unk7C * ((s_3 + 0x18) - s_4)) /
               24;
        posX = (g_CurrentEntity->ext.et_801B15C0.unk7C * -0x98) / 24;
#endif
    } else {
        posY = (g_CurrentEntity->ext.et_801B15C0.unk7C * ((s_3 + 0x40) - s_4)) /
               24;
        posX = 0;
    }
#ifdef VERSION_PSP
    for (; prim != NULL; i++) {
        prim->x0 = prim->x2 = posX + 0x94;
        prim->x1 = prim->x3 = posX + 0xEC;
#else
    if (arg1 & 0x10) {
        s32 temp = g_CurrentEntity->ext.et_801B15C0.unk7C;
        var_s4 = temp / 3;
    } else {
        var_s4 = 0;
    }
    for (; prim != NULL; i++) {
        prim->x0 = prim->x2 = posX + 0x96 + var_s4;
        prim->x1 = prim->x3 = posX + 0xEA - var_s4;
        if (i != 0) {
            prim->u0 = prim->u2 = var_s4;
            prim->u1 = prim->u3 = 0x54 - var_s4;
        }
#endif
        temp_v1 = (g_CurrentEntity->ext.et_801B15C0.unk7C * (s_4 + 2)) / 24;
        if (i != 0) {
            prim->v0 = prim->v1 = temp_v1;
        }
        prim->y0 = prim->y1 = (0x60 - s_3) + temp_v1 + posY;
        temp_v1 = (g_CurrentEntity->ext.et_801B15C0.unk7C *
                   (s_3 * 2 - (s_4 + 0x15))) /
                  24;
        if (i != 0) {
            prim->v2 = prim->v3 = s_3 * 2 - temp_v1;
        }
        prim->y2 = prim->y3 = (s_3 + 0x60) - temp_v1 + posY;
        if (i == 0) {
            func_us_801B1200(prim2, prim);
        }
        prim = prim->next;
    }
}

void func_us_801B15C0(Entity* self) {
    Entity* tempEntity;
    Primitive* otherPrim;
    Primitive* prim;
    s32 primIndex;
    s16 tempVar;
    s32 i;
    u16 tempVar2;
    const char* ptr;
    s16 pad;

#if defined(MOD_PTBR)
    ModPtbrLibText();
#endif
    switch (self->step) {
    case 0:
        InitializeEntity(g_EInitCommon);
        primIndex = g_api.AllocPrimitives(PRIM_G4, 7);
        if (primIndex != -1) {
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = primIndex;
            if (g_GameClearFlag) {
#ifdef VERSION_PSP
                self->params = 3;
#else
                self->params = 2;
#endif
            } else {
                self->params = 1;
            }
            ptr = D_us_80181340[self->params];
            self->ext.et_801B15C0.unk84 = *ptr++;
            for (i = 0; i < self->ext.et_801B15C0.unk84; i++) {
                self->ext.et_801B15C0.unk88[i] = *ptr++;
            }
            self->ext.et_801B15C0.unk86 = (self->ext.et_801B15C0.unk84 * 8) + 4;
            i = 0;
            prim = &g_PrimBuf[primIndex];
            for (; prim != NULL; i++) {
                prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0xC0;
                prim->y0 = prim->y1 = prim->y2 = prim->y3 = 0x60;
                if (i == 0) {
                    PRED(prim) = 0x40;
                    PGRN(prim) = 0x10;
                    PBLU(prim) = 0x10;
#ifdef VERSION_PSP
                    prim->x0 = prim->x2 = 0x95;
                    prim->x1 = prim->x3 = 0xEC;
#else
                    prim->x0 = prim->x2 = 0x98;
                    prim->x1 = prim->x3 = 0xE8;
#endif
                    prim->priority = 0xC0;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 5) {
                    prim->type = PRIM_LINE_G2;
                    prim->r0 = prim->r1 = 0x80;
                    prim->g0 = prim->g1 = 0x80;
                    prim->b0 = prim->b1 = 0x80;
                    prim->priority = 0xC1;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 5) {
                    PRED(prim) = 0;
                    PGRN(prim) = 0;
                    PBLU(prim) = 0x40;
                    prim->priority = 0xBF;
                    prim->drawMode = DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                } else if (i == 6) {
                    prim->type = PRIM_GT4;
                    prim->tpage = 0x10;
                    prim->clut = PAL_UNK_1A1;
                    prim->u0 = prim->u2 = 0;
#ifdef VERSION_PSP
                    prim->u1 = prim->u3 = 0x58;
#else
                    prim->u1 = prim->u3 = 0x54;
#endif
                    prim->v0 = prim->v1 = 0;
                    prim->v2 = prim->v3 = self->ext.et_801B15C0.unk86 * 2;
                    prim->priority = 0xC1;
                    prim->drawMode = DRAW_HIDE;
                }
                prim = prim->next;
            }
#ifdef VERSION_PSP
            func_us_801B11A0(0, 0x100, 0x58, self->ext.et_801B15C0.unk86 * 2);
#else
            func_us_801B11A0(0, 0x100, 0x54, self->ext.et_801B15C0.unk86 * 2);
#endif
            return;
        }
        self->step = 0;
        break;

    case 1:
        for (i = 0; i < self->ext.et_801B15C0.unk84; i++) {
            ptr = D_us_80181310[self->ext.et_801B15C0.unk88[i]];
            tempVar = *ptr++;
            func_us_801B0FBC(ptr, tempVar, 0x108 + i * 16);
        }
        self->step++;
        break;

    case 2:
        if (g_CutsceneFlags & 0x200) {
            SetStep(3);
            self->ext.et_801B15C0.unk7C = 0;
            self->ext.et_801B15C0.unk7E = 0;
            self->ext.et_801B15C0.unk80 = 0;
            g_Settings.D_8003CB04 |= 8;
        }
        break;

    case 3:
        self->ext.et_801B15C0.unk7C++;
        self->ext.et_801B15C0.unk7E += 0x40;
        prim = &g_PrimBuf[self->primIndex];
        prim = prim->next;
        otherPrim = prim;
        for (i = 0; i < 4; i++) {
            prim = prim->next;
        }
        i = 0;
        tempVar2 = self->ext.et_801B15C0.unk86;
        for (; prim != NULL; i++) {
#ifdef VERSION_PSP
            tempVar = (self->ext.et_801B15C0.unk7C * 0x2C) / 0x20;
            if (tempVar > 0x2C) {
                tempVar = 0x2C;
            }
#else
            tempVar = (self->ext.et_801B15C0.unk7C * 0x2A) / 0x20;
            if (tempVar > 0x2A) {
                tempVar = 0x2A;
            }
#endif
            prim->x1 = tempVar + 0xC0;
            prim->x2 = 0xC0 - tempVar;
            tempVar = self->ext.et_801B15C0.unk7C * tempVar2 / 0x20;
            if (tempVar > tempVar2) {
                tempVar = tempVar2;
            }
            prim->y1 = 0x60 - tempVar;
            prim->y2 = tempVar + 0x60;
#ifdef VERSION_PSP
            tempVar = ((s16)self->ext.et_801B15C0.unk7C * 0x2C) / 0x40;
#else
            tempVar = ((s16)self->ext.et_801B15C0.unk7C * 0x2A) / 0x40;
#endif
            tempVar = (rcos(self->ext.et_801B15C0.unk7E) * tempVar) / 0x1000;
            prim->x0 = 0xC0 - tempVar;
            prim->x3 = tempVar + 0xC0;
            tempVar = ((s16)self->ext.et_801B15C0.unk7C * tempVar2) / 0x40;
            tempVar = (rcos(self->ext.et_801B15C0.unk7E) * tempVar) / 0x1000;
            prim->y0 = 0x60 - tempVar;
            prim->y3 = tempVar + 0x60;
            if (self->ext.et_801B15C0.unk7E >= 0x400 &&
                self->ext.et_801B15C0.unk7E < 0xC00) {
                if (i) {
                    prim->priority = 0xBF;
                    prim->drawMode = DRAW_DEFAULT;
                } else {
                    prim->priority = 0xC1;
                }
            } else {
                if (i) {
                    prim->priority = 0xC1;
                } else {
                    prim->priority = 0xBF;
                }
            }
            if (i == 0) {
                func_us_801B1200(otherPrim, prim);
            }
            prim = prim->next;
        }
        if (self->ext.et_801B15C0.unk7C == 0x40) {
            SetStep(4);
        }
        break;

    case 4:
        pad = g_pads[0].repeat;
        if (pad & PAD_DOWN) {
            if (self->ext.et_801B15C0.unk80 < self->ext.et_801B15C0.unk84 - 1) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B15C0.unk80++;
            }
        } else if (pad & PAD_UP) {
            if (self->ext.et_801B15C0.unk80 > 0) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B15C0.unk80--;
            }
        }
        pad = g_pads[0].tapped;
        if (pad & BUTTON_CONFIRM) {
            switch (self->ext.et_801B15C0.unk88[self->ext.et_801B15C0.unk80]) {
            case 0:
                g_CutsceneFlags |= 0x400;
                tempEntity = self + 1;
                CreateEntityFromCurrentEntity(E_ID(ID_27), tempEntity);
                tempEntity++;
                CreateEntityFromCurrentEntity(E_ID(ID_28), tempEntity);
                tempEntity++;
                CreateEntityFromCurrentEntity(E_ID(ID_29), tempEntity);
                tempEntity++;
                CreateEntityFromCurrentEntity(E_ID(ID_2A), tempEntity);
                SetStep(5);
                g_api.PlaySfx(SFX_UI_CONFIRM);
                break;

            case 1:
                g_CutsceneFlags |= 0x400;
                tempEntity = self + 1;
                CreateEntityFromCurrentEntity(E_ID(ID_2D), tempEntity);
                SetStep(5);
                g_api.PlaySfx(SFX_UI_CONFIRM);
                break;

            case 2:
                g_CutsceneFlags |= 0x400;
                tempEntity = self + 1;
                CreateEntityFromCurrentEntity(E_ID(ID_2E), tempEntity);
                SetStep(5);
                g_api.PlaySfx(SFX_UI_CONFIRM);
                break;

            case 3:
                g_CutsceneFlags |= 0x400;
                tempEntity = self + 1;
                CreateEntityFromCurrentEntity(E_ID(ID_48), tempEntity);
                SetStep(5);
                g_api.PlaySfx(SFX_UI_CONFIRM);
                break;

            case 4:
                SetStep(6);
                g_api.PlaySfx(SFX_UI_CONFIRM);
                break;

            case 5:
                g_CutsceneFlags |= 0x400;
                tempEntity = self + 1;
                CreateEntityFromCurrentEntity(E_ID(ID_27), tempEntity);
                tempEntity->params = 1;
                tempEntity++;
                CreateEntityFromCurrentEntity(E_ID(ID_28), tempEntity);
                tempEntity++;
                CreateEntityFromCurrentEntity(E_ID(ID_29), tempEntity);
                tempEntity++;
                CreateEntityFromCurrentEntity(E_ID(ID_2A), tempEntity);
                SetStep(5);
                g_api.PlaySfx(SFX_UI_CONFIRM);
                break;

#ifdef VERSION_PSP
            case 6:
                g_CutsceneFlags |= 0x400;
                tempEntity = self + 1;
                CreateEntityFromCurrentEntity(E_ID(ID_4F), tempEntity);
                SetStep(5);
                g_api.PlaySfx(SFX_UI_CONFIRM);
                break;
#endif
            }
        } else if (g_pads[0].tapped & BUTTON_CANCEL) {
            SetStep(6);
            g_api.PlaySfx(SFX_UI_CONFIRM);
        }
        prim = &g_PrimBuf[self->primIndex];
        prim->y0 = prim->y1 = (0x64 - self->ext.et_801B15C0.unk86) +
                              self->ext.et_801B15C0.unk80 * 0x10;
        prim->y2 = prim->y3 = prim->y0 + 0x10;
        prim->drawMode = DRAW_COLORS;
        break;

    case 5:
        switch (self->step_s) {
        case 0:
            if (g_api.func_80131F68()) {
                g_api.PlaySfx(SET_STOP_MUSIC);
                self->step_s++;
            } else {
                self->step_s++;
            }
            break;

        case 1:
            if (!g_api.func_80131F68()) {
                if (self->ext.et_801B15C0.unk80 != 4) {
                    g_api.PlaySfx(NA_VO_AL_INTERESTED);
                }
                if (!self->ext.et_801B15C0.unk80 ||
                    self->ext.et_801B15C0.unk80 == 1) {
                    self->ext.et_801B15C0.unk82 = 1;
                } else {
                    self->ext.et_801B15C0.unk82 = 0;
                }
#ifndef VERSION_PSP
                if (self->ext.et_801B15C0.unk80 < 3) {
                    self->ext.et_801B15C0.unk82 |= 0x10;
                }
#endif
                SetStep(9);
            }
            break;
        }
        break;

    case 6:
        switch (self->step_s) {
        case 0:
#ifdef VERSION_PSP
            *cutscene_data_offset_four = 0;
#else
            D_us_80183F64 = 0;
#endif
            if (g_api.func_80131F68()) {
                g_api.PlaySfx(SET_STOP_MUSIC);
                self->step_s++;
            } else {
                self->step_s++;
            }
            break;

        case 1:
            if (!g_api.func_80131F68()) {
                g_api.PlaySfx(NA_VO_ML_FAREWELL);
                self->ext.et_801B15C0.unk7C = 0x20;
                self->step_s++;
            }
            break;

        case 2:
            if (!--self->ext.et_801B15C0.unk7C) {
                SetStep(7);
                g_CutsceneFlags |= 0x100;
                g_PauseAllowed = true;
                g_unkGraphicsStruct.pauseEnemies = 0;
            }
            break;
        }
        break;

    case 7:
        prim = &g_PrimBuf[self->primIndex];
        prim->drawMode = DRAW_HIDE;
        self->ext.et_801B15C0.unk7E = 0;
        self->ext.et_801B15C0.unk7C = 0x40;
        self->step++;
        break;

    case 8:
        self->ext.et_801B15C0.unk7C--;
        self->ext.et_801B15C0.unk7E += 0x40;
        prim = &g_PrimBuf[self->primIndex];
        prim = prim->next;
        otherPrim = prim;
        for (i = 0; i < 4; i++) {
            prim = prim->next;
        }
        i = 0;
        tempVar2 = self->ext.et_801B15C0.unk86;
        for (; prim != NULL; i++) {
#ifdef VERSION_PSP
            tempVar = (self->ext.et_801B15C0.unk7C * 0x2C) / 0x20;
            if (tempVar > 0x2C) {
                tempVar = 0x2C;
            }
#else
            tempVar = (self->ext.et_801B15C0.unk7C * 0x2A) / 0x20;
            if (tempVar > 0x2A) {
                tempVar = 0x2A;
            }
#endif
            prim->x3 = tempVar + 0xC0;
            prim->x0 = 0xC0 - tempVar;
            tempVar = (self->ext.et_801B15C0.unk7C * tempVar2) / 0x20;
            if (tempVar > tempVar2) {
                tempVar = tempVar2;
            }
            prim->y0 = 0x60 - tempVar;
            prim->y3 = tempVar + 0x60;
#ifdef VERSION_PSP
            tempVar = ((s16)self->ext.et_801B15C0.unk7C * 0x2C) / 0x40;
#else
            tempVar = ((s16)self->ext.et_801B15C0.unk7C * 0x2A) / 0x40;
#endif
            tempVar = rcos(self->ext.et_801B15C0.unk7E) * tempVar / 0x1000;
            prim->x2 = 0xC0 - tempVar;
            prim->x1 = tempVar + 0xC0;
            tempVar = ((s16)self->ext.et_801B15C0.unk7C * tempVar2) / 0x40;
            tempVar = rcos(self->ext.et_801B15C0.unk7E) * tempVar / 0x1000;
            prim->y1 = 0x60 - tempVar;
            prim->y2 = tempVar + 0x60;
            if (self->ext.et_801B15C0.unk7E >= 0x400 &&
                self->ext.et_801B15C0.unk7E < 0xC00) {
                if (i) {
                    prim->priority = 0xBF;
                } else {
                    prim->priority = 0xC1;
                }
            } else if (i) {
                prim->priority = 0xC1;
                if (self->ext.et_801B15C0.unk7E >= 0xC00) {
                    prim->drawMode = DRAW_HIDE;
                }
            } else {
                prim->priority = 0xBF;
            }
            if (i == 0) {
                if (self->ext.et_801B15C0.unk7C) {
                    func_us_801B1200(otherPrim, prim);
                } else {
                    otherPrim->drawMode = DRAW_HIDE;
                    otherPrim = otherPrim->next;
                    otherPrim->drawMode = DRAW_HIDE;
                    otherPrim = otherPrim->next;
                    otherPrim->drawMode = DRAW_HIDE;
                    otherPrim = otherPrim->next;
                    otherPrim->drawMode = DRAW_HIDE;
                }
            }
            prim = prim->next;
        }
        if (!self->ext.et_801B15C0.unk7C) {
            g_CutsceneFlags &= ~0x200;
            SetStep(2);
        }
        break;

    case 9:
        prim = &g_PrimBuf[self->primIndex];
        prim->drawMode = DRAW_HIDE;
        self->ext.et_801B15C0.unk7C = 0;
        self->step++;
        break;

    case 10:
        self->ext.et_801B15C0.unk7C++;
        func_us_801B12D0(self, self->ext.et_801B15C0.unk82);
        if (self->ext.et_801B15C0.unk7C >= 0x18) {
            g_CutsceneFlags &= ~0x200;
            SetStep(11);
        }
        break;

    case 11:
        if (g_CutsceneFlags & 0x200) {
            SetStep(12);
            self->ext.et_801B15C0.unk7C = 0x18;
        }
        break;

    case 12:
        self->ext.et_801B15C0.unk7C--;
        func_us_801B12D0(self, self->ext.et_801B15C0.unk82);
        if (!self->ext.et_801B15C0.unk7C) {
            SetStep(4);
        }
        break;
    }
}

void func_us_801B245C(Primitive* arg0, u16 arg1, u16 arg2, u16 arg3, u16 arg4,
                      s32 arg5, s32 arg6) {
    Primitive* prim;
    s16 i;
    s16 offset;

    prim = arg0;
    for (i = 0; i < 6; i++) {
        arg0 = arg0->next;
    }
    for (i = 0; i < 2; i++) {
        offset = (rsin(arg1) * arg3) / 0x1000;
        if (i) {
            arg0->x1 = arg0->x3 = arg0->x0 + offset;
            if (arg5 & 1) {
                arg0->r0 = arg0->r1 = (arg2 / 2) + 0x30;
            } else {
                arg0->r0 = arg0->r1 = 0;
            }
            if (arg5 & 2) {
                arg0->r2 = arg0->r3 = (arg2 / 2) + 0x30;
            } else {
                arg0->r2 = arg0->r3 = 0;
            }
            if (arg5 & 4) {
                arg0->g0 = arg0->g1 = (arg2 / 2) + 0x30;
            } else {
                arg0->g0 = arg0->g1 = 0;
            }
            if (arg5 & 8) {
                arg0->g2 = arg0->g3 = (arg2 / 2) + 0x30;
            } else {
                arg0->g2 = arg0->g3 = 0;
            }
            if (arg5 & 0x10) {
                arg0->b0 = arg0->b1 = (arg2 / 2) + 0x30;
            } else {
                arg0->b0 = arg0->b1 = 0;
            }
            if (arg5 & 0x20) {
                arg0->b2 = arg0->b3 = (arg2 / 2) + 0x30;
            } else {
                arg0->b2 = arg0->b3 = 0;
            }
        } else {
            arg0->x0 = arg0->x2 = arg0->x1 - offset;
            if (arg5 & 1) {
                arg0->r0 = arg0->r1 = arg2 + 0x20;
            } else {
                arg0->r0 = arg0->r1 = 0;
            }
            if (arg5 & 2) {
                arg0->r2 = arg0->r3 = arg2 + 0x20;
            } else {
                arg0->r2 = arg0->r3 = 0;
            }
            if (arg5 & 4) {
                arg0->g0 = arg0->g1 = arg2 + 0x20;
            } else {
                arg0->g0 = arg0->g1 = 0;
            }
            if (arg5 & 8) {
                arg0->g2 = arg0->g3 = arg2 + 0x20;
            } else {
                arg0->g2 = arg0->g3 = 0;
            }
            if (arg5 & 0x10) {
                arg0->b0 = arg0->b1 = arg2 + 0x20;
            } else {
                arg0->b0 = arg0->b1 = 0;
            }
            if (arg5 & 0x20) {
                arg0->b2 = arg0->b3 = arg2 + 0x20;
            } else {
                arg0->b2 = arg0->b3 = 0;
            }
        }
        offset = (arg4 * rcos(g_CurrentEntity->ext.et_801B6F30.unk7E)) / 0x1000;
        if (i) {
            arg0->y1 = arg0->y0 - offset;
            arg0->y3 = arg0->y2 + offset;
            prim->x0 = arg0->x1;
            prim->y0 = arg0->y1;
            prim->x1 = arg0->x3;
            prim->y1 = arg0->y3;
            prim->drawMode = DRAW_DEFAULT;
            prim = prim->next;
        } else {
            arg0->y0 = arg0->y1 - offset;
            arg0->y2 = arg0->y3 + offset;
            prim->x0 = arg0->x0;
            prim->y0 = arg0->y0;
            prim->x1 = arg0->x2;
            prim->y1 = arg0->y2;
            prim->drawMode = DRAW_DEFAULT;
            prim = prim->next;
        }
        prim->x0 = arg0->x0;
        prim->y0 = arg0->y0;
        prim->x1 = arg0->x1;
        prim->y1 = arg0->y1;
        prim->drawMode = DRAW_DEFAULT;
        prim = prim->next;

        prim->x0 = arg0->x2;
        prim->y0 = arg0->y2;
        prim->x1 = arg0->x3;
        prim->y1 = arg0->y3;
        prim->drawMode = DRAW_DEFAULT;
        prim = prim->next;

        if (arg6) {
            arg0->drawMode =
                DRAW_DITHERING | DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
        } else {
            arg0->drawMode = DRAW_DITHERING | DRAW_COLORS;
        }
        arg0 = arg0->next;
    }
    for (i = 0; i < 2; i++) {
        offset = (rsin(arg1) * (arg3 - 8)) / 0x1000;
        if (i) {
            arg0->x1 = arg0->x3 = arg0->x0 + offset;
        } else {
            arg0->x0 = arg0->x2 = arg0->x1 - offset;
        }
        offset = (arg4 * rcos(arg1) * 3) / 0x4000;
        if (i) {
            arg0->y1 = arg0->y0 - offset;
            arg0->y3 = arg0->y2 + offset;
            PRED(arg0) = arg2 + 0x60;
            PGRN(arg0) = arg2 + 0x60;
            PBLU(arg0) = arg2 + 0x60;
        } else {
            arg0->y0 = arg0->y1 - offset;
            arg0->y2 = arg0->y3 + offset;
            PRED(arg0) = (arg2 * 2) + 0x40;
            PGRN(arg0) = (arg2 * 2) + 0x40;
            PBLU(arg0) = (arg2 * 2) + 0x40;
        }
#ifdef VERSION_PSP
        if (arg1 == 0x400) {
            if (i) {
                arg0->x1 = arg0->x3 = arg0->x0 + (arg0->u1 - arg0->u0);
            } else {
                arg0->x0 = arg0->x2 = arg0->x1 - (arg0->u1 - arg0->u0);
            }
            arg0->y2 = arg0->y3 = arg0->y0 + (arg0->v2 - arg0->v0);
        }
        arg0->drawMode = DRAW_DEFAULT;
#else
        arg0->drawMode = DRAW_COLORS;
#endif
        arg0 = arg0->next;
    }
}

u16 func_us_801B29C4(void) {
    InventoryItem* currentItem;
    ShopItem* shopItem;
    u8 itemLevel;
    u16 unlockLevel;
    u8 itemCount;
    s32 i;
    u8 spellIndex;

    shopItem = D_us_801D4364;
    currentItem = D_us_8018134C;
    unlockLevel = 0;
    if (g_CastleFlags[MET_MARIA_AFTER_HIPPOGRYPH]) {
        unlockLevel++;
    }
    if (g_CastleFlags[RICHTER_CS_AFTER_M_AND_W]) {
        unlockLevel++;
    }
    if (g_Status.relics[RELIC_SOUL_OF_BAT] & 1) {
        unlockLevel++;
    }
    if (g_CastleFlags[MET_MARIA_IN_CEN]) {
        unlockLevel++;
    }
    if (g_CastleFlags[INVERTED_CASTLE_UNLOCKED]) {
        unlockLevel++;
    }
    if (g_CastleFlags[DEATH_FIGHT_CS]) {
        unlockLevel++;
    }
    if (g_api.TimeAttackController(
            TIMEATTACK_EVENT_GALAMOTH_DEFEAT, TIMEATTACK_GET_RECORD)) {
        unlockLevel++;
    }
    if (g_GameClearFlag) {
        unlockLevel = 8;
    }
    itemCount = 0;
    for (i = 0; i < LEN(D_us_8018134C) - 1; i++) {
        itemLevel = currentItem->unlockLevel;
        switch (itemLevel) {
        case 0xFF:
            if ((g_Status.relics[RELIC_JEWEL_OF_OPEN] & 1) == 0) {
                itemLevel = 0;
            }
            break;

        case 0x80:
            if (!g_CastleFlags[BOUGHT_CASTLE_MAP]) {
                itemLevel = 0;
            }
            break;

#ifdef VERSION_PSP
        case 0x86:
        case 0x87:
        case 0x88:
            if (itemLevel == 0x86) {
                if ((g_Status.relics[RELIC_SOUL_OF_WOLF] & 1) == 0) {
                    itemLevel = 0xFF;
                    break;
                }
                if ((g_Status.relics[RELIC_SKILL_OF_WOLF] & 1) == 0) {
                    itemLevel = 0xFF;
                    break;
                }
            } else if (itemLevel == 0x87) {
                if ((g_Status.relics[RELIC_SOUL_OF_BAT] & 1) == 0) {
                    itemLevel = 0xFF;
                    break;
                }
            } else if (itemLevel == 0x88) {
                if ((g_Status.relics[RELIC_SWORD_CARD] & 1) == 0) {
                    itemLevel = 0xFF;
                    break;
                }
                if (g_Status.statsFamiliars[FAM_STATS_SWORD].level < 50) {
                    itemLevel = 0xFF;
                    break;
                }
            }
#endif
        case 0x81:
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
            itemLevel -= 0x81;
            spellIndex = D_us_80181510[itemLevel];
            if (g_Status.spellsLearnt & (1 << spellIndex)) {
                itemLevel = 0xFF;
            } else {
                itemLevel = D_us_8018151C[itemLevel];
            }
            break;
        }
        if (itemLevel <= unlockLevel) {
            shopItem->category = currentItem->category;
            shopItem->itemId = currentItem->itemId;
            shopItem->price = currentItem->price;
            shopItem++;
            itemCount++;
        }
        currentItem++;
    }
    return itemCount - 7;
}

void func_us_801B2BE4(Entity* self) {
    Primitive* prim;
    s32 i;
    s16 itemId;
    s16 tempVar;
    ShopItem* ptr;
    s32 primIndex;

#ifndef VERSION_PSP
    if (g_CastleFlags[MET_MARIA_AFTER_HIPPOGRYPH]) {
        FntPrint("HIP ");
    }
    if (g_CastleFlags[RICHTER_CS_AFTER_M_AND_W]) {
        FntPrint("ARE ");
    }
    if (g_Status.relics[RELIC_SOUL_OF_BAT] & RELIC_FLAG_FOUND) {
        FntPrint("BAT ");
    }
    if (g_CastleFlags[MET_MARIA_IN_CEN]) {
        FntPrint("CEN ");
    }
    if (g_CastleFlags[INVERTED_CASTLE_UNLOCKED]) {
        FntPrint("REV ");
    }
    if (g_CastleFlags[DEATH_FIGHT_CS]) {
        FntPrint("DET ");
    }
    if (g_api.TimeAttackController(
            TIMEATTACK_EVENT_GALAMOTH_DEFEAT, TIMEATTACK_GET_RECORD)) {
        FntPrint("GAR ");
    }
    FntPrint("\n");
#endif

    switch (self->step) {
    case 0:
        primIndex = g_api.AllocPrimitives(PRIM_G4, 0xD);
        if (primIndex != -1) {
            InitializeEntity(g_EInitCommon);
            i = 0;
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = primIndex;
            prim = &g_PrimBuf[primIndex];
            while (prim != NULL) {
                if (i < 6) {
                    prim->type = PRIM_LINE_G2;
                    prim->r0 = prim->r1 = 0x80;
                    prim->g0 = prim->g1 = 0x80;
                    prim->b0 = prim->b1 = 0x80;
                    prim->priority = 0x1FC;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 8) {
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0x80;
                    prim->y0 = prim->y1 = 0x18;
                    prim->y2 = prim->y3 = 0x78;
                    prim->priority = 0x1FA;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 10) {
                    prim->type = PRIM_GT4;
#ifdef VERSION_PSP
                    prim->tpage = 0x110;
#else
                    prim->tpage = 0x114;
#endif
                    prim->u0 = prim->u2 = (i - 8) * 0x6C + 8;
                    prim->u1 = prim->u3 = prim->u0 + 0x6C;
                    prim->v0 = prim->v1 = 0xD;
                    prim->v2 = prim->v3 = 0x61;
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0x80;
                    prim->y0 = prim->y1 = 0x1E;
                    prim->y2 = prim->y3 = 0x72;
                    prim->priority = 0x1FC;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 10) {
                    prim->x0 = prim->x2 = 0x11;
                    prim->x1 = prim->x3 = 0xEF;
                    prim->y0 = prim->y1 = 0x2B;
                    prim->y2 = prim->y3 = 0x37;
                    PRED(prim) = 0x40;
                    PGRN(prim) = 0x10;
                    PBLU(prim) = 0x10;
                    prim->priority = 0x1FB;
                    prim->drawMode = DRAW_HIDE;
                } else {
                    prim->type = PRIM_GT4;
                    prim->tpage = 0x1E;
                    prim->clut = 0x17F;
                    prim->u0 = prim->u2 = 0x58;
                    prim->u1 = prim->u3 = 0x60;
                    prim->v0 = prim->v1 = (i - 11) * 8 + 0x70;
                    prim->v2 = prim->v3 = 0x78 - (i - 11) * 8;
                    prim->x0 = prim->x2 = 0x7C;
                    prim->x1 = prim->x3 = prim->x0 + 8;
                    prim->y0 = prim->y1 = (i - 11) * 0x60 + 0x14;
                    prim->y2 = prim->y3 = prim->y0 + 8;
                    prim->priority = 0x1FC;
                    prim->drawMode = DRAW_HIDE;
                }
                prim = prim->next;
                i++;
            }
        }
        break;

    case 1:
        if (g_CutsceneFlags & 0x400) {
            SetStep(2);
            if (self->params) {
                // if showing the selling menu
                ptr = D_us_801814D8;
            } else {
                // if showing the buying menu
                self->ext.et_801B6F30.unk88 = func_us_801B29C4();
                ptr = D_us_801D4364;
            }
            for (i = 0; i < LEN(D_us_801D425C); i++) {
                itemId = ptr->itemId;
                switch (ptr->category) {
                case INVENTORY_HAND:
                    tempVar = g_Status.equipHandCount[itemId];
                    if (g_Status.equipment[LEFT_HAND_SLOT] == itemId) {
                        tempVar++;
                    }
                    if (g_Status.equipment[RIGHT_HAND_SLOT] == itemId) {
                        tempVar++;
                    }
                    D_us_801D425C[i] = 99 - tempVar;
                    break;

                case INVENTORY_HEAD:
                    tempVar = g_Status.equipBodyCount[itemId];
                    if (g_Status.equipment[HEAD_SLOT] == itemId) {
                        tempVar++;
                    }
                    D_us_801D425C[i] = 99 - tempVar;
                    break;

                case INVENTORY_BODY:
                    tempVar = g_Status.equipBodyCount[itemId];
                    if (g_Status.equipment[ARMOR_SLOT] == itemId) {
                        tempVar++;
                    }
                    D_us_801D425C[i] = 99 - tempVar;
                    break;

                case INVENTORY_CAPE:
                    tempVar = g_Status.equipBodyCount[itemId];
                    if (g_Status.equipment[CAPE_SLOT] == itemId) {
                        tempVar++;
                    }
                    D_us_801D425C[i] = 99 - tempVar;
                    break;

                case INVENTORY_ACCESSORY:
                    tempVar = g_Status.equipBodyCount[itemId];
                    if (self->params) {
                        D_us_801D425C[i] = tempVar;
                    } else {
                        if (g_Status.equipment[ACCESSORY_1_SLOT] == itemId) {
                            tempVar++;
                        }
                        if (g_Status.equipment[ACCESSORY_2_SLOT] == itemId) {
                            tempVar++;
                        }
                        D_us_801D425C[i] = 99 - tempVar;
                        break;
                    }
                    break;

                case INVENTORY_RELIC:
                    itemId = D_us_801814D4[itemId];
                    if (g_Status.relics[itemId] & RELIC_FLAG_FOUND) {
                        D_us_801D425C[i] = 0;
                    } else {
                        D_us_801D425C[i] = 1;
                    }
                    break;

                default:
                    D_us_801D425C[i] = 1;
                    break;
                }
                if (D_us_801D425C[i]) {
                    D_us_801D415C[i] = 1;
                } else {
                    D_us_801D415C[i] = 0;
                }
                ptr++;
            }
            self->ext.et_801B6F30.unk7C = 0;
            self->ext.et_801B6F30.unk7E = 0;
            self->ext.et_801B6F30.unk80 = 0;
            self->ext.et_801B6F30.unk82 = 0;
        }
        break;

    case 2:
        self->ext.et_801B6F30.unk7C++;
        self->ext.et_801B6F30.unk7E += 0x40;
        prim = &g_PrimBuf[self->primIndex];
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x74, 0x20, 8, 1);
        if (self->ext.et_801B6F30.unk7C == 0x10) {
            if (self->params) {
                SetStep(5);
            } else {
                SetStep(3);
            }
        }
        break;

    case 3:
        tempVar = g_pads[0].repeat;
        FntPrint("kosuu %x\n", self->ext.et_801B6F30.unk82);
        if (tempVar & PAD_DOWN) {
            if (self->ext.et_801B6F30.unk80 < 6) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80++;
            } else if (
                self->ext.et_801B6F30.unk82 < self->ext.et_801B6F30.unk88) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk82++;
            }
        } else if (tempVar & PAD_UP) {
            if (self->ext.et_801B6F30.unk80 > 0) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80--;
            } else if (self->ext.et_801B6F30.unk82) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk82--;
            }
        } else if (tempVar & (PAD_R1 | PAD_R2)) {
            if (self->ext.et_801B6F30.unk80 < 6) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80 = 6;
            } else if (
                self->ext.et_801B6F30.unk82 < self->ext.et_801B6F30.unk88) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk82 += 7;
                if (self->ext.et_801B6F30.unk82 > self->ext.et_801B6F30.unk88) {
                    self->ext.et_801B6F30.unk82 = self->ext.et_801B6F30.unk88;
                }
            }
        } else if (tempVar & (PAD_L1 | PAD_L2)) {
            if (self->ext.et_801B6F30.unk80 > 0) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80 = 0;
            } else if (self->ext.et_801B6F30.unk82) {
                g_api.PlaySfx(SFX_UI_MOVE);
                if (self->ext.et_801B6F30.unk82 > 7) {
                    self->ext.et_801B6F30.unk82 -= 7;
                } else {
                    self->ext.et_801B6F30.unk82 = 0;
                }
            }
        }
        itemId = self->ext.et_801B6F30.unk82 + self->ext.et_801B6F30.unk80;
        if (tempVar & PAD_RIGHT) {
            if (D_us_801D415C[itemId] < D_us_801D425C[itemId]) {
                g_api.PlaySfx(SFX_UI_MOVE);
                D_us_801D415C[itemId]++;
            }
        } else if (tempVar & PAD_LEFT) {
            if (D_us_801D415C[itemId] > 1) {
                g_api.PlaySfx(SFX_UI_MOVE);
                D_us_801D415C[itemId]--;
            }
        }
        tempVar = g_pads[0].tapped;
        if (tempVar & BUTTON_CONFIRM) {
            if (g_Status.gold <
                    D_us_801D415C[itemId] * D_us_801D4364[itemId].price ||
                !D_us_801D415C[itemId]) {
                g_api.PlaySfx(SFX_UI_ERROR);
            } else {
                g_Status.gold -=
                    D_us_801D415C[itemId] * D_us_801D4364[itemId].price;
                switch (D_us_801D4364[itemId].category) {
                case INVENTORY_HAND:
                    tempVar = EQUIP_HAND;
                    break;
                case INVENTORY_HEAD:
                    tempVar = EQUIP_HEAD;
                    break;
                case INVENTORY_BODY:
                    tempVar = EQUIP_ARMOR;
                    break;
                case INVENTORY_CAPE:
                    tempVar = EQUIP_CAPE;
                    break;
                case INVENTORY_ACCESSORY:
                    tempVar = EQUIP_ACCESSORY;
                    break;
                }
                if (D_us_801D4364[itemId].category < NUM_EQUIP_KINDS) {
                    for (i = 0; i < D_us_801D415C[itemId]; i++) {
                        g_api.AddToInventory(D_us_801D4364[itemId].itemId,
                                             (enum EquipKind)tempVar);
                    }
                } else {
                    if (D_us_801D4364[itemId].category == INVENTORY_RELIC) {
                        g_api.func_800FE044(
                            D_us_801814D4[D_us_801D4364[itemId].itemId],
                            0x2000);
                    } else {
                        if (D_us_801D4364[itemId].itemId) {
                            g_api.LearnSpell(
                                D_us_80181510[D_us_801D4364[itemId].itemId -
                                              1]);
                        } else {
                            // castle map
                            g_api.func_800F2288(0);
                            g_CastleFlags[BOUGHT_CASTLE_MAP] = 1;
                        }
                    }
                }
                D_us_801D425C[itemId] -= D_us_801D415C[itemId];
                if (D_us_801D425C[itemId]) {
                    D_us_801D415C[itemId] = 1;
                } else {
                    D_us_801D415C[itemId] = 0;
                }
                SetStep(4);
                g_api.PlaySfx(SFX_UI_CONFIRM);
            }
        } else if (tempVar & BUTTON_CANCEL) {
            SetStep(7);
        }
        prim = &g_PrimBuf[self->primIndex];
        for (i = 0; i < 10; i++) {
            prim = prim->next;
        }
        prim->y0 = prim->y1 = (self->ext.et_801B6F30.unk80 * 0xC) + 0x1F;
        prim->y2 = prim->y3 = prim->y0 + 0xC;
        prim->drawMode = DRAW_COLORS;

        prim = prim->next;
        if (self->ext.et_801B6F30.unk82) {
            prim->drawMode = DRAW_DEFAULT;
        } else {
            prim->drawMode = DRAW_HIDE;
        }

        prim = prim->next;
        if (self->ext.et_801B6F30.unk82 < self->ext.et_801B6F30.unk88) {
            prim->drawMode = DRAW_DEFAULT;
        } else {
            prim->drawMode = DRAW_HIDE;
        }
        break;

    case 4:
        switch (self->step_s) {
        case 0:
            if (g_api.func_80131F68()) {
                g_api.PlaySfx(SET_STOP_MUSIC);
                self->step_s++;
            } else {
                self->step_s++;
            }
            break;

        case 1:
            if (!g_api.func_80131F68()) {
                g_CutsceneFlags |= 0x800;
                g_api.PlaySfx(NA_VO_ML_THANKS);
                SetStep(3);
            }
            break;
        }
        break;

    case 5:
        tempVar = g_pads[0].repeat;
        if (tempVar & PAD_DOWN) {
            if (self->ext.et_801B6F30.unk80 < 6) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80++;
            } else if (self->ext.et_801B6F30.unk82 < 0) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk82++;
            }
        } else if (tempVar & PAD_UP) {
            if (self->ext.et_801B6F30.unk80 > 0) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80--;
            } else if (self->ext.et_801B6F30.unk82) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk82--;
            }
        }
        itemId = self->ext.et_801B6F30.unk82 + self->ext.et_801B6F30.unk80;
        if (tempVar & PAD_RIGHT) {
            if (D_us_801D415C[itemId] < D_us_801D425C[itemId]) {
                g_api.PlaySfx(SFX_UI_MOVE);
                D_us_801D415C[itemId]++;
            }
        } else if (tempVar & PAD_LEFT) {
            if (D_us_801D415C[itemId] > 1) {
                g_api.PlaySfx(SFX_UI_MOVE);
                D_us_801D415C[itemId]--;
            }
        }
        tempVar = g_pads[0].tapped;
        if (tempVar & BUTTON_CONFIRM) {
            if (D_us_801D415C[itemId] == 0) {
                g_api.PlaySfx(SFX_UI_ERROR);
            } else {
                g_Status.gold +=
                    D_us_801D415C[itemId] * D_us_801814D8[itemId].price;
                if (g_Status.gold > 999999) {
                    g_Status.gold = 999999;
                }
                ptr = &D_us_801814D8[itemId];
                g_Status.equipBodyCount[ptr->itemId] -= D_us_801D415C[itemId];
                D_us_801D425C[itemId] -= D_us_801D415C[itemId];
                D_us_801D415C[itemId] = 0;
                SetStep(6);
                g_api.PlaySfx(SFX_UI_CONFIRM);
            }
        } else if (tempVar & BUTTON_CANCEL) {
            SetStep(7);
        }
        prim = &g_PrimBuf[self->primIndex];
        for (i = 0; i < 10; i++) {
            prim = prim->next;
        }
        prim->y0 = prim->y1 = (self->ext.et_801B6F30.unk80 * 0xC) + 0x1F;
        prim->y2 = prim->y3 = prim->y0 + 0xC;
        prim->drawMode = DRAW_COLORS;

        prim = prim->next;
        if (self->ext.et_801B6F30.unk82) {
            prim->drawMode = DRAW_DEFAULT;
        } else {
            prim->drawMode = DRAW_HIDE;
        }

        prim = prim->next;
        if (self->ext.et_801B6F30.unk82 < 0) {
            prim->drawMode = DRAW_DEFAULT;
        } else {
            prim->drawMode = DRAW_HIDE;
        }
        break;

    case 6:
        switch (self->step_s) {
        case 0:
            if (g_api.func_80131F68()) {
                g_api.PlaySfx(SET_STOP_MUSIC);
                self->step_s++;
            } else {
                self->step_s++;
            }
            break;

        case 1:
            if (!g_api.func_80131F68()) {
                g_CutsceneFlags |= 0x800;
                g_api.PlaySfx(NA_VO_ML_THANKS);
                SetStep(5);
            }
            break;
        }
        break;

    case 7:
        if ((g_CutsceneFlags & 0x200) == 0) {
            prim = &g_PrimBuf[self->primIndex];
            for (i = 0; i < 10; i++) {
                prim = prim->next;
            }
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;

            prim->drawMode = DRAW_HIDE;
            prim = prim->next;

            prim->drawMode = DRAW_HIDE;
            self->ext.et_801B6F30.unk7E = 0x400;
            self->ext.et_801B6F30.unk7C = 0x10;
            self->step++;
            g_CutsceneFlags |= 0x200;
        }
        break;

    case 8:
        self->ext.et_801B6F30.unk7C--;
        self->ext.et_801B6F30.unk7E -= 0x40;
        prim = &g_PrimBuf[self->primIndex];
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x74, 0x20, 8, 1);
        if (self->ext.et_801B6F30.unk7C == 1) {
            g_CutsceneFlags &= ~0x400;
        }
        if (!self->ext.et_801B6F30.unk7C) {
            DestroyEntity(self);
        }
        break;
    }
}

Primitive* func_us_801B3EC8(Primitive* prim, u32 number, u16 maxDigits) {
    s32 i;
    s32 currentDigit;

    bool hasADigit = false;
    for (currentDigit = maxDigits - 1; currentDigit >= 0; currentDigit--) {
        u32 digitValue = number;
        for (i = 0; i < currentDigit; i++) {
            digitValue /= 10;
        }
        if (digitValue != 0) {
            if (digitValue < 10) {
                prim->u0 = digitValue * 8;
                prim->v0 = 8;
            } else {
                prim->u0 = 0x28;
                prim->v0 = 0x10;
            }
            for (i = 0; i < currentDigit; i++) {
                digitValue *= 10;
            }
            number -= digitValue;
            hasADigit = true;
            prim->drawMode = DRAW_DEFAULT;
        } else if (hasADigit || currentDigit == 0) {
            prim->u0 = 0;
            prim->v0 = 8;
            prim->drawMode = DRAW_DEFAULT;
        } else {
            prim->drawMode = DRAW_HIDE;
        }
        prim = prim->next;
    }
    return prim;
}

Primitive* func_us_801B3FB4(
    Primitive* prim, const char* str, u16 length, s32 arg3) {
    char ch;
    s32 i;
    u32 max;
    const char* chPtr;

    chPtr = str;
    max = 0;
    for (i = 0; i < length; i++) {
#ifdef VERSION_PSP
        if (*chPtr == 0xFF) {
            break;
        }
        chPtr++;
        max++;
    }
    for (i = 0; i < max; i++) {
#endif
        ch = *str++;
        prim->u0 = (ch & 0x0F) << 3;
        prim->v0 = (ch & 0xF0) >> 1;
        if (arg3 != 0) {
            prim->drawMode = DRAW_DEFAULT;
        }
        prim = prim->next;
    }
#ifdef VERSION_PSP
    for (; i < length; i++) {
        prim->drawMode = DRAW_HIDE;
        prim = prim->next;
    }
#endif
    return prim;
}

#ifdef VERSION_PSP
extern const char** D_us_80181528;
#else
static const char* D_us_80181528[] = {"ATT", "DEF"};
#endif

static u16 D_us_80181530[] = {
    -0x012, -0x022, -0x00A, -0x022, -0x012, -0x01A, -0x00A, -0x01A, -0x02E,
    0x01A,  -0x026, 0x01A,  -0x01E, 0x01A,  -0x016, 0x01A,  -0x006, 0x01A,
    0x002,  0x01A,  0x00A,  0x01A,  0x012,  0x01A,  0x01A,  0x01A,  0x022,
    0x01A,  0x02A,  0x01A,  0x032,  0x01A,  -0x016, 0x010,  -0x00E, 0x010,
    -0x006, 0x010,  0x002,  0x010,  0x00A,  0x010,  0x012,  0x010,  0x01A,
    0x010,  0x01A,  0x010,  0x022,  0x010,  0x02A,  0x010,  0x032,  0x010,
    0x002,  -0x022, 0x00A,  -0x022, 0x012,  -0x022, 0x01A,  -0x023, 0x022,
    -0x022, 0x02A,  -0x022, 0x032,  -0x022, 0x002,  -0x01A, 0x00A,  -0x01A,
    0x012,  -0x01A, 0x01A,  -0x01B, 0x022,  -0x01A, 0x02A,  -0x01A, 0x032,
    -0x01A, 0x002,  -0x00E, 0x00A,  -0x00E, 0x012,  -0x00E, 0x01A,  -0x00F,
    0x022,  -0x00E, 0x02A,  -0x00E, 0x032,  -0x00E, -0x036, -0x002, -0x02E,
    -0x002, -0x026, -0x002, -0x01A, -0x002, -0x012, -0x002, -0x00A, -0x002,
    0x006,  -0x002, 0x00E,  -0x002, 0x016,  -0x002, 0x022,  -0x002, 0x02A,
    -0x002, 0x032,  -0x002, -0x036, 0x006,  -0x02E, 0x006,  -0x026, 0x006,
    -0x01A, 0x006,  -0x012, 0x006,  -0x00A, 0x006,  0x006,  0x006,  0x00E,
    0x006,  0x016,  0x006,  0x022,  0x006,  0x02A,  0x006,  0x032,  0x006};
static char D_us_80181650[] = {
    CH(' '), CH(' '), CH(' '), CH(' '), CH('R'), CH('L'), CH('R'), CH('L')};
static char D_us_80181658[] = {
    SQUARE, CIRCLE, CROSS, TRIANGLE, CH('2'), CH('1'), CH('1'), CH('2')};

#ifdef VERSION_PSP
extern char* D_us_80181660;
extern char* D_us_80181668;
extern char** D_us_80181674;
#else
static char D_us_80181660[] = {
    CIRCLE, SQUARE, CIRCLE, SQUARE, CH('G'), CH('O'), CH('L'), CH('D')};
static char D_us_80181668[] = {
    // clang-format off
    CH('S'), CH('T'), CH('R'),
    CH('C'), CH('O'), CH('N'),
    CH('I'), CH('N'), CH('T'),
    CH('L'), CH('C'), CH('K')
    // clang-format on
};
static char* D_us_80181674[] = {
#if defined(MOD_KO)
    KO_SHOP_LABEL0, _S("Equipped"), _S("Not equipped"), _S("うれるこすう")}; /* user decision 06:42: Equipped / Not equipped stay English */
#else
    _S("Number"), _S("Equipped"), _S("Not equipped"), _S("うれるこすう")};
#endif
#endif

/* label x positions: the English labels are right-aligned (Number 170 + 48 ends at 218, the digits follow at D_us_80181684).
 * The Korean Number label is shorter (12 px per Hangul, space 4: 40 px), so KO starts it at 178 to keep the right edge.
 * Equipped / Not equipped stay English in KO (user decision), so their x stay the retail values. */
#if defined(MOD_KO)
#define SHOP_LBL_NUM_X 0xB2
#define SHOP_LBL_NOTEQ_X 0x9A
#define SHOP_LBL_EQ_X 0xB6
#else
#define SHOP_LBL_NUM_X 0xAA
#define SHOP_LBL_NOTEQ_X 0x9A
#define SHOP_LBL_EQ_X 0xB6
#endif

static u16 D_us_80181684[] = {0xE4, 0xB0, 0xEC, 0xB0};

void func_us_801B4010(u16* arg0) {
    *arg0++ = g_Status.equipment[0];
    *arg0++ = g_Status.equipment[1];
    *arg0++ = g_Status.equipment[2];
    *arg0++ = g_Status.equipment[3];
    *arg0++ = g_Status.equipment[4];
    *arg0++ = g_Status.equipment[5];
    *arg0 = g_Status.equipment[6];
}

void func_us_801B4080(u16* arg0) {
    g_Status.equipment[0] = *arg0++;
    g_Status.equipment[1] = *arg0++;
    g_Status.equipment[2] = *arg0++;
    g_Status.equipment[3] = *arg0++;
    g_Status.equipment[4] = *arg0++;
    g_Status.equipment[5] = *arg0++;
    g_Status.equipment[6] = *arg0;
}

void func_us_801B40F0(u16* arg0) {
    *arg0 = g_Status.attackHands[0];
    arg0 += 2;
    *arg0 = g_Status.attackHands[1];
    arg0 += 2;
    *arg0 = g_Status.defenseEquip;
    arg0 += 2;
    *arg0 = g_Status.statsBase[0] + g_Status.statsEquip[0];
    arg0 += 2;
    *arg0 = g_Status.statsBase[1] + g_Status.statsEquip[1];
    arg0 += 2;
    *arg0 = g_Status.statsBase[2] + g_Status.statsEquip[2];
    arg0 += 2;
    *arg0 = g_Status.statsBase[3] + g_Status.statsEquip[3];
}

Primitive* func_us_801B4194(Primitive* prim) {
    Primitive* prim2;
    s32 i;
    u16* ptr;

    prim = func_us_801B1064(prim, SHOP_LBL_NUM_X, 0xB0, D_us_80181674[0], 0x196);
    prim2 = prim;
    ptr = D_us_80181684;
    for (i = 0; i < 2; i++) {
        prim2->x0 = *ptr++;
        prim2->y0 = *ptr++;
        prim2 = prim2->next;
    }
    return prim;
}

void func_us_801B420C(Primitive* prim, Entity* arg1) {
    u16 index;
    s16 x0;
    s16 x1, y1;
    ShopItem* shopItem;
    s16* condition;
    u16 stats[14];
    u16 equipment[7];
    s16 maxDigits;
    s32 i, j;
    s16 itemId;
    s16 number;
    u16* ptr;
    Primitive* primA;
    Primitive* primB;
    s16 y0;

    for (i = 0; i < 11; i++) {
        prim = prim->next;
    }
    for (i = 0; i < 8; i++) {
        prim->drawMode = DRAW_DEFAULT;
        prim = prim->next;
    }
    prim = func_us_801B3EC8(prim, g_Status.gold, 8);
    index = arg1->ext.et_801B6F30.unk80 + arg1->ext.et_801B6F30.unk82;
#ifdef VERSION_PSP
    condition = 0;
    if (arg1->params) {
        shopItem = &D_us_801814D8[index];
        condition = D_psp_092A4A88;
        primA = prim;
        ptr = D_psp_092A4A78;
        prim = func_us_801B3FB4(prim, D_us_80181674[0], 8, 1);
        primB = prim;
        x0 = 0x28;
        y0 = 0x10;
        prim = func_us_801B3EC8(prim, D_us_801D425C[index], 2);
    } else {
        shopItem = &D_us_801D4364[index];
        primA = prim;
        if (shopItem->category < INVENTORY_RELIC) {
            ptr = D_psp_092A4A78;
            prim = func_us_801B3FB4(prim, D_us_80181674[0], 12, 1);
            condition = D_psp_092A4A88;
            primB = prim;
            x0 = 0x28;
            y0 = 0x10;
            prim = func_us_801B3EC8(prim, 99 - D_us_801D425C[index], 2);
        } else {
            ptr = D_psp_092A4AA0;
            if (D_us_801D425C[index] != 0) {
                prim = func_us_801B3FB4(prim, D_us_80181674[2], 12, 1);
            } else {
                prim = func_us_801B3FB4(prim, D_us_80181674[1], 12, 1);
            }
        }
    }
    x1 = *ptr++;
    y1 = *ptr++;
    for (i = 0; i < 12; i++) {
        primA->x0 = x1 + 0xBA + i * 8;
        primA->y0 = y1 + 0xA0;
        primA = primA->next;
    }
    if (condition) {
        for (i = 0; i < 2; i++) {
            primB->x0 = x0 + 0xBA + i * 8;
            primB->y0 = y0 + 0xA0;
            primB = primB->next;
        }
    }
#else
    primA = prim;
    for (i = 0; i < 11; i++) {
        primA->drawMode = DRAW_HIDE;
        primA = primA->next;
    }
    if (arg1->params) {
        shopItem = &D_us_801814D8[index];
        prim =
            func_us_801B3EC8(func_us_801B4194(prim), D_us_801D425C[index], 2);
    } else {
        shopItem = &D_us_801D4364[index];
        if (shopItem->category < INVENTORY_RELIC) {
            prim = func_us_801B3EC8(
                func_us_801B4194(prim), 99 - D_us_801D425C[index], 2);
        } else {
            if (D_us_801D425C[index] != 0) {
                prim =
                    func_us_801B1064(prim, SHOP_LBL_NOTEQ_X, 0xB0, D_us_80181674[2], 0x196);
            } else {
                prim =
                    func_us_801B1064(prim, SHOP_LBL_EQ_X, 0xB0, D_us_80181674[1], 0x196);
            }
        }
    }
    prim = primA;
#endif
    func_us_801B4010(equipment);
    func_us_801B40F0(stats);
    itemId = shopItem->itemId;
    switch (shopItem->category) {
    case INVENTORY_HAND:
        g_Status.equipment[LEFT_HAND_SLOT] = itemId;
        g_Status.equipment[RIGHT_HAND_SLOT] = itemId;
        break;
    case INVENTORY_HEAD:
        g_Status.equipment[HEAD_SLOT] = itemId;
        break;
    case INVENTORY_BODY:
        g_Status.equipment[ARMOR_SLOT] = itemId;
        break;
    case INVENTORY_CAPE:
        g_Status.equipment[CAPE_SLOT] = itemId;
        break;
    case INVENTORY_ACCESSORY:
        g_Status.equipment[ACCESSORY_1_SLOT] = itemId;
        g_api.MakeAll();
        func_us_801B40F0(stats + 1);
        func_us_801B4080(equipment);
        g_Status.equipment[ACCESSORY_2_SLOT] = itemId;
        g_api.MakeAll();
        i = 3;
        ptr = stats + 1;
        maxDigits = *ptr - g_Status.attackHands[0];
        ptr += 4;
        maxDigits += *ptr - g_Status.defenseEquip;
        if (maxDigits < 0) {
            i++;
        } else if (maxDigits == 0) {
            ptr += 2;
            maxDigits = *ptr - (g_Status.statsBase[0] + g_Status.statsEquip[0]);
            ptr += 2;
            maxDigits +=
                *ptr - (g_Status.statsBase[1] + g_Status.statsEquip[1]);
            ptr += 2;
            maxDigits +=
                *ptr - (g_Status.statsBase[2] + g_Status.statsEquip[2]);
            ptr += 2;
            maxDigits +=
                *ptr - (g_Status.statsBase[3] + g_Status.statsEquip[3]);
            if (maxDigits < 0) {
                i++;
            }
        }
        func_us_801B4080(equipment);
        (g_Status.equipment + 2)[i] = itemId;
        break;
    case INVENTORY_RELIC:
        break;
    }
    g_api.MakeAll();
    func_us_801B40F0(stats + 1);
    func_us_801B4080(equipment);
    g_api.MakeAll();
    ptr = stats;
    for (i = 0; i < 3; i++) {
#ifdef VERSION_PSP
        primA = prim;
        for (j = 0; j < 7; j++) {
            prim->x0 = D_psp_092A49B8[j * 2 + (i * 7) * 2] + 0xBA;
            prim->y0 = D_psp_092A49B8[j * 2 + (i * 7) * 2 + 1] + 0xA0;
            prim = prim->next;
        }
        prim = primA;
#endif
        maxDigits = *ptr++;
        prim = func_us_801B3EC8(prim, maxDigits, 3);
        if (!arg1->params) {
            maxDigits -= *ptr;
            if (maxDigits) {
                if (maxDigits > 0) {
                    prim->u0 = 0x28;
                } else {
                    prim->u0 = 0x18;
                }
            } else {
                prim->u0 = 0x20;
            }
            prim->v0 = 0x70;
            prim->drawMode = DRAW_DEFAULT;
            prim = prim->next;
            prim = func_us_801B3EC8(prim, *ptr++, 3);
        } else {
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
#ifdef VERSION_PSP
            ptr++;
#endif
        }
    }
    if (!arg1->params) {
        for (i = 0; i < 4; i++) {
            maxDigits = *ptr++;
            number = *ptr++;
#ifdef VERSION_PSP
            primA = prim;
            for (j = 0; j < 6; j++) {
                prim->x2 = prim->x0 =
                    D_psp_092A4A10[j * 2 + (i * 6) * 2] + 0xBA;
                prim->y1 = prim->y0 =
                    D_psp_092A4A10[j * 2 + (i * 6) * 2 + 1] + 0xA0;
                prim->x3 = prim->x1 = prim->x0 + 8;
                prim->y3 = prim->y2 = prim->y0 + 8;
                prim->v1 = prim->u1 = 8;
                prim->type = PRIM_SPRT;
                prim->clut = PAL_UNK_196;
                prim->tpage = 0x1E;
                prim->priority = 0x11;
                prim = prim->next;
            }
            prim = primA;
#endif
            if (maxDigits != number) {
                prim = func_us_801B3FB4(prim, &D_us_80181668[i * 3], 3, 1);
                if (maxDigits > number) {
                    number = maxDigits - number;
                    maxDigits = 2;
                    if (number / 10 == 0) {
                        prim->drawMode = DRAW_HIDE;
                        prim = prim->next;
                        maxDigits = 1;
                    }
                    prim->u0 = 0x68;
                } else {
                    number -= maxDigits;
                    maxDigits = 2;
                    if (number / 10 == 0) {
                        prim->drawMode = DRAW_HIDE;
                        prim = prim->next;
                        maxDigits = 1;
                    }
                    prim->u0 = 0x58;
                }
                prim->v0 = 0;
                prim->drawMode = DRAW_DEFAULT;
                prim = prim->next;
                prim = func_us_801B3EC8(prim, number, maxDigits);
            } else {
                for (j = 0; j < 6; j++) {
                    prim->drawMode = DRAW_HIDE;
                    prim = prim->next;
                }
            }
            if (prim == NULL) {
                break;
            }
        }
    }
    while (prim != NULL) {
        prim->drawMode = DRAW_HIDE;
        prim = prim->next;
    }
}

void func_us_801B4830(Entity* self) {
    Primitive* prim;
    s32 primIndex;
    s32 i;
    s32 j;
    s32 count;
    u16* var_s4;
    const char* name;
    Entity* tempEntity;
    u8* unused;

    tempEntity = self - 1;
    switch (self->step) {
    case 0:
#ifdef VERSION_PSP
        primIndex = g_api.AllocPrimitives(PRIM_SPRT, 0x84);
#else
        primIndex = g_api.AllocPrimitives(PRIM_SPRT, 0x53);
#endif
        if (primIndex != -1) {
            InitializeEntity(g_EInitCommon);
            i = 0;
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = primIndex;
            prim = &g_PrimBuf[primIndex];
            count = 0x49;
            j = 0;
            while (prim != NULL) {
                if (i < 6) {
                    prim->type = PRIM_LINE_G2;
                    prim->r0 = prim->r1 = 0x80;
                    prim->g0 = prim->g1 = 0x80;
                    prim->b0 = prim->b1 = 0x80;
                    prim->priority = 0xC0;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 8) {
                    prim->type = PRIM_G4;
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0xBA;
                    prim->y0 = prim->y1 = 0x76;
                    prim->y2 = prim->y3 = 0xCA;
                    prim->priority = 0xBF;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 10) {
                    prim->type = PRIM_GT4;
#ifdef VERSION_PSP
                    prim->tpage = 0x110;
#else
                    prim->tpage = 0x114;
#endif
                    prim->u0 = prim->u2 = (i - 9) * 0x3A + 0xBA;
                    prim->u1 = prim->u3 = prim->u0 + 0x3A;
                    prim->v0 = prim->v1 = 0x7E;
                    prim->v2 = prim->v3 = prim->v0 + 0x44;
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0xBA;
                    prim->y0 = prim->y1 = 0x7E;
                    prim->y2 = prim->y3 = 0xC2;
                    prim->priority = 0xC0;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 10) {
#ifdef VERSION_PSP
                    func_us_801B11A0(0x50, 0x100, 0x24, 0x20);
#else
                    func_us_801B11A0(0x60, 0x100, 0x24, 0x20);
#endif
                    prim->clut = PAL_UNK_1A1;
                    prim->tpage = 0x10;
#ifdef VERSION_PSP
                    prim->u0 = 0x50;
#else
                    prim->u0 = 0x60;
#endif
                    prim->v0 = 0;
                    prim->u1 = 0x24;
                    prim->v1 = 0x20;
                    prim->x0 = 0x80;
                    prim->y0 = 0x7E;
                    prim->priority = 0x11;
                    prim->drawMode = DRAW_HIDE;
                    var_s4 = D_us_80181530;
                } else if (i < 15) {
                    if (i == 11) {
                        D_us_80181660[0] =
                            D_us_80181650[g_Settings.buttonConfig[0]];
                        D_us_80181660[1] =
                            D_us_80181658[g_Settings.buttonConfig[0]];
                        D_us_80181660[2] =
                            D_us_80181650[g_Settings.buttonConfig[1]];
                        D_us_80181660[3] =
                            D_us_80181658[g_Settings.buttonConfig[1]];
                        func_us_801B3FB4(prim, D_us_80181660, 8, 0);
                    }
                    prim->x0 = *var_s4++ + 0xBA;
                    prim->y0 = *var_s4++ + 0xA0;
                    j++;
                    if (g_Settings.buttonConfig[(i - 11) / 2] < 4) {
                        if ((i - 11) & 1) {
                            prim->x0 -= 4;
                        }
                    }
                    prim->clut = PAL_UNK_196;
                    prim->tpage = 0x1E;
                    prim->u1 = 8;
                    prim->v1 = 8;
                    prim->priority = 0x11;
                    prim->drawMode = DRAW_HIDE;
                } else {
                    prim->x0 = *var_s4++ + 0xBA;
                    prim->y0 = *var_s4++ + 0xA0;
                    j++;
                    prim->clut = PAL_UNK_196;
                    prim->tpage = 0x1E;
                    prim->u1 = 8;
                    prim->v1 = 8;
                    prim->priority = 0x11;
                    prim->drawMode = DRAW_HIDE;
                }
                prim = prim->next;
                i++;
#ifdef VERSION_PSP
                if (j >= count) {
                    break;
                }
#endif
            }
#ifdef VERSION_PSP
            while (prim != NULL) {
                prim->drawMode = DRAW_HIDE;
                prim = prim->next;
            }
#endif
        }
        break;

    case 1:
        if (!self->step_s) {
            unused = g_Pix[0];
            for (i = 0; i < 2; i++) {
                name = D_us_80181528[i];
#ifdef VERSION_PSP
                func_us_801B0FBC(name, 0x17, 0x104 + i * 16);
#else
                func_us_801B0FBC(name, 0x1A, 0x104 + i * 16);
#endif
            }
            self->step_s++;
        } else if (g_CutsceneFlags & 0x400) {
            SetStep(2);
            self->ext.et_801B6F30.unk7C = 0x10;
        }
        break;

    case 2:
        if (!--self->ext.et_801B6F30.unk7C) {
            prim = &g_PrimBuf[self->primIndex];
            func_us_801B420C(prim, tempEntity);
            for (i = 0; i < 10; i++) {
                prim = prim->next;
            }
            for (i = 0; i < 7; i++) {
                prim->drawMode = DRAW_DEFAULT;
                prim = prim->next;
            }
            SetStep(3);
            self->ext.et_801B6F30.unk7C = 0;
            self->ext.et_801B6F30.unk7E = 0;
        }
        break;

    case 3:
        self->ext.et_801B6F30.unk7C++;
        self->ext.et_801B6F30.unk7E += 0x40;
        prim = &g_PrimBuf[self->primIndex];
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x42, 0x18, 0x32, 1);
        if (self->ext.et_801B6F30.unk7C == 0x10) {
            SetStep(4);
        }
        break;

    case 4:
        prim = &g_PrimBuf[self->primIndex];
        func_us_801B420C(prim, tempEntity);
        if ((g_CutsceneFlags & 0x400) == 0) {
            self->ext.et_801B6F30.unk7E = 0x400;
            self->ext.et_801B6F30.unk7C = 0x10;
            SetStep(8);
        }
        break;

    case 8:
        self->ext.et_801B6F30.unk7C--;
        self->ext.et_801B6F30.unk7E -= 0x40;
        prim = &g_PrimBuf[self->primIndex];
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x42, 0x18, 0x32, 1);
        if (!self->ext.et_801B6F30.unk7C) {
            for (i = 0; i < 10; i++) {
                prim = prim->next;
            }
            while (prim != NULL) {
                prim->drawMode = DRAW_HIDE;
                prim = prim->next;
            }
            SetStep(9);
        }
        break;

    case 9:
        DestroyEntity(self);
        return;
    }
}

#ifdef VERSION_PSP
extern const char** D_us_8018168C;
#else
static const char* D_us_8018168C[] = {
#if defined(MOD_KO)
    KO_SHOP_DESC0, KO_SHOP_DESC1, KO_SHOP_DESC2,
    KO_SHOP_DESC3, KO_SHOP_DESC4, KO_SHOP_DESC5};
#else
    "Basic map of Dracula’s castle", "Contains ””Dark Metamorphosis””",
    "Contains ””Summon Spirit””",    "Contains ””Hellfire””",
    "Contains ””Tetra Spirits””",    "Contains ””Soul Steal””"};
#endif
#endif

void func_us_801B4ED4(s16 index, u16 arg1) {
    const char* desc;
    u16 itemId;
    ShopItem* shopItem;
    unsigned char* unused;

    if (arg1) {
        shopItem = &D_us_801814D8[index];
    } else {
        shopItem = &D_us_801D4364[index];
    }
    itemId = shopItem->itemId;
    switch (shopItem->category) {
    case INVENTORY_HAND:
        desc = g_api.equipDefs[itemId].description;
        g_api.LoadEquipIcon(g_api.equipDefs[itemId].icon,
                            g_api.equipDefs[itemId].iconPalette, 0x1F);
        break;
    case INVENTORY_HEAD:
    case INVENTORY_BODY:
    case INVENTORY_CAPE:
    case INVENTORY_ACCESSORY:
        desc = g_api.accessoryDefs[itemId].description;
        g_api.LoadEquipIcon(g_api.accessoryDefs[itemId].icon,
                            g_api.accessoryDefs[itemId].iconPalette, 0x1F);
        break;
    case INVENTORY_RELIC:
        itemId = D_us_801814D4[itemId];
        desc = g_api.relicDefs[itemId].desc;
        g_api.LoadEquipIcon(g_api.relicDefs[itemId].icon,
                            g_api.relicDefs[itemId].iconPalette, 0x1F);
        break;
    case INVENTORY_DOCUMENT:
        desc = D_us_8018168C[itemId];
        if (itemId) {
            itemId = 0x112;
        } else {
            itemId = 0x111;
        }
        g_api.LoadEquipIcon(itemId, 0x118, 0x1F);
        break;
    default:
        desc = *D_us_80181528;
        break;
    }
    unused = &g_Pix[0][0x1000];
    itemId = 2;
    func_us_801B0C40(&g_Pix[0][0x800], desc, 2, 0x184, 0x7E);
}

void func_us_801B5068(Entity* self) {
    Primitive* prim;
    Primitive* otherPrim;
    s32 primIndex;
    s32 i;
    s16 offset;
    Entity* tempEntity;

    tempEntity = self - 2;
#ifdef VERSION_PSP
    func_psp_09264E08();
#endif
    switch (self->step) {
    case 0:
        primIndex = g_api.AllocPrimitives(PRIM_GT4, 7);
        if (primIndex != -1) {
            InitializeEntity(g_EInitCommon);
            i = 0;
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = primIndex;
            prim = &g_PrimBuf[primIndex];
            for (; prim != NULL; i++) {
                if (i < 4) {
                    prim->type = PRIM_LINE_G2;
                    prim->r0 = prim->r1 = 0x80;
                    prim->g0 = prim->g1 = 0x80;
                    prim->b0 = prim->b1 = 0x80;
                    prim->priority = 0xBE;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 4) {
                    func_us_801B11A0(0, 0x180, 0xF0, 0x16);
                    prim->clut = PAL_UNK_1A1;
                    prim->tpage = 0x10;
                    prim->u0 = prim->u2 = 0;
                    prim->u1 = prim->u3 = 0xF0;
                    prim->v0 = prim->v1 = 0x80;
                    prim->v2 = prim->v3 = 0x96;
                    prim->priority = 0xBE;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 5) {
                    prim->type = PRIM_G4;
                    prim->r0 = prim->r1 = prim->r2 = prim->r3 = 0;
                    prim->g0 = prim->g1 = 0x20;
                    prim->g2 = prim->g3 = 0x40;
                    prim->b0 = prim->b1 = 0;
                    prim->b2 = prim->b3 = 0;
                    prim->priority = 0xBD;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 6) {
                    prim->tpage = 0x1A;
                    prim->clut = 0x1EF;
                    prim->x0 = prim->x2 = 0x49;
                    prim->x1 = prim->x3 = 0x57;
                    prim->y0 = prim->y1 = 0x96;
                    prim->y2 = prim->y3 = 0xA4;
                    prim->u0 = prim->u2 = 0x71;
                    prim->u1 = prim->u3 = prim->u0 + 0xE;
                    prim->v0 = prim->v1 = 0xB1;
                    prim->v2 = prim->v3 = prim->v0 + 0xE;
                    prim->priority = 0xC0;
                    prim->drawMode = DRAW_HIDE;
                }
                prim = prim->next;
            }
        }
        break;

    case 1:
        if (!self->step_s) {
            self->ext.et_801B6F30.unk80 = 0;
            func_us_801B4ED4(0, tempEntity->params);
            self->step_s++;
        } else if (g_CutsceneFlags & 0x400) {
            SetStep(2);
            self->ext.et_801B6F30.unk7C = 0x18;
        }
        break;

    case 2:
        if (!--self->ext.et_801B6F30.unk7C) {
            SetStep(3);
            self->ext.et_801B6F30.unk7C = 0x10;
            self->ext.et_801B6F30.unk7E = 0;
        }
        break;

    case 3:
        self->ext.et_801B6F30.unk7C--;
        self->ext.et_801B6F30.unk7E += 0x40;
        prim = &g_PrimBuf[self->primIndex];
        otherPrim = prim;
        for (i = 0; i < 4; i++) {
            prim = prim->next;
        }
#ifdef VERSION_PSP
        i = 0;
        if (self->ext.et_801B6F30.unk7C == 0) {
            for (; i < 2; i++) {
                prim->x0 = prim->x2 = 8;
                prim->x1 = prim->x3 = prim->x0 + 0xF0;
                prim->y0 = prim->y1 = 0xC9;
                prim->y2 = prim->y3 = prim->y0 + 0x16;
                if (i) {
                    prim->drawMode =
                        DRAW_DITHERING | DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                    func_us_801B1200(otherPrim, prim);
                } else {
                    prim->drawMode = DRAW_DEFAULT;
                }
                prim = prim->next;
            }
        } else {
            for (; i < 2; i++) {
                offset = (rcos(self->ext.et_801B6F30.unk7E) * 16) / 0x1000;
                prim->x0 = offset + 8;
                prim->x2 = 8 - offset;
                prim->x1 = 0xF8 - offset;
                prim->x3 = offset + 0xF8;
                offset = (rsin(self->ext.et_801B6F30.unk7E) * 11) / 0x1000;
                prim->y0 = prim->y1 = 0xD4 - offset;
                prim->y2 = prim->y3 = offset + 0xD4;
                if (i) {
                    prim->drawMode =
                        DRAW_DITHERING | DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                    func_us_801B1200(otherPrim, prim);
                } else {
                    prim->drawMode = DRAW_DEFAULT;
                }
                prim = prim->next;
            }
        }
#else
        for (i = 0; i < 2; i++) {
            offset = (rcos(self->ext.et_801B6F30.unk7E) * 16) / 0x1000;
            prim->x0 = offset + 8;
            prim->x2 = 8 - offset;
            prim->x1 = 0xF8 - offset;
            prim->x3 = offset + 0xF8;
            offset = (rsin(self->ext.et_801B6F30.unk7E) * 11) / 0x1000;
            prim->y0 = prim->y1 = 0xD4 - offset;
            prim->y2 = prim->y3 = offset + 0xD4;
            if (i) {
                prim->drawMode =
                    DRAW_DITHERING | DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                func_us_801B1200(otherPrim, prim);
            } else {
                prim->drawMode = DRAW_DEFAULT;
            }
            prim = prim->next;
        }
#endif
        if (!self->ext.et_801B6F30.unk7C) {
            SetStep(4);
        }
        break;

    case 4:
        prim = &g_PrimBuf[self->primIndex];
        for (i = 0; i < 6; i++) {
            prim = prim->next;
        }
        if (D_us_801D425C[self->ext.et_801B6F30.unk80]) {
            prim->drawMode = DRAW_DEFAULT;
        } else {
            prim->drawMode = DRAW_HIDE;
        }
        if (!self->step_s) {
            if (self->ext.et_801B6F30.unk80 !=
                (tempEntity->ext.et_801B6F30.unk80 +
                 tempEntity->ext.et_801B6F30.unk82)) {
                func_us_801B11A0(0, 0x180, 0xF0, 0x16);
                prim->drawMode = DRAW_HIDE;
                self->step_s = 1;
            }
        } else {
            self->ext.et_801B6F30.unk80 = tempEntity->ext.et_801B6F30.unk80 +
                                          tempEntity->ext.et_801B6F30.unk82;
            func_us_801B4ED4(self->ext.et_801B6F30.unk80, tempEntity->params);
            self->step_s = 0;
        }
        if ((g_CutsceneFlags & 0x400) == 0) {
            prim->drawMode = DRAW_HIDE;
            SetStep(7);
            self->ext.et_801B6F30.unk7C = 0x10;
        }
        break;

    case 7:
        self->ext.et_801B6F30.unk7C--;
        self->ext.et_801B6F30.unk7E += 0x40;
        prim = &g_PrimBuf[self->primIndex];
        otherPrim = prim;
        for (i = 0; i < 4; i++) {
            prim = prim->next;
        }
        for (i = 0; i < 2; i++) {
            offset = (rcos(self->ext.et_801B6F30.unk7E) * 16) / 0x1000;
            prim->x0 = offset + 8;
            prim->x2 = 8 - offset;
            prim->x1 = 0xF8 - offset;
            prim->x3 = offset + 0xF8;
            offset = (rsin(self->ext.et_801B6F30.unk7E) * 11) / 0x1000;
            prim->y0 = prim->y1 = 0xD4 - offset;
            prim->y2 = prim->y3 = offset + 0xD4;
            if (i) {
                prim->drawMode =
                    DRAW_DITHERING | DRAW_TPAGE | DRAW_COLORS | DRAW_TRANSP;
                func_us_801B1200(otherPrim, prim);
            } else {
                prim->drawMode = DRAW_DEFAULT;
            }
            prim = prim->next;
        }
        if (!self->ext.et_801B6F30.unk7C) {
            func_us_801B11A0(0, 0x180, 0xF0, 0x16);
            DestroyEntity(self);
        }
        break;
    }
}

#ifdef VERSION_PSP
extern char* D_us_801816A4;
extern const char** D_us_801816AC;
extern const char** D_us_801816B0;
#else
static char D_us_801816A4[] = {
    CH('S'), CH('O'), CH('L'), CH('D'), CH(' '), CH('O'), CH('U'), CH('T')};
static const char* D_us_801816AC[] = {_S("Jewel of Open")};
static const char* D_us_801816B0[] = {
#if defined(MOD_KO)
    KO_SHOP_DOC0, KO_SHOP_DOC1, KO_SHOP_DOC2,
    KO_SHOP_DOC3, KO_SHOP_DOC4, KO_SHOP_DOC5};
#elif defined(MOD_SATURN)
    // the PSP port's capitals
    _S("Castle Map"),     _S("Magic Scroll 1"), _S("Magic Scroll 2"),
    _S("Magic Scroll 3"), _S("Magic Scroll 4"), _S("Magic Scroll 5")};
#else
    _S("Castle map"),     _S("Magic scroll 1"), _S("Magic scroll 2"),
    _S("Magic scroll 3"), _S("Magic scroll 4"), _S("Magic scroll 5")};
#endif
#endif

#if defined(MOD_KO)
/* ko_equip_jp: tail bundle loader (absolute, ARC_KO resident).
 * Truth in dra.h (KO_EQUIP_LOAD / KO_JP_BUNDLE_*). */
#define KO_JP_LOAD ((void (*)(s32))0x8009FF40)
#define KO_JP_BUNDLE_LBA 0x4928F /* 2-pass wired (OFF_BIN_KO_JP, disc end) */
#define KO_JP_BUNDLE_SECTORS 1
#endif
void func_us_801B56E4(Entity* self) {
    Entity* tempEntity;
    s32 primIndex;
    s32 i, j;
    ShopItem* shopItem;
    DRAWENV drawEnv;
    DR_ENV* dr_env;
    RECT clipRect = {.x = 0x100, .y = 0x100, .w = 0x100, .h = 0x100};
    const char* name;
    Primitive* prim;
    Primitive* prim2;
    s16 posX, posY;
    s16 itemId;
    s16 index;
    s32 count;
    u32 price;

    tempEntity = self - 3;
    switch (self->step) {
    case 0:
        primIndex = g_api.AllocPrimitives(PRIM_SPRT, 0x13A);
        if (primIndex != -1) {
            InitializeEntity(g_EInitCommon);
            i = 0;
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = primIndex;
            prim = &g_PrimBuf[primIndex];
            dr_env = g_api.func_800EDB08((POLY_GT4*)prim);
            if (dr_env != NULL) {
                prim->type = PRIM_ENV;
                prim->priority = 0x10;
                prim->drawMode = DRAW_DEFAULT;
                drawEnv = g_CurrentBuffer->draw;
                drawEnv.isbg = 1;
                drawEnv.clip = clipRect;
#ifdef VERSION_PSP
                drawEnv.ofs[0] = 0;
#else
                drawEnv.ofs[0] = 0x100;
#endif
                drawEnv.ofs[1] = 0x100;
                SetDrawEnv(dr_env, &drawEnv);
            } else {
                self->step = 0;
                return;
            }
            prim = prim->next;
            dr_env = g_api.func_800EDB08((POLY_GT4*)prim);
            if (dr_env != NULL) {
                prim->type = PRIM_ENV;
                prim->priority = 0x12;
                prim->drawMode = DRAW_UNK_800;
            } else {
                self->step = 0;
                return;
            }
            prim = prim->next;
            while (prim != NULL) {
                prim->clut = PAL_UNK_196;
                prim->tpage = 0x1E;
                prim->u1 = prim->v1 = 8;
                prim->priority = 0x11;
                prim->drawMode = DRAW_HIDE;
                prim = prim->next;
            }
        }
        break;

    case 1:
        if (g_CutsceneFlags & 0x400) {
            if (tempEntity->params) {
                SetStep(4);
            } else {
                SetStep(2);
            }
        }
        break;

    case 2:
#if defined(MOD_KO)
        /* ko_equip_jp: one boot-load; menu-open reloads refresh it.
         * Valid only if stage loads never touch 0x801F0000 (see the shop note). */
        if (*(volatile u32*)0x8009EEDC == 0) {
            KO_JP_LOAD(KO_JP_BUNDLE_LBA);
            *(volatile u32*)0x8009EEDC = 1;
        }
#endif
        posY = tempEntity->ext.et_801B56E4.unk84 + 16;
        prim = &g_PrimBuf[self->primIndex];
        prim = prim->next;
        prim = prim->next;
        index = tempEntity->ext.et_801B56E4.unk82;
        posY = 16;
        for (i = 0; i < 8; i++) {
            if (i == 7 && !tempEntity->ext.et_801B56E4.unk84) {
                break;
            }
            if (tempEntity->params) {
                shopItem = &D_us_801814D8[index];
            } else {
                shopItem = &D_us_801D4364[index];
            }
            itemId = shopItem->itemId;
            switch (shopItem->category) {
            case INVENTORY_HAND:
                name = g_api.equipDefs[itemId].name;
                break;
            case INVENTORY_HEAD:
            case INVENTORY_BODY:
            case INVENTORY_CAPE:
            case INVENTORY_ACCESSORY:
                name = g_api.accessoryDefs[itemId].name;
                break;
            case INVENTORY_RELIC:
#if defined(MOD_KO)
                name = KO_SHOP_RELIC0; /* Jewel of Open: same term as the relic menu (gen_ko_shop.py asserts it); relicDefs names are ShowText-format (space 20, NUL end) and cannot be drawn by func_us_801B1064 */
#else
                name = D_us_801816AC[itemId];
#endif
                break;
            case INVENTORY_DOCUMENT:
                name = D_us_801816B0[itemId];
                break;
            default:
                name = *D_us_80181528;
                break;
            }
            prim = func_us_801B1064(prim, 8, posY, name, 0x196);
            if (D_us_801D415C[index] != 0) {
                posX = 0x7E;
                prim->clut = PAL_UNK_196;
                prim->u0 = 0x40;
                prim->v0 = 0x28;
                prim->drawMode = DRAW_DEFAULT;
                prim->x0 = posX;
                prim->y0 = posY;
                prim = prim->next;
                posX += 8;
                prim2 = prim;
                for (j = 0; j < 2; j++) {
                    prim->clut = PAL_UNK_196;
                    prim->x0 = posX;
                    prim->y0 = posY;
                    prim = prim->next;
                    posX += 8;
                }
                func_us_801B3EC8(prim2, D_us_801D415C[index], 2);
                posX += 10;
                prim2 = prim;
                price = shopItem->price * D_us_801D415C[index];
                for (j = 0; j < 8; j++) {
                    if (g_Status.gold < price) {
                        prim->clut = PAL_UNK_191;
                    } else {
                        prim->clut = PAL_UNK_196;
                    }
                    prim->x0 = posX;
                    prim->y0 = posY;
                    prim = prim->next;
                    posX += 8;
                }
                func_us_801B3EC8(prim2, price, 8);
            } else {
#ifdef VERSION_PSP
                posX = 0x80;
                count = 16;
                prim2 = prim;
                for (j = 0; j < count; j++) {
#else
                posX = 0x96;
                prim2 = prim;
                for (j = 0; j < 8; j++) {
#endif
                    prim->clut = PAL_UNK_196;
                    prim->x0 = posX;
                    prim->y0 = posY;
                    prim = prim->next;
                    posX += 8;
                }
#ifdef VERSION_PSP
                func_us_801B3FB4(prim2, D_us_801816A4, count, 1);
#else
                func_us_801B3FB4(prim2, D_us_801816A4, LEN(D_us_801816A4), 1);
#endif
            }
            posY += 12;
            index++;
        }
        while (prim != NULL) {
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
        }
        if ((g_CutsceneFlags & 0x400) == 0) {
            prim = &g_PrimBuf[self->primIndex];
            prim = prim->next;
            prim = prim->next;
            while (prim != NULL) {
                prim->drawMode = DRAW_HIDE;
                prim = prim->next;
            }
            SetStep(3);
        }
        break;

    case 3:
        DestroyEntity(self);
        return;

    case 4:
        prim = &g_PrimBuf[self->primIndex];
        prim = prim->next;
        prim = prim->next;
        index = tempEntity->ext.et_801B56E4.unk82;
        posY = 0x10;
        for (i = 0; i < 7; i++) {
            shopItem = &D_us_801814D8[index];
            name = g_api.accessoryDefs[shopItem->itemId].name;
            if (D_us_801D425C[index] != 0) {
                itemId = 0x196;
            } else {
                itemId = 0x191;
            }
            prim = func_us_801B1064(prim, 8, posY, name, itemId);
            posX = 0x7E;
            prim->clut = itemId;
            prim->u0 = 0x40;
            prim->v0 = 0x28;
            prim->drawMode = DRAW_DEFAULT;
            prim->x0 = posX;
            prim->y0 = posY;
            prim = prim->next;
            posX += 8;
            prim2 = prim;
            for (j = 0; j < 2; j++) {
                prim->clut = itemId;
                prim->x0 = posX;
                prim->y0 = posY;
                prim = prim->next;
                posX += 8;
            }
            func_us_801B3EC8(prim2, D_us_801D415C[index], 2);
            posX += 10;
            prim2 = prim;
            price = shopItem->price * D_us_801D415C[index];
            for (j = 0; j < 8; j++) {
                prim->clut = itemId;
                prim->x0 = posX;
                prim->y0 = posY;
                prim = prim->next;
                posX += 8;
            }
            func_us_801B3EC8(prim2, price, 8);
            posY += 12;
            index++;
        }
        while (prim != NULL) {
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
        }
        if ((g_CutsceneFlags & 0x400) == 0) {
            prim = &g_PrimBuf[self->primIndex];
            prim = prim->next;
            prim = prim->next;
            while (prim != NULL) {
                prim->drawMode = DRAW_HIDE;
                prim = prim->next;
            }
            SetStep(3);
        }
        break;
    }
}

void func_us_801B5F18(Entity* self) {
    Entity* tempEntity = &g_Entities[192];
    CreateEntityFromCurrentEntity(E_ID(ID_25), tempEntity);
    self->step++;
#ifdef VERSION_PSP
    *cutscene_data_offset_four = 0;
#else
    D_us_80183F64 = 0;
#endif
    if (D_8003C730 == 2) {
        D_8003C730 = 0;
#ifdef VERSION_PSP
        *cutscene_data_offset_four = 1;
#else
        D_us_80183F64 = 1;
#endif
    }
}

void func_us_801B5F84(Entity* self) {
    Entity* player = &PLAYER;
    s16 posY;

    switch (self->step) {
    case 0:
        InitializeEntity(g_EInitCommon);
        if (g_PlayableCharacter) {
            self->step++;
        } else if ((player->posY.i.hi + g_Tilemap.scrollY.i.hi) < 0x100 &&
                   player->posX.i.hi > 0xC0) {
            stopMusicFlag = true;
            currentMusicId = 0;
        }
        break;

    case 1:
        if (D_8003C730 != 2) {
            self->hitboxWidth = 0x10;
            self->hitboxHeight = 0x20;
            if (player->posX.i.hi > 0xE0) {
                posY = player->posY.i.hi + g_Tilemap.scrollY.i.hi;
                if (posY >= 0x60 && posY < 0xA0) {
                    func_us_801B5F18(self);
                }
            }
        } else {
            func_us_801B5F18(self);
        }
        break;

    case 2:
        break;
    }
}

void func_us_801B60C8(Entity* self) {
    switch (self->step) {
    case 0:
        InitializeEntity(g_EInitCommon);
        break;

    case 1:
        GetPlayerCollisionWith(self, 8, 0x48, 1);
        break;
    }
}

static const char* D_us_801816C8[] = {
    _S("Dracula"),
    _S("Olrox"),
    _S("Doppleganger10"),
    _S("Granfaloon"),
    _S("Minotaur&Werewolf"),
    _S("Scylla"),
    _S("Slogra&Gaibon"),
    _S("Hippogryph"),
    _S("Beelzebub"),
    _S("Succubus"),
    _S("Karasuman"),
    _S("Trevor&Grant&Sypha"),
    _S("Death"),
    _S("Cerberus"),
    _S("Richter Belmont"),
    _S("Medusa"),
    _S("The Creature"),
    _S("Lesser Demon"),
    _S("Doppleganger40"),
    _S("Akmodan II"),
    _S("Darkwing Bat"),
    _S("Galamoth"),
    _S("カ"),
    _S("カ"),
    _S("カ"),
    _S("カ"),
    _S("カ"),
    _S("Shaft"),
    _S("Lord Dracula")};

static ShopItem D_us_8018173C[] = {
    // clang-format off
    {0, DRACULA_TACTICS,            200},
    {0, SLO_GAI_TACTICS,            500},
    {0, DOPPLEGANGER10_TACTICS,     700},
    {0, HIPPOGRYPH_TACTICS,         1000},
    {0, SCYLLA_TACTICS,             1200},
    {0, MINOTAUR_WEREWOLF_TACTICS,  1400},
    {0, KARASUMAN_TACTICS,          1800},
    {0, SUCCUBUS_TACTICS,           2000},
    {0, CERBERUS_TACTICS,           2200},
    {0, OLROX_TACTICS,              2500},
    {0, GRANFALOON_TACTICS,         2800},
    {0, RICHTER_TACTICS,            3000},
    {0, DARKWING_BAT_TACTICS,       3500},
    {0, AKMODAN_II_TACTICS,         3500},
    {0, MEDUSA_TACTICS,             3500},
    {0, THE_CREATURE_TACTICS,       3500},
    {0, DEATH_TACTICS,              4000},
    {0, DOPPLEGANGER40_TACTICS,     4500},
    {0, TREVOR_GRANT_SYPHA_TACTICS, 5000},
    {0, BEELZEBUB_TACTICS,          6000},
    {0, GALAMOTH_TACTICS,           7000},
    {0, SHAFT_TACTICS,              8500},
    {0, LORD_DRACULA_TACTICS,       10000},
    // clang-format on
};

static char D_us_801817F4[] = _S("--------------");
static char D_us_80181804[] = {
    CH('-'), CH('-'), CH('-'), CH('-'), CH('-'), CH('-'), CH('-'), CH('-')};

void func_us_801B6124(Primitive* prim, Entity* arg1) {
    s16 posX, posY;
    u16 clut;
    s32 i, j;
    u16 itemId;
    u16 itemMask;
    u16 itemIndex;
    u32 price;
    Primitive* prim2;
    ShopItem* enemyTactics;

#ifdef VERSION_PSP
    g_Settings.D_8003CB00 |= 1;
#endif
    itemIndex = arg1->ext.et_801B6F30.unk82;
    posY = 16;
    for (i = 0; i < g_CurrentEntity->ext.et_801B6F30.unk84; i++) {
        enemyTactics = &D_us_8018173C[itemIndex];
        itemId = enemyTactics->itemId;
        itemMask = g_CastleFlags[(itemId >> 3) + ENEMY_TACTICS_180];
#ifdef VERSION_PSP
        if (g_UnlockAllTactics) {
            itemMask |= (1 << (itemId & 7));
        }
#endif
        if (itemMask & (1 << (itemId & 7))) {
            enemyTactics->category = 2;
            enemyTactics->price = 0;
        } else {
            enemyTactics->category = 0;
        }
#ifdef VERSION_PSP
        price = 0;
#endif
        if (g_Settings.D_8003CB00 & (1 << itemId)) {
            price = enemyTactics->price;
            clut = 0x196;
            if (g_Status.gold < price) {
                clut = 0x183;
            }
#ifdef VERSION_PSP
            prim =
                func_us_801B1064(prim, 8, posY, D_us_801816C8[itemIndex], clut);
#else
            prim = func_us_801B1064(prim, 8, posY, D_us_801816C8[itemId], clut);
#endif
            enemyTactics->category++;
        } else {
            clut = 0x191;
            prim = func_us_801B1064(prim, 8, posY, D_us_801817F4, 0x191);
        }
        posX = 0x80;
        prim2 = prim;
        for (j = 0; j < 8; j++) {
            prim->clut = clut;
            prim->x0 = posX;
            prim->y0 = posY;
            prim = prim->next;
            posX += 8;
        }
        if (enemyTactics->category) {
            func_us_801B3EC8(prim2, price, 8);
        } else {
            func_us_801B3FB4(prim2, D_us_80181804, 8, 1);
        }
        itemIndex++;
        posY += 12;
    }
    while (prim != NULL) {
        prim->drawMode = DRAW_HIDE;
        prim = prim->next;
    }
}

void func_us_801B6324(Entity* self) {
    DRAWENV drawEnv;
    DR_ENV* dr_env;
    Primitive* prim;
    s32 primIndex;
    s16 itemID;
    s32 i;
    s32 var_v1;
    u16 pads;

    RECT clipRect = {.x = 0x100, .y = 0x100, .w = 0x100, .h = 0x100};
    switch (self->step) {
    case 0:
        primIndex = g_api.AllocPrimitives(PRIM_SPRT, 0x140);
        if (primIndex != -1) {
            InitializeEntity(g_EInitCommon);
            i = 0;
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = primIndex;
            prim = &g_PrimBuf[primIndex];
            for (; prim != NULL; i++) {
                if (i < 6) {
                    prim->type = PRIM_LINE_G2;
                    prim->r0 = prim->r1 = 0x80;
                    prim->g0 = prim->g1 = 0x80;
                    prim->b0 = prim->b1 = 0x80;
                    prim->priority = 0x1FA;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 8) {
                    prim->type = PRIM_G4;
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0x80;
                    prim->y0 = prim->y1 = 0x16;
                    prim->y2 = prim->y3 = 0x7A;
                    prim->priority = 0x1FA;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 10) {
                    prim->type = PRIM_GT4;
#ifdef VERSION_PSP
                    prim->tpage = 0x110;
#else
                    prim->tpage = 0x114;
#endif
                    prim->u0 = prim->u2 = (i - 8) * 0x5C + 8;
                    prim->u1 = prim->u3 = prim->u0 + 0x5C;
                    prim->v0 = prim->v1 = 0xD;
                    prim->v2 = prim->v3 = 0x61;
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0x80;
                    prim->y0 = prim->y1 = 0x1E;
                    prim->y2 = prim->y3 = 0x72;
                    prim->priority = 0x1FC;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 10) {
                    prim->type = PRIM_G4;
                    prim->x0 = prim->x2 = 0x21;
                    prim->x1 = prim->x3 = 0xDF;
                    prim->y0 = prim->y1 = 0x2B;
                    prim->y2 = prim->y3 = 0x37;
                    PRED(prim) = 0x40;
                    PGRN(prim) = 0x10;
                    PBLU(prim) = 0x10;
                    prim->priority = 0x1FB;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 13) {
                    prim->type = PRIM_GT4;
                    prim->tpage = 0x1E;
                    prim->clut = 0x17F;
                    prim->u0 = prim->u2 = 0x58;
                    prim->u1 = prim->u3 = 0x60;
                    prim->v0 = prim->v1 = (i - 11) * 8 + 0x70;
                    prim->v2 = prim->v3 = 0x78 - (i - 11) * 8;
                    prim->x0 = prim->x2 = 0x7C;
                    prim->x1 = prim->x3 = prim->x0 + 8;
                    prim->y0 = prim->y1 = (i - 11) * 0x60 + 0x14;
                    prim->y2 = prim->y3 = prim->y0 + 8;
                    prim->priority = 0x1FC;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 13) {
                    dr_env = g_api.func_800EDB08((POLY_GT4*)prim);
                    if (dr_env != NULL) {
                        prim->type = PRIM_ENV;
                        prim->priority = 0x10;
                        prim->drawMode = DRAW_DEFAULT;
                        drawEnv = g_CurrentBuffer->draw;
                        drawEnv.isbg = 1;
                        drawEnv.clip = clipRect;
#ifdef VERSION_PSP
                        drawEnv.ofs[0] = 0;
#else
                        drawEnv.ofs[0] = 0x100;
#endif
                        drawEnv.ofs[1] = 0x100;
                        SetDrawEnv(dr_env, &drawEnv);
                    } else {
                        self->step = 0;
                        g_api.FreePrimitives(self->primIndex);
                        self->flags &= ~FLAG_HAS_PRIMS;
                        return;
                    }
                    prim = prim->next;
                    dr_env = g_api.func_800EDB08((POLY_GT4*)prim);
                    if (dr_env != NULL) {
                        prim->type = PRIM_ENV;
                        prim->priority = 0x12;
                        prim->drawMode = DRAW_UNK_800;
                    } else {
                        self->step = 0;
                        g_api.FreePrimitives(self->primIndex);
                        self->flags &= ~FLAG_HAS_PRIMS;
                        return;
                    }
                } else {
                    prim->clut = PAL_UNK_196;
                    prim->tpage = 0x1E;
                    prim->u1 = prim->v1 = 8;
                    prim->priority = 0x11;
                    prim->drawMode = DRAW_HIDE;
                }
                prim = prim->next;
            }
        }
        break;

    case 1:
        if (g_CutsceneFlags & 0x400) {
            SetStep(2);
            var_v1 = 0;
            for (i = 0; i < LEN(D_us_8018173C); i++) {
                itemID = D_us_8018173C[i].itemId;
#ifdef VERSION_PSP
                if (g_UnlockAllTactics) {
                    g_Settings.D_8003CB00 |= (1 << itemID);
                }
#endif
                if (g_Settings.D_8003CB00 & (1 << itemID)) {
                    var_v1 = i;
                }
            }
            var_v1 += 1;
            if (var_v1 > 7) {
                var_v1 -= 7;
                self->ext.et_801B6F30.unk84 = 7;
                self->ext.et_801B6F30.unk86 = var_v1;
            } else {
                self->ext.et_801B6F30.unk84 = var_v1;
                self->ext.et_801B6F30.unk86 = 0;
            }
            self->ext.et_801B6F30.unk7C = 0;
            self->ext.et_801B6F30.unk7E = 0;
            self->ext.et_801B6F30.unk80 = 0;
            self->ext.et_801B6F30.unk82 = 0;
        }
        break;

    case 2:
        self->ext.et_801B6F30.unk7C++;
        self->ext.et_801B6F30.unk7E += 0x40;
        prim = &g_PrimBuf[self->primIndex];
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x64, 0x20, 0x2C, 1);
        for (i = 0; i < 15; i++) {
            prim = prim->next;
        }
        func_us_801B6124(prim, self);
        if (self->ext.et_801B6F30.unk7C == 0x10) {
            SetStep(3);
        }
        break;

    case 3:
        pads = g_pads[0].repeat;
        if (pads & PAD_DOWN) {
            if (self->ext.et_801B6F30.unk80 <
                (self->ext.et_801B6F30.unk84 - 1)) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80++;
            } else if (
                self->ext.et_801B6F30.unk82 < self->ext.et_801B6F30.unk86) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk82++;
            }
        } else if (pads & PAD_UP) {
            if (self->ext.et_801B6F30.unk80 > 0) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80--;
            } else if (self->ext.et_801B6F30.unk82) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk82--;
            }
        }
        pads = g_pads[0].tapped;
        if (pads & BUTTON_CONFIRM) {
            itemID =
                (self->ext.et_801B6F30.unk82 + self->ext.et_801B6F30.unk80);
            switch (D_us_8018173C[itemID].category) {
            case 0:
                g_api.PlaySfx(SFX_UI_ERROR);
                break;

            case 1:
                if (g_Status.gold < D_us_8018173C[itemID].price) {
                    g_api.PlaySfx(SFX_UI_ERROR);
                } else {
                    g_Status.gold -= D_us_8018173C[itemID].price;
                    itemID = D_us_8018173C[itemID].itemId;
                    g_CastleFlags[(itemID >> 3) + ENEMY_TACTICS_180] |=
                        (1 << (itemID & 7));
                    SetStep(4);
                    g_api.PlaySfx(SFX_UI_CONFIRM);
                }
                break;

            case 2:
            case 3:
                SetStep(4);
                g_api.PlaySfx(SFX_UI_CONFIRM);
                break;
            }
        } else if (pads & BUTTON_CANCEL) {
            SetStep(7);
        }
        prim = &g_PrimBuf[self->primIndex];
        for (i = 0; i < 10; i++) {
            prim = prim->next;
        }
        prim->y0 = prim->y1 = self->ext.et_801B6F30.unk80 * 0xC + 0x1F;
        prim->y2 = prim->y3 = prim->y0 + 0xC;
        prim->drawMode = DRAW_COLORS;
        prim = prim->next;
        if (self->ext.et_801B6F30.unk82) {
            prim->drawMode = DRAW_DEFAULT;
        } else {
            prim->drawMode = DRAW_HIDE;
        }
        prim = prim->next;
        if (self->ext.et_801B6F30.unk82 < self->ext.et_801B6F30.unk86) {
            prim->drawMode = DRAW_DEFAULT;
        } else {
            prim->drawMode = DRAW_HIDE;
        }
        prim = prim->next;
        prim = prim->next;
        prim = prim->next;
        func_us_801B6124(prim, self);
        break;

    case 4:
#ifdef VERSION_PSP
        D_psp_08C630DC =
            self->ext.et_801B6F30.unk82 + self->ext.et_801B6F30.unk80;
#endif
        D_80097C98 = D_us_8018173C[self->ext.et_801B6F30.unk82 +
                                   self->ext.et_801B6F30.unk80]
                         .itemId -
                     0x80000000;
        break;

    case 7:
        if ((g_CutsceneFlags & 0x200) == 0) {
            prim = &g_PrimBuf[self->primIndex];
            for (i = 0; i < 10; i++) {
                prim = prim->next;
            }
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
            prim->drawMode = DRAW_HIDE;
            self->ext.et_801B6F30.unk7E = 0x400;
            self->ext.et_801B6F30.unk7C = 0x10;
            self->step++;
            g_CutsceneFlags |= 0x200;
        }
        break;

    case 8:
        self->ext.et_801B6F30.unk7C--;
        self->ext.et_801B6F30.unk7E -= 0x40;
        prim = &g_PrimBuf[self->primIndex];
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x64, 0x20, 0x2C, 1);
        if (!self->ext.et_801B6F30.unk7C) {
            prim = &g_PrimBuf[self->primIndex];
            for (i = 0; i < 15; i++) {
                prim = prim->next;
            }
            while (prim != NULL) {
                prim->drawMode = DRAW_HIDE;
                prim = prim->next;
            }
            SetStep(9);
        }
        break;

    case 9:
        g_CutsceneFlags &= ~0x400;
        DestroyEntity(self);
        break;
    }
}

static char D_us_8018180C[] = _S("--------------");

void func_us_801B6E20(Primitive* prim, Entity* arg1) {
    s16 posX, posY;
    u16 enemyIndex;
    s32 i;
    u16 enemyId;
    u16 enemyMask;

    enemyIndex = arg1->ext.et_801B6F30.unk82 * 2;
    posY = 4;
    for (i = 0; i < 14; i++) {
        if ((i & 1) == 0) {
#ifdef VERSION_PSP
            posY += 12;
            posX = 4;
#else
            posX = 0;
            posY += 12;
#endif
        } else {
            posX = 0x78;
        }
        enemyId = LIB_ENEMY(enemyIndex);
        if (enemyId != 0xFFFF) {
            enemyMask =
                g_CastleFlags[(LIB_BIT(enemyIndex) >> 3) + ENEMY_LIST_190];
            if (enemyMask & (1 << (LIB_BIT(enemyIndex) & 7))) {
                prim = func_us_801B1064(
                    prim, posX, posY, g_api.enemyDefs[enemyId].name, 0x196);
            } else {
                prim = func_us_801B1064(prim, posX, posY, D_us_8018180C, 0x191);
            }
        }
        enemyIndex++;
    }
    while (prim != NULL) {
        prim->drawMode = DRAW_HIDE;
        prim = prim->next;
    }
}

void func_us_801B6F30(Entity* self) {
    DRAWENV drawEnv;
    DR_ENV* dr_env;
    Primitive* prim;
    s32 primIndex;
    s16 enemyIndex;
    s32 i;
    u16 pads;
    Entity* tempEntity;

    RECT clipRect = {.x = 0x100, .y = 0x100, .w = 0x100, .h = 0x100};
    switch (self->step) {
    case 0:
        primIndex = g_api.AllocPrimitives(PRIM_SPRT, 0x140);
        if (primIndex != -1) {
            InitializeEntity(g_EInitCommon);
            i = 0;
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = primIndex;
            prim = &g_PrimBuf[primIndex];
            for (; prim != NULL; i++) {
                if (i < 6) {
                    prim->type = PRIM_LINE_G2;
                    prim->r0 = prim->r1 = 0x80;
                    prim->g0 = prim->g1 = 0x80;
                    prim->b0 = prim->b1 = 0x80;
                    prim->priority = 0x1FA;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 8) {
                    prim->type = PRIM_G4;
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0x80;
                    prim->y0 = prim->y1 = 0x16;
                    prim->y2 = prim->y3 = 0x7A;
                    prim->priority = 0x1FA;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 10) {
                    prim->type = PRIM_GT4;
#ifdef VERSION_PSP
                    prim->tpage = 0x110;
                    prim->u0 = prim->u2 = (i - 8) * 0x74 + 4;
#else
                    prim->tpage = 0x114;
                    prim->u0 = prim->u2 = (i - 8) * 0x74;
#endif
                    prim->u1 = prim->u3 = prim->u0 + 0x74;
                    prim->v0 = prim->v1 = 0xD;
                    prim->v2 = prim->v3 = 0x61;
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0x80;
                    prim->y0 = prim->y1 = 0x1E;
                    prim->y2 = prim->y3 = 0x72;
                    prim->priority = 0x1FC;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 10) {
                    prim->type = PRIM_G4;
                    prim->y0 = prim->y1 = 0x2B;
                    prim->y2 = prim->y3 = 0x37;
#ifdef VERSION_PSP
                    PRED(prim) = 0x40;
                    PGRN(prim) = 0x10;
                    PBLU(prim) = 0x10;
#else
                    PRED(prim) = 0;
                    PGRN(prim) = 0x20;
                    PBLU(prim) = 0x40;
#endif
                    prim->priority = 0x1FB;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 13) {
                    prim->type = PRIM_GT4;
                    prim->tpage = 0x1E;
                    prim->clut = 0x17F;
                    prim->u0 = prim->u2 = 0x58;
                    prim->u1 = prim->u3 = 0x60;
                    prim->v0 = prim->v1 = (i - 11) * 8 + 0x70;
                    prim->v2 = prim->v3 = 0x78 - (i - 11) * 8;
                    prim->x0 = prim->x2 = 0x7C;
                    prim->x1 = prim->x3 = prim->x0 + 8;
                    prim->y0 = prim->y1 = (i - 11) * 0x60 + 0x14;
                    prim->y2 = prim->y3 = prim->y0 + 8;
                    prim->priority = 0x1FC;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 13) {
                    dr_env = g_api.func_800EDB08((POLY_GT4*)prim);
                    if (dr_env != NULL) {
                        prim->type = PRIM_ENV;
                        prim->priority = 0x10;
                        prim->drawMode = DRAW_DEFAULT;
                        drawEnv = g_CurrentBuffer->draw;
                        drawEnv.isbg = 1;
                        drawEnv.clip = clipRect;
#ifdef VERSION_PSP
                        drawEnv.ofs[0] = 0;
#else
                        drawEnv.ofs[0] = 0x100;
#endif
                        drawEnv.ofs[1] = 0x100;
                        SetDrawEnv(dr_env, &drawEnv);
                    } else {
                        self->step = 0;
                        g_api.FreePrimitives(self->primIndex);
                        self->flags &= ~FLAG_HAS_PRIMS;
                        return;
                    }
                    prim = prim->next;
                    dr_env = g_api.func_800EDB08((POLY_GT4*)prim);
                    if (dr_env != NULL) {
                        prim->type = PRIM_ENV;
                        prim->priority = 0x13;
                        prim->drawMode = DRAW_UNK_800;
                    } else {
                        self->step = 0;
                        g_api.FreePrimitives(self->primIndex);
                        self->flags &= ~FLAG_HAS_PRIMS;
                        return;
                    }
                } else {
                    prim->clut = PAL_UNK_196;
                    prim->tpage = 0x1E;
                    prim->u1 = prim->v1 = 8;
                    prim->priority = 0x11;
                    prim->drawMode = DRAW_HIDE;
                }
                prim = prim->next;
            }
            self->ext.et_801B6F30.unk80 = 0;
            self->ext.et_801B6F30.unk82 = 0;
            self->ext.et_801B6F30.unk84 = 0;
        }
        break;

    case 1:
        if (g_CutsceneFlags & 0x400) {
            SetStep(2);
            self->ext.et_801B6F30.unk7C = 0;
            self->ext.et_801B6F30.unk7E = 0;
            g_CastleFlags[ENEMY_LIST_190] |= 1;
        }
        break;

    case 2:
        self->ext.et_801B6F30.unk7C++;
        self->ext.et_801B6F30.unk7E += 0x40;
        prim = &g_PrimBuf[self->primIndex];
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x7C, 0x20, 7, 1);
        for (i = 0; i < 15; i++) {
            prim = prim->next;
        }
        func_us_801B6E20(prim, self);
        if (self->ext.et_801B6F30.unk7C == 0x10) {
            SetStep(3);
        }
        break;

    case 3:
        pads = g_pads[0].repeat;
        if (pads & PAD_DOWN) {
            if (self->ext.et_801B6F30.unk80 < 6) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80++;
            } else if (self->ext.et_801B6F30.unk82 < LIB_TOP_MAX) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk82++;
            }
        } else if (pads & PAD_UP) {
            if (self->ext.et_801B6F30.unk80 > 0) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80--;
            } else if (self->ext.et_801B6F30.unk82) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk82--;
            }
        }
        if (pads & (PAD_R1 | PAD_R2)) {
            if (self->ext.et_801B6F30.unk80 < 6) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80 = 6;
            } else if (self->ext.et_801B6F30.unk82 < LIB_TOP_MAX) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk82 += 7;
                if (self->ext.et_801B6F30.unk82 > LIB_TOP_MAX) {
                    self->ext.et_801B6F30.unk82 = LIB_TOP_MAX;
                }
            }
        } else {
            if (pads & (PAD_L1 | PAD_L2)) {
                if (self->ext.et_801B6F30.unk80 > 0) {
                    g_api.PlaySfx(SFX_UI_MOVE);
                    self->ext.et_801B6F30.unk80 = 0;
                } else if (self->ext.et_801B6F30.unk82) {
                    g_api.PlaySfx(SFX_UI_MOVE);
                    if (self->ext.et_801B6F30.unk82 > 7) {
                        self->ext.et_801B6F30.unk82 -= 7;
                    } else {
                        self->ext.et_801B6F30.unk82 = 0;
                    }
                }
            }
        }
        if (pads & (PAD_LEFT | PAD_RIGHT)) {
            if (self->ext.et_801B6F30.unk84) {
                self->ext.et_801B6F30.unk84 = 0;
            } else {
                self->ext.et_801B6F30.unk84 = 1;
            }
            g_api.PlaySfx(SFX_UI_MOVE);
        }
        pads = g_pads[0].tapped;
        if (pads & BUTTON_CONFIRM) {
            enemyIndex =
                (self->ext.et_801B6F30.unk82 + self->ext.et_801B6F30.unk80) *
                    2 +
                self->ext.et_801B6F30.unk84;
            if (g_CastleFlags[(LIB_BIT(enemyIndex) >> 3) + ENEMY_LIST_190] &
                (1 << (LIB_BIT(enemyIndex) & 7))) {
                g_api.PlaySfx(SFX_UI_CONFIRM);
                SetStep(4);
            } else {
                g_api.PlaySfx(SFX_UI_ERROR);
            }
        } else if (pads & BUTTON_CANCEL) {
            SetStep(7);
        }
        prim = &g_PrimBuf[self->primIndex];
        for (i = 0; i < 10; i++) {
            prim = prim->next;
        }
        if (!self->ext.et_801B6F30.unk84) {
            prim->x0 = prim->x2 = 9;
            prim->x1 = prim->x3 = 0x80;
        } else {
            prim->x0 = prim->x2 = 0x80;
            prim->x1 = prim->x3 = 0xF7;
        }
        prim->y0 = prim->y1 = self->ext.et_801B6F30.unk80 * 0xC + 0x1F;
        prim->y2 = prim->y3 = prim->y0 + 0xC;
        prim->drawMode = DRAW_COLORS;
        prim = prim->next;
        if (self->ext.et_801B6F30.unk82) {
            prim->drawMode = DRAW_DEFAULT;
        } else {
            prim->drawMode = DRAW_HIDE;
        }
        prim = prim->next;
        if (self->ext.et_801B6F30.unk82 < LIB_TOP_MAX) {
            prim->drawMode = DRAW_DEFAULT;
        } else {
            prim->drawMode = DRAW_HIDE;
        }
        prim = prim->next;
        prim = prim->next;
        prim = prim->next;
        func_us_801B6E20(prim, self);
        break;

    case 4:
        switch (self->step_s) {
        case 0:
            if (g_api.func_80131F68()) {
                g_api.PlaySfx(SET_STOP_MUSIC);
            }
            self->step_s++;
            break;

        case 1:
            if (!g_api.func_80131F68()) {
                if ((g_CutsceneFlags & 0x200) == 0) {
                    prim = &g_PrimBuf[self->primIndex];
                    for (i = 0; i < 10; i++) {
                        prim = prim->next;
                    }
                    prim->drawMode = DRAW_HIDE;
                    prim = prim->next;
                    prim->drawMode = DRAW_HIDE;
                    prim = prim->next;
                    prim->drawMode = DRAW_HIDE;
                    self->ext.et_801B6F30.unk7E = 0x400;
                    self->ext.et_801B6F30.unk7C = 0x10;
                    SetStep(5);
                }
            }
            break;
        }
        break;

    case 5:
        self->ext.et_801B6F30.unk7C--;
        self->ext.et_801B6F30.unk7E -= 0x40;
        prim = &g_PrimBuf[self->primIndex];
#ifdef VERSION_PSP
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C, 0x7C, 0x20, 7, 1);
#else
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C, 0x74, 0x20, 7, 1);
#endif
        if (!self->ext.et_801B6F30.unk7C) {
            for (i = 0; i < 6; i++) {
                prim->drawMode = DRAW_HIDE;
                prim = prim->next;
            }
            for (i = 0; i < 9; i++) {
                prim = prim->next;
            }
            while (prim != NULL) {
                prim->drawMode = DRAW_HIDE;
                prim = prim->next;
            }
            tempEntity = self + 1;
            CreateEntityFromCurrentEntity(E_ID(ID_2F), tempEntity);
            tempEntity->params =
                (self->ext.et_801B6F30.unk82 + self->ext.et_801B6F30.unk80) *
                    2 +
                self->ext.et_801B6F30.unk84;
            g_CutsceneFlags &= ~0x400;
            SetStep(1);
        }
        break;

    case 7:
        if ((g_CutsceneFlags & 0x200) == 0) {
            prim = &g_PrimBuf[self->primIndex];
            for (i = 0; i < 10; i++) {
                prim = prim->next;
            }
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
            prim->drawMode = DRAW_HIDE;
            self->ext.et_801B6F30.unk7E = 0x400;
            self->ext.et_801B6F30.unk7C = 0x10;
            self->step++;
            g_CutsceneFlags |= 0x200;
        }
        break;

    case 8:
        self->ext.et_801B6F30.unk7C--;
        self->ext.et_801B6F30.unk7E -= 0x40;
        prim = &g_PrimBuf[self->primIndex];
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x74, 0x20, 7, 1);
        if (!self->ext.et_801B6F30.unk7C) {
            prim = &g_PrimBuf[self->primIndex];
            for (i = 0; i < 15; i++) {
                prim = prim->next;
            }
            while (prim != NULL) {
                prim->drawMode = DRAW_HIDE;
                prim = prim->next;
            }
            SetStep(9);
        }
        break;

    case 9:
        g_CutsceneFlags &= ~0x400;
        DestroyEntity(self);
        break;
    }
}

#ifdef VERSION_PSP
extern const char** D_us_8018181C;
extern const char** D_us_8018187C;
#else
static const char* D_us_8018181C[] = {
    _S("None"),
    _S("だいハート"),
    _S("$1"),
    _S("$25"),
    _S("$50"),
    _S("$100"),
    _S("$250"),
    _S("$400"),
    _S("$700"),
    _S("$1000"),
    _S("$2000"),
    _S("$5000"),
    _S("ハートのうつわ"),
    _S("にく"),
    _S("ナイフ(サブ)"),
    _S("Axe"),
    _S("クロス(サブ)"),
    _S("せいすい(サブ)"),
    _S("とけい(サブ)"),
    _S("バイブル(サブ)"),
    _S("けんじのいし(サブ)"),
    _S("ビブーティ(サブ)"),
    _S("アグネアのぶき(サブ)"),
    _S("ライフアップポーション")};
static const char* D_us_8018187C[] = {
    _S("HOL"), _S("DRK"), _S("FIR"), _S("THU"), _S("ICE"), _S("H2O"),
    _S("STN"), _S("PSN"), _S("CUR"), _S("CUT"), _S("HIT")};
#endif

static u16 D_us_801818A8[] = {
    ELEMENT_HOLY,  ELEMENT_DARK,  ELEMENT_FIRE,  ELEMENT_THUNDER,
    ELEMENT_ICE,   ELEMENT_WATER, ELEMENT_STONE, ELEMENT_POISON,
    ELEMENT_CURSE, ELEMENT_CUT,   ELEMENT_HIT};

#ifdef VERSION_PSP
extern const char** D_us_801818C0;
#else
static const char* D_us_801818C0[] = {
    _S("LV"),         _S("HP"),       "$EF",       _S("Exp"),
    _S("Drop Items"), _S("No."),      "\x1F\x1F",  _S("Strong VS."),
    _S("Immune"),     _S("Weak VS."), _S("Absorb")};
#endif

static const char* D_us_801818EC = _S("????????");
static const char* D_us_801818F0 = _S("????");

const char* func_us_801B7C94(u16 itemId) {
    const char* name;
    if (itemId >= 0x80) {
        itemId -= 0x80;
#if defined(MOD_SATURN)
        // Saturn port: the new hand items drop as combined ids from
        // MOD_DROP_HAND_BASE (Man Eater: Woodland Shield, Gargoyle: Double
        // Blade Axe); read as body ids they named a string past the list
        if (itemId >= MOD_DROP_HAND_BASE) {
            name = g_api.equipDefs[itemId - (MOD_DROP_HAND_BASE -
                                             NUM_HAND_ITEMS)]
                       .name;
        } else
#endif
            if (itemId < NUM_HAND_ITEMS) {
            name = g_api.equipDefs[itemId].name;
        } else {
            itemId -= NUM_HAND_ITEMS;
            name = g_api.accessoryDefs[itemId].name;
        }
    } else {
        name = D_us_8018181C[itemId];
    }
    return name;
}

Primitive* func_us_801B7D10(Primitive* prim, u16 arg1, s16 posY) {
    s16 x = 0x84;
    s16 y = 8;
    s32 i;
    if ((arg1 & ELEMENT_ALL) == 0) {
        prim = func_us_801B1064(prim, 0x84, posY, D_us_8018181C[0], 0x159);
    } else {
        for (i = 0; i < LEN(D_us_801818A8); i++) {
            if (arg1 & D_us_801818A8[i]) {
                prim = func_us_801B1064(prim, x, posY, D_us_8018187C[i], 0x15C);
                x += 0x1C;
            }
        }
    }
    return prim;
}

void func_us_801B7DF8(Primitive* prim, Entity* arg1, s16 enemyId) {
    s16 posX, posY;
    s32 i;
    EnemyDef* enemyDef;
    u8 params;
    u16 clut;
#ifdef MOD_SATURN
    u16 strengths, immunes, level;
#endif

    enemyDef = &g_api.enemyDefs[enemyId];
#ifdef MOD_SATURN
    strengths = enemyDef->strengths;
    immunes = enemyDef->immunes;
    level = enemyDef->level;
    if (LIB_MARIA_PSP(arg1->params)) {
        strengths = MOD_PSP_MARIA_STRONG;
        immunes = MOD_PSP_MARIA_IMMUNE;
        level = MOD_PSP_MARIA_LEVEL;
    }
#define LIB_STRONG strengths
#define LIB_IMMUNE immunes
#define LIB_LEVEL level
#else
#define LIB_STRONG enemyDef->strengths
#define LIB_IMMUNE enemyDef->immunes
#define LIB_LEVEL enemyDef->level
#endif
    prim->drawMode = DRAW_DEFAULT;
    prim = prim->next;
    prim->drawMode = DRAW_DEFAULT;
    prim = prim->next;

    posX = 0x80;
    posY = 0x34;
    prim = func_us_801B1064(
        prim, posX, posY, D_us_801818C0[7], 0x196); // "Strong VS."
    prim = func_us_801B7D10(prim, LIB_STRONG, 0x3D);
    posY += 0x14;
    prim =
        func_us_801B1064(prim, posX, posY, D_us_801818C0[8], 0x196); // "Immune"
    prim = func_us_801B7D10(prim, LIB_IMMUNE, 0x51);
    posY += 0x14;
    prim = func_us_801B1064(
        prim, posX, posY, D_us_801818C0[9], 0x196); // "Weak VS."
    prim = func_us_801B7D10(prim, enemyDef->weaknesses, 0x65);
    posY += 0x14;
    prim = func_us_801B1064(
        prim, posX, posY, D_us_801818C0[10], 0x196); // "Absorb"
    prim = func_us_801B7D10(prim, enemyDef->absorbs, 0x79);

    posX = 0x18;
    prim = func_us_801B1064(prim, posX, 0x28, enemyDef->name, 0x196);

    posX += 0xA0;
    prim = func_us_801B1064(prim, posX, 0x28, D_us_801818C0[5], 0x196); // "No."

    posX = 0xD0;
    func_us_801B3EC8(prim, arg1->params + 1, 3);
    for (i = 0; i < 3; i++) {
        prim->clut = PAL_UNK_196;
        prim->x0 = posX;
        prim->y0 = 0x28;
        prim = prim->next;
        posX += 8;
    }
    posX = 0x1C;
    prim = func_us_801B1064(prim, posX, 0x34, D_us_801818C0[0], 0x196); // "LV"
    posX += 0x12;
    if (arg1->params) {
        func_us_801B3EC8(prim, LIB_LEVEL, 2);
    } else {
        func_us_801B3FB4(prim, D_us_801818C0[6], 2, 1); // "??"
    }
    for (i = 0; i < 2; i++) {
        prim->clut = PAL_UNK_196;
        prim->x0 = posX;
        prim->y0 = 0x34;
        if (prim->drawMode != DRAW_HIDE) {
            posX += 8;
        }
        prim = prim->next;
    }
    posX = 0x44;
    prim = func_us_801B1064(prim, posX, 0x34, D_us_801818C0[1], 0x196); // "HP"
    posX += 0x12;
    if (enemyDef->hitPoints > 9999) {
        func_us_801B3FB4(prim, D_us_801818F0, 4, 1); // "????"
    } else {
        func_us_801B3EC8(prim, enemyDef->hitPoints, 4);
    }
    for (i = 0; i < 4; i++) {
        prim->clut = PAL_UNK_196;
        prim->x0 = posX;
        prim->y0 = 0x34;
        if (prim->drawMode != DRAW_HIDE) {
            posX += 8;
        }
        prim = prim->next;
    }
    posX = 0x80;
    prim = func_us_801B1064(prim, posX, 0xA6, D_us_801818C0[3], 0x196); // "Exp"
    posX += 0x1C;
    func_us_801B3EC8(prim, enemyDef->exp, 4);
    for (i = 0; i < 4; i++) {
        prim->clut = PAL_UNK_196;
        prim->x0 = posX;
        prim->y0 = 0xA6;
        prim = prim->next;
        posX += 8;
    }
    posX = 0x80;
    prim = func_us_801B1064(
        prim, posX, 0x85, D_us_801818C0[4], 0x196); // "Drop Items"
    posX = enemyDef->uncommonItemId;
    if (posX != 0) {
        clut = 0x196;
    } else {
        clut = 0x159;
    }
    prim = func_us_801B1064(prim, 0x84, 0x8F, func_us_801B7C94(posX), clut);
    params = LIB_BIT(arg1->params);
    if ((g_CastleFlags[(params >> 3) + ENEMY_LIST_RAREDROP_1B0] &
         (1 << (params & 7))) ||
        !enemyDef->rareItemId) {
        prim = func_us_801B1064(
            prim, 0x84, 0x99, func_us_801B7C94(enemyDef->rareItemId), clut);
    } else {
        prim = func_us_801B1064(
            prim, 0x84, 0x99, D_us_801818EC, 0x196); // "????????"
    }
    while (prim != NULL) {
        prim->drawMode = DRAW_HIDE;
        prim = prim->next;
    }
#undef LIB_STRONG
#undef LIB_IMMUNE
#undef LIB_LEVEL
}

void func_us_801B8234(Entity* self) {
    Primitive* prim;
    s32 primIndex;
    s32 i;
    s16 enemyIndex;
    u16 enemyId;
    u16 pads;
    u8* pix;
    const char* s3;
    EnemyDef* enemyDef;
    s32 pad[12];

    enemyId = LIB_ENEMY(self->params);
    switch (self->step) {
    case 0:
#ifdef VERSION_PSP
        primIndex = g_api.AllocPrimitives(PRIM_SPRT, 0xAE);
#else
        primIndex = g_api.AllocPrimitives(PRIM_SPRT, 0x86);
#endif
        if (primIndex != -1) {
            InitializeEntity(g_EInitCommon);
            i = 0;
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = primIndex;
            prim = &g_PrimBuf[primIndex];
            for (; prim != NULL; i++) {
                if (i < 6) {
                    prim->type = PRIM_LINE_G2;
                    prim->r0 = prim->r1 = 0x80;
                    prim->g0 = prim->g1 = 0x80;
                    prim->b0 = prim->b1 = 0x80;
                    prim->priority = 0x1FA;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 8) {
                    prim->type = PRIM_G4;
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0x80;
                    prim->y0 = prim->y1 = 0x1E;
                    prim->y2 = prim->y3 = 0xDE;
                    prim->priority = 0x1FA;
                    prim->drawMode = DRAW_HIDE;
#ifdef VERSION_PSP
                } else if (i < 10) {
                    prim->type = PRIM_GT4;
                    prim->tpage = 0x110;
                    prim->u0 = prim->u2 = (i - 9) * 0x78 + 0x80;
                    prim->u1 = prim->u3 = prim->u0 + 0x78;
                    prim->v0 = prim->v1 = 0x26;
                    prim->v2 = prim->v3 = prim->v0 + 0xB0;
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0x80;
                    prim->y0 = prim->y1 = 0x26;
                    prim->y2 = prim->y3 = 0xD6;
                    prim->priority = 0x1FC;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 10) {
                    prim->type = PRIM_GT4;
                    prim->tpage = 0x2110;
                    prim->u0 = prim->u2 = 0x20;
                    prim->u1 = prim->u3 = prim->u0 + 0x5F;
                    prim->v0 = prim->v1 = 0;
                    prim->v2 = prim->v3 = prim->v0 + 0x6F;
                    prim->x0 = prim->x2 = 0x1C;
                    prim->x1 = prim->x3 = prim->x0 + (prim->u1 - prim->u0);
                    prim->y0 = prim->y1 = 0x3C;
                    prim->y2 = prim->y3 = prim->y0 + (prim->v2 - prim->v0);
                    prim->priority = 0x11;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 11) {
                    prim->tpage = 0x10;
                    prim->clut = PAL_UNK_1A1;
                    prim->u0 = 0;
                    prim->v0 = 0x80;
                    prim->u1 = 0x80;
                    prim->v1 = 0x40;
                    prim->x0 = 0x84;
                    prim->y0 = 0x38;
                    prim->priority = 0x12;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 12) {
                    prim->tpage = 0x10;
                    prim->clut = PAL_UNK_1A1;
                    prim->u0 = 0;
                    prim->v0 = 0xC0;
                    prim->v1 = 0x20;
                    prim->u1 = 0xF0;
                    prim->x0 = 0xA;
                    prim->y0 = 0xB4;
                    prim->priority = 0x12;
                    prim->drawMode = DRAW_HIDE;
#else
                } else if (i < 10) {
                    prim->type = PRIM_GT4;
                    prim->tpage = 0x114;
                    prim->u0 = prim->u2 = (i - 9) * 0x70 + 0x80;
                    prim->u1 = prim->u3 = prim->u0 + 0x70;
                    prim->v0 = prim->v1 = 0x26;
#if defined(MOD_PTBR)
                    prim->v2 = prim->v3 = prim->v0 + 0xB8;
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0x80;
                    prim->y0 = prim->y1 = 0x26;
                    prim->y2 = prim->y3 = 0xDE;
#else
                    prim->v2 = prim->v3 = prim->v0 + 0xB0;
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0x80;
                    prim->y0 = prim->y1 = 0x26;
                    prim->y2 = prim->y3 = 0xD6;
#endif
                    prim->priority = 0x1FC;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 10) {
                    prim->tpage = 0x110;
                    prim->u0 = 0x20;
                    prim->v0 = 0;
                    prim->u1 = 0x60;
                    prim->v1 = 0x70;
                    prim->x0 = 0x18;
                    prim->y0 = 0x3C;
                    prim->priority = 0x11;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 11) {
                    prim->tpage = 0x10;
                    prim->clut = PAL_UNK_1A1;
                    prim->u0 = 0;
#if defined(MOD_PTBR)
                    prim->v0 = MOD_LIB_DESC_Y - 0x100;
                    prim->v1 = 0x2E;
                    prim->u1 = 0xF0;
                    prim->x0 = 0x18;
                    prim->y0 = 0xB0;
#else
                    prim->v0 = 0xC0;
                    prim->v1 = 0x20;
                    prim->u1 = 0xF0;
                    prim->x0 = 0x18;
                    prim->y0 = 0xB4;
#endif
                    prim->priority = 0x12;
                    prim->drawMode = DRAW_HIDE;
#endif
                } else {
                    prim->clut = PAL_UNK_196;
                    prim->tpage = 0x1E;
                    prim->u1 = prim->v1 = 8;
                    prim->priority = 0x12;
                    prim->drawMode = DRAW_HIDE;
                }
                prim = prim->next;
            }
        }
        break;

    case 1:
        switch (self->step_s) {
        case 0:
            self->step_s++;
            break;

        case 1:
            // (Saturn port: bits >= MOD_LIB_RETAIL_ENEMIES load from
            // BIN/MONSTER2.BIN, src/dra/cd.c)
#ifdef MOD_SATURN
            if (g_api.LoadMonsterLibrarianPreview(LIB_PIC(self->params))) {
                self->step_s++;
            }
#else
            if (g_api.LoadMonsterLibrarianPreview(LIB_BIT(self->params))) {
                self->step_s++;
            }
#endif
            break;

        case 2:
            if (!g_IsUsingCd) {
                self->step_s++;
            }
            break;

        case 3:
            func_us_801B11A0(0, 0x180, 0x100, 0x60);
            pix = g_Pix[0];
#ifdef VERSION_PSP
            for (i = 0; i < 4; i++) {
                s3 = D_psp_092A5F98[i];
            }
            enemyDef = &g_api.enemyDefs[enemyId];
            s3 = D_psp_092A5F48[self->params];
            func_us_801B0C40(pix, s3, 0, 0x1C0, 0x6E);
            s3 = D_psp_092A5F40[self->params];
            func_us_801B0C40(pix, s3, 0, 0x1D0, 0x6E);
#elif defined(MOD_SATURN)
            if (LIB_BIT(self->params) >= MOD_LIB_RETAIL_ENEMIES) {
#if defined(MOD_KO)
                // [bit-146] <= 16: ARC_KO slot table (0x8009EAF0, 17 rows),
                // otherwise (bit 163 = 0x1A9) the static literal in the LIB overlay.
                if (LIB_BIT(self->params) - MOD_LIB_RETAIL_ENEMIES <=
                    KO_LIBDESC_SLOT_MAX) {
                    s3 = KO_LIBDESC_TBL[LIB_BIT(self->params) -
                                        MOD_LIB_RETAIL_ENEMIES];
                } else {
                    s3 = g_ModLibKoPlace1A9;
                }
#else
                s3 = g_ModLibDesc[LIB_BIT(self->params) -
                                  MOD_LIB_RETAIL_ENEMIES];
#endif
                // Maria: the PSP port's own description with the PSP and
                // JP dubs (gen_saturn_bestiary.py DESC_PSP)
                if (LIB_BIT(self->params) == MOD_LIB_DESC_PSP_BIT &&
                    MOD_PSP_DUB_ON()) {
#if defined(MOD_KO)
                    s3 = g_ModLibKoPlaceMariaPsp;
#else
                    s3 = g_ModLibDescPsp;
#endif
                }
            } else {
                s3 = D_us_80180FB4[LIB_BIT(self->params)];
            }
#if defined(MOD_PTBR)
            if (MOD_PTBR_TEXT_PT()) {
                s3 = LIB_BIT(self->params) == MOD_LIB_DESC_PSP_BIT &&
                             MOD_PSP_DUB_ON()
                         ? g_ModPtbrLibDescPsp
                         : g_ModPtbrLibDesc[LIB_BIT(self->params)];
            }
#endif
            func_us_801B0C40(pix, s3, 0, MOD_LIB_DESC_Y, 0x6E);
#else
            s3 = D_us_80180FB4[self->params];
            func_us_801B0C40(pix, s3, 0, 0x1C0, 0x6E);
#endif
            self->step_s++;
            break;

        case 4:
            prim = &g_PrimBuf[self->primIndex];
            for (i = 0; i < 10; i++) {
                prim = prim->next;
            }
            func_us_801B7DF8(prim, self, enemyId);
            self->ext.et_801B6F30.unk7C = 0;
            self->ext.et_801B6F30.unk7E = 0;
            SetStep(2);
        }
        break;

    case 2:
        self->ext.et_801B6F30.unk7C++;
        self->ext.et_801B6F30.unk7E += 0x40;
        prim = &g_PrimBuf[self->primIndex];
#ifdef VERSION_PSP
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x80, 0x20, 7, 0);
#else
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x78, 0x20, 7, 0);
#endif
        if (self->ext.et_801B6F30.unk7C == 0x10) {
            SetStep(3);
        }
        break;

    case 3:
        pads = g_pads[0].repeat;
        if (pads & PAD_RIGHT) {
#ifdef VERSION_PSP
            for (enemyIndex = self->params + 1; enemyIndex < 0x94;
                 enemyIndex++) {
#elif defined(MOD_SATURN)
            for (enemyIndex = self->params + 1;
                 enemyIndex < MOD_LIB_NUM_ENEMIES; enemyIndex++) {
#else
            for (enemyIndex = self->params + 1; enemyIndex < 0x92;
                 enemyIndex++) {
#endif
                if (g_CastleFlags[(LIB_BIT(enemyIndex) >> 3) + ENEMY_LIST_190] &
                        (1 << (LIB_BIT(enemyIndex) & 7)) &&
                    LIB_ENEMY(self->params) != 0xFFFF) {
                    self->params = enemyIndex;
                    SetStep(4);
                    break;
                }
            }
        } else if (pads & PAD_LEFT) {
            for (enemyIndex = self->params - 1; enemyIndex >= 0; enemyIndex--) {
                if (g_CastleFlags[(LIB_BIT(enemyIndex) >> 3) + ENEMY_LIST_190] &
                        (1 << (LIB_BIT(enemyIndex) & 7)) &&
                    LIB_ENEMY(self->params) != 0xFFFF) {
                    self->params = enemyIndex;
                    SetStep(4);
                    break;
                }
            }
        }
        pads = g_pads[0].tapped;
        if (pads & BUTTON_CANCEL) {
            SetStep(7);
        }
        break;

    case 4:
        self->ext.et_801B6F30.unk7E = 0x400;
        self->ext.et_801B6F30.unk7C = 0x10;
        SetStep(5);
        break;

    case 5:
        self->ext.et_801B6F30.unk7C--;
        self->ext.et_801B6F30.unk7E -= 0x40;
        prim = &g_PrimBuf[self->primIndex];
#ifdef VERSION_PSP
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C, 0x80, 0x20, 7, 0);
#else
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C, 0x74, 0x20, 7, 0);
#endif
        if (!self->ext.et_801B6F30.unk7C) {
            SetStep(1);
        }
        break;

    case 7:
        if ((g_CutsceneFlags & 0x200) == 0) {
            self->ext.et_801B6F30.unk7E = 0x400;
            self->ext.et_801B6F30.unk7C = 0x10;
            self->step++;
        }
        break;

    case 8:
        self->ext.et_801B6F30.unk7C--;
        self->ext.et_801B6F30.unk7E -= 0x40;
        prim = &g_PrimBuf[self->primIndex];
#ifdef VERSION_PSP
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x80, 0x20, 7, 0);
#else
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x74, 0x20, 7, 0);
#endif
        if (!self->ext.et_801B6F30.unk7C) {
            g_CutsceneFlags |= 0x400;
            DestroyEntity(self);
        }
        break;
    }
}

#ifdef VERSION_PSP
extern const char** D_us_801818F4;
#else
static const char* D_us_801818F4[] = {
    _S("Metamorphosis 1"),
    _S("Prologue"),
    _S("Dance of Illusions"),
    _S("Moonlight Nocturne"),
    _S("Prayer"),
    _S("Dracula's Castle"),
    _S("Dance of Gold"),
    _S("Marble Gallery"),
    _S("Tower of Mist"),
    _S("Wood Carving Partita"),
    _S("Door of Holy Spirits"),
    _S("Festival of Servants"),
    _S("Land of Benediction"),
    _S("Requiem for the Gods"),
    _S("Crystal Teardrop"),
    _S("Abandoned Pit"),
    _S("Rainbow Cemetery"),
    _S("Silence"),
    _S("Lost Painting"),
    _S("Dance of Pales"),
    _S("Curse Zone"),
    _S("Enchanted Banquet"),
    _S("Wandering Ghosts"),
    _S("The Tragic Prince"),
    _S("Door to the Abyss"),
    _S("Heavenly Doorway"),
    _S("Death Ballad"),
    _S("Blood Relations"),
    _S("Metamorphosis 2"),
    _S("Finale Toccata"),
    _S("Black Banquet"),
    _S("Metamorphosis 3"),
    _S("I am the wind"),
};
#endif

static u16 D_us_80181978[] = {
    MU_METAMORPHOSIS,
    MU_PROLOGUE,
    MU_DANCE_OF_ILLUSIONS,
    MU_MOONLIGHT_NOCTURNE,
    MU_PRAYER,
    MU_DRACULAS_CASTLE,
    MU_DANCE_OF_GOLD,
    MU_MARBLE_GALLERY,
    MU_TOWER_OF_MIST,
    MU_WOOD_CARVING_PARTITA,
    MU_DOOR_OF_HOLY_SPIRITS,
    MU_FESTIVAL_OF_SERVANTS,
    MU_LAND_OF_BENEDICTION,
    MU_REQUIEM_FOR_THE_GODS,
    MU_CRYSTAL_TEARDROPS,
    MU_ABANDONED_PIT,
    MU_RAINBOW_CEMETERY,
    MU_SILENCE,
    MU_LOST_PAINTING,
    MU_DANCE_OF_PALES,
    MU_CURSE_ZONE,
    MU_ENCHANTED_BANQUET,
    MU_WANDERING_GHOSTS,
    MU_THE_TRAGIC_PRINCE,
    MU_THE_DOOR_TO_THE_ABYSS,
    MU_HEAVENLY_DOORWAY,
    MU_DEATH_BALLAD,
    MU_BLOOD_RELATIONS,
    MU_METAMORPHOSIS_II,
    MU_FINAL_TOCATTA,
    MU_BLACK_BANQUET,
    MU_METAMORPHOSIS_III,
    MU_STAFF_CREDITS,
};

void func_us_801B8958(Primitive* prim, Entity* self) {
    s16 posY;
    s32 i, j;

    posY = 16;
    j = self->ext.et_801B6F30.unk82;
    for (i = 0; i < 7; i++, j++) {
        prim = func_us_801B1064(prim, 16, posY, D_us_801818F4[j], 0x196);
        posY += 12;
    }
    while (prim != NULL) {
        prim->drawMode = DRAW_HIDE;
        prim = prim->next;
    }
}

void func_us_801B8A00(Entity* self) {
    DRAWENV drawEnv;
    DR_ENV* dr_env;
    Primitive* prim;
    s32 primIndex;
    s32 i;
    u16 pads;
    s16 sfxIndex;
    u8* pix;

    RECT clipRect = {.x = 0x100, .y = 0x100, .w = 0x100, .h = 0x100};
    switch (self->step) {
    case 0:
        primIndex = g_api.AllocPrimitives(PRIM_SPRT, 0x140);
        if (primIndex != -1) {
            InitializeEntity(g_EInitCommon);
            i = 0;
#ifdef VERSION_PSP
            self->ext.et_801B6F30.unk86 = 0;
#endif
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = primIndex;
            prim = &g_PrimBuf[primIndex];
            for (; prim != NULL; i++) {
                if (i < 6) {
                    prim->type = PRIM_LINE_G2;
                    prim->r0 = prim->r1 = 0x80;
                    prim->g0 = prim->g1 = 0x80;
                    prim->b0 = prim->b1 = 0x80;
                    prim->priority = 0x1FA;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 8) {
                    prim->type = PRIM_G4;
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0x80;
                    prim->y0 = prim->y1 = 0x16;
                    prim->y2 = prim->y3 = 0x7A;
                    prim->priority = 0x1FA;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 10) {
                    prim->type = PRIM_GT4;
#ifdef VERSION_PSP
                    prim->tpage = 0x110;
#else
                    prim->tpage = 0x114;
#endif
                    prim->u0 = prim->u2 = (i - 8) * 0x6C + 8;
                    prim->u1 = prim->u3 = prim->u0 + 0x6C;
                    prim->v0 = prim->v1 = 0xE;
                    prim->v2 = prim->v3 = 0x62;
                    prim->x0 = prim->x1 = prim->x2 = prim->x3 = 0x80;
                    prim->y0 = prim->y1 = 0x1E;
                    prim->y2 = prim->y3 = 0x72;
                    prim->priority = 0x1FC;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 10) {
                    prim->type = PRIM_G4;
                    prim->y0 = prim->y1 = 0x4B;
                    prim->y2 = prim->y3 = 0x57;
#ifdef VERSION_PSP
                    PRED(prim) = 0x40;
                    PGRN(prim) = 0x10;
                    PBLU(prim) = 0x10;
#else
                    PRED(prim) = 0;
                    PGRN(prim) = 0x20;
                    PBLU(prim) = 0x40;
#endif
                    prim->priority = 0x1FB;
                    prim->drawMode = DRAW_HIDE;
                } else if (i < 13) {
                    prim->type = PRIM_GT4;
                    prim->tpage = 0x1E;
                    prim->clut = 0x17F;
                    prim->u0 = prim->u2 = 0x58;
                    prim->u1 = prim->u3 = 0x60;
                    prim->v0 = prim->v1 = (i - 11) * 8 + 0x70;
                    prim->v2 = prim->v3 = 0x78 - (i - 11) * 8;
                    prim->x0 = prim->x2 = 0x7C;
                    prim->x1 = prim->x3 = prim->x0 + 8;
                    prim->y0 = prim->y1 = (i - 11) * 0x60 + 0x14;
                    prim->y2 = prim->y3 = prim->y0 + 8;
                    prim->priority = 0x1FC;
                    prim->drawMode = DRAW_HIDE;
                } else if (i == 13) {
                    dr_env = g_api.func_800EDB08((POLY_GT4*)prim);
                    if (dr_env != NULL) {
                        prim->type = PRIM_ENV;
                        prim->priority = 0x10;
                        prim->drawMode = DRAW_DEFAULT;
                        drawEnv = g_CurrentBuffer->draw;
                        drawEnv.isbg = 1;
                        drawEnv.clip = clipRect;
#ifdef VERSION_PSP
                        drawEnv.ofs[0] = 0;
#else
                        drawEnv.ofs[0] = 0x100;
#endif
                        drawEnv.ofs[1] = 0x100;
                        SetDrawEnv(dr_env, &drawEnv);
                    } else {
                        self->step = 0;
                        g_api.FreePrimitives(self->primIndex);
                        self->flags &= ~FLAG_HAS_PRIMS;
                        return;
                    }
                    prim = prim->next;
                    dr_env = g_api.func_800EDB08((POLY_GT4*)prim);
                    if (dr_env != NULL) {
                        prim->type = PRIM_ENV;
                        prim->priority = 0x13;
                        prim->drawMode = DRAW_UNK_800;
                    } else {
                        self->step = 0;
                        g_api.FreePrimitives(self->primIndex);
                        self->flags &= ~FLAG_HAS_PRIMS;
                        return;
                    }
#ifdef VERSION_PSP
                } else if (i == 14) {
                    prim->tpage = 0x10;
                    prim->clut = PAL_UNK_1A1;
                    prim->u0 = 0;
                    prim->v0 = 0x80;
                    prim->u1 = 0xE0;
                    prim->v1 = 0x20;
                    prim->x0 = 0x10;
                    prim->y0 = 0x14;
                    prim->priority = 0x12;
                    prim->drawMode = DRAW_HIDE;
#endif
                } else {
                    prim->clut = PAL_UNK_196;
                    prim->tpage = 0x1E;
                    prim->u1 = prim->v1 = 8;
                    prim->priority = 0x11;
                    prim->drawMode = DRAW_HIDE;
                }
                prim = prim->next;
            }
            self->ext.et_801B6F30.unk80 = 0;
            self->ext.et_801B6F30.unk82 = 0;
#ifdef VERSION_PSP
            self->ext.et_801B6F30.unk84 = 0;
#endif
        }
        break;

    case 1:
#ifdef VERSION_PSP
        if (!self->step_s) {
            func_us_801B11A0(0, 0x180, 0x100, 0x60);
            pix = g_Pix[0];
            pix = func_psp_09269FF0(D_psp_092A4CC8, pix, 0, 0x180);
            self->step_s++;
            break;
        }
#endif
        if (g_CutsceneFlags & 0x400) {
            SetStep(2);
            self->ext.et_801B6F30.unk7C = 0;
            self->ext.et_801B6F30.unk7E = 0;
        }
        break;

    case 2:
        self->ext.et_801B6F30.unk7C++;
        self->ext.et_801B6F30.unk7E += 0x40;
        prim = &g_PrimBuf[self->primIndex];
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x74, 0x20, 7, 1);
        for (i = 0; i < 15; i++) {
            prim = prim->next;
        }
#ifdef VERSION_PSP
        func_psp_0926ADD8(prim, self);
#else
        func_us_801B8958(prim, self);
#endif
        if (self->ext.et_801B6F30.unk7C == 0x10) {
            g_api.PlaySfx(SET_STOP_SEQ);
            SetStep(3);
        }
        break;

    case 3:
#ifdef VERSION_PSP
        pads = g_pads[0].repeat;
        if (pads & PAD_DOWN) {
            if (self->ext.et_801B6F30.unk80 < 3) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80++;
            }
        } else if (pads & PAD_UP) {
            if (self->ext.et_801B6F30.unk80 > 0) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80--;
            }
        }
        if (pads & (PAD_LEFT | PAD_RIGHT)) {
            if (self->ext.et_801B6F30.unk84) {
                self->ext.et_801B6F30.unk84 = 0;
            } else {
                self->ext.et_801B6F30.unk84 = 1;
            }
            g_api.PlaySfx(SFX_UI_MOVE);
        }
#else
        pads = g_pads[0].repeat;
        if (pads & PAD_DOWN) {
            if (self->ext.et_801B6F30.unk80 < 6) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80++;
            } else if (self->ext.et_801B6F30.unk82 < 0x1A) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk82++;
            }
        } else if (pads & PAD_UP) {
            if (self->ext.et_801B6F30.unk80 > 0) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80--;
            } else if (self->ext.et_801B6F30.unk82) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk82--;
            }
        }
        if (pads & (PAD_R1 | PAD_R2)) {
            if (self->ext.et_801B6F30.unk80 < 6) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80 = 6;
            } else if (self->ext.et_801B6F30.unk82 < 0x1A) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk82 += 7;
                if (self->ext.et_801B6F30.unk82 > 0x1A) {
                    self->ext.et_801B6F30.unk82 = 0x1A;
                }
            }
        } else if (pads & (PAD_L1 | PAD_L2)) {
            if (self->ext.et_801B6F30.unk80 > 0) {
                g_api.PlaySfx(SFX_UI_MOVE);
                self->ext.et_801B6F30.unk80 = 0;
            } else if (self->ext.et_801B6F30.unk82) {
                g_api.PlaySfx(SFX_UI_MOVE);
                if (self->ext.et_801B6F30.unk82 > 7) {
                    self->ext.et_801B6F30.unk82 -= 7;
                } else {
                    self->ext.et_801B6F30.unk82 = 0;
                }
            }
        }
#endif
        pads = g_pads[0].tapped;
        if (pads & BUTTON_CONFIRM) {
            g_api.PlaySfx(SFX_UI_CONFIRM);
            SetStep(4);
        } else if (pads & BUTTON_CANCEL) {
            SetStep(6);
        }
        prim = &g_PrimBuf[self->primIndex];
        for (i = 0; i < 10; i++) {
            prim = prim->next;
        }
#ifdef VERSION_PSP
        if (!self->ext.et_801B6F30.unk84) {
            prim->x0 = prim->x2 = 0x11;
            prim->x1 = prim->x3 = 0x80;
        } else {
            prim->x0 = prim->x2 = 0x80;
            prim->x1 = prim->x3 = 0xEF;
        }
        prim->y0 = prim->y1 = self->ext.et_801B6F30.unk80 * 12 + 0x3F;
#else
        prim->x0 = prim->x2 = 0x11;
        prim->x1 = prim->x3 = 0xEF;
        prim->y0 = prim->y1 = self->ext.et_801B6F30.unk80 * 12 + 0x1E;
#endif
        prim->y2 = prim->y3 = prim->y0 + 12;
        prim->drawMode = DRAW_COLORS;
        prim = prim->next;
#ifdef VERSION_PSP
        prim->drawMode = DRAW_HIDE;
        prim = prim->next;
        prim->drawMode = DRAW_HIDE;
#else
        if (self->ext.et_801B6F30.unk82) {
            prim->drawMode = DRAW_DEFAULT;
        } else {
            prim->drawMode = DRAW_HIDE;
        }
        prim = prim->next;
        if (self->ext.et_801B6F30.unk82 < 0x1A) {
            prim->drawMode = DRAW_DEFAULT;
        } else {
            prim->drawMode = DRAW_HIDE;
        }
#endif
        prim = prim->next;
        prim = prim->next;
        prim = prim->next;
#ifdef VERSION_PSP
        func_psp_0926ADD8(prim, self);
#else
        func_us_801B8958(prim, self);
#endif
        break;

    case 4:
#ifdef VERSION_PSP
        switch (self->step_s) {
        case 0:
            func_psp_0892A620(1, true);
            g_api.PlaySfx(SET_STOP_MUSIC);
            self->ext.et_801B6F30.unk86 = 1;
            self->step_s++;
            break;

        case 1:
            if (!g_api.func_80131F68() && (g_CutsceneFlags & 0x200) == 0) {
                sfxIndex = self->ext.et_801B6F30.unk84 +
                           self->ext.et_801B6F30.unk80 * 2;
                g_api.PlaySfx(D_us_80181978[sfxIndex]);
                SetStep(3);
            }
            break;
        }
        break;
#else
        FntPrint("step_s %x\n", self->step_s);
        switch (self->step_s) {
        case 0:
            g_api.PlaySfx(SET_STOP_MUSIC);
            self->poseTimer = 0x30;
            self->step_s++;
            break;

        case 1:
            if (!--self->poseTimer) {
                self->step_s++;
            }
            break;

        case 2:
            if (!g_api.func_80131F68() && (g_CutsceneFlags & 0x200) == 0) {
                sfxIndex =
                    self->ext.et_801B6F30.unk80 + self->ext.et_801B6F30.unk82;
                g_api.PlaySfx(D_us_80181978[sfxIndex]);
                SetStep(3);
            }
            break;
        }
        break;
#endif

    case 6:
        if (g_api.func_80131F68()) {
            g_api.PlaySfx(SET_STOP_MUSIC);
        }
        self->step++;
        break;

    case 7:
        if ((g_CutsceneFlags & 0x200) == 0) {
            prim = &g_PrimBuf[self->primIndex];
            for (i = 0; i < 10; i++) {
                prim = prim->next;
            }
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
            prim->drawMode = DRAW_HIDE;
            self->ext.et_801B6F30.unk7E = 0x400;
            self->ext.et_801B6F30.unk7C = 0x10;
            self->step++;
            g_api.PlaySfx(MU_SEQ_LIBRARIAN);
            g_CutsceneFlags |= 0x200;
        }
        break;

    case 8:
        self->ext.et_801B6F30.unk7C--;
        self->ext.et_801B6F30.unk7E -= 0x40;
        prim = &g_PrimBuf[self->primIndex];
        func_us_801B245C(prim, self->ext.et_801B6F30.unk7E,
                         self->ext.et_801B6F30.unk7C * 2, 0x74, 0x20, 7, 1);
        if (!self->ext.et_801B6F30.unk7C) {
            prim = &g_PrimBuf[self->primIndex];
            for (i = 0; i < 15; i++) {
                prim = prim->next;
            }
            while (prim != NULL) {
                prim->drawMode = DRAW_HIDE;
                prim = prim->next;
            }
            SetStep(9);
        }
        break;

    case 9:
#ifdef VERSION_PSP
        if (self->ext.et_801B6F30.unk86) {
            g_api.PlaySfx(MU_SEQ_LIBRARIAN_PSP);
        }
#endif
        g_CutsceneFlags &= ~0x400;
        DestroyEntity(self);
        return;
    }
}

#if defined(MOD_PTBR)
#include "mod_ptbr_lib_text.h"
#endif
