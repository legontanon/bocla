#ifndef LED_VISUALIZER_H
#define LED_VISUALIZER_H

#include <stdint.h>
#include "fm_synth.h"
#include "led.h"

/**
 * @brief Updates the global status_color based on synth activity.
 * Maps active voice count to brightness and "harmonicity" to color.
 * * @param synth The synthesizer instance to probe.
 * @param color_ptr Pointer to the volatile status_color variable.
 */
static inline void led_visualizer_update(fm_synth_t *synth, volatile uint32_t *color_ptr)
{
    if (!color_ptr)
        return;

    if (!synth)
    {
        *color_ptr = 0x00050010;
        return;
    }

    uint8_t active_count = 0;
    uint32_t total_env = 0;

    // Probe voices for activity and "energy"
    for (int i = 0; i < FM_MAX_VOICES; i++)
    {
        if (synth->voices[i].active)
        {
            active_count++;
            // Use Carrier 0 envelope level as a proxy for "energy"
            total_env += (synth->voices[i].op_state[0].env_level >> 16);
        }
    }

    if (active_count == 0)
    {
        // Idle: Dim Blue/Purple pulse or static
        *color_ptr = 0x00050010;
        return;
    }

    // Map energy to intensity
    // total_env for one voice is max 32767.
    uint8_t intensity = (total_env / (active_count + 1)) >> 7;
    if (intensity > 255)
        intensity = 255;

    // Bach logic: Deep Red/Gold for chords, Cyan for high trills
    uint8_t r, g, b;
    if (active_count > 2)
    {
        // Chordal: Majestic Amber/Orange
        r = intensity;
        g = intensity >> 1;
        b = 0;
    }
    else
    {
        // Solo line: Ethereal Cyan
        r = 0;
        g = intensity >> 1;
        b = intensity;
    }

    *color_ptr = (r << 16) | (g << 8) | b;
}

#endif // LED_VISUALIZER_H
