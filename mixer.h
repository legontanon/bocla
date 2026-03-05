#ifndef MIXER_H
#define MIXER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*mixer_input_cb_t)(int16_t *buffer, size_t num_samples, void *user_data);

typedef struct {
    uint8_t gain;
    uint8_t active;

    mixer_input_cb_t input_cb;
    void *cb_data;
} mixer_channel_t;

typedef struct
{
    uint16_t sample_rate;
    uint16_t block_size;
    uint16_t n_channels;
    mixer_channel_t *channels;
} mixer_cfg_t;


typedef struct
{
    mixer_cfg_t cfg;
    int16_t *mix_buffer; // Interleaved stereo buffer for mixing
} mixer_t;

void mixer_init(mixer_t *mixer, mixer_cfg_t *cfg);
bool mixer_register_channel(mixer_t *mixer, uint8_t channel_index, mixer_input_cb_t input_cb, void *cb_data);
void mixer_set_channel_gain(mixer_t *mixer, uint8_t channel_index, uint8_t gain);
void mixer_set_channel_active(mixer_t *mixer, uint8_t channel_index, bool active);
void mixer_process_block(mixer_t *mixer, int16_t *output_buffer, size_t num_samples);

#endif // MIXER_H