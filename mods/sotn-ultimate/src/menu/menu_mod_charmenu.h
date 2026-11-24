// SPDX-License-Identifier: AGPL-3.0-or-later
// SOTN Ultimate: included by src/dra/menu.c (at file scope, before
// MenuDrawStats)
// Saturn port mod: the pause menu for Richter and the Marias (game_mod.h
// MOD_CHAR_MENU; the prologue keeps the retail PAUSE banner).
// It is Alucard's menu, trimmed: the main box has two rows (Commands,
// System); the status screen and the rows are drawn by
// mods/sotn-ultimate/src/menu/charmenu.c (after ARC_F2.BIN's loaded part),
// which the menu reads into g_Pix[1] when it opens (DRA has no room for it). On
// exit none of Alucard's reloads run (weapons at 0x8017A000, the familiar at
// 0x80170000, his cape palette, make_all): in these games those addresses
// are the character's own file. g_ModDub.charMenu says which menu is open
// (set at MENU_STEP_INIT), g_ModDub.charMenuLoad where its file is:
// 0 not asked, 1 reading, 2 done.

// MENU_STEP_INIT, after the boxes are reset (func_800FB9BC)
static void ModCharMenuInit(void) {
    MenuContext* m = &g_Menus[MENU_DG_MAIN];

    g_ModDub.charMenu = MOD_CHAR_MENU();
    g_ModDub.charMenuLoad = 0;
    MOD_CHARMENU_BLOB->magic = 0;
    if (!g_ModDub.charMenu) {
        return;
    }
    // two 16 px rows, centred on the five rows' box (y 100, 80 high)
    m->cursorY = m->unk1.y = 124;
    m->cursorH = m->unk1.h = 32;
    if (g_MenuNavigation.cursorMain >= 2) {
        g_MenuNavigation.cursorMain = 0;
    }
}

// MENU_STEP_OPENING, once the music has stopped: true once the file is in
// (or could not be read: the menu opens without its drawing)
extern CdFile g_LbaBinDemokey; // src/dra/cd.c (the load borrowed)

static s32 ModCharMenuLoaded(void) {
    if (!g_ModDub.charMenu || g_ModDub.charMenuLoad == 2) {
        return 1;
    }
    if (g_CdStep != CdStep_None || g_IsUsingCd) {
        return 0;
    }
    if (g_ModDub.charMenuLoad == 1) {
        g_ModDub.charMenuLoad = 2;
        g_LbaBinDemokey.size = 0x2000; // (the title demo's key file)
        return 1;
    }
    g_LbaBinDemokey.size = MOD_CHARMENU_SIZE;
    g_LoadOvlIdx = MOD_CHARMENU_OVL;
    g_LoadFile = CdFile_DemoKey;
    g_CdStep = CdStep_LoadInit;
    g_ModDub.charMenuLoad = 1;
    return 0;
}

// MENU_STEP_EXIT_5, after their Extra settings changed the costume /
// version: the
// game load's CD chain (CdFile_GameChr: F_GAME2, then the overlay and its
// banks) before the step puts the room's object graphics back
// (LoadGfxAsync(D_80097904)): F_GAME2 goes over that VRAM. Then the DRA
// animsets the room has (as Engine_Init, src/dra/5087C.c). True while it
// loads
static s32 ModCharSwapLoad(void) {
    if (g_ModDub.charSwap == 0 || g_ModDub.charSwap == 3) {
        return 0;
    }
    if (g_CdStep != CdStep_None) {
        return 1; // (a dub reload, or this)
    }
    if (g_ModDub.charSwap == 1) {
        g_ModDub.charSwap = 2;
        g_LoadFile = CdFile_GameChr;
        g_CdStep = CdStep_LoadInit;
        return 1;
    }
    // F_GAME2 went over the castle map's image too (VRAM 0x340,0x100, its
    // blocks 20 / 22): painted again, as at a game load (Play_Init)
    func_800F2120();
    if (D_8003C708.flags & FLAG_UNK_20) {
        LoadGfxAsync(ANIMSET_DRA(3));
    }
    if (D_8003C708.flags & FLAG_UNK_40) {
        LoadGfxAsync(ANIMSET_DRA(4));
    }
    g_ModDub.charSwap = 3; // (EXIT_12 restarts him)
    return 0;
}

void ModHudSwap(void); // src/dra/5F60C.c

// Maria's state into the version now chosen (user decisions, 2026-11-14;
// in the release notes): HP kept as it is.
//   into the PSP one: hearts at most 99 (her HUD has two digits); her
//     subweapon becomes the thrown one and the other slot the Doll (without
//     MOD_FLAG_MARIA_SUB2 her init, mar2.c MarPsxInit, does just that)
//   into the Saturn one: the thrown subweapon (D_80097C40 outside her
//     MarMain; the Doll is kept there as the Bible's id, and the Saturn
//     Maria has none) carries over; MP 200 / 200 (her init would take a
//     smaller pool for a new game: HP, hearts and the Dragon Helm reset)
static void ModMariaConvert(void) {
    s32 w;

    MOD_STATUS_FLAGS &= ~MOD_FLAG_MARIA_SUB2;
    if (MOD_MARIA_PSP()) {
        if (g_Status.hearts > 99) {
            g_Status.hearts = 99;
        }
        if (g_Status.heartsMax > 99) {
            g_Status.heartsMax = 99;
        }
        return;
    }
    w = g_Status.D_80097C40;
    g_Status.subWeapon = w == SUBWPN_BIBLE ? 0 : w;
    g_Status.D_80097C40 = 0;
    g_Status.mp = g_Status.mpMax = 200;
}

// MENU_STEP_EXIT_12, the files in again: the player's entities (slots
// 1..63) go, Maria's state follows her new version, the overlay's init runs
// as at a game load (Engine_Init), the character stands where it was, and
// Maria's HUD is rebuilt (Richter's costume keeps his)
static void ModCharSwap(void) {
    s32 x = PLAYER.posX.val;
    s32 y = PLAYER.posY.val;
    u16 left = PLAYER.facingLeft;
    Entity* e;

    g_ModDub.charSwap = 0;
    for (e = &g_Entities[1]; e < &g_Entities[STAGE_ENTITY_START]; e++) {
        DestroyEntity(e);
    }
    D_800A3B70[3] = D_800D0F68; // (the PSP Maria's init swaps it)
    if (g_PlayableCharacter == PLAYER_MARIA) {
        ModMariaConvert();
    }
    ((void (*)(s32))g_PlOvl.D_8013C004)(0);
    PLAYER.posX.val = x;
    PLAYER.posY.val = y;
    PLAYER.facingLeft = left;
    if (g_PlayableCharacter == PLAYER_MARIA) {
        ModHudSwap();
    }
}
