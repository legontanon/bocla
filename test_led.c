#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "led.h"

#define PIN_LED 21

volatile uint32_t led_color = 0;

// Updates led_color using the ws2811_set API
void update_hsv(uint8_t h, uint8_t s, uint8_t v)
{
    unsigned char region, remainder, p, q, t;

    if (s == 0)
    {
        ws2811_set(&led_color, v, v, v);
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
    ws2811_set(&led_color, r, g, b);
}

int main()
{
    stdio_init_all();

    // Init WS2811 on Pin 21
    if (!ws2811_bind(pio1, 0, PIN_LED, &led_color))
    {
        printf("Failed to bind WS2811\n");
        return 1;
    }

    printf("LED Driver Started. Using API.\n");

    uint8_t hue = 0;
    while (1)
    {
        // Cycle hue, Saturation 255, Value 128 (50% brightness)
        update_hsv(hue++, 255, 128);

        // 20ms update = 50Hz refresh
        sleep_ms(20);
    }
}