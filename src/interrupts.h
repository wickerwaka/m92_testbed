#if !defined( INTERRUPTS_H )
#define INTERRUPTS_H 1

#include "util.h"

__attribute__((interrupt)) void __far p0_handler();
__attribute__((interrupt)) void __far p1_handler();
__attribute__((interrupt)) void __far p2_handler();
__attribute__((interrupt)) void __far generic_handler();

static inline void enable_interrupts() { asm( "sti" ); }
static inline void disable_interrupts() { asm( "cli" ); }
static inline void __fint() { asm( ".byte 0x0f, 0x92" ); }

#endif