#include "wm/audio.h"
#include "wm/dcs_port_bindings.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    assert(wm_dcs_decoded_stream_count() == 647u);
    assert(wm_dcs_command_program_rows() >= 592u);
    assert(wm_dcs_command_stream_rows() >= 704u);
    assert(wm_dcs_decoded_command_stream_bindings() >= 704u);
    assert(wm_dcs_port_binding_count() >= 704u);

    const wm_dcs_port_binding *command_11 = wm_dcs_find_first_binding_for_command(11u);
    assert(command_11 != 0);
    assert(command_11->dragonfs_path != 0);
    assert(strstr(command_11->dragonfs_path, "rom:/dcs/stream_") == command_11->dragonfs_path);
    assert(strstr(command_11->dragonfs_path, ".wav64") != 0);

    assert(wm_audio_command_has_decoded_asset(11u));
    assert(wm_audio_command_decoded_path(11u) != 0);
    assert(wm_audio_command_decoded_stream_address(11u) != 0u);
    assert(!wm_audio_command_has_decoded_asset(0u));

    wm_audio_state audio;
    wm_audio_init(&audio);

    assert(wm_audio_send_command(&audio, 0u));
    assert(audio.stop_events == 1u);
    assert(!audio.last_command_has_decoded_binding);
    assert(audio.last_decoded_path == 0);

    assert(wm_audio_send_routed_command(&audio, 11u, 0));
    assert(audio.bound_events == 1u);
    assert(audio.last_command == 11u);
    assert(audio.last_command_has_decoded_binding);
    assert(audio.last_stream_address == command_11->stream_address);
    assert(audio.last_decoded_path != 0);
    assert(strcmp(audio.last_decoded_path, command_11->dragonfs_path) == 0);

    wm_audio_event ev;
    assert(wm_audio_pop_event(&audio, &ev));
    assert(ev.command == 0u);
    assert(wm_audio_pop_event(&audio, &ev));
    assert(ev.command == 11u);
    assert(ev.source_channel == 0);

    printf("DCS R2B audio binding regression: PASS\n");
    return 0;
}
