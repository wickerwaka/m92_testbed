#include <stdint.h>
#include <stdbool.h>
#include "printf/printf.h"

#include "util.h"
#include "interrupts.h"
#include "comms.h"

#include "tc0100scn.h"
#include "tc0220ioc.h"


volatile uint16_t *TC011PCR_ADDR = (volatile uint16_t *)0x200000;
volatile uint16_t *TC011PCR_DATA = (volatile uint16_t *)0x200002;
volatile uint16_t *TC011PCR_WHAT = (volatile uint16_t *)0x200004;

TC0100SCN_Layout *TC0100SCN = (TC0100SCN_Layout *)0x800000;
TC0100SCN_Control *TC0100SCN_Ctrl = (TC0100SCN_Control *)0x820000;

TC0220IOC_Control *TC0220IOC = (TC0220IOC_Control *)0x300000;

uint16_t *TC0200OBJ = (uint16_t *)0x900000;

void set_color(int index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= 0x1000) return;

    uint16_t color = ((r >> 3) << 0) | ((g >> 3) << 5) | ((b >> 3) << 10);
    
    *TC011PCR_ADDR = index * 2;
    *TC011PCR_DATA = color;
}

void set_16color_palette(int index, uint8_t r, uint8_t g, uint8_t b)
{
    int color_index = index * 16;
    set_color(color_index, 0, 0, 0);
    for( int i = 1; i < 16; i++ )
    {
        set_color(color_index + i, r, g, b);
    }
}

void set_colors(uint16_t offset, uint16_t count, uint16_t *colors)
{
    for( uint16_t i = 0; i < count; i++ )
    {
        *TC011PCR_ADDR = (offset + i) * 2;
        *TC011PCR_DATA = colors[i];
    }
}



void draw_bg_text(TC0100SCN_BG *bg, int color, uint16_t x, uint16_t y, const char *str)
{
    int ofs = ( y * 64 ) + x;

    while(*str)
    {
        if( *str == '\n' )
        {
            y++;
            ofs = (y * 64) + x;
        }
        else
        {
            bg[ofs].attr = 0;
            bg[ofs].color = color & 0xff;
            bg[ofs].code = *str;
            ofs++;
        }
        str++;
    }
}

uint16_t palette0[4] = { 0x0000, 0x1f << 10, 0x1f << 5, 0x1f << 0 };

volatile uint32_t vblank_count = 0;
volatile uint32_t dma_count = 0;

void level5_handler()
{
    vblank_count++;
    TC0220IOC->watchdog = 0;
}

void level6_handler()
{
    dma_count++;
}

char msg[33];
int msg_idx = 0;

int main(int argc, char *argv[])
{
    uint16_t edge_count = 0;

    memset(msg, 0, 33);

    for( int x = 0; x < 0x8000; x++ )
    {
        TC0200OBJ[x] = 0;
    }

    memset(TC0100SCN, 0, sizeof(TC0100SCN_Layout));

    TC0100SCN_Ctrl->bg1_y = 0;
    TC0100SCN_Ctrl->bg1_x = 0;
    TC0100SCN_Ctrl->fg0_x = 0;
    TC0100SCN_Ctrl->fg0_y = 0;
    TC0100SCN_Ctrl->system_flags = 0;
    TC0100SCN_Ctrl->layer_flags = TC0100SCN_LAYER_BG1_DISABLE | TC0100SCN_LAYER_FG0_DISABLE;
    TC0100SCN_Ctrl->bg0_y = 8;
    TC0100SCN_Ctrl->bg0_x = 16;

    for( uint16_t x = 0; x < 0x8000; x++ )
    {
        TC0220IOC->watchdog = 0;
        set_color(0, x & 0xff, (x>>2) & 0xff, (x>>4) & 0xff);
        *TC011PCR_WHAT = 0;
    }

    set_16color_palette(0, 255, 255, 255);
    set_16color_palette(1, 255, 0, 0);
    set_16color_palette(2, 0, 255, 0);
    set_16color_palette(3, 0, 0, 255);
    set_16color_palette(4, 128, 128, 128);
    *TC011PCR_WHAT = 0;

    enable_interrupts();

    uint8_t write_idx = 0;
    uint32_t comms_count = 0;
    
    while(1)
    {
        comms_update();

        char tmp[64];
        int tmplen = sprintf(tmp, "VBL: %04X\nDMA: %04X\n", vblank_count, dma_count);
        draw_bg_text(TC0100SCN->bg0, 0, 2, 2, tmp);

        sprintf(tmp, "P1: %02X  P2: %02X", TC0220IOC->p1, TC0220IOC->p2);
        draw_bg_text(TC0100SCN->bg0, 1, 2, 5, tmp);

        int read_len = comms_read(msg, 32);
        if (read_len > 0)
        {
            msg[read_len] = 0;
            comms_write("OKAY", 4);
        }
        draw_bg_text(TC0100SCN->bg0, 0, 2, 10, msg);
    }

    return 0;
}

