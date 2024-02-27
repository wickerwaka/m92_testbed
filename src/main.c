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
    uint16_t port = 0x98 + ((idx & 0x3) << 1);
    if (enabled)
        __outw(port, 0x00);
    else
        __outw(port, 0x10);
}

void pf_set_xy(uint8_t idx, uint16_t x, uint16_t y)
{
    uint16_t x_port = 0x84 + ((idx & 0x3) << 3);
    uint16_t y_port = 0x80 + ((idx & 0x3) << 3);

    __outw(x_port, x);
    __outw(y_port, y);
}


__far uint16_t *palette_ram = (__far uint16_t *)0xf0008800;
__far uint16_t *reg_videocontrol = (__far uint16_t *)0xf0009800;
__far uint16_t *reg_spritecontrol = (__far uint16_t *)0xf0009000;
__far uint16_t *vram = (__far uint16_t *)0xd0000000;

uint16_t base_palette[] = {
    0x0000, 0x3d80, 0x3100, 0x2420,
	0x233c, 0x2b1c, 0x0e16, 0x5f59,
	0x0114, 0x322c, 0x2500, 0x2653,
	0x3e30, 0x01f1, 0x0000, 0x0000,
};

void set_timer_interval(uint16_t n)
{
    n = 1024 - n;

    uint8_t low = n & 0x7f;
    uint8_t high = 0x80 | ( ( n >> 7) & 0x7f );

    __outb(0x00, low);
    for( int i = 0; i < 1000; i++ ) { asm(""); };
    __outb(0x00, high);
    for( int i = 0; i < 1000; i++ ) { asm(""); };
}

static uint8_t old_ticks;
static uint16_t ticks = 0;
static inline uint16_t poll_timer()
{
    uint8_t new_ticks = __inb(0x08);
    ticks += (new_ticks - old_ticks);
    old_ticks = new_ticks;
    return ticks;
}


volatile uint32_t vblank_count = 0;
volatile uint8_t prev_vblank_tick = 0;
volatile uint8_t vblank_tick = 0;

__attribute__((interrupt)) void __far vblank_handler()
{
    prev_vblank_tick = vblank_tick;
    vblank_tick = __inb(0x08);
	vblank_count++;
    return;
}

void wait_vblank()
{
    uint32_t cnt = vblank_count;
    while( cnt == vblank_count )
    {
        poll_timer();
    }
}

void draw_pf_text(int color, uint16_t x, uint16_t y, const char *str)
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
            vram[(ofs << 1) + 1] = color;
            vram[(ofs << 1)] = *str;
            ofs++;
        }
        str++;
    }
}

extern uint8_t just_nop();
extern uint8_t ror7();
extern uint8_t rorc80();
extern uint8_t mov_from_mem();
extern uint8_t mov_to_mem();
extern uint8_t add_mem();

typedef struct
{
    const char *name;
    uint8_t (*func)();
} TestDesc;

TestDesc s_tests[] =
{
    { "NOP", just_nop },
    { "ROR DX, 7", ror7 },
    { "RORC DX, 80", rorc80 },
    { "MOV AX, SS:[0]", mov_from_mem },
    { "MOV SS:[0], AX", mov_to_mem },
    { "ADD SS:[0], AX", add_mem }
};

const size_t NUM_TESTS = sizeof(s_tests) / sizeof(TestDesc);

void run_tests(uint8_t *results)
{
    disable_interrupts();

    for( int t = 0; t < NUM_TESTS; t++ )
    {
        results[t] = s_tests[t].func();
    }

    enable_interrupts();
}


int main()
{
    __outb(0x40, 0x13);
    __outb(0x42, 0x08);
    __outb(0x42, 0x0f);

    __outb(0x42, 0xfa); // MASK 0b11111010

    memset(&active_cmd, 0, sizeof(active_cmd));
    last_cmd[0] = 0;

    *reg_videocontrol = 0x2000;

    memcpyw(palette_ram, base_palette, sizeof(base_palette) >> 1);
    reg_spritecontrol[4] = 0;

    memsetw(vram, 0, 0x8000);

    set_timer_interval(100);
    
    pf_enable(0, true);
    pf_enable(1, false);
    pf_enable(2, false);

    pf_set_xy(0, -80, -136);

    char tmp[64];

    enable_interrupts();
    
    uint8_t write_idx = 0;
    uint32_t comms_count = 0;
    uint16_t color = 0;
    uint8_t frame_ticks = 0;
    
    uint8_t test_results[NUM_TESTS];

    while(1)
    {
        if (comms_update() )
        {
            update_cmd(&active_cmd);
            process_cmd(&active_cmd);
        }

        run_tests(test_results);

        snprintf(tmp, sizeof(tmp), "VBLANK: %06X", vblank_count);
        draw_pf_text(0, 2, 1, tmp);

        snprintf(tmp, sizeof(tmp), "AUDIO: %3u", frame_ticks);
        draw_pf_text(0, 2, 2, tmp);

        for( int i = 0; i < NUM_TESTS; i++ )
        {
            snprintf(tmp, sizeof(tmp), "%16s: %3u", s_tests[i].name, test_results[i]);
            draw_pf_text(0, 2, 4 + i, tmp);
        }
 
        //comms_status(tmp, sizeof(tmp));
        //draw_pf_text(0, 2, 2, tmp);
        
        wait_vblank();
        reg_spritecontrol[4] = 0;

        if (prev_vblank_tick > vblank_tick)
        {
            frame_ticks = (vblank_tick + 256) - prev_vblank_tick;
        }
        else
        {
            frame_ticks = vblank_tick - prev_vblank_tick;
        }

    }

    return 0;
}

