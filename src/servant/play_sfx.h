// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef KO_SUBTITLE /* 094_15: ko_sub.h (faerie / demon overlays only) defines these */
#define KO_SUB_SHOW(id)
#define KO_SUB_HIDE()
#endif
void ProcessSfxState(Entity* entity) {
    switch (entity->step) {
    case 0:
        entity->flags = FLAG_UNK_20000 | FLAG_KEEP_ALIVE_OFFCAMERA;
        KO_SUB_HIDE(); /* 094_17: drop a stale subtitle left by a previous voice / room (marker-checked) */
        if (g_CutsceneHasControl) {
            g_PlaySfxStep = 99;
            KO_SUB_HIDE();
            DestroyEntity(entity);
            return;
        }
        if (g_api.CdSoundCommandQueueEmpty()) {
            g_api.PlaySfx(SET_UNK_10);
            entity->step++;
        }
        break;

    case 1:
        if (g_api.func_80133950()) {
            entity->step++;
        }
        break;

    case 2:
        g_api.PlaySfx(entity->params);
        entity->step++;
        break;

    case 3:
        if (g_api.func_80131F68()) {
            entity->step++;
            KO_SUB_SHOW(entity->params);
        }
        break;

    case 4:
        if (!g_api.func_80131F68()) {
            entity->step++;
            KO_SUB_HIDE();
        }
        break;

    case 5:
#ifdef VERSION_PSP
        if (D_8003C708.flags & FLAG_UNK_20 || D_8003C708.flags & FLAG_UNK_40) {
#else
        if (LOW(D_8003C708.flags) & (FLAG_UNK_20 | FLAG_UNK_40)) {
#endif
            g_PlaySfxStep = 99;
            KO_SUB_HIDE();
            DestroyEntity(entity);
            return;
        }
        if (g_api.CdSoundCommandQueueEmpty()) {
            g_api.PlaySfx(SET_XA_PLAYBACK);
            entity->step++;
        }
        break;

    case 6:
        if (g_api.func_80133950()) {
            g_PlaySfxStep = 99;
            KO_SUB_HIDE();
            DestroyEntity(entity);
            return;
        }
        break;

    case 7:
        g_api.PlaySfx(SET_STOP_MUSIC);
        entity->step = 4;
        break;

    case 8:
        g_PlaySfxStep = 99;
        KO_SUB_HIDE();
            DestroyEntity(entity);
        return;
    }

    g_PlaySfxStep = entity->step;
}
