#include "wm/dcs_host_protocol.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    wm_dcs_host_protocol p;
    wm_dcs_host_protocol_init(&p);

    wm_dcs_host_action a = wm_dcs_host_protocol_feed(&p, 1512u);
    assert(a.kind == WM_DCS_HOST_PLAY && a.command == 1512u);

    a = wm_dcs_host_protocol_feed(&p, 0u);
    assert(a.kind == WM_DCS_HOST_STOP_ALL);

    for (uint16_t w = 994u; w <= 997u; ++w) {
        a = wm_dcs_host_protocol_feed(&p, w);
        assert(a.kind == WM_DCS_HOST_STOP_CHANNEL);
        assert(a.source_channel == (int8_t)(w - 994u));
    }

    a = wm_dcs_host_protocol_feed(&p, 0x55AAu);
    assert(a.kind == WM_DCS_HOST_VOLUME_SELECTOR && a.source_channel == -1);
    a = wm_dcs_host_protocol_feed(&p, 0x80u << 8 | 0x7fu);
    assert(a.kind == WM_DCS_HOST_VOLUME && a.source_channel == -1 && a.volume == 0x80u);

    a = wm_dcs_host_protocol_feed(&p, 0x55AEu);
    assert(a.kind == WM_DCS_HOST_VOLUME_SELECTOR && a.source_channel == 3);
    a = wm_dcs_host_protocol_feed(&p, 0xff00u);
    assert(a.kind == WM_DCS_HOST_VOLUME && a.source_channel == 3 && a.volume == 0xffu);

    a = wm_dcs_host_protocol_feed(&p, 0x55ABu);
    assert(a.kind == WM_DCS_HOST_VOLUME_SELECTOR && a.source_channel == 0);
    a = wm_dcs_host_protocol_feed(&p, 0x1234u);
    assert(a.kind == WM_DCS_HOST_MALFORMED_VOLUME && a.source_channel == 0);

    puts("dcs_host_protocol: PASS");
    return 0;
}
