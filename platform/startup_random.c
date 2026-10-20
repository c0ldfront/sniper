#include "startup_random.h"
void sniper87_startup_random_seed(Sniper87StartupRandom *state,uint32_t seconds) {
    state->crt_state=seconds*214013u+2531011u;
    state->frequency=(int32_t)((state->crt_state>>16)&32767u)%4;
}
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
void sniper87_startup_random_load(Sniper87StartupRandom *state,uint32_t seconds) {
    sniper87_startup_random_seed(state,seconds);
    state->frequency=(int32_t)GetPrivateProfileIntA("Dialog","PlayerShipFrequency",state->frequency,"sniper.ini");
}
void sniper87_startup_frequency_save(int32_t frequency) {
    char text[32];snprintf(text,sizeof text,"%ld",(long)frequency);
    WritePrivateProfileStringA("Dialog","PlayerShipFrequency",text,"sniper.ini");
}
#endif
