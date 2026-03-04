

#include "fm_synth.h"

#include <stdint.h>
#include <math.h>

#include "pico/platform.h" // For __not_in_flash_data

/**
 * @brief 1024-entry quarter-sine table.
 * Total size: 2048 bytes.
 * __not_in_flash_data ensures this is copied to SRAM at boot.
 * For maximum performance, use __scratch_y or __scratch_x to isolate it
 * from the main system bus.
 */
__not_in_flash_data(fm_sine_table)
    const int16_t fm_sine_table[1024] = {
// You'll likely generate this at build time, but here is the logic:
// for(int i=0; i<1024; i++) table[i] = round(sin(i * PI / 2048) * 32767);
#include "sine_values.inc"
};

/**
 * @brief Simple build-time generator if you don't want to carry an .inc file.
 * Call this once at init if you prefer to save 2KB of Flash and have 2KB of RAM.
 */
void fm_generate_sine_table(int16_t *table)
{
    for (int i = 0; i < 1024; i++)
    {
        // We use 2048 because 1024 is the quarter, so 4096 is the full cycle.
        // i=1024 results in sin(pi/2) = 1.0.
        table[i] = (int16_t)(sinf(i * M_PI / 2048.0f) * 32767.0f);
    }
}
