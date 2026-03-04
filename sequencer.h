#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <stdbool.h>
#include <stdint.h>

#define SEQUENCER_MAX_TRACKS 8
#define SEQUENCER_MAX_STEPS 64

typedef struct
{
    uint8_t note;
    uint8_t velocity;
    uint8_t gate;
    bool active;
} sequencer_step_t;

typedef struct
{
    sequencer_step_t steps[SEQUENCER_MAX_STEPS];
    uint8_t length;
    bool muted;
} sequencer_track_t;

typedef struct
{
    sequencer_track_t tracks[SEQUENCER_MAX_TRACKS];
    uint16_t bpm;
    uint8_t current_step;
    bool playing;
} sequencer_t;

void sequencer_init(sequencer_t *seq);
void sequencer_set_bpm(sequencer_t *seq, uint16_t bpm);
void sequencer_play(sequencer_t *seq);
void sequencer_pause(sequencer_t *seq);
void sequencer_stop(sequencer_t *seq);
void sequencer_advance_step(sequencer_t *seq);

bool sequencer_set_step(sequencer_t *seq, uint8_t track_index, uint8_t step_index, const sequencer_step_t *step);
const sequencer_step_t *sequencer_get_step(const sequencer_t *seq, uint8_t track_index);

#endif
