#include "filter.h"

static inline int16_t clamp_i16(int32_t val)
{
    if (val > 32767)
        return 32767;
    if (val < -32768)
        return -32768;
    return (int16_t)val;
}

static inline int32_t clamp_24_8(int32_t val)
{
    const int32_t max_val = 32767 << 8;
    const int32_t min_val = -32768 << 8;
    if (val > max_val)
        return max_val;
    if (val < min_val)
        return min_val;
    return val;
}

void filter_init(filter_state_t *state, const filter_config_t *cfg)
{
    if (!state)
        return;

    state->cfg = cfg;
    state->z1 = 0;
    state->z2 = 0;
    state->ext_state = NULL;
    state->process_block = NULL;
}

/**
 * @brief Chamberlin SVF (Low/High/Band Pass)
 */
static size_t process_svf(filter_state_t *state, int16_t *buffer, size_t num_samples, filter_type_t filter_type)
{
    if (!state || !state->cfg || !buffer || num_samples == 0)
        return 0;

    int32_t f = state->cfg->data.params.cutoff >> 3;
    int32_t q = 255 - (state->cfg->data.params.resonance << 1);
    if (q < 8)
        q = 8;

    int32_t low = state->z1;
    int32_t band = state->z2;

    for (size_t i = 0; i < num_samples; i++)
    {
        int32_t in = ((int32_t)buffer[i]) << 8;

        int32_t high = in - low - ((q * band) >> 8);

        band += (f * high) >> 8;
        band = clamp_24_8(band);

        low += (f * band) >> 8;
        low = clamp_24_8(low);

        int32_t out;
        if (filter_type == FILTER_TYPE_LOW_PASS)
            out = low;
        else if (filter_type == FILTER_TYPE_HIGH_PASS)
            out = high;
        else
            out = band;

        buffer[i] = clamp_i16(out >> 8);
    }

    state->z1 = low;
    state->z2 = band;

    return num_samples;
}

/**
 * @brief 1-Pole RC Low Pass (Exponential Moving Average)
 */
static size_t process_1pole_lpf(filter_state_t *state, int16_t *buffer, size_t num_samples)
{
    if (!state || !state->cfg || !buffer || num_samples == 0)
        return 0;

    int32_t alpha = state->cfg->data.simple.alpha;
    int32_t z1 = state->z1;

    for (size_t i = 0; i < num_samples; i++)
    {
        int32_t diff = buffer[i] - (z1 >> 15);
        z1 += diff * alpha;
        buffer[i] = (int16_t)(z1 >> 15);
    }

    state->z1 = z1;
    return num_samples;
}

/**
 * @brief 1-Pole 1-Zero High Pass (DC Blocker)
 */
static size_t process_dc_block(filter_state_t *state, int16_t *buffer, size_t num_samples)
{
    if (!state || !state->cfg || !buffer || num_samples == 0)
        return 0;

    int32_t shift = state->cfg->data.simple.alpha;
    int32_t z1 = state->z1;
    int32_t z2 = state->z2;

    for (size_t i = 0; i < num_samples; i++)
    {
        int32_t in = buffer[i];
        int32_t delta_x = in - z2;
        z2 = in;

        z1 = (delta_x << 14) + z1 - (z1 >> shift);
        buffer[i] = clamp_i16(z1 >> 14);
    }

    state->z1 = z1;
    state->z2 = z2;
    return num_samples;
}

/**
 * @brief Unskew ASRC via Linear Interpolation
 */
static size_t process_unskew(filter_state_t *state, int16_t *buffer, size_t num_samples)
{
    if (!state || !state->ext_state || !buffer || num_samples == 0)
        return 0;

    filter_unskew_state_t *us = (filter_unskew_state_t *)state->ext_state;
    if (!us->cb)
        return 0;

    // Prime the pipeline on the very first execution
    if (us->in_count == 0 && us->out_count == 0)
    {
        int16_t prime[2] = {0, 0};
        us->cb(prime, 2, us->cb_data);
        us->last_sample = prime[0];
        us->curr_sample = prime[1];
        us->in_count += 2;
        if (us->phase_step == 0)
            us->phase_step = 32768; // Defend against divide-by-zero math
    }

    size_t samples_rendered = 0;
    int16_t scratch[64]; // Strictly bound local stack usage

    while (samples_rendered < num_samples)
    {
        size_t chunk = num_samples - samples_rendered;
        if (chunk > 64)
            chunk = 64;

        // Simulate phase accumulation to determine exact fetch requirement.
        // Shrink the chunk dynamically if fetch demands exceed scratch capacity (e.g. fast-forwarding).
        uint32_t temp_phase = us->phase;
        uint32_t needed = 0;
        size_t actual_chunk = 0;

        for (size_t i = 0; i < chunk; i++)
        {
            uint32_t next_needed = needed;
            uint32_t sim_phase = temp_phase;
            while (sim_phase >= 32768)
            {
                next_needed++;
                sim_phase -= 32768;
            }
            if (next_needed > 64)
                break;

            needed = next_needed;
            temp_phase = sim_phase + us->phase_step;
            actual_chunk++;
        }

        chunk = actual_chunk;

        if (needed > 0)
        {
            us->cb(scratch, needed, us->cb_data);
            us->in_count += needed;
        }

        uint32_t src_idx = 0;
        for (size_t i = 0; i < chunk; i++)
        {
            while (us->phase >= 32768)
            {
                us->last_sample = us->curr_sample;
                us->curr_sample = scratch[src_idx++];
                us->phase -= 32768;
            }

            int32_t frac = us->phase;
            int32_t diff = us->curr_sample - us->last_sample;
            buffer[samples_rendered + i] = us->last_sample + ((diff * frac) >> 15);

            us->phase += us->phase_step;
        }

        us->out_count += chunk;
        samples_rendered += chunk;
    }

    return num_samples;
}

// Wrapper functions for manual execution
size_t low_pass_process_block(filter_state_t *state, int16_t *buffer, size_t num_samples)
{
    return process_svf(state, buffer, num_samples, FILTER_TYPE_LOW_PASS);
}

size_t high_pass_process_block(filter_state_t *state, int16_t *buffer, size_t num_samples)
{
    return process_svf(state, buffer, num_samples, FILTER_TYPE_HIGH_PASS);
}

size_t band_pass_process_block(filter_state_t *state, int16_t *buffer, size_t num_samples)
{
    return process_svf(state, buffer, num_samples, FILTER_TYPE_BAND_PASS);
}

size_t unskew_process_block(filter_state_t *state, int16_t *buffer, size_t num_samples)
{
    return process_unskew(state, buffer, num_samples);
}

// Master Dispatcher
void filter_process_block(filter_state_t *state, int16_t *buffer, size_t num_samples)
{
    if (!state || !buffer || num_samples == 0)
        return;

    if (state->process_block)
    {
        state->process_block(state, buffer, num_samples);
    }
    else if (state->cfg)
    {
        switch (state->cfg->type)
        {
        case FILTER_TYPE_LOW_PASS:
        case FILTER_TYPE_HIGH_PASS:
        case FILTER_TYPE_BAND_PASS:
            process_svf(state, buffer, num_samples, state->cfg->type);
            break;
        case FILTER_TYPE_1POLE_LPF:
            process_1pole_lpf(state, buffer, num_samples);
            break;
        case FILTER_TYPE_DC_BLOCK:
            process_dc_block(state, buffer, num_samples);
            break;
        case FILTER_TYPE_UNSKEW:
            process_unskew(state, buffer, num_samples);
            break;
        default:
            break;
        }
    }
}