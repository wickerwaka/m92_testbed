#if !defined(OBJ_H)
#define OBJ_H 1

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if !defined(__FAR)
#define __far
#endif


typedef struct
{
	uint16_t y : 9;
	uint16_t color : 4;
	uint16_t height : 2;
	uint16_t fy : 1;

	uint16_t sprite : 16;

	uint16_t x : 9;
	uint16_t fx : 1;
	uint16_t unk1 : 6;
} OBJ;

_Static_assert(sizeof(OBJ) == 6, "OBJ size mismatch");

static __far OBJ *OBJRAM = (__far OBJ *)0xd000ee00;

#endif // OBJ_H