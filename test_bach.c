/**
 * Bocla - test_bach.c
 * Test file for Bach Toccata sequence in the FM Sequencer.
 * (c) 2024 Luis Enrique Garcia Ontanon
 * See LICENSE.txt for license details.
 *
 * It will start playing and update the LED visualizer based on the active
 * voices in the synthesizer.
 * The sequence is loaded from fm_bach.h and uses the toccata organ patch
 * defined there.
 */

/**
 * @file test_bach.c
 * @brief Test file for Bach Toccata sequence in the FM Sequencer.
 *
 * It will start playing and update the LED visualizer based on the active voices in the synthesizer.
 * The sequence is loaded from fm_bach.h and uses the toccata organ patch defined there.
 */

#include "fm_synth.h"
#include "fm_sequencer.h"
#include "led_visualizer.h"
#include "bach.h"
#include "audio.h"

#include "pico/stdlib.h"
#include <stdio.h>

static volatile uint32_t status_color = 0;
uint16_t buffer[128][2] = {0};
static int16_t *audio_block = buffer[0]; // Start with the first buffer
uint32_t heartbeat = 1;
fm_synth_t *synth;
fm_sequencer_t *seq;

void underflow_handler()
{
    heartbeat++;
    if ((heartbeat % 50) == 0)
    {
        printf(".");
    }

    if (heartbeat % 2500 == 0)
    {
        printf("\n");
    }

    audio_block = (int16_t *)buffer[heartbeat % 2];
    heartbeat++;

    (void)fm_seq_process_block(seq, audio_block, 128); // Process a block of audio (128 samples)
    audio_enqueue(audio_block, 128);                   // Enqueue for playback
    led_visualizer_update(synth, &status_color);

    // check if the sequencer is still playing, if not, restart it
    if (!seq->cfg.playing)
    {
        printf("Restarting sequencer...\n");
        fm_seq_play(seq);
    }
}

int main()
{
    int32_t color = 0x00050010; // Initial LED color (Dim Blue/Purple)

    stdio_init_all();
    sleep_ms(200);

    printf("Starting Bach Toccata Test...\n");

    printf("Initializing LED...\n");
    ws2811_bind(pio1, 0, LED_PIN, &color);

    printf("Initializing Audio...\n");
    audio_i2s_init(pio0, 0, PIN_I2S_DIN, PIN_I2S_BCLK, 48000);

    printf("Initializing Synthesizer and Sequencer...\n");

    // 1. Initialize Synthesizer with Bach Toccata Organ Patch
    fm_voice_config_t bach_organ_patch;
    fm_patch_init_toccata_organ(&bach_organ_patch);

    printf("Initializing Synthesizer...\n");

    fm_synth_cfg_t synth_cfg = {0};
    synth_cfg.sampling_rate = 48000;
    synth_cfg.num_patches = 1;
    synth_cfg.patches = &bach_organ_patch;

    synth = fm_synth_init(&synth_cfg, synth_cfg.sampling_rate, synth_cfg.num_patches, synth_cfg.patches);

    if (!synth)
    {
        // Handle initialization failure (e.g., log error, halt, etc.)
        printf("Failed to initialize synthesizer.\n");
        return -1;
    }

    // 2. Initialize Sequencer and Load Bach Toccata Chords
    seq = fm_seq_init(NULL, synth, 120, 24);
    if (!seq)
    {
        // Handle initialization failure
        printf("Failed to initialize sequencer.\n");
        return -1;
    }
    fm_seq_load_bach_chords(seq);

    // 3. Start Playback
    printf("Starting sequencer playback...\n");
    fm_seq_play(seq);

    while (1)
    {
        tight_loop_contents();
    }

    return 0;
}