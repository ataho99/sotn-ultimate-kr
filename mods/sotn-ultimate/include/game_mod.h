// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate: included by include/game.h (at file scope, in place)
// Inventory capacity. NUM_HAND_ITEMS / NUM_BODY_ITEMS stay the retail counts
// because stage data encodes body items as NUM_HAND_ITEMS + id (pickups,
// drops, shops); the arrays below can hold more.
// Saturn port: the Saturn's 7 new hand items and 3 new body items (the
// Saturn's 2 plus Glaring Cloth, which there replaces Crystal cloak). The
// arrays grow in place like on the Saturn; to keep PlayerStatus at 0x334
// bytes (D_80097C98 follows it), the US-unused stats of the two JP-only
// familiars are dropped. Every field from saveName on moves by 0x14:
// mods/sotn-ultimate/tools/patch_status_asm.py fixes the asm-only users.
#define HAND_ITEMS_CAP 176
#define BODY_ITEMS_CAP 93
#define FAMILIARS_STORED FAM_STATS_YOUSEI
// New body items, appended after the retail ones so retail IDs never move.
// Every body slot up to BODY_ITEMS_CAP has a g_AccessoryDefs record.
#define ITEM_GLARING_CLOTH (NUM_BODY_ITEMS + 0)
#define ITEM_RAINBOW_MANTLE (NUM_BODY_ITEMS + 1)
#define ITEM_SONG_LYRICS_CARD (NUM_BODY_ITEMS + 2)
// New hand items take ids 169..175, like on the Saturn, which inserted them
// in front of the 48 nameless special-attack records (retail 169..216). Those
// move up by MOD_SPECIAL_SHIFT (see MOD_SPECIAL) and the Saturn's 4 new
// special records follow at 224..227: g_EquipDefs has MOD_NUM_EQUIP_DEFS
// records (mods/sotn-ultimate/tools/gen_saturn_items.py writes
// equipment_mod.h).
#define ITEM_SMELLY_RICE (NUM_HAND_ITEMS + 0)
#define ITEM_DELICIOUS_MEAL (NUM_HAND_ITEMS + 1)
#define ITEM_SIGNED_PENCIL_BOARD (NUM_HAND_ITEMS + 2)
#define ITEM_ALUCARD_SPEAR (NUM_HAND_ITEMS + 3)
#define ITEM_WOODLAND_SHIELD (NUM_HAND_ITEMS + 4)
#define ITEM_ASTRAL_DAGGER (NUM_HAND_ITEMS + 5)
#define ITEM_DOUBLE_BLADE_AXE (NUM_HAND_ITEMS + 6)
#define MOD_SPECIAL_SHIFT (HAND_ITEMS_CAP - NUM_HAND_ITEMS)
#define MOD_SPECIAL(retailId) ((retailId) + MOD_SPECIAL_SHIFT)
#define MOD_NUM_EQUIP_DEFS 228
// Pickups and drops use combined ids: hand id, or NUM_HAND_ITEMS + body id.
// New hand items can't use 169.. there (those are body items), so they get
// combined ids from MOD_DROP_HAND_BASE on (ITEMDROP = 0x80 + combined).
#define MOD_DROP_HAND_BASE (NUM_HAND_ITEMS + BODY_ITEMS_CAP)
// The Saturn's new weapons (weaponId 60-63) are in BIN/WEAPON2.BIN
// (src/dra/cd.c, mods/sotn-ultimate/tools/build_saturn_weapons.sh)
#define MOD_FIRST_WEAPON2 60
// Meal ticket pickups carry an equip id in 7 bits; the new foods use these
// codes instead (src/weapon/w_021.c), equip ids 126/127 are never food
#define MOD_MEAL_TICKET_NEW 126
// The Saturn's new stages take unused stage slots (normal / inverted castle
// pairs must be X / X | 0x20). Underground Garden = 0x11, built as the unused
// TE1 disc files (mods/sotn-ultimate/tools/build_gdn.sh).
#define MOD_STAGE_GDN 0x11
// Saturn port: the Cursed Prison (Saturn STAGE_16), in the unused stage
// slot 0x10 (STAGE_NO0_ALT, referenced nowhere), built as the TE2 files
#define MOD_STAGE_PRS 0x10
// Saturn port: the inverted castle's pair (RSTAGE15 / RSTAGE16), forced to
// X | STAGE_INVERTEDCASTLE_FLAG: the shared teleport table serves both
// castles. Soul Prison = 0x30 (TE4 files,
// mods/sotn-ultimate/tools/build_rprs.sh), Hell Garden = 0x31 (TE3 files,
// mods/sotn-ultimate/tools/build_rgdn.sh). lba rows 0x30/0x31 were unused
// fillers.
#define MOD_STAGE_RPRS 0x30
#define MOD_STAGE_RGDN 0x31
#if MOD_STAGE_RPRS != (MOD_STAGE_PRS | STAGE_INVERTEDCASTLE_FLAG) ||           \
    MOD_STAGE_RGDN != (MOD_STAGE_GDN | STAGE_INVERTEDCASTLE_FLAG)
#error "the inverted stages must be the normal ones | 0x20"
#endif
// The Hell Garden (MOD_STAGE_RGDN) exists on the disc (was used by the old
// Saturn-stage Library card test chain; unused now)
#define MOD_HAS_RGDN
// New teleports (D_800A245C in src/dra/5087C.c), appended after the 131
// retail ones (0x00..0x82; src/dra/5087C.c asserts the count). As on the
// Saturn: Castle Entrance shaft (2,44) and the warp room (2,45) lead into the
// Garden, and back.
#define MOD_TP_FIRST 0x83
#define MOD_TP_NP3_TO_GDN 0x83
#define MOD_TP_GDN_TO_NP3 0x84
#define MOD_TP_GDN_TO_WRP 0x85
#define MOD_TP_WRP_TO_GDN 0x86
// the Cursed Prison's links (Saturn teleports 0x49..0x4C)
#define MOD_TP_NO4_TO_PRS 0x87
#define MOD_TP_PRS_TO_NO4 0x88
#define MOD_TP_NO0_TO_PRS 0x89
#define MOD_TP_PRS_TO_NO0 0x8A
// the first visit's Entrance (NO3) shaft into the Garden (the Saturn's
// STAGE_0X has the shaft too)
#define MOD_TP_NO3_TO_GDN 0x8B
// Test aid: the Library card warps along the game's dialogue scenes instead
// (off normally; src/dra/5087C.c ModDlgWarp, list in test_checklist.md
// "Dialogue warp chain"). Hold L2 during the card's flash to repeat the last
// stop, R2 to go back one. Turn it OFF before any release build.
#include "mod_ptbr_build.h"
// test aids (warp chain, cards, gift): OFF since 2026-11-17 (the user: the
// whole game is tested; DRA needed the room for the Richter / Maria
// Classic balance and the held-START menu)
// #define MOD_SATURN_STAGE_TEST
// Test aid, with MOD_SATURN_STAGE_TEST: the Library card walks the areas
// with a first-visit name banner instead of the dialogue scenes, the
// banners' "seen" flags cleared at each warp (src/dra/5087C.c)
// #define MOD_TEST_AREA_NAMES // off
#if defined(MOD_SATURN_STAGE_TEST)
// its arrival teleports that no retail one covers
#define MOD_TP_DLG_FERRY1 0x8C // Underground Caverns room 25, west lake
#define MOD_TP_DLG_FERRY2 0x8D // Underground Caverns room 23, east lake
#define MOD_TP_DLG_DAI 0x8E    // Royal Chapel room 0 (Maria), right door
#define MOD_TP_DLG_NZ0 0x8F    // Alchemy Lab room 1 (Maria), right edge
#define MOD_TP_END 0x90
#else
#define MOD_TP_END 0x8C
#endif
// Test aid: loading a save adds the new items that work so far
// (src/save_mgr.h ModItemsTestGift). Off for normal builds.
// #define MOD_SATURN_ITEMS_TEST // test item gift: off

// The cut Outer Wall "Ghost" (flaming skull, record 0xB1, NO1 entity 0x51,
// src/st/no1/unk_54400.c): no retail room places it. On: NO1 room 13 (the
// tower, layout 14, src/st/no1/mod_layout.h from
// mods/sotn-ultimate/tools/gen_mod_layouts.py) gets three of them, and each
// appears only in some weathers (g_CastleFlags[NO1_WEATHER]: params bits 0-2
// = rain / clear / fog), plus the name "Ghost Skull"
// (mods/sotn-ultimate/tools/gen_menu_text.py; retail has another Ghost,
// 0x09C) and a Librarian entry No.165 at the end of the list (bit 164, set by
// NO1's src/st/collision.h; text mods/sotn-ultimate/tools/
// gen_saturn_bestiary.py, picture BIN/MONSTER2.BIN record 19 from
// mods/sotn-ultimate/tools/ghost_portrait.py). Comment out to get NO1 exactly
// as before (and no name, no list entry).
// #define MOD_OUTER_WALL_GHOST // the Ghost Skull: off

// Soul Prison Wight, PS1-style effects
// (mods/sotn-ultimate/src/enemies/e_mod_wight.h, palettes from
// mods/sotn-ultimate/tools/wight_ps1.py via rprs_assets.py). SPIRIT: the death
// spirit is the Wight's own standing frame in yellow, additive and stretched
// the Saturn's way, instead of the Saturn's low-res frame 71 polygon. GLOW:
// the olive spirit layer of its sprites (palette indices 14/15) becomes an
// additive pulsing yellow copy of the frame.
#define MOD_WIGHT_PS1_SPIRIT
#define MOD_WIGHT_PS1_GLOW
// Underground Garden / Hell Garden: the PS1 Castle Entrance's art for their
// stairs and stonework (the Saturn art narrowed to 256 px makes them uneven):
// mods/sotn-ultimate/tools/gdn_tile_swap.py, the chosen cells in
// mods/sotn-ultimate/tools/gdn_ps1_tiles. Read by the asset tools only.
#define MOD_GDN_PS1_TILES
// Cursed Prison / Soul Prison: their staircases (step teeth, rim, stringer)
// drawn from the PS1 Underground Caverns' stairs and recoloured into each
// cell's palette, and the Prison corridor's ceiling slopes from the Soul
// Prison's clean floor slopes:
// mods/sotn-ultimate/tools/gdn_tile_swap.py with the cells in
// mods/sotn-ultimate/tools/prs_ps1_tiles. Read by the asset tools only.
#define MOD_PRS_PS1_TILES
#include "mod_ptbr_build.h"
#if defined(MOD_PTBR)
#define MOD_PTBR_BLOB ((u32*)0x801F0000)
#define MOD_PTBR_MAGIC 0x31425450
#define MOD_PTBR_MAX 0x7800
#define MOD_PTBR_CODE 0x801F0090
#define MOD_PTBR_TEXT_PT()                                                     \
    (MOD_PTBR_BLOB[1] == MOD_PTBR_MAGIC && !MOD_PTBR_BLOB[2])
#endif
// The Saturn's New Game character select (src/st/sel/2C048.c): Maria's
// portrait, and she can be chosen.
#define MOD_SATURN_MARIA
// START skips any dialogue / cutscene from a fresh game.
// Retail allows the skip only once the scene's "seen" bit in
// g_Settings.D_8003CB04 is set (or g_GameClearFlag); the Saturn discs use the
// same rule. Those bits are read ONLY by the skip checks (every stage, boss
// and ST0 cutscene, C and asm), so DRA starts with all of them set instead of
// 0 (boot init in 42398.c, new game / Richter init in 5F60C.c); loading a
// save ORs the saved bits in (save_mgr.h), so they stay set. The skip runs
// the rest of the script in fast mode, so scene results stay the same.
// Saves keep the bits (harmless: they only allow the skip).
#define MOD_SKIP_ANY_CUTSCENE
#define MOD_ALL_SCENES_SEEN 0xFFFFFFFF
// Richter's alternate costume (the character select's Richter "Saturn"
// version). Stored in g_Status.D_80097C40, which only Alucard
// (Muramasa kill count) and Maria (second subweapon) use, so it survives a
// Richter-mode save/load without changing the save layout. SEL writes it on
// every Richter start (0 = normal costume, also on the retail "RICHTER" name
// path); the prologue's Richter plays in Alucard's game, so it never matches
// there. src/dra/cd.c loads BIN/RIC2.BIN
// (mods/sotn-ultimate/tools/build_ric2.py) instead of RIC.BIN when it is set:
#define MOD_RIC_COSTUME2_MAGIC 0x32434952 // "RIC2"
#define MOD_RIC_COSTUME2()                                                     \
    (g_PlayableCharacter == PLAYER_RICHTER &&                                  \
     g_Status.D_80097C40 == MOD_RIC_COSTUME2_MAGIC)
// Per-game mod choices made on the character select, kept in
// g_Status.saveName[11]: saveName is 12 bytes but only 0..9 are ever
// written (SEL's name entry writes 0..7 and zeroes 8/9, name changes strcpy
// at most 9 bytes) or read (the save summary copies 0..9), and new-game
// setup (InitStatsAndGear) never clears it, so it survives from SEL into the
// game and through save/load (whole PlayerStatus; the retail/SAT1 save
// converters copy saveName..D_80097C40 as is) without changing the save
// layout. Retail saves hold 0 there. SEL writes the byte on every new game.
#define MOD_STATUS_FLAGS g_Status.saveName[11]
// Saturn port: Alucard sprinting with the Godspeed Shoes (his sprint runs
// in player step 0x13 = Player_FlameWhip, the same number as on the
// Saturn). The Saturn's doors open in that step too (warp 0x060DEEB8).
#define MOD_SPRINTING()                                                        \
    (PLAYER.step == 0x13 && g_PlayableCharacter == PLAYER_ALUCARD)
#define MOD_FLAG_BOSS_RIC_SATURN 1 // Alucard game: boss Richter's Saturn look
#define MOD_FLAG_MARIA_PSP 2       // Maria game: PSP version (BIN/MARIA2.BIN)
#define MOD_BOSS_RIC_SATURN()                                                  \
    (g_PlayableCharacter == PLAYER_ALUCARD &&                                  \
     (MOD_STATUS_FLAGS & MOD_FLAG_BOSS_RIC_SATURN))
#define MOD_MARIA_PSP() (MOD_STATUS_FLAGS & MOD_FLAG_MARIA_PSP)
// the PSP Maria's prize drops as on the PSP (its e_collect.h): the cross,
// stopwatch, rebound stone and agunea drops (items 16, 18, 20, 22) are
// small hearts (item 0); the others show her own subweapons
// (mods/sotn-ultimate/src/maria_psp/ mar2.c swaps their frames)
#define MOD_MARIA_PSP_NO_SUBWPN(id)                                            \
    (MOD_MARIA_PSP() && (id) >= 16 && (id) < 23 && !((id) & 1))
// Alucard game: the PSP Maria fight in the Castle Center (src/dra/cd.c
// loads BIN/CENP.BIN, mods/sotn-ultimate/tools/build_cenp.sh, instead of
// CEN.BIN)
#define MOD_FLAG_MARIA_BOSS_PSP 4
#define MOD_MARIA_BOSS_PSP()                                                   \
    (g_PlayableCharacter == PLAYER_ALUCARD &&                                  \
     (MOD_STATUS_FLAGS & MOD_FLAG_MARIA_BOSS_PSP))
// her PSP stats where the PSP's own record (PSP dra.bin enemy 0x190 "Maria
// Renard") differs from the mod's 0x198 (the Saturn boss's, HP 400 on both,
// weak DARK on both, no exp or drops): the CENP fight's damage (cen_psp/
// mod_cenp.c CenpDealDamage) and the Library page (lib/e_shop.c); her
// contact attack 25 is mod_cenp.c CENP_ATTACK
#define MOD_PSP_MARIA_ENEMY 0x198
#define MOD_PSP_MARIA_LEVEL 46
#define MOD_PSP_MARIA_DEFENSE 10
#define MOD_PSP_MARIA_STRONG 0x1000 // ELEMENT_HOLY
#define MOD_PSP_MARIA_IMMUNE 0x0200 // ELEMENT_STONE
// PSP dub toggle (Player Select, every character): bit set = the PS1 dub and
// script ("classic"), clear = the PSP English dub and script, so retail and
// older mod saves (0) get the PSP dub. Voices: src/dra/sound.c ModXaConfig
// (the PSP recordings' table: mods/sotn-ultimate/tools/repack_xa.py); scripts:
// the stages' SetCutsceneScript swaps a retail script for its PSP copy
// (generated mod_psp_script.h, mods/sotn-ultimate/tools/psp_script.py; CEN:
// mod_psp_cen.h)
#define MOD_FLAG_PS1_DUB 8
#define MOD_PSP_DUB_ON() (!(MOD_STATUS_FLAGS & MOD_FLAG_PS1_DUB))
// The field voices (the sound banks of the grunts, bosses, enemies and the
// familiars) of a KO build made with KO_FIELD_VOICE=1 are the Japanese PS1's
// for every Dub choice: the
// retail files hold them (mods/sotn-ultimate/tools/gen_ps1jp_banks.py), so
// the loaders take the retail banks and not the PSP / JP dub's own ones
#if defined(MOD_FIELD_JP_VOICE)
#define MOD_FIELD_PSP_BANKS() 0
#define MOD_FIELD_JP_BANKS() 0
#else
#define MOD_FIELD_PSP_BANKS() MOD_PSP_DUB_ON()
#define MOD_FIELD_JP_BANKS() MOD_JP_DUB_ON()
#endif
// PSP Maria game: her two subweapon slots are stored in the two-slot layout
// (mods/sotn-ultimate/src/maria_psp/mar2.c: D_80097C40 = front, subWeapon =
// back). Clear = a new game (SEL rewrites the byte) or a save from before the
// two slots (subWeapon = her only subweapon, D_80097C40 = anything: it is
// Alucard's Muramasa counter): MarPsxInit converts that (back = the Doll, as a
// PSP new game) and sets the bit
#define MOD_FLAG_MARIA_SUB2 0x10
// 097: easy controls: the tick (ARC_F2 r1_easy.c) puts this bit, the physical L3 (PAD_L3 = 0x0200 in the PS1 layout; 0x0002 is R2 = the wolf, never use it; the tick clears a real stick
// click first), into g_pads[0].pressed while R1 is held; DRA's spell checks (704D0.c) read it from g_Player.padPressed
#define MOD_EASY_R1 0x0200
// 099: the new resident area (memory map: 0x801F3000..0x801FAFFF, 32 KB, filled upward from the bottom; the ARC_KO.BIN chain loads it at every game load,
// like ARC_F2.BIN, so its state resets the same way). Header (16 B): +0 magic, +4 version, +8 entry of the easy tick, +12 reserved. The marker is
// written by ARC_F2's glue on the first tick and checked every frame: if it changes, something overwrote the area's top.
#define MOD_NEWAREA_BASE 0x801F3000
#define MOD_NEWAREA_LIMIT 0x8000
#define MOD_NEWAREA_MAGIC 0x4B4F5231 // 'KOR1'
#define MOD_NEWAREA_MARKER_ADDR 0x801FAFFC
#define MOD_NEWAREA_MARKER 0x4B4F5245 // 'KORE'
#define MOD_NEWAREA_ALERT_ADDR 0x801FAFF8 // set to 0xDEAD0001 by the glue when the marker is found changed (readable from the emulator)
// 099b: the area's 32 B header: +0 magic, +4 version, +8 easy tick, +12 cheat tick, +16 popup draw (u32 entries, 0 = none), the code from +32
#define MOD_NEWAREA_CODE (MOD_NEWAREA_BASE + 32)
#define MOD_NEWAREA_SLOT_TICK 2
#define MOD_NEWAREA_SLOT_CHEAT 3
#define MOD_NEWAREA_SLOT_POP 4
#define MOD_NEWAREA_SLOT_SUB 5  // SubCmd (the familiars' subtitle)
#define MOD_NEWAREA_SLOT_ACT 6  // CheatAct (the cheat popup's instant actions)
// the cheats' bit numbers in g_CheatMask (ARC_F2's sysopt.c) and the popup's layout (shared with the new area: src/easy/newarea_cheat.c / newarea_pop.c)
#define CHEAT_MP 0
#define CHEAT_HEARTS 1
#define CHEAT_MPREGEN 2
#define CHEAT_STAIRS 3
#define CHEAT_FOOD 4
#define CHEAT_INVINC 5
#define CHEAT_HP 6
#define CHEAT_GOLD 7
#define CHEAT_JUMP 8
#define CHEAT_MOON 9
#define MOD_SYS_NOPT 4    // the Player Select options, rows 0 to 3 of the popup
#define MOD_SYS_NCHEAT 10 // the cheats, rows NOPT and after
#define MOD_SYS_NACT 8    // the instant actions after the cheats (CheatAct in the new area): the popup scrolls over NCHEAT + NACT rows
#define MOD_SYS_VALUE_DX 112
// each option: the flag bits it owns, and per choice (the Player Select's order, src/st/sel/2C048.c) their value and its name
typedef struct {
    u8 mask;
    u8 n;
    u8 val[3];
    u8 name[3];
} SysOptRow;
// what the popup's draw needs from ARC_F2's statics (passed in, never read by address)
typedef struct {
    s32 top;       // the first row shown
    s32 popn;      // rows of the open popup
    s32 locks;     // the locked options (bit per option)
    s32 cheatMask; // g_CheatMask
    const SysOptRow* rows;
} ModPopArgs;
typedef struct {
    s32 eat;      // the cross, used up by a form's special: hidden from the game until released
    s32 ringPrim; // the ring's first prim (-1: none)
    s32 sel;      // 100: the place chosen while the ring was open (index + 1, 0: none)
    s32 anim;     // 100: the ring's open / close step 0..3
} ModEasyState;
#define MOD_PSP_XA_FIRST 0x358 // first / last voice id with a PSP version
#define MOD_PSP_XA_LAST 0x532
// The third dub, "JP" (Player Select "Dub" row: PSP / PS1 / JP): the
// Japanese voices (the PSP's Japanese recordings; Maria's CEN scene: the
// Saturn's takes) with the PSP English script as subtitles. Bit set and
// MOD_FLAG_PS1_DUB clear = JP; MOD_PSP_DUB_ON() stays true (the PSP script
// copies, and the PSP sound banks where the Japanese clips are the same);
// the voices (sound.c ModXaConfig), the grunts and the banks whose clips
// differ (BIN/JP_SD.BIN, src/dra/cd.c) check MOD_JP_DUB_ON() first
#define MOD_FLAG_JP_DUB 0x20
#define MOD_JP_DUB_ON()                                                        \
    ((MOD_STATUS_FLAGS & (MOD_FLAG_PS1_DUB | MOD_FLAG_JP_DUB)) ==              \
     MOD_FLAG_JP_DUB)
// Player Select "Balance" row (every character): bit set = the PS1 balance,
// clear = the Saturn's, so new games and retail / older mod saves (0) get the
// Saturn's. The DRA tables hold the PS1 values; src/dra/game_handlers.c
// ModApplyBalance writes the chosen equipment / enemy fields
// (mods/sotn-ultimate/tools/ gen_saturn_balance.py) every frame of play. PS1
// balance also brings back Alucard's time-stop dim (src/dra/692E8.c BP_120),
// which the Saturn lacks.
#define MOD_FLAG_PS1_BALANCE 0x40
// Richter / Maria game: the Succubus, Minotaur & Werewolf and Death can be
// fought (their time attack records are real victories). Unset in a save
// from before 2026-11-15, whose records the game's start set: the title's
// load clears them once (src/save_mgr.h ApplySaveData), then sets this
#define MOD_FLAG_BOSSES_ON 0x80
#define MOD_PS1_BALANCE() (MOD_STATUS_FLAGS & MOD_FLAG_PS1_BALANCE)
// Richter and the Marias have a pause menu, Alucard's trimmed
// (mods/sotn-ultimate/src/menu/menu_mod_charmenu.h), and their own button
// set-up, in both balances. Ultimate: START opens it. Classic: START gives
// the retail PAUSE banner; held MOD_PAUSE_HOLD frames while paused, their
// menu (src/dra/5087C.c; user, 2026-11-17). Never at the prologue.
#define MOD_CHAR_MENU()                                                        \
    (g_PlayableCharacter != PLAYER_ALUCARD && g_StageId != STAGE_ST0)
#define MOD_PAUSE_HOLD 90 // 1.5 s
// Richter / a Maria with the Ultimate balance (Player Select "Classic" =
// MOD_FLAG_PS1_BALANCE = the retail rules): every room open
// (src/menu/game_handlers_mod_access.h, the stage hooks), the Succubus,
// Minotaur & Werewolf and Death fightable, 100 HP, running in save rooms,
// Richter's stagger cancel (user, 2026-11-17)
#define MOD_RIC_ULTIMATE()                                                     \
    (g_PlayableCharacter != PLAYER_ALUCARD && !MOD_PS1_BALANCE())
// the menu open is theirs (set when it opens: src/dra/menu.c)
#define MOD_IN_CHAR_MENU() (g_ModDub.charMenu)
#define MOD_MAIN_ROWS() (MOD_IN_CHAR_MENU() ? 2 : 5) // the main box's rows
// its drawing code, src/menu/charmenu.c (after ARC_F2.BIN's loaded part), read
// into g_Pix[1] while the menu is open (the demo-key load with g_LoadOvlIdx
// MOD_CHARMENU_OVL, src/dra/cd.c): the magic, then its entries
#define MOD_CHARMENU_MAGIC 0x554E4D43
#define MOD_CHARMENU_MAGIC_STR "0x554E4D43"
#define MOD_CHARMENU_OVL 0x7E
// its size: g_Pix[1] to [3] (24 KB; tools/build_charmenu.sh checks it)
#define MOD_CHARMENU_SIZE 0x6000
typedef struct {
    u32 magic;
    void (*main)(void* ctx, s32 x, s32 y); // the main box's rows
    void (*stats)(void* ctx);              // the status screen
    void (*sysDraw)(void* ctx);            // MenuSystemDraw
    void (*sysStep)(void);                 // MENU_STEP_SYSTEM
    void (*btnDraw)(void* ctx);            // MenuButtonConfigDraw
    void (*btnStep)(void);                 // MENU_STEP_SYSTEM_BUTTONS
    s32 (*popDraw)(void* ctx);  // MenuSoundConfigDraw: Extra settings' popup
    void (*cmdDraw)(void* ctx); // MenuSpellsDraw: Commands
    void (*cmdStep)(void);      // MENU_STEP_SPELL_INIT: Commands
} ModCharMenuBlob;
// Richter's / the Marias' own button set-up (their menu's System screen),
// saved with the game: one word each in g_Settings.timeAttackRecords'
// unused slots 28..30 (zero at a new game; the time attack reads 0..27
// only). 3 bits per action f (Alucard's row order, the default button
// g_ButtonMask[f]): its button ^ f, so 0 = the default buttons. Applied in
// gameplay by src/dra/pads.c (MOD_CHAR_MENU: not at the prologue)
#define MOD_PAD_CHAR()                                                         \
    (g_PlayableCharacter == PLAYER_RICHTER ? 0 : MOD_MARIA_PSP() ? 1 : 2)
#define MOD_PAD_WORD(c) (g_Settings.timeAttackRecords[28 + (c)])
#define MOD_CHARMENU_BLOB ((ModCharMenuBlob*)g_Pix[1])
#define MOD_CHARMENU_OK() (MOD_CHARMENU_BLOB->magic == MOD_CHARMENU_MAGIC)
// The Saturn's third hand ("Holder", its equipment[2];
// mods/sotn-ultimate/src/items/mod_holder.h): Saturn balance, Alucard only. Its
// item id is kept in saveName[10] (name entry writes 0..9, the save summary
// copies 0..9; cleared at new game in InitStatsAndGear). Equip menu row
// MOD_HOLDER_SLOT (after the 7 retail slots), button config action 8 (default
// L2, virtual PAD_L2)
#define MOD_HOLDER_ITEM g_Status.saveName[10]
#define MOD_HOLDER_ON()                                                        \
    (!MOD_PS1_BALANCE() && g_PlayableCharacter == PLAYER_ALUCARD)
#define MOD_HOLDER_SLOT 7
// The pause menu System screen's "Ultimate settings" row (Alucard games):
// a popup with the Player Select's four Alucard options
// (mods/sotn-ultimate/src/mod_sysopt/ sysopt.c,
// mods/sotn-ultimate/tools/build_sysopt.sh). Its code follows Alucard's sprites
// in BIN/ARC_F2.BIN (retail ARC_F.BIN + the code, a new file at the disc's
// end: no file before it moves; src/dra/cd.c g_LbaBinAlucard loads it,
// MOD_ARC_F_LEN bytes from 0x8013C000 = up to the familiar overlay at
// 0x80170000, a RAM range only Alucard games use this way). DRA's three
// stubs (src/config_us.h) check the magic, its header's first word: the
// address its %hi loads, (ADDR + 0x8000) & ~0xFFFF (build_sysopt.sh checks
// it and that no other player file holds it there). Header: magic, the
// three entries plus Tick, then the row's info bar text (menu.c D_800A2D48[7])
#define MOD_SYSOPT_ADDR 0x8016DBF0
#define MOD_SYSOPT_MAGIC 0x80170000
#define MOD_SYSOPT_MAGIC_STR "0x80170000"
#define MOD_SYSOPT_BAR_TEXT (MOD_SYSOPT_ADDR + 20)
// v1.5 had +16 here for its 3-entry header; merged header has Tick so +20, see M2 report
#define MOD_SYSOPT_BAR_TEXT2 (MOD_SYSOPT_ADDR + 52) // 094_12 KO: the Cheats row's text (sysopt.c header .org 52)
// the third hand's code there too (src/items/holder_code.c): its table in the
// file's last 16 bytes (tools/build_sysopt.sh): magic, use, food entity
#define MOD_HOLDER_TBL ((u32*)(0x80170000 - 16))
#define MOD_HOLDER_MAGIC 0x484F4C44
#define MOD_ARC_F_LEN 0x34000 // ARC_F2.BIN: up to 0x80170000
// The JP dub's tables are SEL rodata (no room in DRA): SEL copies them into
// g_ModDub.jp every frame its menu runs (src/st/sel/2C048.c Update), which
// every way into a game passes (new game, the prologue, load, after the
// ending or a game over); DRA plays the JP versions only once magic is set
#define MOD_JP_MAGIC 0x4A504455
#define MOD_JP_XA_MAX 256
#define MOD_JP_SFX_MAX 28
typedef struct {
    u32 magic;
    u32 xaBase; // the JP streams' first sector (repack_xa.py --dub-jp)
    // the stage / familiar banks whose JP clips differ from the PSP's:
    // {the PSP bank (its SFX_PSP.BIN sector + 1, as lba_stage.c / cd.c
    // keep it), the JP one: its sector in BIN/JP_SD.BIN + 1, or 0xFFFF =
    // the retail bank}; {0, 0} ends (mods/sotn-ultimate/tools/gen_psp_sfx.py
    // --set jp)
    u16 sfx[MOD_JP_SFX_MAX][2];
    u8 xaBits[60]; // as g_ModPspXaBits, over the same ids
    // per set bit: sector - xaBase (bits 0-14), unk228 (15-25), file (26-31)
    u32 xa[MOD_JP_XA_MAX];
} ModJpDubTable;
// The menu text that follows the Dub row
// (mods/sotn-ultimate/tools/gen_menu_text.py): SEL copies each string's DRA
// address and both versions (PS1, then PSP padded to the PS1 length) here;
// DRA's ModApplyDubText writes the chosen one over the string every frame of
// play (src/dra/game_handlers.c)
#define MOD_DUB_TEXT_MAGIC 0x54585455
#define MOD_DUB_TEXT_MAX 16
#define MOD_DUB_TEXT_BYTES 384
#define MOD_FIXED_TEXT_BYTES 768
typedef struct {
    // bits 0-2: the table (0 equipment, 1 accessories, 2 enemies, 3 relics,
    // 4 g_MenuStr); bits 3-7: where in the string the versions differ
    u8 table;
    u16 off; // byte offset of the string's pointer in that table
    u8 len;  // bytes of the differing span
} ModDubTextSrc;
typedef struct {
    u8 table; // as ModDubTextSrc
    u8 pos;   // where in DRA's retail string the edit starts
    u16 off;  // byte offset of the string's pointer in that table
    u8 del;   // bytes dropped there (0xFF: the string is the inserted bytes)
    u8 ins;   // bytes inserted there (g_ModFixedTextBytes, in order)
    u8 room;  // its room in g_ModDub.fixedText
} ModFixedTextSrc;
typedef struct {
    u32 magic;
    u32 count;
    struct {
        u8* dst;
        const u8* src[2]; // the PS1 version, the PSP one
        u32 len;
    } e[MOD_DUB_TEXT_MAX];
    u8 bytes[MOD_DUB_TEXT_BYTES];
} ModDubText;
#define MOD_GRATE_MAX 128
#define MOD_GRATE_MAGIC 0x47520000
// DRA's dead VSync-effect buffers (4 KB, src/dra/47BB8.c)
typedef struct {
    u32 cfg[4]; // ModXaConfig's redirected XaMusicConfig
    ModJpDubTable jp;
    ModDubText text;
    // the PSP port's fixed menu strings, which DRA's table entries point to
    // (SEL copies them and sets the pointers:
    // mods/sotn-ultimate/tools/gen_menu_text.py)
    u8 fixedText[MOD_FIXED_TEXT_BYTES];
    // DRA's StoreSaveData, set every frame (game_handlers.c UpdateGame):
    // SEL's saves go through it (save_mgr.h)
    void* storeSave;
    // the castle-access hook's room key and scan countdown
    // (src/menu/game_handlers_mod_access.h): here, not in DRA's BSS, so
    // g_ModDub keeps the address SEL writes to
    u32 accessRoom;
    s32 accessScan;
    // the pause menu open is Richter's / a Maria's, and where its file is
    // (src/menu/menu_mod_charmenu.h: 0 not asked, 1 reading, 2 done)
    u8 charMenu;
    u8 charMenuLoad;
    // their Extra settings changed the costume / version: 1 to load (the
    // menu's exit), 2 loading, 3 loaded (src/menu/menu_mod_charmenu.h
    // ModCharSwapLoad / ModCharSwap)
    u8 charSwap;
    u8 pauseHold; // frames START held while paused (src/dra/5087C.c)
    // the grate tiles the castle-access hook took out (they are the stage
    // overlay's own room data: out until the stage reloads), with their
    // tiles, so a switch to Classic (src/menu/charmenu.c PopBalance) puts
    // them back: grateStage = MOD_GRATE_MAGIC | the stage they are in
    u32 grateStage;
    u32 grateCount;
    struct {
        u16* p;
        u16 v;
        u16 pad;
    } grate[MOD_GRATE_MAX];
    // Richter's / the Marias' cheats, set in their Extra settings (src/menu/charmenu.c): bit 0 infinite
    // HP, bit 1 hearts, bit 2 MP (the Saturn Maria's); DRA HandlePlay refills them every frame
    u8 cheats;
    u8 unused[4096 - 16 - sizeof(ModJpDubTable) - sizeof(ModDubText) -
              MOD_FIXED_TEXT_BYTES - sizeof(void*) - 12 - 8 -
              MOD_GRATE_MAX * 8 - 1];
} ModDubState;
extern ModDubState g_ModDub;
