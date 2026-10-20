/* Controlled host fixture environment, then the actual gameplay initializer. */
#include "numeric_environment.h"
unsigned short sniper_test_flight_numeric_initialize(void) {
    unsigned short control=0x037f;
    __asm__ volatile("fldcw %0"::"m"(control));
    sniper_numeric_initialize();
    __asm__ volatile("fnstcw %0":"=m"(control));
    return control;
}
