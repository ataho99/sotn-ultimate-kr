#define MOD_LBA_DEMOKEY ((s32*)MOD_PTBR_DEMOKEY)
#define MOD_PTBR_FRAME(step) ((*(void (**)(s32))MOD_PTBR_CODE)(step))
#define MOD_PTBR_ENTRY(n) (((void* const*)MOD_PTBR_CODE)[n])
#define MOD_PTBR_BLOCK_MENU 0 // the title menus' text (Upd_Eng_MenuInit)
#define MOD_PTBR_BLOCK_TITLE                                                   \
    1                      // the language screen (3410C.c HandleTitleScreen)
static s32 g_ModPtbrChunk; // chunk in flight + 1, 0 = none, -1 = done
static s32 g_ModPtbrDemoLoc;
s32 ModPtbrLoad(s32 block) {
    u8* dst;
    s32 n;

    if (MOD_PTBR_BLOB[1] == MOD_PTBR_MAGIC) {
        return ((s32(*)(s32, s32))MOD_PTBR_ENTRY(1))(
            block, OFF_BIN_MONSTER2 + MOD_PTBR_SECTOR + MOD_PTBR_RESIDENT * 4);
    }
    if (g_ModPtbrChunk < 0) {
        return 0; // (it failed: English)
    }
    if (g_CdStep != CdStep_None) {
        return 1;
    }
    if (g_ModPtbrChunk) {
        dst = (u8*)MOD_PTBR_BLOB + (g_ModPtbrChunk - 1) * 0x2000;
        // the last chunk stops at the block end (the stack is above it)
        for (n = 0; n < 0x2000 && dst + n < (u8*)MOD_PTBR_BLOB + MOD_PTBR_MAX;
             n++) {
            dst[n] = ((u8*)DEMO_KEY_PTR)[n];
        }
        *MOD_LBA_DEMOKEY = g_ModPtbrDemoLoc;
        if (g_ModPtbrChunk == MOD_PTBR_RESIDENT) {
            g_ModPtbrChunk = -1; // once per SEL load, even if it failed
            MOD_PTBR_BLOB[1] =
                MOD_PTBR_BLOB[0] == MOD_PTBR_MAGIC ? MOD_PTBR_MAGIC : 0;
            return 1; // (the block next frame)
        }
    }
    g_ModPtbrDemoLoc = *MOD_LBA_DEMOKEY;
    *MOD_LBA_DEMOKEY = OFF_BIN_MONSTER2 + MOD_PTBR_SECTOR;
    g_LoadOvlIdx = g_ModPtbrChunk;
    g_LoadFile = CdFile_DemoKey;
    g_CdStep = CdStep_LoadInit;
    g_ModPtbrChunk++;
    return 1;
}
