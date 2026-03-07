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

static volatile uint32_t status_color = 0;
static int16_t audio_block[128];

int main()
{
    // 1. Initialize Synthesizer with Bach Toccata Organ Patch
    fm_voice_config_t bach_organ_patch;
    fm_patch_init_toccata_organ(&bach_organ_patch);

    fm_synth_cfg_t synth_cfg = {0};
    synth_cfg.sampling_rate = 48000;
    synth_cfg.num_patches = 1;
    synth_cfg.patches = &bach_organ_patch;

    fm_synth_t *synth = fm_synth_init(&synth_cfg, synth_cfg.sampling_rate, synth_cfg.num_patches, synth_cfg.patches);
    if (!synth)
    {
        // Handle initialization failure (e.g., log error, halt, etc.)
        return -1;
    }

    // 2. Initialize Sequencer and Load Bach Toccata Chords
    fm_sequencer_t *seq = fm_seq_init(NULL, synth, 120, 24);
    if (!seq)
    {
        // Handle initialization failure
        return -1;
    }
    fm_seq_load_bach_chords(seq);

    // 3. Start Playback
    fm_seq_play(seq);

    // 4. Main Loop: Update LED Visualizer based on Synth Activity
    while (1)
    {
        (void)fm_seq_process_block(seq, audio_block, 128); // Process a block of audio (128 samples)
        led_visualizer_update(synth, &status_color);
        // Optional: Add a small sleep here to prevent maxing out the CPU in this test loop
        // sleep_ms(10);
    }

    return 0;
}