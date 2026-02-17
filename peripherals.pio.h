#ifndef PERIPHERALS_PIO_H
#define PERIPHERALS_PIO_H

#include "hardware/pio.h"

static const uint16_t i2s_out_program_instructions[] = {
    0x0000
};

static const struct pio_program i2s_out_program = {
    .instructions = i2s_out_program_instructions,
    .length = 1,
    .origin = -1,
};

static inline void i2s_out_program_init(PIO pio, uint sm, uint offset, uint pin_data, uint pin_bclk) {
    (void)offset;
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_out_pins(&c, pin_data, 1);
    sm_config_set_set_pins(&c, pin_data, 1);
    pio_gpio_init(pio, pin_data);
    pio_gpio_init(pio, pin_bclk);
    pio_sm_set_consecutive_pindirs(pio, sm, pin_data, 1, true);
    pio_sm_set_consecutive_pindirs(pio, sm, pin_bclk, 2, true);
    pio_sm_init(pio, sm, 0, &c);
    pio_sm_set_enabled(pio, sm, true);
}

#endif
