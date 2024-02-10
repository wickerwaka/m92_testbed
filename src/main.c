#include <stdint.h>
#include <stdbool.h>
#include "printf/printf.h"

#include "util.h"
#include "interrupts.h"
#include "comms.h"

char last_cmd[32];

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
            //if (cmd->is_new) snprintf(last_cmd, sizeof(last_cmd), "WRITE %X BYTES @ %08X", cmd->total_bytes, cmd->arg0);
            uint8_t __far *addr = (__far uint8_t *)(cmd->arg0 + cmd->total_bytes_consumed);
            if( cmd->bytes_avail > 0 )
            {
                memcpyb(addr, cmd->buffer, cmd->bytes_avail);
                cmd->bytes_consumed = cmd->bytes_avail;
            }
            break;
        }
        case CMD_WRITE_WORDS:
        {
            //if (cmd->is_new) snprintf(last_cmd, sizeof(last_cmd), "WRITE %X WORDS @ %08X", cmd->total_bytes >> 1, cmd->arg0);
            uint16_t __far *addr = (__far uint16_t *)(cmd->arg0 + cmd->total_bytes_consumed);
            memcpyw(addr, cmd->buffer, cmd->bytes_avail >> 1);
            cmd->bytes_consumed = (cmd->bytes_avail & ~0x1);
            break;
        }
        case CMD_READ_BYTES:
        {
            //if (cmd->is_new) snprintf(last_cmd, sizeof(last_cmd), "READ %X BYTES @ %08X", cmd->arg1, cmd->arg0);
            uint8_t __far *addr = (__far uint8_t *)cmd->arg0;
            comms_write(addr, cmd->arg1);
            break;
        }
        case CMD_READ_WORDS:
        {
            //if (cmd->is_new) snprintf(last_cmd, sizeof(last_cmd), "READ %X WORDS @ %08X", cmd->arg1, cmd->arg0);
            uint8_t __far *addr = (__far uint8_t *)cmd->arg0;
            for( int ofs = 0; ofs < cmd->arg1; ofs++)
            {
                comms_write(addr + (ofs << 1), 2);
            }
            break;
        }
        case CMD_FILL_BYTES:
        {
            //if (cmd->is_new) snprintf(last_cmd, sizeof(last_cmd), "FILL %X BYTES @ %08X", cmd->arg1, cmd->arg0);
            if (cmd->bytes_avail > 0)
            {
                int v = *(uint8_t *)cmd->buffer;
                cmd->bytes_consumed = cmd->bytes_avail; 
                memsetb((__far void *)cmd->arg0, v, cmd->arg1);
            }
            break;
        }
        case CMD_FILL_WORDS:
        {
            //if (cmd->is_new) snprintf(last_cmd, sizeof(last_cmd), "FILL %X WORDS @ %08X", cmd->arg1, cmd->arg0);
            if (cmd->bytes_avail > 1)
            {
                uint16_t v = *(uint16_t *)cmd->buffer;
                cmd->bytes_consumed = cmd->bytes_avail; 
                memsetw((__far void *)cmd->arg0, v, cmd->arg1);
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

void pf_enable(uint8_t idx, bool enabled)
{
    uint16_t port = 0x90 + (idx << 1);
    if (enabled)
        __outw(port, 0x00);
    else
        __outw(port, 0x80);
}

void pf_set_xy(uint8_t idx, uint16_t x, uint16_t y)
{
    uint16_t x_port = 0x82 + (idx << 2);
    uint16_t y_port = 0x80 + (idx << 2);

    __outw(x_port, x);
    __outw(y_port, y);
}


__far uint16_t *palette_ram = (__far uint16_t *)0xf0009000;
__far uint16_t *vram = (__far uint16_t *)0xd0000000;

uint16_t base_palette[] = {
    0x0000, 0x7FFF, 0x67FF, 0x53FF, 0x07FF, 0x035F, 0x029F, 0x025F,
    0x7FFD, 0x7F93, 0x7ECD, 0x7E28, 0x79A3, 0x6940, 0x2108, 0x0C63,

    0x002A, 0x56B5, 0x5293, 0x4E72, 0x4A51, 0x4610, 0x45CF, 0x45AE,
    0x55D1, 0x518F, 0x4D4D, 0x490B, 0x44C9, 0x4087, 0x3C45, 0x454C,

    0x0000, 0x7FFF, 0x3FFF, 0x03FF, 0x02FF, 0x01FF, 0x015F, 0x001F,
    0x03F4, 0x0327, 0x02A4, 0x7FE0, 0x5AC0, 0x35A0, 0x2108, 0x0C63,

    0x0000, 0x7FFF, 0x7FF6, 0x7FF2, 0x7FCD, 0x736A, 0x6707, 0x5EC7,
    0x5687, 0x4E45, 0x45E4, 0x3DA2, 0x3541, 0x28E0, 0x574B, 0x1000,

    0x0000, 0x739C, 0x027F, 0x01DF, 0x011F, 0x0037, 0x040C, 0x5AD6,
    0x4610, 0x316B, 0x18C6, 0x0F78, 0x128B, 0x05A4, 0x0120, 0x0401,

    0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF,
    0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF,

    0x0000, 0x7BDE, 0x739B, 0x6B3A, 0x5F18, 0x5AD5, 0x5274, 0x4652,
    0x420F, 0x39AE, 0x2D8C, 0x2949, 0x20E8, 0x0215, 0x010C, 0x18C6,
};

volatile uint32_t vblank_count = 0;
__attribute__((interrupt)) void __far vblank_handler()
{
	vblank_count++;
    return;
}

void wait_vblank()
{
    uint32_t cnt = vblank_count;
    while( cnt == vblank_count ) {}
}

void draw_pf_text(int color, uint16_t x, uint16_t y, const char *str)
{
    int ofs = ( x * 64 ) + y;

    while(*str)
    {
        if( *str == '\n' )
        {
            x++;
            ofs = (x * 64) + y;
        }
        else
        {
            vram[(ofs << 1) + 1] = color;
            vram[(ofs << 1)] = *str;
            ofs += 64;
        }
        str++;
    }
}

char tmp[64];


int main()
{
    __outb(0x40, 0x13);
    __outb(0x42, 0x08);
    __outb(0x42, 0x0f);
    __outb(0x42, 0xf2);

    memset(&active_cmd, 0, sizeof(active_cmd));
    last_cmd[0] = 0;

    memcpyw(palette_ram, base_palette, sizeof(base_palette) >> 1);

    memsetw(vram, 0, 0x8000);

    __outw(0xb0, 0x0800);
    __outw(0x04, 0x0800);
    
    pf_enable(0, true);
    pf_enable(1, false);
    pf_enable(2, false);
    pf_enable(3, false);

    pf_set_xy(0, -80, -136);

    //enable_interrupts();

    
    draw_pf_text(6, 5, 5, "HELLO WORLD");
    
    snprintf(tmp, sizeof(tmp), "VBLANK: %06X", vblank_count);
    draw_pf_text(6, 2, 1, tmp);
    
    __outw(0xdead, 0xffff);

    while(1) {}
    
    uint8_t write_idx = 0;
    uint32_t comms_count = 0;
    uint16_t color = 0;
    
    while(1)
    {
        if (comms_update() )
        {
            update_cmd(&active_cmd);
            process_cmd(&active_cmd);
        }

        snprintf(tmp, sizeof(tmp), "VBLANK: %06X", vblank_count);
        draw_pf_text(6, 2, 1, tmp);

        comms_status(tmp, sizeof(tmp));
        draw_pf_text(6, 2, 2, tmp);

        wait_vblank();
    }

    return 0;
}

