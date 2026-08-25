#ifndef WM_ARCADE_SOUND_TABLES_H
#define WM_ARCADE_SOUND_TABLES_H

#include "wm/arcade_sound.h"

/* Generated from the pinned original DCSSOUND.ASM by tools/import_dcssound.py.
 * The checked-in fallback is intentionally sparse but has the full 0x303
 * addressable table shape so untranslated entries fail closed. */
extern const wm_sound_entry wm_arcade_sound_table[0x303];
extern const uint16_t wm_arcade_sound_table_count;
extern const wm_sound_script wm_arcade_sound_scripts[];
extern const uint16_t wm_arcade_sound_script_count;
extern const wm_sound_random_table wm_arcade_random_tables[];
extern const uint16_t wm_arcade_random_table_count;
extern const wm_sound_wrestler_matrix wm_arcade_wrestler_matrix;
extern const wm_sound_speech_table wm_arcade_speech_tables[];
extern const uint16_t wm_arcade_speech_table_count;
extern const wm_sound_crowd_table wm_arcade_crowd_tables[];
extern const uint16_t wm_arcade_crowd_table_count;

void wm_arcade_sound_bind_default_tables(wm_arcade_sound *s);

#endif
