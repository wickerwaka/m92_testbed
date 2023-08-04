#include "interrupts.h"

__attribute__((interrupt,weak)) void __far p0_handler()
{
	__fint();
	return;
}

__attribute__((interrupt,weak)) void __far p1_handler()
{
	__fint();
	return;
}

__attribute__((interrupt,weak)) void __far p2_handler()
{
	__fint();
	return;
}

__attribute__((interrupt,weak)) void __far generic_handler()
{
	return;
}
