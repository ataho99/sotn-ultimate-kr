// SPDX-License-Identifier: AGPL-3.0-or-later
#include "memcard.h"

void SetupEvents(void) {
#ifndef VERSION_PC
    EnterCriticalSection();
#endif
    g_EvSwCardEnd = OpenEvent(SwCARD, EvSpIOE, EvMdNOINTR, NULL);
    g_EvSwCardErr = OpenEvent(SwCARD, EvSpERROR, EvMdNOINTR, NULL);
    g_EvSwCardTmo = OpenEvent(SwCARD, EvSpTIMOUT, EvMdNOINTR, NULL);
    g_EvSwCardNew = OpenEvent(SwCARD, EvSpNEW, EvMdNOINTR, NULL);
    g_EvHwCardEnd = OpenEvent(HwCARD, EvSpIOE, EvMdNOINTR, NULL);
    g_EvHwCardErr = OpenEvent(HwCARD, EvSpERROR, EvMdNOINTR, NULL);
    g_EvHwCardTmo = OpenEvent(HwCARD, EvSpTIMOUT, EvMdNOINTR, NULL);
    g_EvHwCardNew = OpenEvent(HwCARD, EvSpNEW, EvMdNOINTR, NULL);
#ifndef VERSION_PC
    ExitCriticalSection();
#endif
    EnableEvent(g_EvSwCardEnd);
    EnableEvent(g_EvSwCardErr);
    EnableEvent(g_EvSwCardTmo);
    EnableEvent(g_EvSwCardNew);
    EnableEvent(g_EvHwCardEnd);
    EnableEvent(g_EvHwCardErr);
    EnableEvent(g_EvHwCardTmo);
    EnableEvent(g_EvHwCardNew);
}

s32 _peek_event_with_retry(void) {
    if (TestEvent(g_EvSwCardEnd) == 1) {
        return 1;
    } else if (TestEvent(g_EvSwCardErr) == 1) {
        return 2;
    } else if (TestEvent(g_EvSwCardTmo) == 1) {
        return 3;
    } else if (TestEvent(g_EvSwCardNew) == 1) {
        return 4;
    } else if (g_MemcardRetryCount-- < 0) {
        return 2;
    }
    return 0;
}

s32 _peek_event(void) {
    if (TestEvent(g_EvSwCardEnd) == 1) {
        return 1;
    } else if (TestEvent(g_EvSwCardErr) == 1) {
        return 2;
    } else if (TestEvent(g_EvSwCardTmo) == 1) {
        return 3;
    } else if (TestEvent(g_EvSwCardNew) == 1) {
        return 4;
    }
    return 0;
}

void _clear_event(void) {
    TestEvent(g_EvSwCardEnd);
    TestEvent(g_EvSwCardErr);
    TestEvent(g_EvSwCardTmo);
    TestEvent(g_EvSwCardNew);
}

s32 _card_event_x(void) {
    while (true) {
        if (TestEvent(g_EvHwCardEnd) == 1) {
            return 1;
        } else if (TestEvent(g_EvHwCardErr) == 1) {
            return 2;
        } else if (TestEvent(g_EvHwCardTmo) == 1) {
            return 3;
        } else if (TestEvent(g_EvHwCardNew) == 1) {
            return 4;
        }
    }
}

void _clear_event_x(void) {
    TestEvent(g_EvHwCardEnd);
    TestEvent(g_EvHwCardErr);
    TestEvent(g_EvHwCardTmo);
    TestEvent(g_EvHwCardNew);
}

void MemcardInit(void) { g_MemcardStep = 0; }

void MemcardInfoInit(void) {
    g_MemcardInfo[0].nBlockUsed = 0;
    g_MemcardInfo[1].nBlockUsed = 0;
}

s32 MemcardParse(s32 nPort, s32 nCard) {
    char cardName[32];
    struct DIRENTRY* dirent;
    s32 totalEntrySize;
    s32 i;

    if (g_MemcardStep == 0) {
        sprintf(cardName, "bu%1d%1d:", nPort, nCard);
        dirent = &g_MemcardInfo[nPort].entries;
        g_MemcardBlockRead = 0;
        if (firstfile(cardName, dirent) == dirent) {
            g_MemcardBlockRead++;
            g_MemcardStep++;
            return -1;
        } else {
            g_MemcardStep = 2;
            return -1;
        }
    } else if (g_MemcardStep == 1) {
        dirent = &g_MemcardInfo[nPort].entries[g_MemcardBlockRead];
        if (nextfile(dirent) == dirent) {
            g_MemcardBlockRead++;
            return -1;
        } else {
            g_MemcardStep++;
        }
    } else {
        g_MemcardInfo[nPort].nBlockUsed = g_MemcardBlockRead;
        dirent = &g_MemcardInfo[nPort].entries;
        totalEntrySize = 0;
        for (i = 0; i < g_MemcardBlockRead; i++) {
            totalEntrySize += dirent[i].size;
        }

        totalEntrySize /= CARD_BLOCK_SIZE;
        g_MemcardInfo[nPort].nFreeBlock = BLOCK_PER_CARD - totalEntrySize;
        do {
            if (g_MemcardInfo[nPort].nFreeBlock <= 0) {
                return 0;
            }
        } while (0);
        return g_MemcardInfo[nPort].nFreeBlock;
    }
    return -1;
}

s32 GetMemcardFreeBlockCount(s32 nPort) {
    return g_MemcardInfo[nPort].nFreeBlock;
}

int MemcardDetectSave(s32 nPort, u8* expectedSaveName, s32 block) {
    s32 nBlocks;
    s32 didNotMatch;
    s32 i, j;
    bool isCastlevaniaSave;
    struct DIRENTRY* entries;

    isCastlevaniaSave = false;
    entries = g_MemcardInfo[nPort].entries;
    nBlocks = g_MemcardInfo[nPort].nBlockUsed;
    for (i = 0; i < nBlocks; i++) {
        didNotMatch = 0;
        for (j = 0; j <= 20; j++) {
            char ch = expectedSaveName[j];
            if (ch == '\0') {
                break;
            }
            if (ch != entries[i].name[j]) {
                didNotMatch++;
                break;
            }
        }

        if (didNotMatch == 0) {
            isCastlevaniaSave = true;
            break;
        }
    }

    g_MemcardInfo[nPort].blocks[block] = isCastlevaniaSave;
    return isCastlevaniaSave;
}

#if !defined(                                                                  \
    MOD_DRA_STRIP) // only MemcardReadFile (dropped in DRA) and SEL use it
s32 IsMemcardBlockUsed(u32 nPort, u32 block) {
    return g_MemcardInfo[nPort].blocks[block];
}
#endif

#if !defined(MOD_DRA_STRIP) // DRA never reads a file (SEL does: no
                            // MOD_DRA_STRIP there)
s32 MemcardReadFile(
    s32 nPort, s32 nCard, const char* name, void* data, s32 nBlock) {
    char savePath[32];
    s32 fd;
    s32 nBytes;

    sprintf(savePath, "bu%1d%1d:%s", nPort, nCard, name);
    if (nBlock == 0) {
#if defined(MOD_SATURN)
        // the file list reads up to the status end (ModSaveExtra and the
        // "SAT4" magic, at 0x554..0x56C): a mod area save names its place
        nBytes = 0x580;
#else
        nBytes = 0x2B8;
#endif
    } else {
        nBytes = nBlock * CARD_BLOCK_SIZE;
    }

    fd = open(savePath, FREAD | FASYNC);
    if (fd == -1) {
        return -1;
    }

    g_MemcardFd = fd;
    _clear_event();
    read(fd, data, nBytes);
    return 0;
}
#endif

s32 MemcardWriteFile(s32 nPort, s32 nCard, const char* name, void* data,
                     s32 flags, bool create) {
    char savePath[32];
    s32 len;
    s32 fd;

    sprintf(savePath, &"bu%1d%1d:%s", nPort, nCard, name);

    // known PSX bug: when creating a a file with open(), any read or write
    // will immediately fail. The workaround is to close the file and open
    // it again.
    if (create == 1) {
        fd = open(savePath, (flags << 0x10) | FCREAT);
        if (fd == -1) {
            return -2;
        } else {
            close(fd);
        }
    }

    len = flags << 0xD;
    fd = open(savePath, FWRITE | FASYNC);

    if (fd == -1) {
        return -1;
    } else {
        g_MemcardFd = fd;
        _clear_event();
        write(fd, data, len);
    }
    return 0;
}

#if !defined(                                                                  \
    MOD_DRA_STRIP) // DRA never erases (SEL does: no MOD_DRA_STRIP there)
s32 MemcardEraseFile(s32 nPort, s32 nCard, const char* name) {
    char savePath[0x20];

    sprintf(savePath, "bu%1d%1d:%s", nPort, nCard, name);
    return -(erase(savePath) == 0);
}
#endif

s32 MemcardClose(s32 nPort) {
    s32 eventStep = _peek_event();

    if (eventStep == 0) {
        return 0;
    }

    close(g_MemcardFd);
    if (eventStep != 1) {
        return -3;
    }

    D_8006C3AC |= eventStep << nPort;
    return 1;
}

s32 func_800E9880(s32 nPort, s32 nCard) {
    s32 channel;
    MemcardInfo* memcard;
    channel = (nPort * 0x10) + nCard;
    memcard = &g_MemcardInfo[nPort];
    switch (g_MemcardStep) {
    case 0:
        memcard->unk25C = 0;
        g_MemcardRetryCount = 0x80;
        _card_info(channel);
        g_MemcardStep++;
        break;

    case 1:
        switch (_peek_event_with_retry()) {
        case 0:
            break;

        case 1:
            memcard->unk258 = 1;
            g_MemcardStep++;
            break;

        case 3:
            D_8006C3AC &= g_MemcardPortMask[nPort];
            memcard->unk258 = -1;
            g_MemcardStep = 4;
            break;

        case 4:
            D_8006C3AC &= g_MemcardPortMask[nPort];
            memcard->unk258 = 2;
            _clear_event_x();
            _card_clear(channel);
            _card_event_x();
            g_MemcardStep++;
            break;

        case 2:
            D_8006C3AC &= g_MemcardPortMask[nPort];
            memcard->unk258 = -3;
            g_MemcardStep = 4;
            break;
        }
        break;

    case 2:
        _clear_event();
        _card_load(channel);
        g_MemcardRetryCount = 0x80;
        g_MemcardStep++;
        break;

    case 3:
        switch (_peek_event_with_retry()) {
        case 0:
            break;

        case 1:
            g_MemcardStep++;
            break;

        case 3:
            D_8006C3AC &= g_MemcardPortMask[nPort];
            memcard->unk258 = -1;
            g_MemcardStep++;
            break;

        case 4:
            D_8006C3AC &= g_MemcardPortMask[nPort];
            memcard->unk258 = -2;
            g_MemcardStep++;
            break;

        case 2:
            D_8006C3AC &= g_MemcardPortMask[nPort];
            memcard->unk258 = -3;
            g_MemcardStep++;
            break;
        }

        break;

    case 4:
        g_MemcardStep = 0;
        memcard->unk25C = memcard->unk258;
        break;
    }

    return memcard->unk25C;
}

s32 MemcardFormat(s32 nPort, s32 nCard) {
    char savePath[0x8];
    s32 ret;

    D_8006C3AC &= g_MemcardPortMask[nPort];
    sprintf(savePath, "bu%1d%1d:", nPort, nCard);
    _clear_event_x();
    format(savePath);
    ret = _card_event_x();

    if (ret != 1) {
        if (ret == 3) {
            ret = -1;
        } else {
            ret = -3;
        }
    }
    return ret;
}

void GetSavePalette(u8* dst, s32 palIdx) {
    u16* dst_u16 = (u16*)dst;
    s32 i;
    u16* src = g_saveIconPal;

    src += palIdx * 16;
    for (i = 0; i < COLORS_PER_PAL; i++) {
        *dst_u16++ = *src++;
    }
}

u8* g_saveIconTexture[] = {
    g_saveIcon0,  g_saveIcon1,  g_saveIcon2,  g_saveIcon3,
    g_saveIcon4,  g_saveIcon5,  g_saveIcon6,  g_saveIcon7,
    g_saveIcon8,  g_saveIcon9,  g_saveIcon10, g_saveIcon11,
    g_saveIcon12, g_saveIcon13, g_saveIcon14, g_saveIcon15,
};

void GetSaveIcon(u8* dst, s32 iconIdx) {
    const s32 IconSize = sizeof(((MemcardHeader*)0)->Icon);
    s32 i;
    u8* src;

    src = g_saveIconTexture[iconIdx];
    for (i = 0; i < IconSize; i++) {
        *dst++ = *src++;
    }
}

#if defined(VERSION_US)
s32 g_saveUnkData[] = {
    1, 1, 3, 1, 1, 1, 3, 1, 2, 2, 1, 1, 1, 1, 1, 2,
};
#endif

const char* g_saveBlockName[] = {
    "　１", "　２", "　３", "　４", "　５", "　６", "　７", "　８",
    "　９", "１０", "１１", "１２", "１３", "１４", "１５",
};

char g_AsciiSet[] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',  'j', 'k',
    'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',  'u', 'v',
    'w', 'x', 'y', 'z', '&', '!', '-', '.', '\'', ' ', ' ',
};

char* g_ShiftJisSet[] = {
    "Ａ", "Ｂ", "Ｃ", "Ｄ", "Ｅ", "Ｆ", "Ｇ", "Ｈ", "Ｉ", "Ｊ", "Ｋ",
    "Ｌ", "Ｍ", "Ｎ", "Ｏ", "Ｐ", "Ｑ", "Ｒ", "Ｓ", "Ｔ", "Ｕ", "Ｖ",
    "Ｗ", "Ｘ", "Ｙ", "Ｚ", "＆", "！", "−",  "．", "’",  "　",
};
const char* g_SaveAreaNames[] = {
    "大理石の廊下",
    "崖側外壁",
    "蔵書庫",
    "地下墓地",
    "オルロックの間",
    "えん道",
    "礼拝堂",
    "城入り口",
    "",
    "地下水脈",
    "闘技場",
    "悪魔城最上部",
    "錬金研究棟",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "卑鉱石の廊下",
    "異界側外壁",
    "禁書保管庫",
    "空中墓地",
    "死翼の間",
    "洞窟",
    "異端礼拝堂",
    "逆さ城入り口",
    "",
    "天井水脈",
    "裏闘技場",
    "逆さ城最下部",
    "黒魔術研究棟",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "城入り口",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

#if defined(MOD_SATURN)
// Saturn port: saves carry this marker (see include/game.h MOD_SAVE_MAGIC)
static void ModMarkSave(PlayerStatus* status) {
    status->modPadding[0] = MOD_SAVE_MAGIC & 0xFF;
    status->modPadding[1] = (MOD_SAVE_MAGIC >> 8) & 0xFF;
    status->modPadding[2] = (MOD_SAVE_MAGIC >> 16) & 0xFF;
    status->modPadding[3] = MOD_SAVE_MAGIC >> 24;
}

#if !defined(MOD_SAVE_DRA)
// The equip menu lists body POSITIONS by the category of the item whose id
// is the position (func_800FAF44), then shows the item stored there: every
// position must hold an item of its own category (sorting swaps within one
// list only). Fill dst[0..n) from the order src[0..nSrc) that way, keeping
// each category's sequence; ids >= n are left out.
static void ModRegroupBodyOrder(
    u8* dst, const u8* src, s32 n, s32 nSrc, Accessory* defs) {
    s32 next[8];
    s32 p, k, cat;

    for (k = 0; k < LEN(next); k++) {
        next[k] = 0;
    }
    for (p = 0; p < n; p++) {
        cat = defs[p].equipType & 7;
        for (k = next[cat]; k < nSrc; k++) {
            if (src[k] < n && (defs[src[k]].equipType & 7) == cat) {
                break;
            }
        }
        dst[p] = k < nSrc ? src[k] : p;
        next[cat] = k + 1;
    }
}
#endif

#if !defined(MOD_SAVE_DRA)
// (the conversions on load: SEL only; DRA loads back only its own demo
// snapshot, stored as it is)
static bool ModHasMagic(PlayerStatus* status, u32 magic) {
    return status->modPadding[0] == (magic & 0xFF) &&
           status->modPadding[1] == ((magic >> 8) & 0xFF) &&
           status->modPadding[2] == ((magic >> 16) & 0xFF) &&
           status->modPadding[3] == (magic >> 24);
}

// Rebuild the inventory arrays from a copy of an older status whose arrays
// have nHand / nBody slots (hand counts, body counts, hand order, body order,
// right after the relics and spells). Slots the old layout lacks start empty,
// listed in id order after the old ones.
static void ModRebuildInventory(
    PlayerStatus* status, u8* old, s32 nHand, s32 nBody) {
    u8* handCount = old + NUM_RELICS + NUM_SPELLS;
    u8* bodyCount = handCount + nHand;
    u8* handOrder = bodyCount + nBody;
    u8* bodyOrder = handOrder + nHand;
    s32 i;

    for (i = 0; i < HAND_ITEMS_CAP; i++) {
        if (i < nHand) {
            status->equipHandCount[i] = handCount[i];
            status->equipHandOrder[i] = handOrder[i];
        } else {
            status->equipHandCount[i] = 0;
            status->equipHandOrder[i] = i;
        }
    }
    for (i = 0; i < BODY_ITEMS_CAP; i++) {
        if (i < nBody) {
            status->equipBodyCount[i] = bodyCount[i];
            status->equipBodyOrder[i] = bodyOrder[i];
        } else {
            status->equipBodyCount[i] = 0;
            status->equipBodyOrder[i] = i;
        }
    }
    // each position back to its own category (also repairs the saves of
    // the first compatible build, whose order lost it)
    {
        u8 tmp[BODY_ITEMS_CAP];

        for (i = 0; i < BODY_ITEMS_CAP; i++) {
            tmp[i] = status->equipBodyOrder[i];
        }
        ModRegroupBodyOrder(status->equipBodyOrder, tmp, BODY_ITEMS_CAP,
                            BODY_ITEMS_CAP, g_api.accessoryDefs);
    }
}

// Saturn port: rebuild a status loaded from a retail-layout save (the unmodded
// game, downloaded saves, or mod builds before the inventory expansion)
static void ModConvertRetailStatus(PlayerStatus* status) {
    PlayerStatusRetail old;
    u8* src;
    u8* dst;
    s32 i;
    s32 n;

    src = (u8*)status;
    dst = (u8*)&old;
    for (i = 0; i < sizeof(old); i++) {
        dst[i] = src[i];
    }

    // relics and spells sit before the arrays and did not move
    ModRebuildInventory(status, (u8*)&old, NUM_HAND_ITEMS, NUM_BODY_ITEMS);

    // saveName up to D_80097C40 has the same layout in both versions
    src = (u8*)old.saveName;
    dst = (u8*)status->saveName;
    n = (u8*)old.statsFamiliars - (u8*)old.saveName;
    for (i = 0; i < n; i++) {
        dst[i] = src[i];
    }
    for (i = 0; i < FAMILIARS_STORED; i++) {
        status->statsFamiliars[i] = old.statsFamiliars[i];
    }
    ModMarkSave(status);
}

// Saturn port: "SAT1" saves (mod builds with 92 body slots) differ only in
// the arrays: 2 alignment bytes padded them to the same saveName offset.
static void ModConvertSat1Status(PlayerStatus* status) {
    u8 old[0x240];
    u8* src;
    s32 i;

    src = (u8*)status;
    for (i = 0; i < LEN(old); i++) {
        old[i] = src[i];
    }
    ModRebuildInventory(status, old, HAND_ITEMS_CAP, 92);
    ModMarkSave(status);
}
#endif

// Saturn port: saves are written in the retail layout ("SAT4") so that the
// unmodded game loads them too (the mod's own items, rooms and choices are
// then simply missing there). What the retail layout cannot hold goes into
// the two JP-only familiars' stat slots, which the US game never reads,
// right before the magic: the counts of the new items, the new items that
// are equipped (the retail layout gets the slot's empty item instead) and
// the save room of a mod-only area (the retail one gets the Castle
// Entrance's save room, or the Reverse Entrance's).
typedef struct {
    u8 hand[HAND_ITEMS_CAP - NUM_HAND_ITEMS];
    u8 body[BODY_ITEMS_CAP - NUM_BODY_ITEMS];
    u8 equip[7]; // equipment[] / wornEquipment[] as one array; 0: retail
    u8 stage;    // 0: a retail save room; | 0x80: the cards below are kept
    u8 roomX;    // | the Sprite card's RELIC_FLAG_* bits << 6
    u8 roomY;    // | the Nose Devil card's << 6
} ModSaveExtra;
// The two JP familiars' cards are cleared from the retail layout: the US
// game lists those familiars (and the extra bytes above as their stats)
#define MOD_EXTRA_CARDS 0x80
typedef char ModSaveExtraCheck[sizeof(ModSaveExtra) <= 24 - 4 ? 1 : -1];
#define MOD_SAVE_EXTRA(status)                                                 \
    ((ModSaveExtra*)((u8*)(status) + sizeof(PlayerStatus) - 4 -                \
                     sizeof(ModSaveExtra)))
// The retail layout's sort order cannot hold the new items, so the load put
// them at the end of the lists. The mod's whole order (hand then body, as in
// PlayerStatus) goes after the SaveData, in the rest of the card block that
// StoreSaveData zeroes and writes anyway (the retail game's own saves zero
// it too, so it is only there in a save the mod wrote).
#define MOD_SAVE_ORDER(st)                                                     \
    ((u8*)(st) - MOD_OFFSETOF(SaveData, status) + sizeof(SaveData))
#define MOD_SAVE_ORDER_LEN (HAND_ITEMS_CAP + BODY_ITEMS_CAP)
typedef char ModSaveOrderCheck
    [MOD_OFFSETOF(PlayerStatus, equipBodyOrder) ==
             MOD_OFFSETOF(PlayerStatus, equipHandOrder) + HAND_ITEMS_CAP &&
             sizeof(SaveData) + MOD_SAVE_ORDER_LEN <= CARD_BLOCK_SIZE
         ? 1
         : -1];
#if defined(MOD_SAVE_DRA)
// main.exe's BIOS memcpy (retail overlays call it too); DRA's link has no
// symbol for it. Smaller than a copy loop
#define MOD_MEMCPY ((void* (*)(void*, const void*, s32))0x800160C4)
typedef char ModRelicsSpellsCheck
    [MOD_OFFSETOF(PlayerStatus, spells) == NUM_RELICS &&
             MOD_OFFSETOF(PlayerStatusRetail, spells) == NUM_RELICS
         ? 1
         : -1];
static const u8 g_ModEmptySlot[7] = {
    ITEM_EMPTY_HAND, ITEM_EMPTY_HAND,   ITEM_EMPTY_HEAD,  ITEM_NO_ARMOR,
    ITEM_NO_CAPE,    ITEM_NO_ACCESSORY, ITEM_NO_ACCESSORY};

static void ModStoreRetailStatus(PlayerStatusRetail* dst) {
    ModSaveExtra* x = MOD_SAVE_EXTRA(dst);
    u32* eq = dst->equipment;
    s32 i, n;

    // (dst is zeroed by StoreSaveData) relics, then spells, in both layouts
    MOD_MEMCPY(dst->relics, g_Status.relics, NUM_RELICS + NUM_SPELLS);
    for (i = 0, n = 0; i < HAND_ITEMS_CAP; i++) {
        if (i < NUM_HAND_ITEMS) {
            dst->equipHandCount[i] = g_Status.equipHandCount[i];
        } else {
            x->hand[i - NUM_HAND_ITEMS] = g_Status.equipHandCount[i];
        }
        // the sort order without the new items (a permutation again)
        if (g_Status.equipHandOrder[i] < NUM_HAND_ITEMS) {
            dst->equipHandOrder[n++] = g_Status.equipHandOrder[i];
        }
    }
    for (i = 0; i < BODY_ITEMS_CAP; i++) {
        if (i < NUM_BODY_ITEMS) {
            dst->equipBodyCount[i] = g_Status.equipBodyCount[i];
        } else {
            x->body[i - NUM_BODY_ITEMS] = g_Status.equipBodyCount[i];
        }
    }
    // the retail positions keep their own category (the equip menu lists
    // positions by it; a compaction shifted the cloak list onto accessories):
    // a new item sorted into a retail position gives it the retail item now
    // at the new items' positions (two capes, then an accessory: in the
    // order the retail capes, then accessories, come). SEL's load regroups
    // the order the same way, so the mod's own order always allows this.
    for (i = 0, n = NUM_BODY_ITEMS; i < NUM_BODY_ITEMS; i++) {
        u8 v = g_Status.equipBodyOrder[i];

        while (v >= NUM_BODY_ITEMS && n < BODY_ITEMS_CAP) {
            v = g_Status.equipBodyOrder[n++];
        }
        dst->equipBodyOrder[i] = v < NUM_BODY_ITEMS ? v : i;
    }
    // saveName up to the stored familiars: the same layout in both
    MOD_MEMCPY(dst->saveName, g_Status.saveName,
           (u8*)&g_Status.statsFamiliars[FAMILIARS_STORED] -
               (u8*)g_Status.saveName);
    for (i = 0; i < 7; i++) {
        if (eq[i] >= (i < 2 ? NUM_HAND_ITEMS : NUM_BODY_ITEMS)) {
            x->equip[i] = eq[i];
            eq[i] = g_ModEmptySlot[i];
        }
    }
    MOD_MEMCPY(MOD_SAVE_ORDER(dst), g_Status.equipHandOrder, MOD_SAVE_ORDER_LEN);
    ModMarkSave((PlayerStatus*)dst);
}
#endif

#if defined(MOD_SATURN_ITEMS_TEST)
// Saturn port test aid (off by default, see include/game.h): loading a save
// adds the new items that already work, so they can be tried before the
// Saturn stages that hold them are ported.
static void ModItemsTestGift(PlayerStatus* status) {
    if (!status->equipHandCount[ITEM_SMELLY_RICE]) {
        status->equipHandCount[ITEM_SMELLY_RICE] = 9;
    }
    if (!status->equipHandCount[ITEM_DELICIOUS_MEAL]) {
        status->equipHandCount[ITEM_DELICIOUS_MEAL] = 9;
    }
    if (!status->equipHandCount[ITEM_SIGNED_PENCIL_BOARD]) {
        status->equipHandCount[ITEM_SIGNED_PENCIL_BOARD] = 3;
    }
    if (!status->equipBodyCount[ITEM_SONG_LYRICS_CARD]) {
        status->equipBodyCount[ITEM_SONG_LYRICS_CARD] = 1;
    }
    // the four new weapons (Saturn weapon overlays 60-63)
    if (!status->equipHandCount[ITEM_ALUCARD_SPEAR]) {
        status->equipHandCount[ITEM_ALUCARD_SPEAR] = 1;
    }
    if (!status->equipHandCount[ITEM_WOODLAND_SHIELD]) {
        status->equipHandCount[ITEM_WOODLAND_SHIELD] = 1;
    }
    if (!status->equipHandCount[ITEM_ASTRAL_DAGGER]) {
        status->equipHandCount[ITEM_ASTRAL_DAGGER] = 1;
    }
    if (!status->equipHandCount[ITEM_DOUBLE_BLADE_AXE]) {
        status->equipHandCount[ITEM_DOUBLE_BLADE_AXE] = 1;
    }
    // a retail Combat knife, to compare with the Astral Dagger (same attack)
    if (!status->equipHandCount[ITEM_COMBAT_KNIFE]) {
        status->equipHandCount[ITEM_COMBAT_KNIFE] = 1;
    }
    // Library cards to use the test warp freely (topped up on every load)
    if (status->equipHandCount[ITEM_LIBRARY_CARD] < 99) {
        status->equipHandCount[ITEM_LIBRARY_CARD] = 99;
    }
    // for the dialogue warp chain: the strongest sword (quick bosses) and
    // the Holy Glasses (Shaft's orb in the Richter fight), unless held/worn
    if (!status->equipHandCount[ITEM_CRISSAEGRIM] &&
        status->equipment[LEFT_HAND_SLOT] != ITEM_CRISSAEGRIM &&
        status->equipment[RIGHT_HAND_SLOT] != ITEM_CRISSAEGRIM) {
        status->equipHandCount[ITEM_CRISSAEGRIM] = 1;
    }
    if (!status->equipBodyCount[ITEM_HOLY_GLASSES] &&
        ((u32*)status->equipment)[HEAD_SLOT] != ITEM_HOLY_GLASSES) {
        status->equipBodyCount[ITEM_HOLY_GLASSES] = 1;
    }
}
#endif
#endif

#if defined(MOD_SATURN) && !defined(MOD_SAVE_DRA)
// Saturn port: SEL's saves (the ending's, the file menu's) are DRA's
// StoreSaveData, which writes the retail layout; DRA sets the pointer every
// frame (at the fixed address SEL's 2C048.c MOD_DUB_STATE uses too)
void StoreSaveData(SaveData* save, s32 block, s32 cardIcon) {
    ((void (*)(SaveData*, s32, s32))((ModDubState*)0x8013644C)->storeSave)(
        save, block, cardIcon);
}
#else
void StoreSaveData(SaveData* save, s32 block, s32 cardIcon) {
    const int RoomCount = 942;
    MemcardHeader h;

    MemcardHeader* dstHeader;
    PlayerStatus* dstStatus;
    MenuNavigation* dstNav;
    GameSettings* dstSettings;

    char saveTitle[64];
    u32* saveRaw;
    SaveData* dst;
    s32 saveNameLen;
    s32 i, j;

    saveRaw = (u32*)save;
    for (i = 0; i < 0x800; i++) {
        *saveRaw++ = 0;
    }

    h.Magic[0] = 'S';
    h.Magic[1] = 'C';
    h.Type = 0x13;
    h.BlockEntry = 1;

    // The h.Title content will look like this:
    // US: ＣＡＳＴＬＥＶＡＮＩＡ－１１　ＡＬＵＣＡＲＤ　２００％
    // HD: ドラキュラＸ−１１　ＡＬＵＣＡＲＤ　オルロックの間　２００％
    for (i = 0; i < 0x5C; i++) {
        h.Title[i] = 0;
    }

#if defined(VERSION_US)
    strcpy(h.Title, "ＣＡＳＴＬＥＶＡＮＩＡ−");
#else
    strcpy(h.Title, "ドラキュラＸ−");
#endif

    // writes slot number
    if (block > 8) {
        strcpy(saveTitle, "００");
        saveTitle[1] += (block + 1) / 10;
        saveTitle[3] += (block + 1) % 10;
        strcat(h.Title, saveTitle);
    } else {
        strcpy(saveTitle, "０");
        saveTitle[1] += block + 1;
        strcat(h.Title, saveTitle);
    }

    strcat(h.Title, "　");
    for (saveNameLen = 7; saveNameLen > 0; saveNameLen--) {
        if (g_Status.saveName[saveNameLen] != 0x20) {
            break;
        }
    }

    // writes save name
    for (i = 0; i < saveNameLen + 1; i++) {
        char ch = g_Status.saveName[i];
        for (j = 0; j < 0x20; j++) {
            // Converts ASCII into Shift-JIS
            if (ch == g_AsciiSet[j]) {
                strcat(h.Title, g_ShiftJisSet[j]);
                break;
            }
        }
    }

#if defined(VERSION_HD)
    // writes stage name
    strcat(h.Title, "　");
    strcat(h.Title, g_SaveAreaNames[g_StageId]);
#endif

    // writes room completion percentage
    strcat(h.Title, "　");
    i = g_RoomCount * 100 / RoomCount;
    if (i >= 100) {
        strcpy(saveTitle, "０００");
        saveTitle[1] += i / 100;
        saveTitle[3] += (i / 10) % 10;
        saveTitle[5] += i % 10;
        strcat(h.Title, saveTitle);
    } else if (i > 9) {
        strcpy(saveTitle, "００");
        saveTitle[1] += i / 10;
        saveTitle[3] += i % 10;
        strcat(h.Title, saveTitle);
    } else {
        strcpy(saveTitle, "０");
        saveTitle[1] += i;
        strcat(h.Title, saveTitle);
    }
    strcat(h.Title, "％");

    GetSavePalette(h.Clut, cardIcon);
    GetSaveIcon(h.Icon, cardIcon);

    dst = save;
    dstHeader = &dst->header;
    dstStatus = &dst->status;
    dstNav = &dst->menuNavigation;
    dstSettings = &dst->settings;

    *dstHeader = h;
    for (i = 0; i < 10; i++) {
        dst->info.name[i] = g_Status.saveName[i];
    }

    dst->info.level = g_Status.level;
    dst->info.gold = g_Status.gold;
    dst->info.playHours = g_Status.timerHours;
    dst->info.playMinutes = g_Status.timerMinutes;
    dst->info.playSeconds = g_Status.timerSeconds;
    dst->info.cardIcon = cardIcon;
    dst->info.stage = g_StageId;
    dst->info.endGameFlags = g_GameClearFlag;
    dst->info.character = g_PlayableCharacter;
    dst->info.nRoomsExplored = g_RoomCount;
    dst->info.roomX = g_Tilemap.left;
    dst->info.roomY = g_Tilemap.top;
    dst->info.saveSize = sizeof(SaveData);
#if defined(MOD_SATURN)
    if (save == (SaveData*)SAVE_DATA_PTR) {
        // the Library demo's snapshot (5087C.c): loaded back by
        // ApplySaveData as it is
        *dstStatus = g_Status;
    } else {
        ModSaveExtra* x = MOD_SAVE_EXTRA(dstStatus);

        ModStoreRetailStatus((PlayerStatusRetail*)dstStatus);

        // the Entrance save rooms moved one screen (see ApplySaveData):
        // their retail spot; a mod-only area's save room: the Entrance's
        if ((g_StageId == STAGE_NP3 || g_StageId == STAGE_NO3) &&
            g_Tilemap.top == 43) {
            dst->info.roomY = 42;
        } else if (g_StageId == STAGE_RNO3 && g_Tilemap.top == 20) {
            dst->info.roomY = 21;
        } else if (g_StageId == MOD_STAGE_GDN || g_StageId == MOD_STAGE_PRS ||
                   g_StageId == MOD_STAGE_RGDN || g_StageId == MOD_STAGE_RPRS) {
            x->stage = g_StageId;
            x->roomX = g_Tilemap.left;
            x->roomY = g_Tilemap.top;
            if (g_StageId & STAGE_INVERTEDCASTLE_FLAG) {
                dst->info.stage = STAGE_RNO3;
                dst->info.roomX = 60;
                dst->info.roomY = 21;
            } else {
                dst->info.stage = STAGE_NP3;
                dst->info.roomX = 3;
                dst->info.roomY = 42;
            }
        }
        // (rooms are below 64: their top bits are free)
        x->stage |= MOD_EXTRA_CARDS;
        x->roomX |= (g_Status.relics[RELIC_JP_0] & 3) << 6;
        x->roomY |= (g_Status.relics[RELIC_JP_1] & 3) << 6;
        ((PlayerStatusRetail*)dstStatus)->relics[RELIC_JP_0] = 0;
        ((PlayerStatusRetail*)dstStatus)->relics[RELIC_JP_1] = 0;
    }
#else
    *dstStatus = g_Status;
#endif
    *dstNav = g_MenuNavigation;
#if defined(MOD_SATURN)
    // the equip menu's Holder row is not a retail slot
    if (dstNav->cursorEquip == MOD_HOLDER_SLOT) {
        dstNav->cursorEquip = 0;
    }
    // the retail lists are shorter (no new items, no Extra settings row):
    // their cursors from the top, or the US game shows stale rows
    if (save != (SaveData*)SAVE_DATA_PTR) {
        s32 k;

        dstNav->cursorHandEquipType = 0;
        for (k = 0; k < LEN(dstNav->cursorEquipType); k++) {
            dstNav->cursorEquipType[k] = 0;
        }
        for (k = 0; k < LEN(dstNav->scrollEquipType); k++) {
            dstNav->scrollEquipType[k] = 0;
        }
        if (dstNav->cursorSettings > 5) {
            dstNav->cursorSettings = 0;
        }
    }
#endif
    *dstSettings = g_Settings;

    for (i = 0; i < LEN(g_CastleFlags); i++) {
        dst->castleFlags[i] = g_CastleFlags[i];
    }

    for (i = 0; i < LEN(g_CastleMap); i++) {
        dst->castleMap[i] = g_CastleMap[i];
    }

    dst->rng = g_randomNext;
}
#endif

#if defined(MOD_SATURN) && !defined(MOD_SAVE_DRA)
// is o[0..n) a whole sort order (each id below n once)?
static bool ModIsWholeOrder(const u8* o, s32 n) {
    u8 seen[HAND_ITEMS_CAP];
    s32 i;

    for (i = 0; i < n; i++) {
        seen[i] = 0;
    }
    for (i = 0; i < n; i++) {
        if (o[i] >= n || seen[o[i]]) {
            return false;
        }
        seen[o[i]] = 1;
    }
    return true;
}

// the order after the SaveData (MOD_SAVE_ORDER), where the player sorted the
// new items; without it (an older save, or one the retail game wrote) the
// rebuilt order stays, new items last
static void ModLoadSavedOrder(PlayerStatus* status, const u8* o) {
    const u8* body = o + HAND_ITEMS_CAP;
    s32 i;

    if (!ModIsWholeOrder(o, HAND_ITEMS_CAP) ||
        !ModIsWholeOrder(body, BODY_ITEMS_CAP)) {
        return;
    }
    for (i = 0; i < HAND_ITEMS_CAP; i++) {
        status->equipHandOrder[i] = o[i];
    }
    // (the mod's own order already keeps each position's category)
    ModRegroupBodyOrder(status->equipBodyOrder, body, BODY_ITEMS_CAP,
                        BODY_ITEMS_CAP, g_api.accessoryDefs);
}
#endif

s32 ApplySaveData(SaveData* save) {
    s32 i;
    u32 prevCompletionFlags1;
    u32 prevCompletionFlags2;
    PlayerStatus* srcStatus;
    MenuNavigation* srcNav;
    GameSettings* settings;
    SaveData* savePtr = save;
#if defined(MOD_SATURN) && !defined(MOD_SAVE_DRA)
    s32 oldGodspeed;
#endif

    if (savePtr->info.saveSize != (sizeof(SaveData))) {
        return -1;
    }
    srcStatus = &savePtr->status;
    srcNav = &savePtr->menuNavigation;
    settings = &savePtr->settings;

    g_StageId = savePtr->info.stage;
    g_GameClearFlag = savePtr->info.endGameFlags;
    g_PlayableCharacter = savePtr->info.character;
    g_RoomCount = savePtr->info.nRoomsExplored;
    g_Tilemap.left = savePtr->info.roomX;
    g_Tilemap.top = savePtr->info.roomY;
#if defined(MOD_SATURN) && !defined(MOD_SAVE_DRA)
    // the Castle Entrance save room moved from (3,42) to (3,43), behind the
    // Saturn shaft's alcove (mods/sotn-ultimate/tools/gen_mod_rooms.py): move
    // saves made in the old spot (retail, SAT1) with it; in the first visit's
    // Entrance (NO3) too, which has the same shaft now
    if ((g_StageId == STAGE_NP3 || g_StageId == STAGE_NO3) &&
        g_Tilemap.left == 3 && g_Tilemap.top == 42) {
        g_Tilemap.top = 43;
    }
    // and the Reverse Entrance's, its mirror, from (60,21) to (60,20)
    if (g_StageId == STAGE_RNO3 && g_Tilemap.left == 60 &&
        g_Tilemap.top == 21) {
        g_Tilemap.top = 20;
    }
#endif

    g_Status = *srcStatus;
#if defined(MOD_SATURN) && !defined(MOD_SAVE_DRA)
    // every save older than "SAT3" kept God Speed Shoes in RELIC_JP_0 (the
    // Sprite card's slot); they move to their castle flag byte below
    oldGodspeed = 0;
    if (ModHasMagic(&g_Status, MOD_SAVE_MAGIC)) {
        // "SAT4": the retail layout, plus what it could not hold
        ModSaveExtra x = *MOD_SAVE_EXTRA(&g_Status);
        u32* eq;

        ModConvertRetailStatus(&g_Status);
        ModLoadSavedOrder(&g_Status, MOD_SAVE_ORDER(srcStatus));
        for (i = 0; i < LEN(x.hand); i++) {
            g_Status.equipHandCount[NUM_HAND_ITEMS + i] = x.hand[i];
        }
        for (i = 0; i < LEN(x.body); i++) {
            g_Status.equipBodyCount[NUM_BODY_ITEMS + i] = x.body[i];
        }
        eq = g_Status.equipment;
        for (i = 0; i < 7; i++) {
            if (x.equip[i]) {
                eq[i] = x.equip[i];
            }
        }
        if (x.stage & MOD_EXTRA_CARDS) {
            g_Status.relics[RELIC_JP_0] = x.roomX >> 6;
            g_Status.relics[RELIC_JP_1] = x.roomY >> 6;
            x.roomX &= 0x3F;
            x.roomY &= 0x3F;
        }
        if (x.stage & ~MOD_EXTRA_CARDS) {
            g_StageId = x.stage & ~MOD_EXTRA_CARDS;
            g_Tilemap.left = x.roomX;
            g_Tilemap.top = x.roomY;
        }
    } else if (!ModHasMagic(&g_Status, MOD_SAVE_MAGIC_SAT3)) {
        oldGodspeed = g_Status.relics[RELIC_JP_0] | 0x100;
        g_Status.relics[RELIC_JP_0] = RELIC_FLAG_DISABLE;
        if (ModHasMagic(&g_Status, MOD_SAVE_MAGIC_SAT1)) {
            ModConvertSat1Status(&g_Status);
        } else if (ModHasMagic(&g_Status, MOD_SAVE_MAGIC_SAT2)) {
            ModMarkSave(&g_Status);
        } else {
            ModConvertRetailStatus(&g_Status);
        }
    }
#if defined(MOD_SATURN_ITEMS_TEST)
    ModItemsTestGift(&g_Status);
#endif
#endif
    g_MenuNavigation = *srcNav;

    prevCompletionFlags1 = g_Settings.D_8003CB00;
    prevCompletionFlags2 = g_Settings.D_8003CB04;
    g_Settings = *settings;
    g_Settings.D_8003CB00 |= prevCompletionFlags1;
    g_Settings.D_8003CB04 |= prevCompletionFlags2;
#if defined(MOD_SATURN) && !defined(MOD_SAVE_DRA)
    // Richter and Maria have no menu to leave mono (the retail new-game
    // default, kept by saves made before stereo became the default): their
    // saves load in stereo
    if (g_PlayableCharacter != PLAYER_ALUCARD) {
        g_Settings.isSoundMono = false;
    }
#endif

    for (i = 0; i < LEN(g_CastleFlags); i++) {
        g_CastleFlags[i] = savePtr->castleFlags[i];
    }
#if defined(MOD_SATURN) && !defined(MOD_SAVE_DRA)
    if (oldGodspeed) {
        // one-time move of God Speed Shoes out of RELIC_JP_0 (see above);
        // the familiar stat bytes next to it were never used by old saves
        g_CastleFlags[MOD_CF_GODSPEED] = oldGodspeed;
        for (i = 0; i < (NUM_FAMILIARS - FAMILIARS_STORED) * 4; i++) {
            g_CastleFlags[MOD_CF_FAMILIARS + i] = 0;
        }
    }
#endif

    for (i = 0; i < LEN(g_CastleMap); i++) {
        g_CastleMap[i] = savePtr->castleMap[i];
    }

    g_randomNext = savePtr->rng;
    return 0;
}

void MakeMemcardPath(char* dstSaveName, s32 block) {
    strcpy(dstSaveName, MEMCARD_ID);
    dstSaveName[0x10] += block / 10;
    dstSaveName[0x11] += block % 10;
}
