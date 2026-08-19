#include <libdragon.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dcs_bank.h"

/*
 * These are RAW 16-bit DCS track commands, not the logical triple_sound IDs.
 * The WAV64 files are generated from the user's WWF U2-U5 ROM set through
 * DCSDecoderNative at the native 31,250 Hz reference level.
 */
typedef struct {
    uint16_t command;
    const char *path;
    uint8_t mixer_channel;
} wm_dcs_bank_entry;

static const wm_dcs_bank_entry bank[] = {
    {  176, "rom:/dcs/cmd_0176.wav64", 1 }, /* frontend whoosh */
    {  208, "rom:/dcs/cmd_0208.wav64", 2 }, /* random-select move */
    {  244, "rom:/dcs/cmd_0244.wav64", 2 }, /* P1 choose */
    {  248, "rom:/dcs/cmd_0248.wav64", 2 }, /* P2 choose */
    {  460, "rom:/dcs/cmd_0460.wav64", 2 }, /* select clock tick */
    { 1376, "rom:/dcs/cmd_1376.wav64", 1 }, /* coin */
    { 1456, "rom:/dcs/cmd_1456.wav64", 2 }, /* P2 cursor move */
    { 1460, "rom:/dcs/cmd_1460.wav64", 2 }, /* P1 cursor move */
    { 1512, "rom:/dcs/cmd_1512.wav64", 1 }, /* buy-in */
    { 1556, "rom:/dcs/cmd_1556.wav64", 1 },
    { 1560, "rom:/dcs/cmd_1560.wav64", 1 },
    { 1564, "rom:/dcs/cmd_1564.wav64", 1 },
    { 1568, "rom:/dcs/cmd_1568.wav64", 1 },

    { 2560, "rom:/dcs/cmd_2560.wav64", 3 }, /* Howard Finkel */
    { 2564, "rom:/dcs/cmd_2564.wav64", 3 },
    { 2568, "rom:/dcs/cmd_2568.wav64", 3 },
    { 2572, "rom:/dcs/cmd_2572.wav64", 3 },
    { 2576, "rom:/dcs/cmd_2576.wav64", 3 },

    { 3640, "rom:/dcs/cmd_3640.wav64", 3 }, /* Doink */
    { 3644, "rom:/dcs/cmd_3644.wav64", 3 }, /* Shawn */
    { 3648, "rom:/dcs/cmd_3648.wav64", 3 }, /* Razor */
    { 3652, "rom:/dcs/cmd_3652.wav64", 3 }, /* Bam Bam */
    { 3656, "rom:/dcs/cmd_3656.wav64", 3 }, /* Undertaker */
    { 3660, "rom:/dcs/cmd_3660.wav64", 3 }, /* Lex */
    { 3664, "rom:/dcs/cmd_3664.wav64", 3 }, /* Bret */
    { 3668, "rom:/dcs/cmd_3668.wav64", 3 }, /* Yokozuna */
};

#define BANK_CHANNEL_FIRST 1
#define BANK_CHANNEL_LAST  3

static wav64_t active_wave[BANK_CHANNEL_LAST + 1];
static bool active_open[BANK_CHANNEL_LAST + 1];

static const wm_dcs_bank_entry *find_entry(uint16_t command) {
    for (size_t i = 0; i < sizeof(bank)/sizeof(bank[0]); ++i)
        if (bank[i].command == command) return &bank[i];
    return NULL;
}

void wm_dcs_bank_init(void) {
    for (int ch = BANK_CHANNEL_FIRST; ch <= BANK_CHANNEL_LAST; ++ch)
        active_open[ch] = false;
    debugf("audio: exact WWF DCS frontend/select bank ready (%u tracks)\n",
           (unsigned)(sizeof(bank)/sizeof(bank[0])));
}

bool wm_dcs_bank_play(uint16_t command, uint32_t source_tick) {
    const wm_dcs_bank_entry *e = find_entry(command);
    if (!e) return false;

    const int ch = e->mixer_channel;
    mixer_ch_stop(ch);
    if (active_open[ch]) {
        wav64_close(&active_wave[ch]);
        active_open[ch] = false;
    }

    wav64_open(&active_wave[ch], e->path);
    wav64_set_loop(&active_wave[ch], false);
    active_open[ch] = true;
    wav64_play(&active_wave[ch], ch);
    mixer_ch_set_vol(ch, 1.0f, 1.0f);

    debugf("audio: exact DCS cmd %u started ch%d @ source tick %lu\n",
           (unsigned)command, ch, (unsigned long)source_tick);
    return true;
}

void wm_dcs_bank_stop(void) {
    for (int ch = BANK_CHANNEL_FIRST; ch <= BANK_CHANNEL_LAST; ++ch) {
        mixer_ch_stop(ch);
        if (active_open[ch]) {
            wav64_close(&active_wave[ch]);
            active_open[ch] = false;
        }
    }
}
