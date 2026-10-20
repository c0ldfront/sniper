#ifndef SNIPER_NUMERIC_ENVIRONMENT_H
#define SNIPER_NUMERIC_ENVIRONMENT_H
/* Original CRT startup41a5c0 selects53-bit x87 precision. Preserve rounding
 * and exception masks; call on the gameplay thread before any simulation. */
static inline void sniper_numeric_initialize(void) {
    unsigned short control;
    __asm__ volatile("fnstcw %0":"=m"(control));
    control=(unsigned short)((control&~0x0300u)|0x0200u);
    __asm__ volatile("fldcw %0"::"m"(control));
}
#endif
