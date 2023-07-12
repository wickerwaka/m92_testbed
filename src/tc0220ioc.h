#if !defined( TC022IOC_H )
#define TC022IOC_H 1

#include <stdint.h>

typedef volatile struct TC0220IOC_Control
{
    union
    {
        uint16_t sw_a;
        uint16_t watchdog;
    };

    uint8_t _pad1;
    uint8_t sw_b;

    uint8_t _pad2;
    uint8_t p1;

    uint8_t _pad3;
    uint8_t p2;

    uint8_t _pad4;
    uint8_t coin_count;

    uint8_t _pad5;
    uint8_t unknown0;

    uint8_t _pad6;
    uint8_t unknown1;
    
    uint8_t _pad7;
    uint8_t coin;
} TC0220IOC_Control;

_Static_assert(sizeof(TC0220IOC_Control) == 0x10, "TC0220IOC_Control mismatch");

#endif