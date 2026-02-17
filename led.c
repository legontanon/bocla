#include "led.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "peripherals.pio.h" 

static ws2811_t led_driver;

// Shenzhen Special: Input 0x00RRGGBB -> Output RGB (MSB First)
static inline uint32_t remap_color(uint32_t c) {
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8) & 0xFF;
    uint8_t b = c & 0xFF;
    
    // PIO is configured for Left Shift (MSB first). 
    // We pack data into the top 24 bits.
    // Order: Red, Green, Blue.
    return (r << 24) | (g << 16) | (b << 8);
}

static bool ws2811_timer_callback(struct repeating_timer *t) {
    ws2811_t *ctx = (ws2811_t *)t->user_data;
    
    if (!ctx->color_ptr) return true; 

    uint32_t raw = *ctx->color_ptr;
    uint32_t packet = remap_color(raw);

    pio_sm_put_blocking(ctx->pio, ctx->sm, packet);
    
    // Flush the line with a zero packet to prevent "stuck high" glitch
    pio_sm_put_blocking(ctx->pio, ctx->sm, 0);

    return true; 
}

ws2811_t *ws2811_bind(PIO pio, uint sm, uint pin, volatile uint32_t *color_ptr) {
    led_driver.pio = pio;
    led_driver.sm = sm;
    led_driver.color_ptr = color_ptr;

    // Assumes program loaded in main.c or peripherals.pio.h defaults
    // If you need to re-init the pins/config here, do so.
    // ws2811_program_init(pio, sm, offset, pin, 800000);

    // Start 50ms watchdog
    if (!add_repeating_timer_ms(-50, ws2811_timer_callback, &led_driver, &led_driver.timer)) {
        return NULL;
    }

    return &led_driver;
}
