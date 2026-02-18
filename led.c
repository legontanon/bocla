#include "led.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "led.pio.h"

static ws2811_t led_driver;

// Shenzhen Special: Input 0x00RRGGBB -> Output RGB (MSB First)
static inline uint32_t remap_color(uint32_t c)
{
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8) & 0xFF;
    uint8_t b = c & 0xFF;

    // PIO is configured for Left Shift (MSB first).
    // We pack data into the top 24 bits.
    // Order: Red, Green, Blue.
    return (r << 24) | (g << 16) | (b << 8);
}

static bool ws2811_timer_callback(struct repeating_timer *t)
{
    ws2811_t *ctx = (ws2811_t *)t->user_data;

    if (!ctx->color_ptr)
        return true;

    uint32_t raw = *ctx->color_ptr;
    uint32_t packet = remap_color(raw);

    pio_sm_put_blocking(ctx->pio, ctx->sm, packet);

    // Flush the line with a zero packet to prevent "stuck high" glitch
    pio_sm_put_blocking(ctx->pio, ctx->sm, 0);

    return true;
}

ws2811_t *ws2811_bind(PIO pio, uint sm, uint pin, volatile uint32_t *color_ptr)
{
    led_driver.pio = pio;
    led_driver.sm = sm;
    led_driver.color_ptr = color_ptr;

    uint offset = pio_add_program(pio, &ws2811_program);
    ws2811_program_init(pio, sm, offset, pin, 800000.0f);

    // Start 50ms watchdog
    if (!add_repeating_timer_ms(-50, ws2811_timer_callback, &led_driver, &led_driver.timer))
    {
        return NULL;
    }

    return &led_driver;
}

// Updates led_color using the ws2811_set API
void ws2811_set_hsv(uint32_t *led_color, uint8_t h, uint8_t s, uint8_t v)
{
    unsigned char region, remainder, p, q, t;

    if (s == 0)
    {
        ws2811_set(led_color, v, v, v);
        return;
    }

    region = h / 43;
    remainder = (h - (region * 43)) * 6;
    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    uint8_t r, g, b;
    switch (region)
    {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    default:
        r = v;
        g = p;
        b = q;
        break;
    }

    // Use the API to set the volatile memory
    ws2811_set(led_color, r, g, b);
}
