#ifndef FM_BACH_H
#define FM_BACH_H

#include "fm_synth.h"
#include "fm_sequencer.h"
#include <string.h>

/**
 * @brief Baroque Pipe Organ (Toccata style)
 * Uses Algorithm 7 (4 Parallel Carriers) to simulate additive synthesis
 * of different organ stops (ranks).
 */
static inline void fm_patch_init_toccata_organ(fm_voice_config_t *patch)
{
    memset(patch, 0, sizeof(fm_voice_config_t));
    patch->algorithm = 7;
    patch->n_ops = 4;

    // Rank 1: 8' Principal (Fundamental)
    patch->ops[0].freq_mult = 0x0100; // 1.0
    patch->ops[0].attack = 2000;
    patch->ops[0].decay = 0;
    patch->ops[0].sustain = 32767;
    patch->ops[0].release = 800;
    patch->ops[0].volume = 12000;

    // Rank 2: 4' Octave
    patch->ops[1].freq_mult = 0x0200; // 2.0
    patch->ops[1].attack = 2200;
    patch->ops[1].decay = 0;
    patch->ops[1].sustain = 32767;
    patch->ops[1].release = 800;
    patch->ops[1].volume = 8000;

    // Rank 3: 2 2/3' Quinte (12th)
    patch->ops[2].freq_mult = 0x0300; // 3.0
    patch->ops[2].attack = 2400;
    patch->ops[2].decay = 0;
    patch->ops[2].sustain = 32767;
    patch->ops[2].release = 800;
    patch->ops[2].volume = 6000;

    // Rank 4: 2' Super Octave (15th)
    patch->ops[3].freq_mult = 0x0400; // 4.0
    patch->ops[3].attack = 2600;
    patch->ops[3].decay = 0;
    patch->ops[3].sustain = 32767;
    patch->ops[3].release = 800;
    patch->ops[3].volume = 4000;
}

/**
 * @brief Loads the full opening chords of the Toccata.
 * Spreads the harmony across 3 tracks to simulate polyphony.
 * Sequence: Dim7 (C# dim) -> D Minor.
 */
static inline void fm_seq_load_bach_chords(fm_sequencer_t *seq)
{
    // Clear first three tracks
    for (int t = 0; t < 3; t++)
        memset(seq->cfg.tracks[t].steps, 0, sizeof(fm_seq_step_t) * FM_SEQ_PATTERN_LEN);

    // Timing (PPQN=24): 48 = Half Note, 96 = Whole Note

    // Track 0: Top Voice
    seq->cfg.tracks[0].steps[0] = (fm_seq_step_t){61, 127, 95, 0, 48, true}; // C#4
    seq->cfg.tracks[0].steps[1] = (fm_seq_step_t){62, 127, 95, 0, 96, true}; // D4

    // Track 1: Middle Voice
    seq->cfg.tracks[1].steps[0] = (fm_seq_step_t){55, 120, 95, 0, 48, true}; // G3
    seq->cfg.tracks[1].steps[1] = (fm_seq_step_t){57, 120, 95, 0, 96, true}; // A3

    // Track 2: Bottom Voice / Pedal
    seq->cfg.tracks[2].steps[0] = (fm_seq_step_t){49, 127, 95, 0, 48, true}; // C#3
    seq->cfg.tracks[2].steps[1] = (fm_seq_step_t){50, 127, 95, 0, 96, true}; // D3
}

/**
 * @brief Toccata and Fugue in D Minor - Iconic Opening Motive
 * Notes mapped to MIDI: A4=69, D5=74, etc.
 * Duration: Based on PPQN=24. 6=16th note, 12=8th note, 48=Half note.
 */
static inline void fm_seq_load_bach_toccata(fm_sequencer_t *seq, uint8_t track_idx)
{
    // {note, velocity, gate, intensity, duration_ticks, active}
    const fm_seq_step_t toccata_motive[] = {
        {81, 127, 80, 0, 6, true},   // A5 (16th)
        {80, 127, 80, 0, 6, true},   // G#5 (16th)
        {81, 127, 95, 0, 48, true},  // A5 (Half Note/Fermata)
        {0, 0, 0, 0, 12, false},     // Rest
        {74, 127, 80, 0, 6, true},   // D5
        {72, 127, 80, 0, 6, true},   // C5
        {70, 127, 80, 0, 6, true},   // Bb4
        {69, 127, 80, 0, 6, true},   // A4
        {67, 127, 80, 0, 6, true},   // G4
        {65, 127, 80, 0, 6, true},   // F4
        {64, 127, 80, 0, 6, true},   // E4
        {62, 127, 95, 0, 48, true},  // D4 (Hold)
        {0, 0, 0, 0, 12, false},     // Rest
        {45, 127, 100, 0, 96, true}, // A2 (Pedal Whole Note)
    };

    uint16_t num_steps = sizeof(toccata_motive) / sizeof(fm_seq_step_t);
    for (uint16_t i = 0; i < num_steps && i < FM_SEQ_PATTERN_LEN; i++)
    {
        fm_seq_set_step(seq, track_idx, i, &toccata_motive[i]);
    }
}

#endif // FM_BACH_H