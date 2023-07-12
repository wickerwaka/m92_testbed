#if !defined(TC0100SCN_H)
#define TC0100SCN_H 1

#include <stdint.h>

typedef struct TC0100SCN_BG
{
    uint8_t attr;
    uint8_t color;
    uint16_t code;
} TC0100SCN_BG;

typedef struct TC0100SCN_Layout
{
    TC0100SCN_BG bg0[0x1000]; // 0x0000
    uint16_t fg0[0x1000]; // 0x4000
    uint16_t fg0_gfx[0x800]; // 0x6000
    uint16_t unknown0[0x800]; // 0x7000
    TC0100SCN_BG bg1[0x1000]; // 0x8000
    uint16_t bg0_rowscroll[0x200]; // 0xc000
    uint16_t bg1_rowscroll[0x200]; // 0xc400
    uint16_t unknown1[0x400]; // 0xc800
    uint16_t unknown2[0x800]; // 0xd000
    uint16_t bg1_colscroll[0x80]; // 0xe000
    uint16_t unknown3[0xf80]; // 0xe100
} TC0100SCN_Layout;

#define TC0100SCN_LAYER_BG0_DISABLE ((uint16_t)(1 << 0))
#define TC0100SCN_LAYER_BG1_DISABLE ((uint16_t)(1 << 1))
#define TC0100SCN_LAYER_FG0_DISABLE ((uint16_t)(1 << 2))
#define TC0100SCN_LAYER_BG0_PRIO    ((uint16_t)(1 << 3))
#define TC0100SCN_LAYER_WIDE        ((uint16_t)(1 << 4))

#define TC0100SCN_SYSTEM_FLIP       ((uint16_t)(1 << 0))
#define TC0100SCN_SYSTEM_SECONDARY  ((uint16_t)(1 << 1))

typedef struct TC0100SCN_Control
{
    uint16_t bg0_x;
    uint16_t bg1_x;
    uint16_t fg0_x;

    uint16_t bg0_y;
    uint16_t bg1_y;
    uint16_t fg0_y;

    uint16_t layer_flags;
    uint16_t system_flags;
} TC0100SCN_Control;

_Static_assert(sizeof(TC0100SCN_Control) == 0x10, "TC0100SCN_Control mismatch");
_Static_assert(sizeof(TC0100SCN_Layout) == 0x10000, "TC0100SCN_Layout mismatch");

#endif