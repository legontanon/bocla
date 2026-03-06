#include "fm_sequencer.h"
#include "fm_synth.h"

#include <stddef.h>
#include <string.h>

static fm_sequencer_t g_fm_sequencer;

/* Pre-calculated MIDI Note to Hz lookup table (Standard 440Hz tuning).
 * Completely eliminates the need for math.h and float calculations on the Pico. */
static const uint16_t g_midi_freq_table[128] = {
    8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15,
    16, 17, 18, 19, 20, 22, 23, 24, 26, 28, 29, 31,
    33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62,
    65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123,
    131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
    262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
    523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
    1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
    2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
    4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
    8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544};

static uint8_t clamp_u8(uint8_t value, uint8_t min_value, uint8_t max_value)
{
    if (value < min_value)
        return min_value;
    if (value > max_value)
        return max_value;
    return value;
}

fm_sequencer_t *fm_seq_init(fm_sequencer_cfg_t *seq_cfg, fm_synth_t *synth, uint16_t bpm, uint16_t ppqn)
{
    fm_sequencer_t *seq = &g_fm_sequencer;

    if (!seq_cfg)
    {
        memset(&seq->cfg, 0, sizeof(seq->cfg));
    }
    else
    {
        seq->cfg = *seq_cfg;
    }

    seq->cfg.synth = synth;
    seq->cfg.bpm = bpm;
    seq->cfg.ppqn = ppqn;

    if (seq->cfg.bpm < 20)
        seq->cfg.bpm = 20;
    else if (seq->cfg.bpm > 300)
        seq->cfg.bpm = 300;

    uint32_t sample_rate = (synth != NULL) ? synth->cfg.sampling_rate : 48000u;
    uint32_t safe_ppqn = seq->cfg.ppqn == 0 ? 24u : seq->cfg.ppqn;

    // Note: The variable is named ticks_per_sample, but the math calculates samples_per_tick.
    // This is mathematically correct for sub-block scheduling.
    seq->cfg.ticks_per_sample = (sample_rate * 60u) / (safe_ppqn * seq->cfg.bpm);
    if (seq->cfg.ticks_per_sample == 0)
    {
        seq->cfg.ticks_per_sample = 1;
    }

    seq->cfg.tick_accumulator = 0;
    seq->cfg.current_tick = 0;
    seq->cfg.current_step = 0;
    seq->cfg.playing = false;

    seq->active_voices = NULL;
    seq->inactive_voices = NULL;

    for (uint8_t track = 0; track < FM_SEQ_MAX_TRACKS; ++track)
    {
        seq->cfg.tracks[track].channel_volume = 255;
        seq->cfg.tracks[track].voice_mask = 0xFF; // All voices enabled by default
        seq->cfg.tracks[track].muted = false;
        for (uint8_t step = 0; step < FM_SEQ_PATTERN_LEN; ++step)
        {
            seq->cfg.tracks[track].steps[step].note = 0;
            seq->cfg.tracks[track].steps[step].velocity = 0;
            seq->cfg.tracks[track].steps[step].gate = 0;
            seq->cfg.tracks[track].steps[step].intensity = 0;
            seq->cfg.tracks[track].steps[step].duration_ticks = 0;
            seq->cfg.tracks[track].steps[step].active = false;
        }
    }

    return seq;
}

void fm_seq_set_bpm(fm_sequencer_t *seq, uint16_t bpm)
{
    if (!seq)
        return;

    if (bpm < 20)
        seq->cfg.bpm = 20;
    else if (bpm > 300)
        seq->cfg.bpm = 300;
    else
        seq->cfg.bpm = bpm;

    uint32_t sample_rate = (seq->cfg.synth != NULL) ? seq->cfg.synth->cfg.sampling_rate : 48000u;
    uint32_t safe_ppqn = seq->cfg.ppqn == 0 ? 24u : seq->cfg.ppqn;

    seq->cfg.ticks_per_sample = (sample_rate * 60u) / (safe_ppqn * seq->cfg.bpm);
    if (seq->cfg.ticks_per_sample == 0)
        seq->cfg.ticks_per_sample = 1;
}

void fm_seq_play(fm_sequencer_t *seq)
{
    if (seq)
        seq->cfg.playing = true;
}
void fm_seq_pause(fm_sequencer_t *seq)
{
    if (seq)
        seq->cfg.playing = false;
}
void fm_seq_stop(fm_sequencer_t *seq)
{
    if (seq)
    {
        seq->cfg.playing = false;
        seq->cfg.current_step = 0;
        seq->cfg.tick_accumulator = 0;
        seq->cfg.current_tick = 0;
    }
}

bool fm_seq_set_step(fm_sequencer_t *seq, uint8_t track_index, uint8_t step_index, const fm_seq_step_t *step)
{
    if (!seq || !step || track_index >= FM_SEQ_MAX_TRACKS || step_index >= FM_SEQ_PATTERN_LEN)
        return false;

    fm_seq_track_t *track = &seq->cfg.tracks[track_index];
    track->steps[step_index].note = step->note;
    track->steps[step_index].velocity = clamp_u8(step->velocity, 0, 127);
    track->steps[step_index].gate = clamp_u8(step->gate, 0, 100);
    track->steps[step_index].intensity = clamp_u8(step->intensity, 0, 127);
    track->steps[step_index].duration_ticks = step->duration_ticks;
    track->steps[step_index].active = step->active;

    return true;
}

size_t fm_seq_process_block(fm_sequencer_t *seq, int16_t *buffer, uint16_t num_samples)
{
    if (!seq || !buffer || num_samples == 0)
        return 0;

    if (!seq->cfg.playing || !seq->cfg.synth)
    {
        memset(buffer, 0, num_samples * sizeof(int16_t));
        return num_samples;
    }

    uint16_t samples_rendered = 0;

    // For now: render silence and advance sequencer state
    // TODO: When fm_synth backend is implemented, integrate voice allocation and rendering
    memset(buffer, 0, num_samples * sizeof(int16_t));

    // Sub-block rendering: Interleave sample generation with sequencer ticks
    while (samples_rendered < num_samples)
    {
        uint32_t samples_until_tick = seq->cfg.ticks_per_sample - seq->cfg.tick_accumulator;
        uint16_t chunk = num_samples - samples_rendered;

        if (chunk > samples_until_tick)
        {
            chunk = samples_until_tick;
        }

        if (chunk > 0)
        {
            samples_rendered += chunk;
            seq->cfg.tick_accumulator += chunk;
        }

        // 2. Check if we reached a sequencer tick boundary
        if (seq->cfg.tick_accumulator >= seq->cfg.ticks_per_sample)
        {
            seq->cfg.tick_accumulator = 0; // Reset for next tick

            // Advance ticks and steps
            seq->cfg.current_tick++;
            if (seq->cfg.current_tick >= seq->cfg.ppqn)
            {
                seq->cfg.current_tick = 0;
                seq->cfg.current_step = (seq->cfg.current_step + 1) % FM_SEQ_PATTERN_LEN;
            }
        }
    }

    return num_samples;
}