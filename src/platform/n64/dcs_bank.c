#include <libdragon.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dcs_bank.h"

typedef struct {
    uint16_t command;
    const char *path;
    uint8_t mixer_channel;
} wm_dcs_bank_entry;

static const wm_dcs_bank_entry bank[] = {
    {  176, "rom:/dcs/cmd_0176.wav64", 1 },
    {  208, "rom:/dcs/cmd_0208.wav64", 2 },
    {  244, "rom:/dcs/cmd_0244.wav64", 2 },
    {  248, "rom:/dcs/cmd_0248.wav64", 2 },
    {  460, "rom:/dcs/cmd_0460.wav64", 2 },
    { 1376, "rom:/dcs/cmd_1376.wav64", 1 },
    { 1456, "rom:/dcs/cmd_1456.wav64", 2 },
    { 1460, "rom:/dcs/cmd_1460.wav64", 2 },
    { 1512, "rom:/dcs/cmd_1512.wav64", 1 },
    { 1556, "rom:/dcs/cmd_1556.wav64", 1 },
    { 1560, "rom:/dcs/cmd_1560.wav64", 1 },
    { 1564, "rom:/dcs/cmd_1564.wav64", 1 },
    { 1568, "rom:/dcs/cmd_1568.wav64", 1 },

    { 2560, "rom:/dcs/cmd_2560.wav64", 3 },
    { 2564, "rom:/dcs/cmd_2564.wav64", 3 },
    { 2568, "rom:/dcs/cmd_2568.wav64", 3 },
    { 2572, "rom:/dcs/cmd_2572.wav64", 3 },
    { 2576, "rom:/dcs/cmd_2576.wav64", 3 },

    { 3640, "rom:/dcs/cmd_3640.wav64", 3 },
    { 3644, "rom:/dcs/cmd_3644.wav64", 3 },
    { 3648, "rom:/dcs/cmd_3648.wav64", 3 },
    { 3652, "rom:/dcs/cmd_3652.wav64", 3 },
    { 3656, "rom:/dcs/cmd_3656.wav64", 3 },
    { 3660, "rom:/dcs/cmd_3660.wav64", 3 },
    { 3664, "rom:/dcs/cmd_3664.wav64", 3 },
    { 3668, "rom:/dcs/cmd_3668.wav64", 3 },
};

#define BANK_CHANNEL_FIRST 1
#define BANK_CHANNEL_LAST  3
#define ANNOUNCER_CHANNEL   3
#define ANNOUNCER_QUEUE_CAPACITY 16

typedef struct {
    uint16_t command;
    uint32_t source_tick;
} queued_voice;

static wav64_t active_wave[BANK_CHANNEL_LAST + 1];
static bool active_open[BANK_CHANNEL_LAST + 1];

static queued_voice announcer_queue[ANNOUNCER_QUEUE_CAPACITY];
static unsigned announcer_read;
static unsigned announcer_write;
static unsigned announcer_count;

static const wm_dcs_bank_entry *find_entry(uint16_t command) {
    for (size_t i = 0; i < sizeof(bank)/sizeof(bank[0]); ++i)
        if (bank[i].command == command) return &bank[i];
    return NULL;
}

static void close_channel(int ch) {
    mixer_ch_stop(ch);
    if (active_open[ch]) {
        wav64_close(&active_wave[ch]);
        active_open[ch] = false;
    }
}

static bool start_entry(const wm_dcs_bank_entry *e, uint32_t source_tick) {
    if (!e) return false;
    const int ch = e->mixer_channel;

    close_channel(ch);
    wav64_open(&active_wave[ch], e->path);
    wav64_set_loop(&active_wave[ch], false);
    active_open[ch] = true;
    wav64_play(&active_wave[ch], ch);
    mixer_ch_set_vol(ch, 1.0f, 1.0f);

    debugf("audio: exact DCS cmd %u started ch%d @ source tick %lu\n",
           (unsigned)e->command, ch, (unsigned long)source_tick);
    return true;
}

static bool queue_announcer(uint16_t command, uint32_t source_tick) {
    if (announcer_count >= ANNOUNCER_QUEUE_CAPACITY) {
        debugf("audio: announcer queue full; dropped DCS cmd %u\n", (unsigned)command);
        return false;
    }
    announcer_queue[announcer_write] = (queued_voice){command, source_tick};
    announcer_write = (announcer_write + 1u) % ANNOUNCER_QUEUE_CAPACITY;
    ++announcer_count;
    debugf("audio: queued announcer DCS cmd %u (%u waiting)\n",
           (unsigned)command, announcer_count);
    return true;
}

void wm_dcs_bank_init(void) {
    for (int ch = BANK_CHANNEL_FIRST; ch <= BANK_CHANNEL_LAST; ++ch)
        active_open[ch] = false;
    announcer_read = announcer_write = announcer_count = 0;
    debugf("audio: exact WWF DCS frontend/select bank ready (%u tracks)\n",
           (unsigned)(sizeof(bank)/sizeof(bank[0])));
}

bool wm_dcs_bank_play(uint16_t command, uint32_t source_tick) {
    const wm_dcs_bank_entry *e = find_entry(command);
    if (!e) return false;

    /*
     * ADD_VOICE is a queue in the arcade.  Channel 3 is therefore serialized:
     * GOOD_EVENING 1FB/1FC/1FD and wrestler names may never interrupt one
     * another merely because three host commands arrived in one N64 frame.
     */
    if (e->mixer_channel == ANNOUNCER_CHANNEL) {
        if (mixer_ch_playing(ANNOUNCER_CHANNEL) || announcer_count != 0u)
            return queue_announcer(command, source_tick);
    }

    return start_entry(e, source_tick);
}

void wm_dcs_bank_service(void) {
    if (mixer_ch_playing(ANNOUNCER_CHANNEL))
        return;

    if (active_open[ANNOUNCER_CHANNEL])
        close_channel(ANNOUNCER_CHANNEL);

    if (announcer_count == 0u)
        return;

    queued_voice q = announcer_queue[announcer_read];
    announcer_read = (announcer_read + 1u) % ANNOUNCER_QUEUE_CAPACITY;
    --announcer_count;

    const wm_dcs_bank_entry *e = find_entry(q.command);
    if (e)
        (void)start_entry(e, q.source_tick);
}

void wm_dcs_bank_stop(void) {
    announcer_read = announcer_write = announcer_count = 0;
    for (int ch = BANK_CHANNEL_FIRST; ch <= BANK_CHANNEL_LAST; ++ch)
        close_channel(ch);
}
