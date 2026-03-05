#include "mixer.h"

#include <string.h>

static int16_t clamp_i16(int32_t value)
{
    if (value > 32767)
    {
        return 32767;
    }
    if (value < -32768)
    {
        return -32768;
    }
    return (int16_t)value;
}

void mixer_init(mixer_t *mixer, mixer_cfg_t *cfg)
{
    if (!mixer || !cfg)
    {
        return;
    }

    mixer->cfg = *cfg;
    if (mixer->mix_buffer && mixer->cfg.block_size)
    {
        memset(mixer->mix_buffer, 0, mixer->cfg.block_size * sizeof(int16_t));
    }
}

bool mixer_register_channel(mixer_t *mixer, uint8_t channel_index, mixer_input_cb_t input_cb, void *cb_data)
{
    if (!mixer || !mixer->cfg.channels || channel_index >= mixer->cfg.n_channels)
    {
        return false;
    }

    mixer_channel_t *channel = &mixer->cfg.channels[channel_index];
    channel->input_cb = input_cb;
    channel->cb_data = cb_data;
    channel->active = input_cb != NULL;
    if (channel->gain == 0)
    {
        channel->gain = 255;
    }
    return true;
}

void mixer_set_channel_gain(mixer_t *mixer, uint8_t channel_index, uint8_t gain)
{
    if (!mixer || !mixer->cfg.channels || channel_index >= mixer->cfg.n_channels)
    {
        return;
    }

    mixer->cfg.channels[channel_index].gain = gain;
}

void mixer_set_channel_active(mixer_t *mixer, uint8_t channel_index, bool active)
{
    if (!mixer || !mixer->cfg.channels || channel_index >= mixer->cfg.n_channels)
    {
        return;
    }

    mixer->cfg.channels[channel_index].active = active;
}

void mixer_process_block(mixer_t *mixer, int16_t *output_buffer, size_t num_samples)
{
    if (!mixer || !output_buffer || !mixer->cfg.channels)
    {
        return;
    }

    memset(output_buffer, 0, num_samples * sizeof(int16_t));

    int16_t *work_buffer = mixer->mix_buffer ? mixer->mix_buffer : output_buffer;

    for (uint16_t channel_index = 0; channel_index < mixer->cfg.n_channels; ++channel_index)
    {
        mixer_channel_t *channel = &mixer->cfg.channels[channel_index];
        if (!channel->active || !channel->input_cb)
        {
            continue;
        }

        memset(work_buffer, 0, num_samples * sizeof(int16_t));
        channel->input_cb(work_buffer, num_samples, channel->cb_data);

        for (size_t sample_index = 0; sample_index < num_samples; ++sample_index)
        {
            int32_t scaled = ((int32_t)work_buffer[sample_index] * (int32_t)channel->gain) / 255;
            int32_t mixed = (int32_t)output_buffer[sample_index] + scaled;
            output_buffer[sample_index] = clamp_i16(mixed);
        }
    }
}
