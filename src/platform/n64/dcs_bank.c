#include <libdragon.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "dcs_bank.h"
typedef struct{uint16_t command;const char*path;uint8_t fallback_channel;}entry;
static const entry bank[]={
{176,"rom:/dcs/cmd_0176.wav64",1},{208,"rom:/dcs/cmd_0208.wav64",2},{244,"rom:/dcs/cmd_0244.wav64",2},{248,"rom:/dcs/cmd_0248.wav64",2},{460,"rom:/dcs/cmd_0460.wav64",2},{1376,"rom:/dcs/cmd_1376.wav64",1},{1456,"rom:/dcs/cmd_1456.wav64",2},{1460,"rom:/dcs/cmd_1460.wav64",2},{1512,"rom:/dcs/cmd_1512.wav64",1},{1556,"rom:/dcs/cmd_1556.wav64",1},{1560,"rom:/dcs/cmd_1560.wav64",1},{1564,"rom:/dcs/cmd_1564.wav64",1},{1568,"rom:/dcs/cmd_1568.wav64",1},
{2560,"rom:/dcs/cmd_2560.wav64",3},{2564,"rom:/dcs/cmd_2564.wav64",3},{2568,"rom:/dcs/cmd_2568.wav64",3},{2572,"rom:/dcs/cmd_2572.wav64",3},{2576,"rom:/dcs/cmd_2576.wav64",3},{3640,"rom:/dcs/cmd_3640.wav64",3},{3644,"rom:/dcs/cmd_3644.wav64",3},{3648,"rom:/dcs/cmd_3648.wav64",3},{3652,"rom:/dcs/cmd_3652.wav64",3},{3656,"rom:/dcs/cmd_3656.wav64",3},{3660,"rom:/dcs/cmd_3660.wav64",3},{3664,"rom:/dcs/cmd_3664.wav64",3},{3668,"rom:/dcs/cmd_3668.wav64",3}};
#define FIRST 1
#define LAST 4
static wav64_t wave[LAST+1];static bool opened[LAST+1];
static const entry*find(uint16_t c){size_t i;for(i=0;i<sizeof(bank)/sizeof(bank[0]);++i)if(bank[i].command==c)return &bank[i];return 0;}
static void closech(int ch){if(ch<FIRST||ch>LAST)return;mixer_ch_stop(ch);if(opened[ch]){wav64_close(&wave[ch]);opened[ch]=false;}}
static bool start(const entry*e,int ch,uint16_t raw,uint32_t tick){if(!e||ch<FIRST||ch>LAST)return false;closech(ch);wav64_open(&wave[ch],e->path);wav64_set_loop(&wave[ch],false);opened[ch]=true;wav64_play(&wave[ch],ch);mixer_ch_set_vol(ch,1.0f,1.0f);debugf("audio: DCS %u asset %u ch%d @%lu\\n",(unsigned)raw,(unsigned)e->command,ch,(unsigned long)tick);return true;}
void wm_dcs_bank_init(void){int ch;for(ch=FIRST;ch<=LAST;++ch)opened[ch]=false;}
bool wm_dcs_bank_play_source(uint16_t c,uint32_t tick,int8_t sc){const entry*e=find(c);if(!e)e=find((uint16_t)(c&~3u));if(!e)return false;return start(e,(int)sc+1,c,tick);}
bool wm_dcs_bank_play(uint16_t c,uint32_t tick){const entry*e=find(c);return e?start(e,e->fallback_channel,c,tick):false;}
void wm_dcs_bank_stop_source(int8_t sc){if(sc>=0&&sc<4)closech((int)sc+1);}
void wm_dcs_bank_set_source_volume(int8_t sc,uint8_t v){if(sc>=0&&sc<4){float f=(float)v/255.0f;mixer_ch_set_vol((int)sc+1,f,f);}}
void wm_dcs_bank_set_master_volume(uint8_t v){mixer_set_vol((float)v/255.0f);}
void wm_dcs_bank_service(void){}
void wm_dcs_bank_stop(void){int ch;for(ch=FIRST;ch<=LAST;++ch)closech(ch);}
