#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "led.h"

#define PIN_LED 21

volatile uint32_t led_color = 0;

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

    ws2811_set(&led_color, 255, 255, 255); // White
    sleep_ms(1000);

    uint8_t hue = 0;
    while (1)
    {
        // Cycle hue, Saturation 255, Value 128 (50% brightness)
        ws2811_set_hsv(&led_color, hue++, 255, 128);

        // 20ms update = 50Hz refresh
        sleep_ms(20);
    }
}