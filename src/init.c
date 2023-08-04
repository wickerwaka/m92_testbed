#include <stdint.h>
#include <stddef.h>

#include "util.h"

#include "interrupts.h"

__attribute__ ((section(".vectors"))) __attribute__((used))
static const __far void * exception_vectors[256] =
{
    // 0
	generic_handler,
	generic_handler,
	generic_handler,
	generic_handler,

	generic_handler,
	generic_handler,
	generic_handler,
	generic_handler,

    // 8
	generic_handler,
	generic_handler,
	generic_handler,
	generic_handler,

	generic_handler,
	generic_handler,
	generic_handler,
	generic_handler,

    // 16
	generic_handler,
	generic_handler,
	generic_handler,
	generic_handler,

	generic_handler,
	generic_handler,
	generic_handler,
	generic_handler,

    // 24
    p0_handler,
	p1_handler,
	p2_handler,
	generic_handler,

	generic_handler,
	generic_handler,
	generic_handler,
	generic_handler,
};

void putchar_(char c) {}
