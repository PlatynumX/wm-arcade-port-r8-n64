#include <libdragon.h>
#include <stdbool.h>
#include <stdint.h>

#include "audio_backend.h"

#define WM_DCS_OUTPUT_HZ 31250
#define WM_N64_AUDIO_BUFFERS 4
#define WM_N64_AUDIO_CHANNELS 8
#define WM_DCS_CMD1005_CHANNEL 0
#define WM_DCS_CMD1005_PATH "rom:/dcs/wwf_dcs_cmd1005_31250hz_ref.wav64"

static bool audio_ready;
static wav64_t dcs_cmd1005_wave;

static void stop_all_channels(void) {
    for (int ch = 0; ch < WM_N64_AUDIO_CHANNELS; ++ch)
        mixer_ch_stop(ch);
}

static void play_dcs_cmd1005(uint32_t source_tick) {
    /* Exact output recovered by executing WWF's real DCS command 1005
     * (decimal / 0x03ED) against its U2/U3 DCS ROM streams. */
    mixer_ch_stop(WM_DCS_CMD1005_CHANNEL);
    wav64_play(&dcs_cmd1005_wave, WM_DCS_CMD1005_CHANNEL);
    mixer_ch_set_vol(WM_DCS_CMD1005_CHANNEL, 1.0f, 1.0f);
    debugf("audio: exact DCS cmd 1005 started @ source tick %lu\n",
           (unsigned long)source_tick);
}

static void dispatch_dcs_command(const wm_audio_event *event) {
    if (!event) return;
    switch (event->command) {
        case 0:
            stop_all_channels();
            debugf("audio: DCS stop cmd 0 @ source tick %lu\n",
                   (unsigned long)event->source_tick);
            break;
        case 1005:
            play_dcs_cmd1005(event->source_tick);
            break;
        default:
            debugf("audio: untranslated DCS cmd %u @ source tick %lu\n",
                   (unsigned)event->command,
                   (unsigned long)event->source_tick);
            break;
    }
}

void wm_n64_audio_init(void) {
    if (audio_ready) return;
    audio_init(WM_DCS_OUTPUT_HZ, WM_N64_AUDIO_BUFFERS);
    mixer_init(WM_N64_AUDIO_CHANNELS);
    mixer_set_vol(1.0f);

    /* Level-0 WAV64 is uncompressed and needs no codec initialization. */
    wav64_open(&dcs_cmd1005_wave, WM_DCS_CMD1005_PATH);
    wav64_set_loop(&dcs_cmd1005_wave, false);

    audio_ready = true;
    debugf("audio: N64 AI + RSP mixer initialized at %d Hz\n",
           audio_get_frequency());
    debugf("audio: exact DCS cmd 1005 asset open: %s\n",
           WM_DCS_CMD1005_PATH);
}

void wm_n64_audio_service(wm_app *app) {
    if (!audio_ready || !app) return;
    wm_audio_event event;
    while (wm_audio_pop_event(&app->audio, &event))
        dispatch_dcs_command(&event);
    while (audio_can_write()) {
        int16_t *out = audio_write_begin();
        mixer_poll(out, audio_get_buffer_length());
        audio_write_end();
    }
}
