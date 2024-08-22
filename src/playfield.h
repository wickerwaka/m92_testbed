#if !defined(PLAYFIELD_H)
#define PLAYFIELD_H 1

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if !defined(__FAR)
#define __far
#endif

static __far uint16_t *VRAM = (__far uint16_t *)0xd0000000;

#define PF_WIDE      0x04
#define PF_DISABLE   0x10
#define PF_ROWSCROLL 0x20
#define PF_ROWSELECT 0x40
#define PF_UNK1      0x80

#define PF_PRIO1 (1 << 5)
#define PF_PRIO0 (1 << 4)
#define PF_FLIPX (1 << 6)
#define PF_FLIPY (1 << 7)

void pf_reset();
void pf_enable(uint8_t idx, bool enabled);
void pf_set_vram(uint8_t idx, uint16_t base);
void pf_set_flags(uint8_t idx, uint8_t flags);
void pf_set_xy(uint8_t idx, uint16_t x, uint16_t y);
uint16_t pf_get_x(uint8_t idx);
uint16_t pf_get_y(uint8_t idx);

__far uint16_t *pf_addr(uint8_t idx);
__far uint16_t *pf_rowscroll_addr(uint8_t idx);
__far uint16_t *pf_rowselect_addr(uint8_t idx);

void pf_text(uint8_t layer, uint16_t color, uint16_t x, uint16_t y, const char *str);
void pf_sym(uint8_t layer, uint16_t color, uint16_t x, uint16_t y, uint16_t sym);

#endif // PLAYFIELD_H