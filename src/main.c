#include <stdint.h>
#include <stdbool.h>
#include "printf/printf.h"

#include "input.h"
#include "playfield.h"
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

__far uint16_t *palette_ram = (__far uint16_t *)0xf0009000;

uint16_t base_palette[] = {
    // Light Gray
    0x0000, 0x7FFF, 0x7FFF, 0x77BD, 0x6F7B, 0x5EF7, 0x56B5, 0x4E73,
    0x7FFD, 0x7F93, 0x7ECD, 0x7E28, 0x79A3, 0x6940, 0x2108, 0x0C63,

    // Red
    0x0000, 0x73FF, 0x4F3F, 0x329F, 0x29FF, 0x297F, 0x08DD, 0x1419,
    0x53FF, 0x3B5F, 0x269F, 0x0DDE, 0x00FE, 0x005A, 0x0010, 0x0007,

    // Orange
    0x0000, 0x7FFF, 0x3FFF, 0x03FF, 0x02FF, 0x01FF, 0x015F, 0x001F,
    0x03F4, 0x0327, 0x02A4, 0x7FE0, 0x5AC0, 0x35A0, 0x2108, 0x0C63,

    // Light Blue
    0x0000, 0x7FFF, 0x7FF6, 0x7FF2, 0x7FCD, 0x736A, 0x6707, 0x5EC7,
    0x5687, 0x4E45, 0x45E4, 0x3DA2, 0x3541, 0x28E0, 0x574B, 0x1000,

    // Green
    0x0000, 0x77FF, 0x53FD, 0x3BD7, 0x2F90, 0x1B09, 0x0280, 0x01E0,
    0x4E73, 0x3DEF, 0x2D6B, 0x227F, 0x7FFF, 0x7FFF, 0x2108, 0x0C63,

    // Dark Blue
    0x0000, 0x7FFF, 0x7F2C, 0x7E43, 0x7DA0, 0x7CE0, 0x6440, 0x4000,
    0x3000, 0x2F7F, 0x0E1F, 0x011F, 0x040E, 0x0407, 0x0035, 0x0000,

    // Light yellow
    0x01CA, 0x7FFF, 0x7BFF, 0x73FF, 0x5BFF, 0x37FF, 0x17FF, 0x039F,
    0x031F, 0x029F, 0x021F, 0x019F, 0x011F, 0x009F, 0x001F, 0x0424,
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


char tmp[64];

extern void vram_timing();

typedef enum
{
    COMMS = 0,
    PF_BASIC,
    PF_DBG,

    NUM_TEST_MODES
} TestMode;

TestMode current_mode = COMMS;

void init_comms_test()
{
    pf_reset();

    memcpyw(palette_ram, base_palette, sizeof(base_palette) >> 1);

    __outw(0xb0, 0x0800);
    __outw(0x04, 0x0800);

    __outw(0x98, 0x0000);


    pf_enable(0, true);
}

void update_comms_test()
{
    if (comms_update() )
    {
        update_cmd(&active_cmd);
        process_cmd(&active_cmd);
    }

    comms_status(tmp, sizeof(tmp));
    pf_text(0, 6, 2, 2, tmp);
}

void init_pf_test()
{
    pf_reset();

    memcpyw(palette_ram, base_palette, sizeof(base_palette) >> 1);

    memsetw(VRAM + (0xf000 >> 1), 0x08f0, 0x800);

    __outw(0xb0, 0x0800);
    __outw(0x04, 0x0800);

    __outw(0x98, 0x0000);

    pf_enable(0, true);
    pf_enable(1, true);
    pf_enable(2, true);
    pf_enable(3, true);

    for( int i = 0; i < 4; i++ )
    {
        pf_sym(i, i, i, i, 0x10);
        pf_sym(i, i, 27 - i, i, 0x11);
        pf_sym(i, i, 27 - i, 39 - i, 0x13);
        pf_sym(i, i, i, 39 - i, 0x12);
    }

    pf_text(0, 0, 10, 10, "LAYER 1");
    pf_text(1, 1, 10, 10, "LAYER 2");
    pf_text(2, 2, 10, 10, "LAYER 3");
    pf_text(3, 3, 10, 9, "LAYER 4");

    pf_text(1, 1, 8, 15, "NO  SCROLL");
    pf_text(3, 3, 8, 15, "ROW SCROLL");

    pf_text(1, 1, 8, 18, "NO  SELECT");
    pf_text(2, 2, 8, 17, "ROW SELECT");

    pf_set_flags(3, PF_ROWSCROLL);
    pf_set_flags(2, PF_ROWSELECT);

    __far uint16_t *sel = pf_rowselect_addr(2);
    __far uint16_t *scroll = pf_rowscroll_addr(3);

    uint16_t ofs = 0;
    for( int r = 8 * 8; r < 8 * 20; r++ )
    {
        scroll[r] = ofs >> 3;
        ofs--;
    } 

    ofs = 0;
    for( int r = 8 * 8; r < 12 * 8; r++ )
    {
        sel[r] = ofs;
        ofs--;
    } 
    for( int r = 12 * 8; r < 20 * 8; r++ )
    {
        sel[r] = ofs;
        ofs++;
    } 

}

void update_pf_test()
{
}

void init_pf_debug_test()
{
    pf_reset();

    memcpyw(palette_ram, base_palette, sizeof(base_palette) >> 1);

    __outw(0xb0, 0x0800);
    __outw(0x04, 0x0800);

    __outw(0x98, 0x0000);

    pf_enable(0, true);
    pf_enable(3, true);

    pf_set_xy(3, 0, 0);

    pf_set_flags(3, PF_DEBUG);
}

void update_pf_debug_test()
{
    static uint16_t accel = 0;
    if (input_down(LEFT | RIGHT | UP | DOWN))
    {
        accel = accel + 1;
        if (accel > 127) accel = 127;
    }
    else
    {
        accel = 0;
    }

    uint16_t x = pf_get_x(3);
    uint16_t y = pf_get_y(3);


    if (input_down(LEFT)) x = x + (accel >> 2);
    if (input_down(RIGHT)) x = x - (accel >> 2);
    if (input_down(UP)) y = y + (accel >> 2);
    if (input_down(DOWN)) y = y - (accel >> 2);

    snprintf(tmp, sizeof(tmp), "X: %04X   Y: %04X", x, y);
    pf_text(0, 3, 10, 10, tmp);

    pf_set_xy(3, x, y);
}

void init_mode()
{
    switch(current_mode)
    {
        case COMMS:
            init_comms_test();
            break;

        case PF_BASIC:
            init_pf_test();
            break;
        
        case PF_DBG:
            init_pf_debug_test();
            break;

        default:
            break;
    }
}

void update_mode()
{
    switch(current_mode)
    {
        case COMMS:
            update_comms_test();
            break;

        case PF_BASIC:
            update_pf_test();
            break;

        case PF_DBG:
            update_pf_debug_test();
            break;

        default:
            break;
    }
}

int main()
{
    __outb(0x40, 0x13);
    __outb(0x42, 0x08);
    __outb(0x42, 0x0f);
    __outb(0x42, 0xf2);

    memset(&active_cmd, 0, sizeof(active_cmd));
    last_cmd[0] = 0;

    enable_interrupts();

    current_mode = COMMS;
    init_mode();
    
    while(1)
    {
        input_update();

        if (input_pressed(START))
        {
            current_mode = (current_mode + 1) % NUM_TEST_MODES;
            init_mode();
        }

        update_mode();
        
        wait_vblank();

    }

    return 0;
}

