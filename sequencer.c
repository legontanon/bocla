#include "sequencer.h"

#include <string.h>

static uint8_t clamp_u8(uint8_t value, uint8_t min_value, uint8_t max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

void sequencer_init(sequencer_t *seq)
{
    if (!seq)
    {
        return;
    }

    memset(seq, 0, sizeof(*seq));
    seq->bpm = 120;

    for (uint8_t track = 0; track < SEQUENCER_MAX_TRACKS; ++track)
    {
        seq->tracks[track].length = SEQUENCER_MAX_STEPS;
    }
}

void sequencer_set_bpm(sequencer_t *seq, uint16_t bpm)
{
    if (!seq)
    {
        return;
    }

    if (bpm < 20)
    {
        seq->bpm = 20;
    }
    else if (bpm > 300)
    {
        seq->bpm = 300;
    }
    else
    {
        seq->bpm = bpm;
    }
}

void sequencer_play(sequencer_t *seq)
{
    if (!seq)
    {
        return;
    }

    seq->playing = true;
}

void sequencer_pause(sequencer_t *seq)
{
    if (!seq)
    {
        return;
    }

    seq->playing = false;
}

void sequencer_stop(sequencer_t *seq)
{
    if (!seq)
    {
        return;
    }

    seq->playing = false;
    seq->current_step = 0;
}

bool sequencer_set_step(sequencer_t *seq, uint8_t track_index, uint8_t step_index, const sequencer_step_t *step)
{
    if (!seq || !step)
    {
        return false;
    }

    if (track_index >= SEQUENCER_MAX_TRACKS || step_index >= SEQUENCER_MAX_STEPS)
    {
        return false;
    }

    sequencer_track_t *track = &seq->tracks[track_index];
    track->steps[step_index] = *step;
    track->steps[step_index].velocity = clamp_u8(track->steps[step_index].velocity, 0, 127);
    track->steps[step_index].gate = clamp_u8(track->steps[step_index].gate, 0, 100);
    return true;
}

const sequencer_step_t *sequencer_get_step(const sequencer_t *seq, uint8_t track_index)
{
    if (!seq || track_index >= SEQUENCER_MAX_TRACKS)
    {
        return NULL;
    }

    const sequencer_track_t *track = &seq->tracks[track_index];
    uint8_t length = track->length == 0 ? 1 : track->length;
    uint8_t step_index = (uint8_t)(seq->current_step % length);
    return &track->steps[step_index];
}

void sequencer_advance_step(sequencer_t *seq)
{
    if (!seq || !seq->playing)
    {
        return;
    }

    uint8_t max_length = 1;
    for (uint8_t track = 0; track < SEQUENCER_MAX_TRACKS; ++track)
    {
        uint8_t length = seq->tracks[track].length;
        if (length > max_length)
        {
            max_length = length;
        }
    }

    if (max_length == 0)
    {
        max_length = 1;
    }

    seq->current_step = (uint8_t)((seq->current_step + 1) % max_length);
}
