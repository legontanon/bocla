/**
 * Bocla - fm_sequencer.c
 * (c) 2024 Luis Enrique Garcia Ontanon
 * See LICENSE.txt for license details.
 */

#include "fm_sequencer.h"
#include "fm_synth.h"

#include <stddef.h>
#include <stdlib.h>
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

static bool ensure_event_capacity(fm_seq_track_t *track, size_t required_count)
{
    if (!track)
        return false;

    if (track->event_capacity >= required_count)
        return true;

    size_t new_capacity = (track->event_capacity == 0) ? 8 : track->event_capacity;
    while (new_capacity < required_count)
    {
        new_capacity *= 2;
    }

    fm_seq_event_t *new_events = (fm_seq_event_t *)realloc(track->events, new_capacity * sizeof(fm_seq_event_t));
    if (!new_events)
        return false;

    track->events = new_events;
    track->event_capacity = new_capacity;
    return true;
}

static size_t lower_bound_event_index(const fm_seq_track_t *track, uint32_t step_index)
{
    size_t left = 0;
    size_t right = track ? track->event_count : 0;

    while (left < right)
    {
        size_t mid = left + ((right - left) / 2);
        if (track->events[mid].step_index < step_index)
        {
            left = mid + 1;
        }
        else
        {
            right = mid;
        }
    }

    return left;
}

static uint8_t resolve_patch_index(const fm_sequencer_t *seq, const fm_seq_track_t *track, uint8_t track_index)
{
    if (!seq || !seq->cfg.synth || seq->cfg.synth->cfg.num_patches == 0)
        return 0;

    if (track && track->patch)
    {
        for (size_t i = 0; i < seq->cfg.synth->cfg.num_patches; ++i)
        {
            if (&seq->cfg.synth->cfg.patches[i] == track->patch)
                return (uint8_t)i;
        }
    }

    return (uint8_t)(track_index % seq->cfg.synth->cfg.num_patches);
}

fm_sequencer_t *fm_seq_init(fm_sequencer_cfg_t *seq_cfg, fm_synth_t *synth, uint16_t bpm, uint16_t ppqn)
{
    fm_sequencer_t *seq = &g_fm_sequencer;

    for (uint8_t track = 0; track < FM_SEQ_MAX_TRACKS; ++track)
    {
        free(seq->cfg.tracks[track].events);
        seq->cfg.tracks[track].events = NULL;
    }

    memset(seq, 0, sizeof(*seq));

    seq->cfg.synth = synth;
    seq->cfg.loop_length_steps = FM_SEQ_DEFAULT_LOOP_LEN;

    if (seq_cfg && seq_cfg->loop_length_steps > 0)
    {
        seq->cfg.loop_length_steps = seq_cfg->loop_length_steps;
    }

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
        seq->cfg.tracks[track].events = NULL;
        seq->cfg.tracks[track].event_count = 0;
        seq->cfg.tracks[track].event_capacity = 0;
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

bool fm_seq_set_step(fm_sequencer_t *seq, uint8_t track_index, uint32_t step_index, const fm_seq_step_t *step)
{
    if (!seq || !step || track_index >= FM_SEQ_MAX_TRACKS)
        return false;

    fm_seq_track_t *track = &seq->cfg.tracks[track_index];

    size_t insert_idx = lower_bound_event_index(track, step_index);
    bool exists = (insert_idx < track->event_count && track->events[insert_idx].step_index == step_index);

    if (exists)
    {
        if (!step->active)
        {
            memmove(&track->events[insert_idx],
                    &track->events[insert_idx + 1],
                    (track->event_count - insert_idx - 1) * sizeof(fm_seq_event_t));
            track->event_count--;
            return true;
        }

        track->events[insert_idx].step.note = step->note;
        track->events[insert_idx].step.velocity = clamp_u8(step->velocity, 0, 127);
        track->events[insert_idx].step.gate = clamp_u8(step->gate, 0, 100);
        track->events[insert_idx].step.intensity = clamp_u8(step->intensity, 0, 127);
        track->events[insert_idx].step.duration_ticks = step->duration_ticks;
        track->events[insert_idx].step.active = true;
        return true;
    }

    if (!step->active)
        return true;

    // Real-time safety: never grow allocations while sequencer is playing.
    // This prevents malloc/realloc activity from edit calls during audio rendering.
    if (seq->cfg.playing && track->event_count >= track->event_capacity)
        return false;

    if (!ensure_event_capacity(track, track->event_count + 1))
        return false;

    if (insert_idx < track->event_count)
    {
        memmove(&track->events[insert_idx + 1],
                &track->events[insert_idx],
                (track->event_count - insert_idx) * sizeof(fm_seq_event_t));
    }

    fm_seq_event_t *event = &track->events[insert_idx];
    track->event_count++;
    event->step_index = step_index;
    event->step.note = step->note;
    event->step.velocity = clamp_u8(step->velocity, 0, 127);
    event->step.gate = clamp_u8(step->gate, 0, 100);
    event->step.intensity = clamp_u8(step->intensity, 0, 127);
    event->step.duration_ticks = step->duration_ticks;
    event->step.active = true;

    return true;
}

void fm_seq_set_loop_length(fm_sequencer_t *seq, uint32_t loop_length_steps)
{
    if (!seq)
        return;

    seq->cfg.loop_length_steps = (loop_length_steps == 0) ? FM_SEQ_DEFAULT_LOOP_LEN : loop_length_steps;
    if (seq->cfg.current_step >= seq->cfg.loop_length_steps)
    {
        seq->cfg.current_step %= seq->cfg.loop_length_steps;
    }
}

void fm_seq_clear_track(fm_sequencer_t *seq, uint8_t track_index)
{
    if (!seq || track_index >= FM_SEQ_MAX_TRACKS)
        return;

    fm_seq_track_t *track = &seq->cfg.tracks[track_index];
    free(track->events);
    track->events = NULL;
    track->event_count = 0;
    track->event_capacity = 0;
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
            (void)fm_synth_render_block(seq->cfg.synth, buffer + samples_rendered, chunk);
            samples_rendered += chunk;
            seq->cfg.tick_accumulator += chunk;
        }

        // 2. Check if we reached a sequencer tick boundary
        if (seq->cfg.tick_accumulator >= seq->cfg.ticks_per_sample)
        {
            seq->cfg.tick_accumulator = 0; // Reset for next tick

            // Process note offs by decrementing per-voice tick counters.
            for (uint8_t i = 0; i < FM_MAX_VOICES; ++i)
            {
                fm_voice_runtime_t *voice = &seq->cfg.synth->voices[i];
                if (!voice->active || voice->tick_counter == 0)
                    continue;

                voice->tick_counter--;
                if (voice->tick_counter == 0)
                {
                    fm_synth_note_off(seq->cfg.synth, i);
                }
            }

            // Trigger note events at each step boundary.
            if (seq->cfg.current_tick == 0)
            {
                for (uint8_t t = 0; t < FM_SEQ_MAX_TRACKS; ++t)
                {
                    fm_seq_track_t *track = &seq->cfg.tracks[t];
                    if (track->muted || track->event_count == 0)
                        continue;

                    size_t start_idx = lower_bound_event_index(track, seq->cfg.current_step);
                    for (size_t e = start_idx; e < track->event_count; ++e)
                    {
                        fm_seq_event_t *event = &track->events[e];
                        if (event->step_index != seq->cfg.current_step)
                            break;

                        fm_seq_step_t *step = &event->step;
                        if (!step->active || step->note == 0)
                            continue;

                        int8_t voice_index = fm_synth_get_idle_voice_index(seq->cfg.synth);
                        if (voice_index < 0)
                            continue;

                        uint8_t patch_index = resolve_patch_index(seq, track, t);
                        uint32_t freq = g_midi_freq_table[step->note & 0x7F];

                        fm_synth_note_on(seq->cfg.synth,
                                         (uint8_t)voice_index,
                                         patch_index,
                                         freq,
                                         step->velocity,
                                         step->intensity,
                                         step->duration_ticks);

                        if (step->duration_ticks > 0)
                        {
                            uint32_t active_ticks = step->duration_ticks;
                            if (step->gate > 0 && step->gate <= 100)
                            {
                                active_ticks = (active_ticks * step->gate) / 100;
                            }
                            seq->cfg.synth->voices[(uint8_t)voice_index].tick_counter = (active_ticks > 0) ? active_ticks : 1;
                        }
                        else
                        {
                            // 0 duration means sustain until explicit note-off.
                            seq->cfg.synth->voices[(uint8_t)voice_index].tick_counter = 0;
                        }
                    }
                }
            }

            // Advance ticks and steps
            seq->cfg.current_tick++;
            if (seq->cfg.current_tick >= seq->cfg.ppqn)
            {
                seq->cfg.current_tick = 0;

                uint32_t loop_len = (seq->cfg.loop_length_steps == 0) ? FM_SEQ_DEFAULT_LOOP_LEN : seq->cfg.loop_length_steps;
                seq->cfg.current_step = (seq->cfg.current_step + 1) % loop_len;
            }
        }
    }

    return num_samples;
}