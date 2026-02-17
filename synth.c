#include "synth.h"
#include <string.h> // For memset/memcpy

// ... [Previous LUT and Helper functions remain unchanged] ...

static const sample_t DEFAULT_SINE[256] = {
    0, 804, 1607, 2410, 3211, 4011, 4807, 5601, 6392, 7179, 7961, 8739, 9511, 10278, 11038, 11792,
    12539, 13278, 14009, 14732, 15446, 16150, 16845, 17530, 18204, 18867, 19519, 20159, 20787, 21402, 22004, 22594,
    23169, 23731, 24278, 24811, 25329, 25831, 26318, 26789, 27244, 27683, 28105, 28510, 28897, 29268, 29621, 29955,
    30272, 30571, 30851, 31113, 31356, 31580, 31785, 31970, 32137, 32284, 32412, 32520, 32609, 32678, 32727, 32757,
    32767, 32757, 32727, 32678, 32609, 32520, 32412, 32284, 32137, 31970, 31785, 31580, 31356, 31113, 30851, 30571,
    30272, 29955, 29621, 29268, 28897, 28510, 28105, 27683, 27244, 26789, 26318, 25831, 25329, 24811, 24278, 23731,
    23169, 22594, 22004, 21402, 20787, 20159, 19519, 18867, 18204, 17530, 16845, 16150, 15446, 14732, 14009, 13278,
    12539, 11792, 11038, 10278, 9511, 8739, 7961, 7179, 6392, 5601, 4807, 4011, 3211, 2410, 1607, 804,
    0, -804, -1607, -2410, -3211, -4011, -4807, -5601, -6392, -7179, -7961, -8739, -9511, -10278, -11038, -11792,
    -12539, -13278, -14009, -14732, -15446, -16150, -16845, -17530, -18204, -18867, -19519, -20159, -20787, -21402, -22004, -22594,
    -23169, -23731, -24278, -24811, -25329, -25831, -26318, -26789, -27244, -27683, -28105, -28510, -28897, -29268, -29621, -29955,
    -30272, -30571, -30851, -31113, -31356, -31580, -31785, -31970, -32137, -32284, -32412, -32520, -32609, -32678, -32727, -32757,
    -32767, -32757, -32727, -32678, -32609, -32520, -32412, -32284, -32137, -31970, -31785, -31580, -31356, -31113, -30851, -30571,
    -30272, -29955, -29621, -29268, -28897, -28510, -28105, -27683, -27244, -26789, -26318, -25831, -25329, -24811, -24278, -23731,
    -23169, -22594, -22004, -21402, -20787, -20159, -19519, -18867, -18204, -17530, -16845, -16150, -15446, -14732, -14009, -13278,
    -12539, -11792, -11038, -10278, -9511, -8739, -7961, -7179, -6392, -5601, -4807, -4011, -3211, -2410, -1607, -804};

static void osc_render_lut(synth_osc_t *self, sample_t *buf, size_t len)
{
    uint32_t ph = self->phase;
    uint32_t inc = self->phase_inc;
    const sample_t *tbl = self->table;
    uint32_t mask = self->table_mask;

    // Quick and dirty shift calculation
    int shift = 24;
    if (mask == 511)
        shift = 23;
    else if (mask == 1023)
        shift = 22;

    for (size_t i = 0; i < len; i++)
    {
        uint32_t idx = (ph >> shift) & mask;
        buf[i] = tbl[idx];
        ph += inc;
    }
    self->phase = ph;
}

// ... [Primitives: init_table, set_freq, osc_process, etc. same as before] ...

void synth_osc_init_table(synth_osc_t *osc, const sample_t *table, size_t table_len, float freq_hz)
{
    osc->phase = 0;
    osc->table = table ? table : DEFAULT_SINE;
    osc->table_mask = table ? (table_len - 1) : 255;
    osc->func = osc_render_lut;
    synth_osc_set_freq(osc, freq_hz);
}

void synth_osc_set_freq(synth_osc_t *osc, float freq_hz)
{
    osc->phase_inc = (uint32_t)((freq_hz * 4294967296.0f) / SYNTH_SAMPLE_RATE);
}

void synth_osc_process(synth_osc_t *osc, sample_t *buf, size_t len)
{
    if (osc->func)
    {
        osc->func(osc, buf, len);
    }
}

void synth_adsr_init(synth_adsr_t *env, float a, float d, float s, float r)
{
    env->state = ADSR_IDLE;
    env->current_level = 0;
    env->gate = false;

    if (a <= 0.001f)
        a = 0.001f;
    if (d <= 0.001f)
        d = 0.001f;
    if (r <= 0.001f)
        r = 0.001f;

    env->attack_rate = (int32_t)(32767.0f / (a * SYNTH_SAMPLE_RATE));
    env->decay_rate = (int32_t)(32767.0f / (d * SYNTH_SAMPLE_RATE));
    env->release_rate = (int32_t)(32767.0f / (r * SYNTH_SAMPLE_RATE));
    env->sustain_level = (int32_t)(s * 32767.0f);
}

void synth_adsr_gate(synth_adsr_t *env, bool gate)
{
    env->gate = gate;
    if (gate)
    {
        if (env->state == ADSR_IDLE || env->state == ADSR_RELEASE)
        {
            env->state = ADSR_ATTACK;
        }
    }
    else
    {
        if (env->state != ADSR_IDLE)
        {
            env->state = ADSR_RELEASE;
        }
    }
}

void synth_adsr_process(synth_adsr_t *env, sample_t *buf, size_t len)
{
    int32_t lvl = env->current_level;
    for (size_t i = 0; i < len; i++)
    {
        switch (env->state)
        {
        case ADSR_IDLE:
            lvl = 0;
            break;
        case ADSR_ATTACK:
            lvl += env->attack_rate;
            if (lvl >= 32767)
            {
                lvl = 32767;
                env->state = ADSR_DECAY;
            }
            break;
        case ADSR_DECAY:
            lvl -= env->decay_rate;
            if (lvl <= env->sustain_level)
            {
                lvl = env->sustain_level;
                env->state = ADSR_SUSTAIN;
            }
            break;
        case ADSR_SUSTAIN:
            lvl = env->sustain_level;
            break;
        case ADSR_RELEASE:
            lvl -= env->release_rate;
            if (lvl <= 0)
            {
                lvl = 0;
                env->state = ADSR_IDLE;
            }
            break;
        }
        buf[i] = (sample_t)lvl;
    }
    env->current_level = lvl;
}

void synth_mix(sample_t *out, const sample_t *a, const sample_t *b, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        int32_t sum = (int32_t)a[i] + b[i];
        if (sum > 32767)
            sum = 32767;
        if (sum < -32768)
            sum = -32768;
        out[i] = (sample_t)sum;
    }
}

void synth_modulate(sample_t *out, const sample_t *carrier, const sample_t *modulator, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        int32_t prod = (int32_t)carrier[i] * (int32_t)modulator[i];
        out[i] = (sample_t)(prod >> 15);
    }
}

// ============================================================================
// BATCH PROCESSOR
// ============================================================================

void synth_process_batch(const synth_cmd_t *cmds, size_t cmd_count, sample_t **buffers, size_t buffer_count, size_t len)
{
    for (size_t i = 0; i < cmd_count; i++)
    {
        const synth_cmd_t *cmd = &cmds[i];

        switch (cmd->type)
        {
        case SYNTH_CMD_CLEAR:
            if (cmd->buf.dst < buffer_count)
            {
                memset(buffers[cmd->buf.dst], 0, len * sizeof(sample_t));
            }
            break;

        case SYNTH_CMD_COPY:
            if (cmd->buf.src < buffer_count && cmd->buf.dst < buffer_count)
            {
                memcpy(buffers[cmd->buf.dst], buffers[cmd->buf.src], len * sizeof(sample_t));
            }
            break;

        case SYNTH_CMD_OSC:
            if (cmd->osc.state && cmd->osc.out < buffer_count)
            {
                synth_osc_process(cmd->osc.state, buffers[cmd->osc.out], len);
            }
            break;

        case SYNTH_CMD_ADSR:
            if (cmd->adsr.state && cmd->adsr.out < buffer_count)
            {
                synth_adsr_process(cmd->adsr.state, buffers[cmd->adsr.out], len);
            }
            break;

        case SYNTH_CMD_MIX:
            if (cmd->mix.in_a < buffer_count && cmd->mix.in_b < buffer_count && cmd->mix.out < buffer_count)
            {
                synth_mix(buffers[cmd->mix.out], buffers[cmd->mix.in_a], buffers[cmd->mix.in_b], len);
            }
            break;

        case SYNTH_CMD_AMP:
            if (cmd->mix.in_a < buffer_count && cmd->mix.in_b < buffer_count && cmd->mix.out < buffer_count)
            {
                synth_modulate(buffers[cmd->mix.out], buffers[cmd->mix.in_a], buffers[cmd->mix.in_b], len);
            }
            break;

        default:
            break;
        }
    }
}
