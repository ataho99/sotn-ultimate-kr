#if defined(MOD_PTBR)
#include "../../include/mod_ptbr_names.h"
#endif

#if defined(MOD_PTBR) && !defined(MOD_PTBR_NAME_NONE)
#if defined(MOD_PTBR_NAME_BLOCK)
#define SN_DATA ((u_long*)(0x801E8000 + MOD_PTBR_NAME_BLOCK))
#define SN_PT()                                                                \
    (MOD_PTBR_TEXT_PT() && *(u32*)0x801E8000 == MOD_PTBR_NAMES_MAGIC)
#else
#define SN_DATA ((u_long*)g_ModPtbrStageName)
#define SN_PT() (MOD_PTBR_TEXT_PT())
#endif
#define SN_ROWS snRows
#define SN_SH snSh
#define SN_TOP snTop
#define SN_STRIPS snStrips
#define SN_LOAD()                                                              \
    (snRows = self->ext.stpopup.modRows, snSh = snRows > 40,                   \
     snTop = 177 - (snRows >> 1), snStrips = (snRows + snSh) >> snSh)
#define SN_LOCAL                                                               \
    s32 snRows, snSh, snTop, snStrips;                                         \
    SN_LOAD()

static void ModPtbrStageName(Entity* self) {
    GfxBank** banks;
    GfxBank* keep;

    self->ext.stpopup.modRows = 40;
    if (!SN_PT()) {
        return;
    }
    self->ext.stpopup.modRows = MOD_PTBR_NAME_ROWS;
    banks = g_api.o.gfxBanks;
    keep = banks[0];
    banks[0] = (GfxBank*)g_ModPtbrNameBank;
    g_api.LoadGfxAsync(ANIMSET_OVL(0));
    banks[0] = keep;
}
#else
#define SN_ROWS 40
#define SN_SH 0
#define SN_STRIPS 40
#define SN_LOCAL
#define ModPtbrStageName(self)

// the text's top row on screen (157 for the retail 40 rows: centred on 177)
#define SN_TOP (177 - (SN_ROWS >> 1))
#endif
