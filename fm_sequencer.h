#ifndef FM_SEQUENCER_H
#define FM_SEQUENCER_H

#include "cfg.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "fm_synth.h"

#define FM_SEQ_MAX_TRACKS 16
#define FM_SEQ_PATTERN_LEN 64
#define FM_SEQ_MAX_VOICES 32

/**
 * @brief A single sequencer step
 */
typedef struct
{
    uint8_t note;     // MIDI note number (0 = rest)
    uint8_t velocity; // 0-127
    uint8_t gate;     // 0-100 (% of step duration)
    bool active;      // Step trigger flag
} fm_seq_step_t;

/**
 * @brief A single track
 */
typedef struct
{
    const fm_voice_config_t *patch;
    fm_seq_step_t steps[FM_SEQ_PATTERN_LEN];
    uint8_t channel_volume;
    uint8_t voice_mask;
    bool muted;
} fm_seq_track_t;

/**
 * @brief Global Sequencer State
 */
typedef struct
{
    fm_seq_track_t tracks[FM_SEQ_MAX_TRACKS];
    fm_voice_runtime_t *voices[FM_SEQ_MAX_VOICES]; // Array of pointers to your static voice pool

    uint16_t bpm;
    uint32_t sample_rate;

    /* Timing Logic (Fixed Point 16.16) */
    uint32_t tick_accumulator;
    uint32_t ticks_per_sample;

    uint32_t current_tick;
    uint16_t current_step;
    uint16_t ppqn;

    bool playing;
} fm_sequencer_t;

void fm_seq_init(fm_sequencer_t *seq, uint32_t sample_rate, uint16_t ppqn);
void fm_seq_set_bpm(fm_sequencer_t *seq, uint16_t bpm);
void fm_seq_process_block(fm_sequencer_t *seq, int16_t *buffer, uint16_t num_samples);
void fm_seq_sync_logic(fm_sequencer_t *seq);
fm_voice_runtime_t *fm_seq_allocate_voice(fm_sequencer_t *seq, const fm_voice_config_t *patch);

#endif // FM_SEQUENCER_H