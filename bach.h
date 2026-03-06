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

    patch->ops[0].freq_mult = 0x0100;
    patch->ops[0].attack = 2000;
    patch->ops[0].decay = 0;
    patch->ops[0].sustain = 32767;
    patch->ops[0].release = 800;
    patch->ops[0].volume = 12000;

    patch->ops[1].freq_mult = 0x0200;
    patch->ops[1].attack = 2200;
    patch->ops[1].decay = 0;
    patch->ops[1].sustain = 32767;
    patch->ops[1].release = 800;
    patch->ops[1].volume = 8000;

    patch->ops[2].freq_mult = 0x0300;
    patch->ops[2].attack = 2400;
    patch->ops[2].decay = 0;
    patch->ops[2].sustain = 32767;
    patch->ops[2].release = 800;
    patch->ops[2].volume = 6000;

    patch->ops[3].freq_mult = 0x0400;
    patch->ops[3].attack = 2600;
    patch->ops[3].decay = 0;
    patch->ops[3].sustain = 32767;
    patch->ops[3].release = 800;
    patch->ops[3].volume = 4000;
}

/**
 * @brief Loads the opening chords of the Toccata.
 * Spreads harmony across 3 tracks to simulate polyphony.
 */
static inline void fm_seq_load_bach_chords(fm_sequencer_t *seq)
{
    if (!seq)
        return;

    for (uint8_t t = 0; t < 3; ++t)
    {
        fm_seq_clear_track(seq, t);
    }

    (void)fm_seq_set_step(seq, 0, 0, &(fm_seq_step_t){61, 127, 95, 0, 48, true});
    (void)fm_seq_set_step(seq, 0, 1, &(fm_seq_step_t){62, 127, 95, 0, 96, true});

    (void)fm_seq_set_step(seq, 1, 0, &(fm_seq_step_t){55, 120, 95, 0, 48, true});
    (void)fm_seq_set_step(seq, 1, 1, &(fm_seq_step_t){57, 120, 95, 0, 96, true});

    (void)fm_seq_set_step(seq, 2, 0, &(fm_seq_step_t){49, 127, 95, 0, 48, true});
    (void)fm_seq_set_step(seq, 2, 1, &(fm_seq_step_t){50, 127, 95, 0, 96, true});
}

/**
 * @brief Toccata and Fugue in D Minor - opening motive.
 */
static inline void fm_seq_load_bach_toccata(fm_sequencer_t *seq, uint8_t track_idx)
{
    if (!seq)
        return;

    const fm_seq_step_t toccata_motive[] = {
        {81, 127, 80, 0, 6, true},
        {80, 127, 80, 0, 6, true},
        {81, 127, 95, 0, 48, true},
        {0, 0, 0, 0, 12, false},
        {74, 127, 80, 0, 6, true},
        {72, 127, 80, 0, 6, true},
        {70, 127, 80, 0, 6, true},
        {69, 127, 80, 0, 6, true},
        {67, 127, 80, 0, 6, true},
        {65, 127, 80, 0, 6, true},
        {64, 127, 80, 0, 6, true},
        {62, 127, 95, 0, 48, true},
        {0, 0, 0, 0, 12, false},
        {45, 127, 100, 0, 96, true},
    };

    uint32_t num_steps = (uint32_t)(sizeof(toccata_motive) / sizeof(toccata_motive[0]));
    for (uint32_t i = 0; i < num_steps; ++i)
    {
        (void)fm_seq_set_step(seq, track_idx, i, &toccata_motive[i]);
    }
}

/**
 * @brief Extended Toccata - continuation with running sixteenth passages.
 */
static inline void fm_seq_load_bach_toccata_extended(fm_sequencer_t *seq, uint8_t track_idx)
{
    if (!seq)
        return;

    const fm_seq_step_t toccata_extended[] = {
        {62, 120, 80, 0, 6, true},
        {64, 120, 80, 0, 6, true},
        {65, 120, 80, 0, 6, true},
        {67, 120, 80, 0, 6, true},
        {69, 120, 80, 0, 6, true},
        {71, 120, 80, 0, 6, true},
        {72, 120, 80, 0, 6, true},
        {74, 127, 95, 0, 12, true},
        {0, 0, 0, 0, 12, false},
        {74, 127, 90, 0, 6, true},
        {72, 127, 90, 0, 6, true},
        {70, 127, 90, 0, 6, true},
        {69, 127, 90, 0, 6, true},
        {67, 127, 95, 0, 48, true},
        {0, 0, 0, 0, 24, false},
    };

    uint32_t num_steps = (uint32_t)(sizeof(toccata_extended) / sizeof(toccata_extended[0]));
    for (uint32_t i = 0; i < num_steps; ++i)
    {
        (void)fm_seq_set_step(seq, track_idx, i, &toccata_extended[i]);
    }
}

#endif // FM_BACH_H
