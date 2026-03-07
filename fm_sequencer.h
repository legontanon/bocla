/**
 * Bocla - fm_sequencer.h
 * (c) 2024 Luis Enrique Garcia Ontanon
 * See LICENSE.txt for license details.
 */

/**
 * @file fm_sequencer.h
 * @brief Header for the FM Sequencer module of the Bocla Synthesizer.
 * This module provides a step sequencer that can be used to trigger
 * notes on the FM synthesizer. It supports multiple tracks, each
 * with its own set of steps, and it can be configured with different
 * BPM and PPQN settings. The sequencer is designed to be efficient and
 * suitable for real-time audio processing on a microcontroller.
 * The sequencer uses a fixed-point timing system to manage the
 * scheduling of note events,
 */

#ifndef FM_SEQUENCER_H
#define FM_SEQUENCER_H

#include "fm_synth.h"

#define FM_SEQ_MAX_TRACKS 16
#define FM_SEQ_DEFAULT_LOOP_LEN 64
#define FM_SEQ_MAX_VOICES 32

/**
 * @brief A single sequencer step definition
 */
typedef struct
{
    uint8_t note;            // MIDI note number (0 is rest)
    uint8_t velocity;        // 0-127
    uint8_t gate;            // 0-100 (% of step duration)
    uint8_t intensity;       // 0-127 for additional modulation control
    uint16_t duration_ticks; // Duration of the note in ticks (for sequencer use), or 0 for infinite until note off.
    bool active;             // Step trigger flag
} fm_seq_step_t;

/**
 * @brief Sequencer note event mapped to a timeline step index
 */
typedef struct
{
    uint32_t step_index;
    fm_seq_step_t step;
} fm_seq_event_t;

/**
 * @brief A single track definition
 */
typedef struct
{
    const fm_voice_config_t *patch;
    fm_seq_event_t *events;
    size_t event_count;
    size_t event_capacity;
    uint8_t channel_volume;
    uint8_t voice_mask;
    bool muted;
} fm_seq_track_t;

/**
 * @brief Global Sequencer definition
 */
typedef struct
{
    fm_synth_t *synth;
    fm_seq_track_t tracks[FM_SEQ_MAX_TRACKS];

    uint16_t bpm;

    /* Timing Logic (Fixed Point 16.16) */
    uint32_t tick_accumulator;
    uint32_t ticks_per_sample;

    uint32_t current_tick;
    uint32_t current_step;
    uint32_t loop_length_steps;
    uint16_t ppqn; // Pulses Per Quarter Note

    bool playing;
} fm_sequencer_cfg_t;

typedef struct
{
    fm_sequencer_cfg_t cfg;
    fm_voice_runtime_t *active_voices;
    fm_voice_runtime_t *inactive_voices;
} fm_sequencer_t;

fm_sequencer_t *fm_seq_init(fm_sequencer_cfg_t *seq_cfg, fm_synth_t *synth, uint16_t bpm, uint16_t ppqn);
void fm_seq_set_bpm(fm_sequencer_t *seq, uint16_t bpm);
void fm_seq_set_loop_length(fm_sequencer_t *seq, uint32_t loop_length_steps);
bool fm_seq_set_step(fm_sequencer_t *seq, uint8_t track_index, uint32_t step_index, const fm_seq_step_t *step);

/*
 * Real-time note:
 * - While playing, fm_seq_set_step will NOT grow memory capacity.
 * - If capacity is exhausted, it returns false instead of reallocating.
 */
void fm_seq_clear_track(fm_sequencer_t *seq, uint8_t track_index);

void fm_seq_play(fm_sequencer_t *seq);
void fm_seq_pause(fm_sequencer_t *seq);
void fm_seq_stop(fm_sequencer_t *seq);

size_t fm_seq_process_block(fm_sequencer_t *seq, int16_t *buffer, uint16_t num_samples);

#endif // FM_SEQUENCER_H