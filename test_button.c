/**
 * Bocla - test_button.c
 * Test program for buttons and LED integration.
 * (c) 2024 Luis Enrique Garcia Ontanon
 * See LICENSE.txt for license details.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "buttons.h"
#include "led.h"

#define PIN_BTN_BASE 16
#define PIN_LED 21

volatile uint32_t led_color = 0;

// Callbacks
void cb_btn_0(void *_)
{
    printf("Btn 16\n");
    ws2811_set(&led_color, 255, 0, 0);
} // Red
void cb_btn_1(void *_)
{
    printf("Btn 17\n");
    ws2811_set(&led_color, 0, 255, 0);
} // Green
void cb_btn_2(void *_)
{
    printf("Btn 18\n");
    ws2811_set(&led_color, 0, 0, 255);
} // Blue
void cb_btn_3(void *_)
{
    printf("Btn 19\n");
    ws2811_set(&led_color, 255, 255, 0);
} // Yellow
void cb_btn_4(void *_)
{
    printf("Btn 20\n");
    ws2811_set(&led_color, 255, 0, 255);
} // Magenta

void cb_chord(void *_)
{
    printf("Chord (16+17)\n");
    ws2811_set(&led_color, 255, 255, 255); // White
}

int main()
{
    stdio_init_all();

    ws2811_bind(pio1, 0, PIN_LED, &led_color);
    buttons_init(pio1, 1, PIN_BTN_BASE);

    // Register 1:1 mappings
    buttons_register(0x01, cb_btn_0, NULL); // Pin 16
    buttons_register(0x02, cb_btn_1, NULL); // Pin 17
    buttons_register(0x04, cb_btn_2, NULL); // Pin 18
    buttons_register(0x08, cb_btn_3, NULL); // Pin 19
    buttons_register(0x10, cb_btn_4, NULL); // Pin 20

    // Register a chord (Pin 16 + 17)
    buttons_register(0x03, cb_chord, NULL);

    printf("Button Test. Press buttons to change LED color.\n");

    while (1)
    {
        tight_loop_contents();
    }
}
