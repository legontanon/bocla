#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "audio.h"
#include "synth.h"
#include "buttons.h"
#include "led.h"

// --- Config ---
#define PIN_I2S_BCLK 26
#define PIN_I2S_DIN 28
#define PIN_BTN_BASE 16
#define PIN_LED 21

#define BLOCK_SIZE 1024
#define REG_COUNT 3
#define SR 48000

// --- Globals ---
sample_t buf_pool[REG_COUNT][BLOCK_SIZE];
sample_t *reg_file[REG_COUNT];

sample_t wave_sine[256];
sample_t wave_saw[256];
sample_t wave_square[256];
sample_t *waveforms[] = {wave_sine, wave_saw, wave_square};

synth_osc_t osc;
volatile uint32_t led_color = 0;

struct
{
    int volume;
    int wave_idx;
    float freq;
    float sweep_dir;
    float sweep_speed;
} state = {
    .volume = 8000,
    .wave_idx = 0,
    .freq = 300.0f,
    .sweep_dir = 1.0f,
    .sweep_speed = 5.0f};

// --- Helpers ---
void generate_tables()
{
    for (int i = 0; i < 256; i++)
    {
        wave_sine[i] = (sample_t)(sinf(i * 6.28318f / 256.0f) * 32767.0f);
        wave_saw[i] = (sample_t)(((float)i / 256.0f * 2.0f - 1.0f) * 32767.0f);
        wave_square[i] = (i < 128) ? 32000 : -32000;
    }
}

void fill_dc(sample_t *buf, int16_t val, size_t len)
{
    for (size_t i = 0; i < len; i++)
        buf[i] = val;
}

// --- Callbacks ---

// 16: Vol Up
void cb_vol_up(void *_)
{
    state.volume += 2000;
    if (state.volume > 32767)
        state.volume = 32767;
    ws2811_set(&led_color, 0, 50, 0); // Dim Green
}

// 17: Cycle Wave
void cb_cycle(void *_)
{
    state.wave_idx = (state.wave_idx + 1) % 3;
    osc.table = waveforms[state.wave_idx];
    ws2811_set(&led_color, 0, 0, 50); // Dim Blue
}

// 18: Vol Down
void cb_vol_down(void *_)
{
    state.volume -= 2000;
    if (state.volume < 0)
        state.volume = 0;
    ws2811_set(&led_color, 50, 0, 0); // Dim Red
}

// 19: Slow Down
void cb_slow(void *_)
{
    state.sweep_speed *= 0.8f;
    if (state.sweep_speed < 0.1f)
        state.sweep_speed = 0.1f;
    ws2811_set(&led_color, 50, 0, 50); // Purple
}

// 20: Accelerate
void cb_fast(void *_)
{
    state.sweep_speed *= 1.2f;
    if (state.sweep_speed > 200.0f)
        state.sweep_speed = 200.0f;
    ws2811_set(&led_color, 50, 50, 0); // Yellow
}

// OSC -> REG 0, MIX (OSC * VOL) -> REG 2
synth_cmd_t patch[] = {
    {.type = SYNTH_CMD_OSC, .osc = {.state = &osc, .out = 0}},
    {.type = SYNTH_CMD_AMP, .mix = {.in_a = 0, .in_b = 1, .out = 2}}};

int main()
{
    set_sys_clock_khz(133000, true);
    stdio_init_all();

    // Wire memory
    for (int i = 0; i < REG_COUNT; i++)
        reg_file[i] = buf_pool[i];

    generate_tables();
    synth_osc_init_table(&osc, wave_sine, 256, 300.0f);

    ws2811_bind(pio1, 0, PIN_LED, &led_color);
    buttons_init(pio1, 1, PIN_BTN_BASE);
    audio_init(pio0, 0, PIN_I2S_DIN, PIN_I2S_BCLK, SR);

    // Register Controls
    buttons_register(0x01, cb_vol_up, NULL);   // 16
    buttons_register(0x02, cb_cycle, NULL);    // 17
    buttons_register(0x04, cb_vol_down, NULL); // 18
    buttons_register(0x08, cb_slow, NULL);     // 19
    buttons_register(0x10, cb_fast, NULL);     // 20

    printf("Audio Driver. Sweep 300-3300Hz.\n");

    while (1)
    {
        // 1. Update Sweep
        state.freq += (state.sweep_speed * state.sweep_dir);

        if (state.freq >= 3300.0f)
        {
            state.freq = 3300.0f;
            state.sweep_dir = -1.0f;
        }
        else if (state.freq <= 300.0f)
        {
            state.freq = 300.0f;
            state.sweep_dir = 1.0f;
        }

        synth_osc_set_freq(&osc, state.freq);

        // 2. Fill Volume Control Buffer
        fill_dc(reg_file[1], (int16_t)state.volume, BLOCK_SIZE);

        // 3. Process
        synth_process_batch(patch, 2, reg_file, REG_COUNT, BLOCK_SIZE);

        // 4. Enqueue
        while (!audio_enqueue(reg_file[2], BLOCK_SIZE))
        {
            tight_loop_contents();
        }
    }
}