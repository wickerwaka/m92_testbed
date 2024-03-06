#include <stdint.h>
#include <stdbool.h>
#include "printf/printf.h"

#include "util.h"
#include "interrupts.h"
#include "comms.h"

#include "timing_tests.h"

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
__far volatile uint16_t *hwcounter = (__far volatile uint16_t *)0xb0000000;

uint16_t base_palette[] = 
{
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x7FFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x001F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x7C1F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x03E0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x7FE0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x03FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x7C00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x6318, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
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
    }
}

void draw_pf_text(uint16_t x, uint16_t y, const char *str)
{
    int ofs = ( y * 64 ) + x;
    uint16_t color = 0;

    while(*str)
    {
        if( *str == '\n' )
        {
            y++;
            ofs = (y * 64) + x;
        }
        else if (*str == '^')
        {
            *str++;
            if (*str == '\0') break;
            color = *str - '0';
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

#if !defined(FORCE_PAGE)
#define FORCE_PAGE -1
#endif

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

    set_timer_interval(50);
    
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
    
    int current_test = 0;
    uint16_t test_results[NUM_TIMING_TESTS];

    uint16_t page = FORCE_PAGE >= 0 ? FORCE_PAGE : 0;
    uint16_t num_pages = 1;
    for( int i = 0; i < NUM_TIMING_TESTS; i++ )
    {
        if (timing_tests[i].page + 1 > num_pages)
            num_pages = timing_tests[i].page + 1;
    }

    int page_delay = 0;

    while(1)
    {
        if (comms_update() )
        {
            update_cmd(&active_cmd);
            process_cmd(&active_cmd);
        }

        test_results[current_test] = run_timing_test(current_test);
        current_test = ( current_test + 1 ) % NUM_TIMING_TESTS;

        int y = 0;
        for( int i = 0; i < NUM_TIMING_TESTS; i++ )
        {
            if (timing_tests[i].page == page)
            {
                int tick_color = 3;
                if (test_results[i] != timing_tests[i].hw_ticks) tick_color = 5;

                snprintf(tmp, sizeof(tmp), "^6%c ^7%20s: ^%u%3u ^4(%u)",
                    i == current_test ? '*' : ' ',
                    timing_tests[i].name,
                    tick_color,
                    test_results[i],
                    timing_tests[i].hw_ticks
                );
                draw_pf_text(0, 2 + y, tmp);
                y++;
            }
        }


        wait_vblank();

        page_delay++;

        if (page_delay > 300 && FORCE_PAGE == -1)
        {
            page = ( page + 1 ) % num_pages;
            page_delay = 0;
            memsetw(vram, 0, 0x8000);
        }

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

