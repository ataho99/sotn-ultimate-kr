// SPDX-License-Identifier: AGPL-3.0-or-later
#include "dai.h"

enum EndChimeSteps {
    END_CHIME_INIT,
    END_CHIME_WAIT,
    END_CHIME_START_MUSIC,
};

extern EInit g_EInitInteractable;

// Used by e_confessional/EntityConfessionalGhost
bool g_confessionalChimeActive = {0}; // bss on pspeu

// Switches from looping confessional chime to dai music
void EntityEndConfessionalChime(Entity* self) {
    if (!g_confessionalChimeActive) {
        DestroyEntity(self);
        return;
    }
    switch (self->step) {
    case END_CHIME_INIT:
        InitializeEntity(g_EInitInteractable);
        g_api.PlaySfx(SET_STOP_SEQ);
        stopMusicFlag = true;
        self->step = 2;
        if (g_api.func_80131F68()) {
            g_api.PlaySfx(SET_UNK_80);
#if defined(MOD_SATURN_CASTLE) && !defined(VERSION_PSP)
            // SOTN Ultimate: retail went on to step 3 (no such step), so the
            // chapel music never came back if a CD stream was still playing:
            // never in retail, but the priest's prayer (0x4E7, voiced in
            // e_confessional.c) still is when Alucard leaves mid-sentence.
            // Wait for it to stop instead, then start the music.
            self->step = END_CHIME_WAIT;
#else
            self->step++;
#endif
        }
        break;
    case END_CHIME_WAIT: // Probably waiting for the chime sfx to not be going
        if (!g_api.func_80131F68()) {
            self->step++;
        }
        break;
    case END_CHIME_START_MUSIC:
        g_api.PlaySfx(currentMusicId);
        stopMusicFlag = false;
        g_confessionalChimeActive = false;
        self->step++; // No case defined, resulting in nop
        break;
    }
}
