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

__far uint16_t *palette_ram = (__far uint16_t *)0xf0008800;
__far uint16_t *reg_videocontrol = (__far uint16_t *)0xf0009800;
__far uint16_t *reg_spritecontrol = (__far uint16_t *)0xf0009000;

int x = 0;

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

int main()
{
    __outb(0x40, 0x13);
    __outb(0x42, 0x08);
    __outb(0x42, 0x0f);
    __outb(0x42, 0xf2);

    memset(&active_cmd, 0, sizeof(active_cmd));
    last_cmd[0] = 0;

    *reg_videocontrol = 0x2000;
    palette_ram[0] = 0xffff;
    reg_spritecontrol[4] = 0;

    enable_interrupts();
    
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
        palette_ram[0] = color;
        color++;
        wait_vblank();
        reg_spritecontrol[4] = 0;
    }
    x++;

    return 0;
}

