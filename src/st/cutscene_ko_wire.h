// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
// MOD_KO cutscene script selector (ko_dlg2).
// Default: retail script (passthrough) — behavior identical to OFF build.
// A stage opts in by defining KO_DLG_SCRIPT (u8*, rodata KO full script from
// the build-time splice of gen_ko_dlg.py TABLEs + yaml skeleton) before
// including its SetCutsceneScript header.
#ifdef MOD_KO
static u8* ModKoScript(u8* script) {
#ifdef KO_DLG_SCRIPT
    return KO_DLG_SCRIPT;
#else
    return script;
#endif
}
#endif
