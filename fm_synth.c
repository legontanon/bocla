/**
 * Bocla - fm_synth.c
 * (c) 2024 Luis Enrique Garcia Ontanon
 * See LICENSE.txt for license details.
 */

#include "fm_synth.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#define FM_SCRATCH_X_ATTR __attribute__((section(".scratch_x.fm_sine")))

/**
 * @brief Pre-computed sine table for quarter wave (0 to pi/2).
 * Resolution: 1024 entries covering 0° to 90°.
 * Maps to 16-bit signed values (±32767).
 */
const int16_t FM_SCRATCH_X_ATTR fm_sine_table[FM_SINE_QUARTER_SIZE] = {
#include "fm_sine_quarter.inc"
};

/**
 * @brief Initialize the FM synthesizer with a configuration.
 */
fm_synth_t *fm_synth_init(fm_synth_cfg_t *cfg, uint32_t sampling_rate, size_t num_patches, fm_voice_config_t *patches)
{
    if (!cfg || !patches || num_patches == 0 || sampling_rate == 0)
        return NULL;

    fm_synth_t *synth = malloc(sizeof(fm_synth_t));
    if (!synth)
        return NULL;

    memset(synth, 0, sizeof(fm_synth_t));

    synth->cfg.sampling_rate = sampling_rate;
    synth->cfg.tick_rate = 0; // Will be set by caller if needed
    synth->cfg.patches = patches;
    synth->cfg.num_patches = num_patches;

    // Initialize all voices to idle
    for (uint8_t i = 0; i < FM_MAX_VOICES; i++)
    {
        synth->voices[i].active = 0;
        synth->voices[i].patch = NULL;
        for (uint8_t j = 0; j < FM_OPS_PER_VOICE; j++)
        {
            synth->voices[i].op_state[j].env_state = FM_ENV_IDLE;
            synth->voices[i].op_state[j].env_level = 0;
        }
    }

    return synth;
}

/**
 * @brief Fast 32-bit phase to 16-bit sine lookup.
 * Phase [0, 2^32-1] maps to 4096 entries (12-bit).
 * Exploits quarter-wave symmetry.
 */
static inline int16_t lookup_sine(uint32_t phase)
{
    uint16_t idx = (phase >> FM_PHASE_SHIFT) & 0x0FFF;
    uint8_t quadrant = idx >> 10;
    idx &= 0x03FF;

    if (quadrant & 1)
    {
        idx = 1023 - idx;
    }

    int16_t val = fm_sine_table[idx];
    return (quadrant & 2) ? -val : val;
}

/**
 * @brief Process a single operator's envelope.
 * 16.16 fixed point linear ramps.
 */
static inline int32_t process_envelope(fm_operator_runtime_t *rt, const fm_operator_config_t *cfg)
{
    int32_t target;
    switch (rt->env_state)
    {
    case FM_ENV_ATTACK:
        rt->env_level += (cfg->attack << 8); // Scale rate to 16.16
        target = ((int32_t)cfg->volume) << 16;
        if (rt->env_level >= target)
        {
            rt->env_level = target;
            rt->env_state = FM_ENV_DECAY;
        }
        break;
    case FM_ENV_DECAY:
        rt->env_level -= (cfg->decay << 8);
        target = ((int32_t)cfg->sustain) << 16;
        if (rt->env_level <= target)
        {
            rt->env_level = target;
            rt->env_state = FM_ENV_SUSTAIN;
        }
        break;
    case FM_ENV_SUSTAIN:
        // Holds level, decay handled in release
        break;
    case FM_ENV_RELEASE:
        rt->env_level -= (cfg->release << 8);
        if (rt->env_level <= 0)
        {
            rt->env_level = 0;
            rt->env_state = FM_ENV_IDLE;
        }
        break;
    case FM_ENV_IDLE:
    default:
        return 0;
    }
    return rt->env_level >> 16; // Return 16-bit multiplier
}

/**
 * @brief Process a single FM operator
 */
static inline int16_t process_operator(fm_operator_runtime_t *rt, const fm_operator_config_t *cfg, int32_t modulation)
{
    if (rt->env_state == FM_ENV_IDLE)
        return 0;

    int32_t env_amp = process_envelope(rt, cfg);

    // Apply feedback if configured (usually only OP1)
    int32_t phase_mod = modulation;
    if (cfg->feedback > 0)
    {
        phase_mod += ((rt->last_out[0] + rt->last_out[1]) >> (9 - cfg->feedback)) << FM_PHASE_SHIFT;
    }

    int16_t osc_out = lookup_sine(rt->phase + phase_mod);
    rt->phase += rt->phase_inc;

    // Apply envelope amplitude (Fixed point 16-bit * 16-bit -> 32-bit >> 16)
    int16_t out = (int16_t)(((int32_t)osc_out * env_amp) >> 16);

    // Update feedback history
    rt->last_out[1] = rt->last_out[0];
    rt->last_out[0] = out;

    return out;
}

/**
 * @brief Get the index of an idle voice.
 *
 * @param synth The synthesizer instance.
 * @return The index of an idle voice, or -1 if none are available.
 */
int8_t fm_synth_get_idle_voice_index(fm_synth_t *synth)
{
    for (uint8_t i = 0; i < FM_MAX_VOICES; i++)
    {
        if (!synth->voices[i].active)
            return i;
    }
    return -1;
}

void fm_synth_note_on(fm_synth_t *synth, uint8_t voice_index, uint8_t patch_idx, uint32_t freq, uint8_t velocity, uint8_t intensity, uint16_t duration_ticks)
{
    if (voice_index >= FM_MAX_VOICES || patch_idx >= synth->cfg.num_patches)
        return;

    fm_voice_runtime_t *voice = &synth->voices[voice_index];
    const fm_voice_config_t *patch = &synth->cfg.patches[patch_idx];

    voice->patch = patch;
    voice->base_freq = freq;
    voice->velocity = velocity;
    voice->active = 1;
    voice->n_ops = patch->n_ops;

    // Calculate base phase increment: (freq * 2^32) / sample_rate
    // Using 64-bit math to prevent overflow during setup. This is fine since it's outside the render loop.
    uint32_t base_inc = (uint32_t)(((uint64_t)freq << 32) / synth->cfg.sampling_rate);

    for (uint8_t i = 0; i < voice->n_ops; i++)
    {
        fm_operator_runtime_t *op = &voice->op_state[i];
        const fm_operator_config_t *op_cfg = &patch->ops[i];

        op->phase = 0;
        op->last_out[0] = 0;
        op->last_out[1] = 0;

        // freq_mult is 8.8 fixed point
        op->phase_inc = (base_inc >> 8) * op_cfg->freq_mult;

        op->env_state = FM_ENV_ATTACK;
        op->env_level = 0;
    }
}

void fm_synth_note_off(fm_synth_t *synth, uint8_t voice_index)
{
    if (voice_index >= FM_MAX_VOICES)
        return;
    fm_voice_runtime_t *voice = &synth->voices[voice_index];

    for (uint8_t i = 0; i < voice->n_ops; i++)
    {
        if (voice->op_state[i].env_state != FM_ENV_IDLE)
        {
            voice->op_state[i].env_state = FM_ENV_RELEASE;
        }
    }
}

size_t fm_synth_render_block(fm_synth_t *synth, int16_t *buffer, uint16_t num_samples)
{
    memset(buffer, 0, num_samples * sizeof(int16_t));
    uint8_t active_voices_count = 0;

    for (uint8_t v = 0; v < FM_MAX_VOICES; v++)
    {
        fm_voice_runtime_t *voice = &synth->voices[v];
        if (!voice->active)
            continue;

        active_voices_count++;
        const fm_voice_config_t *patch = voice->patch;
        bool voice_still_alive = false;

        for (uint16_t s = 0; s < num_samples; s++)
        {
            int16_t op_out[FM_OPS_PER_VOICE] = {0};

            // Depending on the algorithm, wire the operators.
            // Phase modulation needs to be shifted up to match the 32-bit phase space.
            // Using a standard routing approach (Alg 0: 4->3->2->1, Alg 7: Parallel)
            switch (patch->algorithm)
            {
            case 0: // Series (4 -> 3 -> 2 -> 1)
                op_out[3] = process_operator(&voice->op_state[3], &patch->ops[3], 0);
                op_out[2] = process_operator(&voice->op_state[2], &patch->ops[2], ((int32_t)op_out[3]) << (FM_PHASE_SHIFT - 4));
                op_out[1] = process_operator(&voice->op_state[1], &patch->ops[1], ((int32_t)op_out[2]) << (FM_PHASE_SHIFT - 4));
                op_out[0] = process_operator(&voice->op_state[0], &patch->ops[0], ((int32_t)op_out[1]) << (FM_PHASE_SHIFT - 4));
                break;
            case 4: // 2-carrier (2->1 + 4->3)
                op_out[3] = process_operator(&voice->op_state[3], &patch->ops[3], 0);
                op_out[2] = process_operator(&voice->op_state[2], &patch->ops[2], ((int32_t)op_out[3]) << (FM_PHASE_SHIFT - 4));
                op_out[1] = process_operator(&voice->op_state[1], &patch->ops[1], 0);
                op_out[0] = process_operator(&voice->op_state[0], &patch->ops[0], ((int32_t)op_out[1]) << (FM_PHASE_SHIFT - 4));
                op_out[0] = (op_out[0] >> 1) + (op_out[2] >> 1);
                break;
            case 7: // Parallel (1 + 2 + 3 + 4)
            default:
                op_out[0] = process_operator(&voice->op_state[0], &patch->ops[0], 0);
                op_out[1] = process_operator(&voice->op_state[1], &patch->ops[1], 0);
                op_out[2] = process_operator(&voice->op_state[2], &patch->ops[2], 0);
                op_out[3] = process_operator(&voice->op_state[3], &patch->ops[3], 0);
                op_out[0] = (op_out[0] >> 2) + (op_out[1] >> 2) + (op_out[2] >> 2) + (op_out[3] >> 2);
                break;
            }

            // Accumulate into main buffer
            // In a real application, you'd apply global velocity scaling and hard-clipping here.
            int32_t mix = buffer[s] + ((op_out[0] * voice->velocity) >> 7);

            // Saturation clipper to avoid harsh wrap-around distortion
            if (mix > 32767)
                mix = 32767;
            else if (mix < -32768)
                mix = -32768;
            buffer[s] = (int16_t)mix;

            // Check if voice is still alive (any operator not idle)
            for (uint8_t i = 0; i < voice->n_ops; i++)
            {
                if (voice->op_state[i].env_state != FM_ENV_IDLE)
                {
                    voice_still_alive = true;
                    break;
                }
            }
        }

        if (!voice_still_alive)
        {
            voice->active = 0;
        }
    }

    return active_voices_count == 0 ? num_samples : 0;
}