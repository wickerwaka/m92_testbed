#include <stdint.h>
#include "printf/printf.h"
#include "interrupts.h"

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

_Static_assert(sizeof(TC0100SCN_Layout) == 0x10000, "TC0100SCNLayout mismatch");

uint16_t *TC011PCR_ADDR = (uint16_t *)0x200000;
uint16_t *TC011PCR_DATA = (uint16_t *)0x200002;

TC0100SCN_Layout *TC0100SCN = (TC0100SCN_Layout *)0x800000;

void set_colors(uint16_t offset, uint16_t count, uint16_t *colors)
{
    for( uint16_t i = 0; i < count; i++ )
    {
        *TC011PCR_ADDR = (offset + count) * 2;
        *TC011PCR_DATA = colors[i];
    }
}

void draw_bg_text(TC0100SCN_BG *bg, uint16_t x, uint16_t y, const char *str)
{
    int ofs = ( y * 64 ) + x;

    while(*str)
    {
        bg[ofs].attr = 0;
        bg[ofs].color = 0;
        bg[ofs].code = *str;
        str++;
        ofs++;
    }
}

uint16_t palette0[8] = { 0x0000, 0xffff, 0xf000, 0x0f00, 0x00f0, 0x000f, 0x4444, 0x8888 };
uint16_t palette1[8] = { 0xffff, 0xf000, 0x0f00, 0x00f0, 0x000f, 0x4444, 0x8888, 0x0000 };

volatile uint32_t irq_count[7] = { 0, };

void level1_handler() { irq_count[0]++; }
void level2_handler() { irq_count[1]++; }
void level3_handler() { irq_count[2]++; }
void level4_handler() { irq_count[3]++; }
void level5_handler() { irq_count[4]++; }
void level6_handler() { irq_count[5]++; }
void level7_handler() { irq_count[6]++; }

int main(int argc, char *argv[])
{
    /*for( uint16_t ofs = 0; ofs < 0x1000; ofs++ )
    {
        set_colors(ofs, 1, &ofs);
    }*/

    set_colors(0, 8, palette0);
    set_colors(1, 8, palette1);

    asm( "andi #0xf0ff, %sr" );

    while(1)
    {
        char tmp[32];
        sprintf(tmp, "%08X %08X %08X %08X", irq_count[0], irq_count[1], irq_count[2], irq_count[3]);
        draw_bg_text(TC0100SCN->bg0, 3, 10, tmp);
        sprintf(tmp, "%08X %08X %08X", irq_count[4], irq_count[5], irq_count[6]);
        draw_bg_text(TC0100SCN->bg0, 3, 12, tmp);
    }

    return 0;
}

