#ifndef WS2811_H
#define WS2811_H

#include "cfg.h"

#include <stdint.h>
#include <stdbool.h>
#include "hardware/pio.h"
#include "pico/time.h"

// Your color pointer target.
// Format: 0x00RRGGBB (Standard Hex)
// The driver handles the bit-shifting and protocol quirks.
typedef struct
{
    PIO pio;
    uint sm;
    volatile uint32_t *color_ptr;
    struct repeating_timer timer;
} ws2811_t;

enum
{
    LED_COLOR_RED = 0x00FF0000,
    LED_COLOR_GREEN = 0x0000FF00,
    LED_COLOR_BLUE = 0x000000FF,
    LED_COLOR_MAGENTA = 0x00FF00FF,
    LED_COLOR_CYAN = 0x0000FFFF,
    LED_COLOR_YELLOW = 0x00FFFF00,
    LED_COLOR_WHITE = 0x00FFFFFF,

    LED_COLOR_IDLE = 0x00050010,    // Dim Blue/Purple
    LED_COLOR_READY = 0x00002000,   // Green
    LED_COLOR_CHORD = 0x00FF8000,   // Amber/Orange
    LED_COLOR_TRILL = 0x0000FF80,   // Cyan
    LED_COLOR_WAITING = 0x00000020, // Blue
    LED_COLOR_ERROR = 0x00FF0000,   // Red
} led_colors_t;

/**
 * @brief Initialize the LED watcher.
 * This sets up the PIO state machine and starts a watchdog timer to refresh the LED.
 * @param pio The PIO instance (pio0 or pio1)
 * @param sm The State Machine index (0-3)
 * @param pin The GPIO pin connected to the LED data line
 * @param color_ptr Pointer to the 32-bit integer holding your color (0x00RRGGBB)
 * @return Pointer to the driver instance (handle) on success, NULL on failure.
 */
ws2811_t *ws2811_bind(PIO pio, uint sm, uint pin, volatile uint32_t *color_ptr);

// --- Color Manipulation Helpers ---

// Converts HSV to RGB and updates led_color using the ws2811_set API
void ws2811_set_hsv(volatile uint32_t *led_color, uint8_t h, uint8_t s, uint8_t v);

// --- State Manipulation Helpers ---

/**
 * @brief Set the LED color atomically-ish.
 * Memory Layout: 0x00RRGGBB
 */
static inline void ws2811_set(volatile uint32_t *addr, uint8_t r, uint8_t g, uint8_t b)
{
    *addr = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

/**
 * @brief Increment/Decrement channels with saturation.
 * Takes signed ints to allow subtraction. Clamps to 0-255.
 */
static inline void ws2811_inc(volatile uint32_t *addr, int16_t dr, int16_t dg, int16_t db)
{
    uint32_t c = *addr;
    int32_t r = (c >> 16) & 0xFF;
    int32_t g = (c >> 8) & 0xFF;
    int32_t b = c & 0xFF;

    r += dr;
    g += dg;
    b += db;

    if (r < 0)
        r = 0;
    else if (r > 255)
        r = 255;
    if (g < 0)
        g = 0;
    else if (g > 255)
        g = 255;
    if (b < 0)
        b = 0;
    else if (b > 255)
        b = 255;

    *addr = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

#ifndef WS2811_PIO_PROGRAM
#define WS2811_PIO_PROGRAM 1
#endif

#ifndef WS2811_PIO_SM
#define WS2811_PIO_SM 0
#endif

#endif
