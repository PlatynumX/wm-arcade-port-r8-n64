/* Auto-generated from historical SELECT.ASM by tools/select_source.py. */
#include "wm/select.h"

const wm_select_point wm_select_crouton_positions[WM_SELECT_VISIBLE_SLOTS] = {
    {164, 45},
    {204, 45},
    {164, 90},
    {204, 90},
    {164, 135},
    {204, 135},
    {164, 180},
    {204, 180},
};

const uint8_t wm_select_slot_source_wrestlers[WM_SELECT_VISIBLE_SLOTS] = {
    6, 1, 2, 3, 4, 5, 0, 8,
};

const wm_select_attributes wm_select_source_attributes[WM_SELECT_SOURCE_WRESTLERS] = {
    {4, 9, 9, 3},
    {7, 6, 2, 5},
    {8, 4, 7, 6},
    {9, 2, 4, 6},
    {3, 9, 8, 7},
    {8, 6, 5, 3},
    {4, 8, 7, 8},
    {9, 5, 4, 7},
    {9, 5, 4, 7},
};

const wm_select_player_def wm_select_players[2] = {
    {0, {20, 175}, false, 0x00c8, 0x00cb},
    {1, {382, 175}, true, 0x00c7, 0x00cc},
};

const wm_select_bmod_entry wm_select_background_modules[2] = {
    {"wwfselbkBMOD", -40, 0},
    {"choiceBMOD", 3, 256},
};
