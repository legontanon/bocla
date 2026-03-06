#ifndef FM_PATCHES_H
#define FM_PATCHES_H

#include "fm_synth.h"
#include <string.h>

/*
 * Envelope Rate approximations for 48kHz / 16.16 fixed point:
 * target_level = volume << 16
 * inc_per_sample = rate << 8
 * * Rates:
 * 8000 ~ 15ms (Click/Snap)
 * 1000 ~ 120ms (Fast)
 * 200  ~ 600ms (Medium)
 * 50   ~ 2.5s (Slow)
 * * Max Carrier Volume: 32767 (Unity gain)
 * Modulator Volume: 500 - 8000 (Controls modulation index/brightness)
 */

/**
 * @brief DX7-style Electric Piano (Tine + Body)
 * Uses Algorithm 4 (2-carrier: Op1+Op2 -> Out, Op3+Op4 -> Out)
 */
static inline void fm_patch_init_epiano(fm_voice_config_t *patch)
{
    memset(patch, 0, sizeof(fm_voice_config_t));
    patch->algorithm = 4;
    patch->n_ops = 4;

    // Carrier 1 (Body)
    patch->ops[0].freq_mult = 0x0100; // 1.0
    patch->ops[0].attack = 8000;
    patch->ops[0].decay = 100;
    patch->ops[0].sustain = 16000;
    patch->ops[0].release = 100;
    patch->ops[0].volume = 32767;

    // Modulator 1 (Body grit)
    patch->ops[1].freq_mult = 0x0100; // 1.0
    patch->ops[1].attack = 8000;
    patch->ops[1].decay = 200;
    patch->ops[1].sustain = 0;
    patch->ops[1].release = 200;
    patch->ops[1].volume = 4000; // Moderate modulation

    // Carrier 2 (Tine percussive transient)
    patch->ops[2].freq_mult = 0x0100; // 1.0
    patch->ops[2].attack = 8000;
    patch->ops[2].decay = 400; // Fast decay
    patch->ops[2].sustain = 0; // No sustain
    patch->ops[2].release = 400;
    patch->ops[2].volume = 20000;

    // Modulator 2 (Tine high frequency metallic)
    patch->ops[3].freq_mult = 0x0E00; // 14.0
    patch->ops[3].attack = 8000;
    patch->ops[3].decay = 800; // Very fast decay
    patch->ops[3].sustain = 0;
    patch->ops[3].release = 800;
    patch->ops[3].volume = 6000; // High mod index for bright ping
}

/**
 * @brief Solid punchy FM Bass (Lately Bass style)
 * Uses Algorithm 0 (Series: Op4 -> Op3 -> Op2 -> Op1 -> Out)
 * We only use 2 operators to save cycles, zeroing the rest.
 */
static inline void fm_patch_init_bass(fm_voice_config_t *patch)
{
    memset(patch, 0, sizeof(fm_voice_config_t));
    patch->algorithm = 0;
    patch->n_ops = 2; // Only run the first two ops

    // Carrier
    patch->ops[0].freq_mult = 0x0100; // 1.0
    patch->ops[0].attack = 8000;      // Instant
    patch->ops[0].decay = 150;
    patch->ops[0].sustain = 8000; // Low sustain for thump
    patch->ops[0].release = 300;
    patch->ops[0].volume = 32767;

    // Modulator (Sub-octave bite)
    patch->ops[1].freq_mult = 0x0080; // 0.5 (Sub octave)
    patch->ops[1].attack = 8000;
    patch->ops[1].decay = 300; // Decays faster than carrier
    patch->ops[1].sustain = 0; // Pluck only
    patch->ops[1].release = 300;
    patch->ops[1].volume = 9000; // Heavy modulation for buzz
    patch->ops[1].feedback = 5;  // Self-feedback for saw-like wave
}

/**
 * @brief Vangelis-style Brass
 * Uses Algorithm 4 (Two parallel stacks)
 */
static inline void fm_patch_init_brass(fm_voice_config_t *patch)
{
    memset(patch, 0, sizeof(fm_voice_config_t));
    patch->algorithm = 4;
    patch->n_ops = 4;

    // Carrier 1 (Fundamental)
    patch->ops[0].freq_mult = 0x0100; // 1.0
    patch->ops[0].attack = 300;       // Slight swell
    patch->ops[0].decay = 50;
    patch->ops[0].sustain = 24000;
    patch->ops[0].release = 80;
    patch->ops[0].volume = 20000;

    // Modulator 1 (Filter sweep effect)
    patch->ops[1].freq_mult = 0x0100; // 1.0
    patch->ops[1].attack = 200;       // Mod swells slightly slower than carrier
    patch->ops[1].decay = 100;
    patch->ops[1].sustain = 3000;
    patch->ops[1].release = 80;
    patch->ops[1].volume = 7000;
    patch->ops[1].feedback = 4;

    // Carrier 2 (Detuned/Higher harmonic)
    patch->ops[2].freq_mult = 0x0101; // ~1.004 (Slight detune)
    patch->ops[2].attack = 300;
    patch->ops[2].decay = 50;
    patch->ops[2].sustain = 24000;
    patch->ops[2].release = 80;
    patch->ops[2].volume = 18000;

    // Modulator 2
    patch->ops[3].freq_mult = 0x0200; // 2.0
    patch->ops[3].attack = 400;
    patch->ops[3].decay = 100;
    patch->ops[3].sustain = 0;
    patch->ops[3].release = 80;
    patch->ops[3].volume = 4000;
}

/**
 * @brief Inharmonic Bell / Tubular Chime
 * Uses Algorithm 4
 */
static inline void fm_patch_init_bell(fm_voice_config_t *patch)
{
    memset(patch, 0, sizeof(fm_voice_config_t));
    patch->algorithm = 4;
    patch->n_ops = 4;

    // Carrier 1 (Fundamental ring)
    patch->ops[0].freq_mult = 0x0100; // 1.0
    patch->ops[0].attack = 8000;
    patch->ops[0].decay = 40; // Very slow decay
    patch->ops[0].sustain = 0;
    patch->ops[0].release = 20; // Long ring out
    patch->ops[0].volume = 32767;

    // Modulator 1 (Inharmonic splash)
    patch->ops[1].freq_mult = 0x0380; // 3.5
    patch->ops[1].attack = 8000;
    patch->ops[1].decay = 300; // Fast splash
    patch->ops[1].sustain = 0;
    patch->ops[1].release = 200;
    patch->ops[1].volume = 8000;

    // Carrier 2 (Secondary ring)
    patch->ops[2].freq_mult = 0x0200; // 2.0
    patch->ops[2].attack = 8000;
    patch->ops[2].decay = 60;
    patch->ops[2].sustain = 0;
    patch->ops[2].release = 30;
    patch->ops[2].volume = 16000;

    // Modulator 2 (Inharmonic overtone)
    patch->ops[3].freq_mult = 0x016A; // ~1.414 (sqrt 2)
    patch->ops[3].attack = 8000;
    patch->ops[3].decay = 150;
    patch->ops[3].sustain = 0;
    patch->ops[3].release = 100;
    patch->ops[3].volume = 5000;
}

/**
 * @brief TR-808 Style Kick Drum
 * Fake pitch-envelope transient using a fast FM click.
 * Alg 0 (Only 2 ops needed).
 */
static inline void fm_patch_init_808_kick(fm_voice_config_t *patch)
{
    memset(patch, 0, sizeof(fm_voice_config_t));
    patch->algorithm = 0;
    patch->n_ops = 2;

    // Carrier (The Boom)
    patch->ops[0].freq_mult = 0x0100; // 1.0 (Play this at low MIDI notes, e.g. 50Hz)
    patch->ops[0].attack = 8000;      // Instant
    patch->ops[0].decay = 100;        // Long decay
    patch->ops[0].sustain = 0;
    patch->ops[0].release = 100;
    patch->ops[0].volume = 32767;

    // Modulator (The Click/Transient)
    patch->ops[1].freq_mult = 0x0100; // 1.0
    patch->ops[1].attack = 8000;
    patch->ops[1].decay = 2000; // Extremely fast decay
    patch->ops[1].sustain = 0;
    patch->ops[1].release = 2000;
    patch->ops[1].volume = 18000; // High index for brief pitch-shift illusion
}

/**
 * @brief TR-808 Style Snare
 * Alg 4: Carrier 1 (Body) + Carrier 2 (Noise Tail)
 */
static inline void fm_patch_init_808_snare(fm_voice_config_t *patch)
{
    memset(patch, 0, sizeof(fm_voice_config_t));
    patch->algorithm = 4;
    patch->n_ops = 4;

    // Carrier 1 (Tonal Body - ~180-250Hz target)
    patch->ops[0].freq_mult = 0x0100;
    patch->ops[0].attack = 8000;
    patch->ops[0].decay = 400;
    patch->ops[0].sustain = 0;
    patch->ops[0].release = 400;
    patch->ops[0].volume = 32767;

    // Modulator 1 (Body snap)
    patch->ops[1].freq_mult = 0x0100;
    patch->ops[1].attack = 8000;
    patch->ops[1].decay = 800;
    patch->ops[1].sustain = 0;
    patch->ops[1].release = 800;
    patch->ops[1].volume = 6000;

    // Carrier 2 (Snappy Noise Tail)
    patch->ops[2].freq_mult = 0x0400; // 4.0
    patch->ops[2].attack = 8000;
    patch->ops[2].decay = 250; // Slower decay than body for the "rattle"
    patch->ops[2].sustain = 0;
    patch->ops[2].release = 250;
    patch->ops[2].volume = 20000;

    // Modulator 2 (Noise generator via self-feedback destruction)
    patch->ops[3].freq_mult = 0x0F00; // 15.0 (High freq alias)
    patch->ops[3].attack = 8000;
    patch->ops[3].decay = 250;
    patch->ops[3].sustain = 0;
    patch->ops[3].release = 250;
    patch->ops[3].volume = 32000; // Max modulation into Carrier 2
    patch->ops[3].feedback = 7;   // Max feedback for pseudo-white noise
}

/**
 * @brief TR-808 Style Closed Hi-Hat
 * Alg 4: Two inharmonic parallel stacks pushed into noise.
 */
static inline void fm_patch_init_808_hat(fm_voice_config_t *patch)
{
    memset(patch, 0, sizeof(fm_voice_config_t));
    patch->algorithm = 4;
    patch->n_ops = 4;

    // Carrier 1 (Metallic ring A)
    patch->ops[0].freq_mult = 0x0300; // 3.0
    patch->ops[0].attack = 8000;
    patch->ops[0].decay = 1000; // Very short (Closed hat)
    patch->ops[0].sustain = 0;
    patch->ops[0].release = 1000;
    patch->ops[0].volume = 25000;

    // Modulator 1
    patch->ops[1].freq_mult = 0x0433; // ~4.2
    patch->ops[1].attack = 8000;
    patch->ops[1].decay = 1000;
    patch->ops[1].sustain = 0;
    patch->ops[1].release = 1000;
    patch->ops[1].volume = 20000;

    // Carrier 2 (Metallic ring B)
    patch->ops[2].freq_mult = 0x0520; // ~5.1
    patch->ops[2].attack = 8000;
    patch->ops[2].decay = 800;
    patch->ops[2].sustain = 0;
    patch->ops[2].release = 800;
    patch->ops[2].volume = 25000;

    // Modulator 2 (Dirt/Hash)
    patch->ops[3].freq_mult = 0x06A0; // ~6.6
    patch->ops[3].attack = 8000;
    patch->ops[3].decay = 800;
    patch->ops[3].sustain = 0;
    patch->ops[3].release = 800;
    patch->ops[3].volume = 30000;
    patch->ops[3].feedback = 6; // Hash it up
}

/**
 * @brief Sine Organ Major 7th Chord
 * Alg 7 (4 Parallel Carriers). Zero modulation.
 * Freq Multipliers in 8.8 fixed point:
 * Root = 1.0 (0x0100)
 * Maj 3rd = ~1.2599 (0x0143)
 * Perf 5th = 1.5 (0x0180) - Using Just Intonation for smoother blending
 * Maj 7th = ~1.8877 (0x01E3)
 */
static inline void fm_patch_init_maj7_organ(fm_voice_config_t *patch)
{
    memset(patch, 0, sizeof(fm_voice_config_t));
    patch->algorithm = 7;
    patch->n_ops = 4;

    const uint32_t mults[4] = {0x0100, 0x0143, 0x0180, 0x01E3};

    for (int i = 0; i < 4; i++)
    {
        patch->ops[i].freq_mult = mults[i];
        patch->ops[i].attack = 1000; // Soft organ attack
        patch->ops[i].decay = 0;
        patch->ops[i].sustain = 32767; // Full sustain
        patch->ops[i].release = 300;
        patch->ops[i].volume = 8190; // 32767 / 4 to prevent mix clipping
    }
}

/**
 * @brief Sine Organ Minor 7th Chord
 * Alg 7 (4 Parallel Carriers).
 * Min 3rd = ~1.1892 (0x0130)
 * Min 7th = ~1.7818 (0x01C8)
 */
static inline void fm_patch_init_min7_organ(fm_voice_config_t *patch)
{
    memset(patch, 0, sizeof(fm_voice_config_t));
    patch->algorithm = 7;
    patch->n_ops = 4;

    const uint32_t mults[4] = {0x0100, 0x0130, 0x0180, 0x01C8};

    for (int i = 0; i < 4; i++)
    {
        patch->ops[i].freq_mult = mults[i];
        patch->ops[i].attack = 1000;
        patch->ops[i].decay = 0;
        patch->ops[i].sustain = 32767;
        patch->ops[i].release = 300;
        patch->ops[i].volume = 8190;
    }
}

/**
 * @brief FM Power Chord (Root + 5th)
 * Alg 4: Carrier 1 (Root) + Carrier 2 (5th) with independent modulators.
 */
static inline void fm_patch_init_power_chord(fm_voice_config_t *patch)
{
    memset(patch, 0, sizeof(fm_voice_config_t));
    patch->algorithm = 4;
    patch->n_ops = 4;

    // Carrier 1 (Root)
    patch->ops[0].freq_mult = 0x0100;
    patch->ops[0].attack = 2000;
    patch->ops[0].decay = 300;
    patch->ops[0].sustain = 20000;
    patch->ops[0].release = 500;
    patch->ops[0].volume = 16000; // Half volume

    // Modulator 1 (Root timbre)
    patch->ops[1].freq_mult = 0x0100;
    patch->ops[1].attack = 2000;
    patch->ops[1].decay = 500;
    patch->ops[1].sustain = 5000;
    patch->ops[1].release = 500;
    patch->ops[1].volume = 8000;
    patch->ops[1].feedback = 3;

    // Carrier 2 (Perfect 5th)
    patch->ops[2].freq_mult = 0x0180; // 1.5
    patch->ops[2].attack = 2000;
    patch->ops[2].decay = 300;
    patch->ops[2].sustain = 20000;
    patch->ops[2].release = 500;
    patch->ops[2].volume = 16000; // Half volume

    // Modulator 2 (5th timbre)
    patch->ops[3].freq_mult = 0x0180;
    patch->ops[3].attack = 2000;
    patch->ops[3].decay = 500;
    patch->ops[3].sustain = 5000;
    patch->ops[3].release = 500;
    patch->ops[3].volume = 8000;
    patch->ops[3].feedback = 3;
}

#endif // FM_PATCHES_H