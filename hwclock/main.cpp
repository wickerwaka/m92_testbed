#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "counter.pio.h"

#define GPIO_DATA_BASE 0
#define GPIO_OE 16
#define GPIO_CLK 18
#define GPIO_LED 25

int main() {
    stdio_init_all();

    PIO pio = pio0;
    uint offset;
    pio_sm_config config;

    for( int i = 0; i < 16; i++ )
    {
        gpio_init(GPIO_DATA_BASE + i);
        pio_gpio_init(pio, GPIO_DATA_BASE + i);
        gpio_set_pulls(GPIO_DATA_BASE + i, false, false);
        gpio_set_dir(GPIO_DATA_BASE + i, false);
    }

    gpio_init(GPIO_CLK);
    gpio_set_dir(GPIO_CLK, false);
    gpio_set_pulls(GPIO_CLK, true, false);

    gpio_init(GPIO_OE);
    gpio_set_dir(GPIO_OE, false);
    gpio_set_pulls(GPIO_OE, true, false);

    pio_gpio_init(pio, GPIO_LED);

    pio_sm_set_consecutive_pindirs(pio, 0, GPIO_LED, 1, true);
    offset = pio_add_program(pio, &oe_check_program);
    config = oe_check_program_get_default_config(offset);
    pio_sm_init(pio, 0, offset, &config);
    pio_sm_set_out_pins(pio, 0, GPIO_DATA_BASE, 16);
    pio_sm_set_in_pins(pio, 0, GPIO_OE);
    pio_sm_set_set_pins(pio, 0, GPIO_LED, 1);
    pio_sm_set_enabled(pio, 0, true);

    offset = pio_add_program(pio, &counter_program);
    config = counter_program_get_default_config(offset);
    pio_sm_init(pio, 1, offset, &config);
    pio_sm_set_out_pins(pio, 1, GPIO_DATA_BASE, 16);
    pio_sm_set_in_pins(pio, 1, GPIO_CLK);
    pio_sm_set_enabled(pio, 1, true);

    while(true)
    {
    }

    return 0;
}
