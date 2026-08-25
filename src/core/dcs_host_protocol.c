#include "wm/dcs_host_protocol.h"

#define WM_DCS_STOP_CH1 994u
#define WM_DCS_STOP_CH4 997u
#define WM_DCS_VOL_MASTER 0x55AAu
#define WM_DCS_VOL_CH4    0x55AEu

static wm_dcs_host_action make_action(wm_dcs_host_action_kind kind,
                                      uint16_t word) {
    wm_dcs_host_action a;
    a.kind = kind;
    a.raw_word = word;
    a.command = word;
    a.source_channel = -2;
    a.volume = 0u;
    return a;
}

void wm_dcs_host_protocol_init(wm_dcs_host_protocol *p) {
    if (p) p->pending_volume_channel = -2;
}

bool wm_dcs_host_volume_word_valid(uint16_t word, uint8_t *volume_out) {
    const uint8_t value = (uint8_t)(word >> 8);
    const uint8_t inverse = (uint8_t)(word & 0xffu);
    if ((uint8_t)~value != inverse) return false;
    if (volume_out) *volume_out = value;
    return true;
}

wm_dcs_host_action wm_dcs_host_protocol_feed(wm_dcs_host_protocol *p,
                                               uint16_t word) {
    wm_dcs_host_action a;
    if (!p) return make_action(WM_DCS_HOST_PLAY, word);

    if (p->pending_volume_channel != -2) {
        const int8_t target = p->pending_volume_channel;
        p->pending_volume_channel = -2;
        uint8_t value = 0u;
        if (!wm_dcs_host_volume_word_valid(word, &value)) {
            a = make_action(WM_DCS_HOST_MALFORMED_VOLUME, word);
            a.source_channel = target;
            return a;
        }
        a = make_action(WM_DCS_HOST_VOLUME, word);
        a.command = 0u;
        a.source_channel = target;
        a.volume = value;
        return a;
    }

    if (word >= WM_DCS_VOL_MASTER && word <= WM_DCS_VOL_CH4) {
        /* 55AA is master. 55AB..55AE are source logical channels 1..4. */
        const int16_t delta = (int16_t)(word - WM_DCS_VOL_MASTER);
        p->pending_volume_channel = (int8_t)(delta - 1);
        a = make_action(WM_DCS_HOST_VOLUME_SELECTOR, word);
        a.command = 0u;
        a.source_channel = p->pending_volume_channel;
        return a;
    }

    if (word == 0u) {
        p->pending_volume_channel = -2;
        a = make_action(WM_DCS_HOST_STOP_ALL, word);
        a.command = 0u;
        return a;
    }

    if (word >= WM_DCS_STOP_CH1 && word <= WM_DCS_STOP_CH4) {
        a = make_action(WM_DCS_HOST_STOP_CHANNEL, word);
        a.command = 0u;
        a.source_channel = (int8_t)(word - WM_DCS_STOP_CH1);
        return a;
    }

    return make_action(WM_DCS_HOST_PLAY, word);
}
