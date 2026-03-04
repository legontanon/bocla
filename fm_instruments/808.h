/**
 * @file 808.h
 * @brief 808 Bass Drum Synthesizer
 * This is a simple implementation of a classic 808-style bass drum using a sine wave generator and an amplitude envelope.
 * The sound is created by generating a sine wave at a specific frequency and applying an ADSR envelope to shape the sound over time.
 * The parameters can be adjusted to create variations of the bass drum sound.
 */

#ifndef _808_H
#define _808_H

#include "fm_synth.h"

// these are inline functions that set the parameters of a patch struct to create a basic 808 bass drum sound.

inline void fm_808_clap(fm_voice_config_t *patch)
{
    patch->algorithm = 0;
    patch->lfo_depth = 0;
    patch->n_ops = 2;
    patch->ops[0].freq_mult = 1;
    patch->ops[0].attack = 0;
    patch->ops[0].decay = 100;
    patch->ops[0].sustain = 0;
    patch->ops[0].release = 200;
    patch->ops[0].volume = 100;
    patch->ops[0].feedback = 0;
    patch->ops[1].freq_mult = 1;
    patch->ops[1].attack = 0;
    patch->ops[1].decay = 150;
    patch->ops[1].sustain = 0;
    patch->ops[1].release = 300;
    patch->ops[1].volume = 80;
    patch->ops[1].feedback = 0;
}

inline void fm_808_kick(fm_voice_config_t *patch)
{
    patch->algorithm = 0;
    patch->lfo_depth = 0;
    patch->n_ops = 2;
    patch->ops[0].freq_mult = 1;
    patch->ops[0].attack = 0;
    patch->ops[0].decay = 200;
    patch->ops[0].sustain = 0;
    patch->ops[0].release = 300;
    patch->ops[0].volume = 100;
    patch->ops[0].feedback = 0;
    patch->ops[1].freq_mult = 1;
    patch->ops[1].attack = 0;
    patch->ops[1].decay = 150;
    patch->ops[1].sustain = 0;
    patch->ops[1].release = 250;
    patch->ops[1].volume = 80;
    patch->ops[1].feedback = 0;
}

inline void fm_808_snare(fm_voice_config_t *patch)
{
    patch->algorithm = 0;
    patch->lfo_depth = 0;
    patch->n_ops = 2;
    patch->ops[0].freq_mult = 1;
    patch->ops[0].attack = 0;
    patch->ops[0].decay = 150;
    patch->ops[0].sustain = 0;
    patch->ops[0].release = 250;
    patch->ops[0].volume = 100;
    patch->ops[0].feedback = 0;
    patch->ops[1].freq_mult = 1;
    patch->ops[1].attack = 0;
    patch->ops[1].decay = 100;
    patch->ops[1].sustain = 0;
    patch->ops[1].release = 200;
    patch->ops[1].volume = 80;
    patch->ops[1].feedback = 0;
}
#endif // _808_H
