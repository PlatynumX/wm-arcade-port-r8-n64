#include <libdragon.h>
#include <stdbool.h>
#include <stdint.h>
#include "audio_backend.h"
#include "dcs_bank.h"
#include "wm/dcs_host_protocol.h"
#define WM_DCS_OUTPUT_HZ 31250
#define WM_N64_AUDIO_BUFFERS 4
#define WM_N64_AUDIO_CHANNELS 8
#define WM_DCS_CMD1005_CHANNEL 0
#define WM_DCS_CMD1005_PATH "rom:/dcs/wwf_dcs_cmd1005_31250hz_ref.wav64"
static bool ready;static wav64_t logo;static wm_dcs_host_protocol proto;
static void stopall(void){int ch;wm_dcs_bank_stop();for(ch=0;ch<WM_N64_AUDIO_CHANNELS;++ch)mixer_ch_stop(ch);}
static void play_logo(uint32_t t){mixer_ch_stop(0);wav64_play(&logo,0);mixer_ch_set_vol(0,1.0f,1.0f);debugf("audio: DCS logo 1005 @%lu\\n",(unsigned long)t);}
static void dispatch(const wm_audio_event*e){wm_dcs_host_action a;if(!e)return;a=wm_dcs_host_protocol_feed(&proto,e->command);switch(a.kind){
case WM_DCS_HOST_STOP_ALL:stopall();break;case WM_DCS_HOST_STOP_CHANNEL:wm_dcs_bank_stop_source(a.source_channel);break;
case WM_DCS_HOST_VOLUME:if(a.source_channel<0)wm_dcs_bank_set_master_volume(a.volume);else wm_dcs_bank_set_source_volume(a.source_channel,a.volume);break;
case WM_DCS_HOST_VOLUME_SELECTOR:break;case WM_DCS_HOST_MALFORMED_VOLUME:debugf("audio: malformed DCS volume word %u\\n",(unsigned)e->command);break;
case WM_DCS_HOST_PLAY:if(e->command==1005u)play_logo(e->source_tick);else if(e->source_channel>=0&&e->source_channel<4){if(!wm_dcs_bank_play_source(e->command,e->source_tick,e->source_channel))debugf("audio: missing DCS PCM %u routed ch%d\\n",(unsigned)e->command,(int)e->source_channel+1);}else if(!wm_dcs_bank_play(e->command,e->source_tick))debugf("audio: untranslated raw DCS %u\\n",(unsigned)e->command);break;}}
void wm_n64_audio_init(void){if(ready)return;audio_init(WM_DCS_OUTPUT_HZ,WM_N64_AUDIO_BUFFERS);mixer_init(WM_N64_AUDIO_CHANNELS);mixer_set_vol(1.0f);wm_dcs_bank_init();wm_dcs_host_protocol_init(&proto);wav64_open(&logo,WM_DCS_CMD1005_PATH);wav64_set_loop(&logo,false);ready=true;}
void wm_n64_audio_service(wm_app*app){wm_audio_event e;if(!ready||!app)return;while(wm_audio_pop_event(&app->audio,&e))dispatch(&e);wm_dcs_bank_service();while(audio_can_write()){int16_t*out=audio_write_begin();mixer_poll(out,audio_get_buffer_length());audio_write_end();}}
