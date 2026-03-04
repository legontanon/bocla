#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

// Your Modules
#include "cfg.h"
#include "audio.h"
#include "buttons.h"
#include "led.h"
#include "usb_manager.h"

// --- Hardware Config ---
// Audio (Verified: LRC=27, BCLK=26 => WS=SCK+1)
#define PIN_I2S_BCLK 26
#define PIN_I2S_LRC 27
#define PIN_I2S_DIN 28

// UI
#define PIN_BTN_BASE 16 // 5 Pins: 16-20
#define PIN_LED 21

// --- Globals ---
volatile uint32_t status_color = 0; // 0x00RRGGBB

// --- Callbacks ---

void cb_vol_up(void *user_data)
{
    usb_send_volume(1);
    // Visual feedback: Flash Green
    ws2811_inc(&status_color, 0, 40, 0);
}

void cb_vol_down(void *user_data)
{
    usb_send_volume(-1);
    // Visual feedback: Dim Green
    ws2811_inc(&status_color, 0, -40, 0);
}

void cb_mute_toggle(void *user_data)
{
    usb_send_volume(0);
    // Visual feedback: Red
    ws2811_set(&status_color, 100, 0, 0);
}

void cb_play_pause(void *user_data)
{
    // Standard HID Usage for Play/Pause is 0xCD, needs support in usb_manager
    // For now, toggle Blue
    ws2811_set(&status_color, 0, 0, 100);
}

// --- Main ---

void init()
{
    // 1. System Init
    // Overclocking to 133MHz or higher is often safer for USB+I2S jitter,
    // but standard 125MHz usually works for 48kHz.
    set_sys_clock_khz(133000, true);
    stdio_init_all();

    // LED (PIO1, SM0)
    // Blue defaults to "Idle/Boot"
    status_color = 0x00000020;
    ws2811_bind(pio1, 0, PIN_LED, &status_color);

    // Audio (PIO0, SM0) - 48kHz
    // Note: Pin order in function is (DIN, BCLK). LRC is implied as BCLK+1.
    audio_i2s_init(pio0, 0, PIN_I2S_DIN, PIN_I2S_BCLK, 48000);

    // Buttons (PIO1, SM1)
    buttons_init(pio1, 1, PIN_BTN_BASE);

    // 3. Register Controls
    // Mask 0x01 (Btn 0): Vol -
    buttons_register(0x01, cb_vol_down, NULL);

    // Mask 0x02 (Btn 1): Vol +
    buttons_register(0x02, cb_vol_up, NULL);

    // Mask 0x03 (Btn 0+1 Chord): Mute
    buttons_register(0x03, cb_mute_toggle, NULL);

    // Mask 0x04 (Btn 2): Play/Pause (Optional)
    buttons_register(0x04, cb_play_pause, NULL);

    // 4. USB Init
    usb_manager_init();

    // Update LED to Green to indicate "Ready for Host"
    status_color = 0x00002000;
}

void main_core1()
{
    // Core 1 handles real-time tasks: Audio and Buttons
    while (1)
    {
        tight_loop_contents();
    }
}

int main()
{
    init();
    // Core 0 is for USB and Main Loop
    // Core 1 is for PIO and real-time tasks
    multicore_launch_core1(main_core1);

    // Core 0 can be mostly idle, just handle USB tasks
    while (1)
    {
        usb_manager_task();
        // Optional: Add a low-power sleep here if desired
        // sleep_ms(10);
    }
}
