#include <stdint.h>
#include <stdbool.h>
#include "printf/printf.h"

#include "util.h"
#include "interrupts.h"
#include "comms.h"
#include "v35_sfr.h"
#include "playfield.h"
#include "obj.h"

#include "game_palette.h"

__far V35_SFR *init_sfr = (__far V35_SFR *)0xfff00000;
__far V35_SFR *sfr = (__far V35_SFR *)0x9ff00000;

__far uint16_t *palette_ram = (__far uint16_t *)0xe0000000;
__far uint16_t *vram = (__far uint16_t *)0xd0000000;

uint16_t base_palette[] = {
    0x0000, 0xffff, 0x3100, 0x2420,
	0x233c, 0x2b1c, 0x0e16, 0x5f59,
	0x0114, 0x322c, 0x2500, 0x2653,
	0x3e30, 0x01f1, 0x0000, 0x0000,
};

volatile uint32_t vblank_count = 0;
__attribute__((interrupt)) void __far p0_handler()
{
	vblank_count++;
    __fint();
    return;
}

void wait_vblank()
{
    uint32_t cnt = vblank_count;
    while( cnt == vblank_count ) {}
}

int main()
{
    init_sfr->idb = 0x9f;
    sfr->wtc = 0xffff; // respect READY
    sfr->portmc1 = 0x80; // enable READY pin
    sfr->exic0 = 0x7; // enable p0
    sfr->exic1 = 0x47; // disable p1
    sfr->exic2 = 0x47; // disable p2
    sfr->prc = 0x0c; // disable internal ram, x/2 scaling

    *palette_ram = 0x0000;

    memsetw(vram, 0x00, 0x8000);
    memsetw(OBJRAM, 0x0, 256);

    pf_enable(0, true);
    pf_enable(1, true);

    pf_set_flags(0, PF_ROWSELECT | PF_ROWSCROLL);

    pf_set_vram(0, 0x8000);
    pf_set_xy(0, -82, -136);
    pf_set_xy(1, -78, -136);


    memcpyw(palette_ram, game_palette, 512);

    char tmp[64];
 
    enable_interrupts();
    
    uint8_t write_idx = 0;
    uint32_t comms_count = 0;

    __far uint16_t *yofs = (__far uint16_t *)0xd000f800;
    __far uint16_t *xofs = (__far uint16_t *)0xd000f000;

    memsetw(yofs, -136, 512);
    memsetw(xofs, -82, 512);

    uint16_t line_adj = 0;

    while(1)
    {
        wait_vblank();

        __far OBJ *obj = &OBJRAM[0];
        obj->x = 199;
        obj->color = 3;
        obj->fy = 0;
        obj->fx = 0;
        obj->sprite = 0x20;
        obj->height = 1;
        obj->y = 233;
        obj++;

        obj->x = 215;
        obj->color = 3;
        obj->fy = 0;
        obj->fx = 1;
        obj->sprite = 0x20;
        obj->height = 1;
        obj->y = 233;
        obj++;

        obj->x = 239;
        obj->color = 3;
        obj->fy = 0;
        obj->fx = 0;
        obj->sprite = 0x20;
        obj->height = 1;
        obj->y = 232;
        obj++;

        obj->x = 255;
        obj->color = 3;
        obj->fy = 0;
        obj->fx = 1;
        obj->sprite = 0x20;
        obj->height = 1;
        obj->y = 232;
        obj++;

        int adj = 0;
        for( int i = 0; i < 16; i++)
        {
            yofs[136 + 80 + i] = -136 - adj;
            xofs[96 + i] = -81 - i;
            if (i & 1) adj++;
        }

        snprintf(tmp, sizeof(tmp), "VLBLANK: %04X", vblank_count);
        pf_text(0, 3, 10, 4, tmp);

        snprintf(tmp, sizeof(tmp), "P1_P2: %04X   P3_P4: %04X", __inw(0x00), __inw(0x06));
        pf_text(0, 3, 10, 5, tmp);
        snprintf(tmp, sizeof(tmp), "SYSTEM: %04X    DSW: %04X", __inw(0x02), __inw(0x04));
        pf_text(0, 3, 10, 6, tmp);

        snprintf(tmp, sizeof(tmp), "ROW OFFSET");
        pf_text(0, 3, 20, 10, tmp);
        snprintf(tmp, sizeof(tmp), "ROW SCROLL");
        pf_text(0, 4, 20, 12, tmp);

        snprintf(tmp, sizeof(tmp), "NO OFFSET");
        pf_text(1, 0, 10, 10, tmp);
        snprintf(tmp, sizeof(tmp), "NO SCROLL");
        pf_text(1, 0, 10, 12, tmp);

        pf_sym(0, 1, 0, 0, 0x10);
        pf_sym(1, 1, 1, 1, 0x10);

        pf_sym(0, 1, 0, 29, 0x11);
        pf_sym(1, 1, 1, 28, 0x11);


        pf_sym(0, 1, 39, 0, 0x12);
        pf_sym(1, 1, 38, 1, 0x12);

        pf_sym(0, 1, 39, 29, 0x13);
        pf_sym(1, 1, 38, 28, 0x13);

        pf_sym(0, 1, 14, 15, 0x2);
        pf_sym(0, 1, 13, 15, 0x4);
        pf_sym(0, 1, 14, 14, 0x4);

        pf_sym(0, 1, 20, 15, 0x2);
    }

    return 0;
}

