#include "../platform/startup_random.h"
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
int main(void) {
    const uint32_t seeds[]={0,1,123456789,0x7fffffff,0xffffffff};
    const int32_t saved[]={0,3,400,-1,INT32_MIN,INT32_MAX};
    unsigned count=0;
    for(unsigned i=0;i<sizeof(seeds)/sizeof(*seeds);i++) {
        Sniper87StartupRandom expected,got;
        sniper87_startup_random_seed(&expected,seeds[i]);
        assert(WritePrivateProfileStringA("Dialog","PlayerShipFrequency",NULL,"sniper.ini"));
        sniper87_startup_random_load(&got,seeds[i]);
        assert(got.crt_state==expected.crt_state&&got.frequency==expected.frequency);count++;
        for(unsigned j=0;j<sizeof(saved)/sizeof(*saved);j++) {
            sniper87_startup_frequency_save(saved[j]);
            sniper87_startup_random_load(&got,seeds[i]);
            assert(got.crt_state==expected.crt_state&&got.frequency==saved[j]);count++;
        }
    }
    printf("PASS %u Windows profile cases: absent random default, saved signed frequency, exact one-draw RNG state\n",count);
    return 0;
}
