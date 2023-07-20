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
char last_cmd[32];

void level5_handler()
{
    vblank_count++;
    TC0220IOC->watchdog = 0;
}

void level6_handler()
{
    dma_count++;
}

enum
{
    CMD_IDLE = 0,
    CMD_WRITE_BYTES = 1,
    CMD_WRITE_WORDS = 2,
    CMD_READ_BYTES = 3,
    CMD_READ_WORDS = 4,
    CMD_FILL_BYTES = 5,
    CMD_FILL_WORDS = 6,
};

typedef struct Cmd
{
    uint8_t cmd;
    uint32_t arg0;
    uint32_t arg1;

    uint16_t total_bytes;
    uint16_t total_bytes_read;
    uint16_t total_bytes_consumed;

    uint16_t bytes_avail;
    uint16_t bytes_consumed;

    bool is_new;

    uint8_t buffer[32] __attribute__((aligned(2)));
} Cmd;

void update_cmd(Cmd *cmd)
{
    uint8_t buf[10];

    if (cmd->bytes_consumed > 0)
    {
        memcpy(cmd->buffer, cmd->buffer + cmd->bytes_consumed, cmd->bytes_avail - cmd->bytes_consumed);
        cmd->bytes_avail -= cmd->bytes_consumed;
        cmd->total_bytes_consumed += cmd->bytes_consumed;
        cmd->bytes_consumed = 0;
    }

    if (cmd->total_bytes_consumed == cmd->total_bytes && cmd->cmd != CMD_IDLE)
    {
        comms_write(&cmd->cmd, 1);
        cmd->cmd = CMD_IDLE;

    }

    if (cmd->cmd == CMD_IDLE)
    {
        if( comms_read(&cmd->cmd, 1) == 0 )
        {
            return;
        }

        if (cmd->cmd == CMD_IDLE)
        {
            return;
        }

        int pos = 0;
        while (pos < 10)
        {
            pos += comms_read(buf + pos, 10 - pos);
        }

        cmd->arg0 = *(uint32_t *)(buf + 0);
        cmd->arg1 = *(uint32_t *)(buf + 4);
        cmd->total_bytes = *(uint16_t *)(buf + 8);

        cmd->bytes_avail = 0;
        cmd->bytes_consumed = 0;
        cmd->total_bytes_read = 0;
        cmd->total_bytes_consumed = 0;
        cmd->is_new = true;
    }
    else
    {
        cmd->is_new = false;
    }

    if (cmd->total_bytes_read < cmd->total_bytes && cmd->bytes_avail < sizeof(cmd->buffer))
    {
        uint16_t remaining = cmd->total_bytes - cmd->total_bytes_read;
        uint16_t space = sizeof(cmd->buffer) - cmd->bytes_avail;

        uint16_t max_read = space < remaining ? space : remaining;
        uint16_t bytes_read = comms_read(cmd->buffer + cmd->bytes_avail, max_read);
        cmd->total_bytes_read += bytes_read;
        cmd->bytes_avail += bytes_read;
    }
}

void process_cmd(Cmd *cmd)
{
    switch(cmd->cmd)
    {
        case CMD_WRITE_BYTES:
        {
            if (cmd->is_new) snprintf(last_cmd, sizeof(last_cmd), "WRITE %X BYTES @ %08X", cmd->total_bytes, cmd->arg0);
            uint8_t *addr = (uint8_t *)(cmd->arg0 + cmd->total_bytes_consumed);
            if( cmd->bytes_avail > 0 )
            {
                memcpyb(addr, cmd->buffer, cmd->bytes_avail);
                cmd->bytes_consumed = cmd->bytes_avail;
            }
            break;
        }
        case CMD_WRITE_WORDS:
        {
            if (cmd->is_new) snprintf(last_cmd, sizeof(last_cmd), "WRITE %X WORDS @ %08X", cmd->total_bytes >> 1, cmd->arg0);
            uint16_t *addr = (uint16_t *)(cmd->arg0 + cmd->total_bytes_consumed);
            memcpyw(addr, cmd->buffer, cmd->bytes_avail >> 1);
            cmd->bytes_consumed = (cmd->bytes_avail & ~0x1);
            break;
        }
        case CMD_READ_BYTES:
        {
            if (cmd->is_new) snprintf(last_cmd, sizeof(last_cmd), "READ %X BYTES @ %08X", cmd->arg1, cmd->arg0);
            uint8_t *addr = (uint8_t *)cmd->arg0;
            comms_write(addr, cmd->arg1);
            break;
        }
        case CMD_READ_WORDS:
        {
            if (cmd->is_new) snprintf(last_cmd, sizeof(last_cmd), "READ %X WORDS @ %08X", cmd->arg1, cmd->arg0);
            uint8_t *addr = (uint8_t *)cmd->arg0;
            for( int ofs = 0; ofs < cmd->arg1; ofs++)
            {
                comms_write(addr + (ofs << 1), 2);
            }
            break;
        }
        case CMD_FILL_BYTES:
        {
            if (cmd->is_new) snprintf(last_cmd, sizeof(last_cmd), "FILL %X BYTES @ %08X", cmd->arg1, cmd->arg0);
            if (cmd->bytes_avail > 0)
            {
                int v = *(uint8_t *)cmd->buffer;
                cmd->bytes_consumed = cmd->bytes_avail; 
                memsetb((void *)cmd->arg0, v, cmd->arg1);
            }
            break;
        }
        case CMD_FILL_WORDS:
        {
            if (cmd->is_new) snprintf(last_cmd, sizeof(last_cmd), "FILL %X WORDS @ %08X", cmd->arg1, cmd->arg0);
            if (cmd->bytes_avail > 1)
            {
                uint16_t v = *(uint16_t *)cmd->buffer;
                cmd->bytes_consumed = cmd->bytes_avail; 
                memsetw((void *)cmd->arg0, v, cmd->arg1);
            }
            break;
        }

        case CMD_IDLE: break;

        default:
            cmd->bytes_consumed = cmd->bytes_avail;
            break;
    }
}

Cmd active_cmd;

int main(int argc, char *argv[])
{
    uint16_t edge_count = 0;

    memset(&active_cmd, 0, sizeof(active_cmd));
    last_cmd[0] = 0;

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
        if (comms_update() )
        {
            update_cmd(&active_cmd);
            process_cmd(&active_cmd);
        }

        char tmp[64];
        int tmplen = sprintf(tmp, "VBL: %05X  DMA: %05X    ", vblank_count, dma_count);
        draw_bg_text(TC0100SCN->bg0, 0, 2, 2, tmp);

        sprintf(tmp, "CMD: %s", last_cmd);
        draw_bg_text(TC0100SCN->bg0, 1, 2, 3, tmp);

        sprintf(tmp, "%X %04X %04X %04X", active_cmd.cmd, active_cmd.arg0, active_cmd.arg1, active_cmd.total_bytes);
        draw_bg_text(TC0100SCN->bg0, 1, 2, 5, tmp);

        comms_status(tmp, 64);
        draw_bg_text(TC0100SCN->bg0, 2, 2, 7, tmp);
    }

    return 0;
}

