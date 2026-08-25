#ifndef WM_DCS_HOST_PROTOCOL_H
#define WM_DCS_HOST_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Host-word protocol used by WWF's main CPU when talking to the DCS board.
 * This parser is platform-neutral: the N64 backend consumes the decoded action
 * and decides how that maps to WAV64/mixer voices.
 */
typedef enum {
    WM_DCS_HOST_PLAY = 0,
    WM_DCS_HOST_STOP_ALL,
    WM_DCS_HOST_STOP_CHANNEL,
    WM_DCS_HOST_VOLUME,
    WM_DCS_HOST_VOLUME_SELECTOR,
    WM_DCS_HOST_MALFORMED_VOLUME
} wm_dcs_host_action_kind;

typedef struct {
    wm_dcs_host_action_kind kind;
    uint16_t raw_word;
    uint16_t command;
    int8_t source_channel; /* -1 master, 0..3 logical channel, -2 n/a */
    uint8_t volume;
} wm_dcs_host_action;

typedef struct {
    int8_t pending_volume_channel; /* -2 none; -1 master; 0..3 channels */
} wm_dcs_host_protocol;

void wm_dcs_host_protocol_init(wm_dcs_host_protocol *p);
wm_dcs_host_action wm_dcs_host_protocol_feed(wm_dcs_host_protocol *p,
                                               uint16_t word);
bool wm_dcs_host_volume_word_valid(uint16_t word, uint8_t *volume_out);

#endif
