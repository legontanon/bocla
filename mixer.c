#include "mixer.h"

#include <string.h>

static inline int16_t clamp_i16(int32_t value)
{
    if (value > 32767)
        return 32767;
    if (value < -32768)
        return -32768;
    return (int16_t)value;
}

void mixer_init(mixer_t *mixer, mixer_cfg_t *cfg)
{
    if (!mixer || !cfg)
        return;

    mixer->cfg = *cfg;
    if (mixer->mix_buffer && mixer->cfg.block_size)
    {
        memset(mixer->mix_buffer, 0, mixer->cfg.block_size * sizeof(int16_t));
    }
}

bool mixer_register_channel(mixer_t *mixer, uint8_t channel_index, mixer_input_cb_t input_cb, void *cb_data)
{
    if (!mixer || !mixer->cfg.channels || channel_index >= mixer->cfg.n_channels)
        return false;

    mixer_channel_t *channel = &mixer->cfg.channels[channel_index];
    channel->input_cb = input_cb;
    channel->cb_data = cb_data;
    channel->active = input_cb != NULL;

    // Default to 255 (maximum for uint8_t gain value)
    if (channel->gain == 0)
        channel->gain = 255;

    return true;
}

bool mixer_register_channels(mixer_t *mixer, size_t n_channels, mixer_channel_t *channels)
{
    if (!mixer || !mixer->cfg.channels || n_channels > mixer->cfg.n_channels || !channels)
        return false;

    for (size_t i = 0; i < n_channels; ++i)
    {
        mixer_channel_t *src = &channels[i];
        mixer_channel_t *dst = &mixer->cfg.channels[i];
        dst->input_cb = src->input_cb;
        dst->cb_data = src->cb_data;
        dst->gain = src->gain ? src->gain : 256; // Default to 256 (Unity)
        dst->active = src->input_cb != NULL;
    }
    return true;
}

void mixer_set_channel_gain(mixer_t *mixer, uint8_t channel_index, uint8_t gain)
{
    if (!mixer || !mixer->cfg.channels || channel_index >= mixer->cfg.n_channels)
        return;
    mixer->cfg.channels[channel_index].gain = gain;
}

void mixer_set_channel_active(mixer_t *mixer, uint8_t channel_index, bool active)
{
    if (!mixer || !mixer->cfg.channels || channel_index >= mixer->cfg.n_channels)
        return;
    mixer->cfg.channels[channel_index].active = active;
}

size_t mixer_process_block(mixer_t *mixer, int16_t *output_buffer, size_t num_samples)
{
    if (!mixer || !output_buffer || !mixer->cfg.channels)
        return 0;

    // Zero out the final destination
    memset(output_buffer, 0, num_samples * sizeof(int16_t));

    bool is_first_channel = true;

    for (uint16_t channel_index = 0; channel_index < mixer->cfg.n_channels; ++channel_index)
    {
        mixer_channel_t *channel = &mixer->cfg.channels[channel_index];
        if (!channel->active || !channel->input_cb)
            continue;

        int16_t *render_target;

        // The first channel renders directly into the output_buffer to avoid a useless copy.
        // Subsequent channels MUST render into the mix_buffer, otherwise their internal memsets
        // will destroy the previously mixed channels.
        if (is_first_channel)
        {
            render_target = output_buffer;
        }
        else
        {
            if (!mixer->mix_buffer)
            {
                // Failsafe: If you have >1 channel but forgot to allocate a mix_buffer,
                // we must abort to avoid corrupting the output_buffer.
                break;
            }
            render_target = mixer->mix_buffer;
        }

        // Render the audio block (synth or sequencer)
        channel->input_cb(render_target, num_samples, channel->cb_data);

        // Mix down
        if (is_first_channel)
        {
            // First channel is already in output_buffer. Just apply gain if it's not unity (256).
            if (channel->gain != 256)
            {
                for (size_t i = 0; i < num_samples; ++i)
                {
                    output_buffer[i] = (int16_t)(((int32_t)output_buffer[i] * channel->gain) >> 8);
                }
            }
            is_first_channel = false;
        }
        else
        {
            // Mix the scratch buffer into the main output_buffer
            if (channel->gain == 256)
            {
                // Optimization: Skip multiplication entirely for unity gain
                for (size_t i = 0; i < num_samples; ++i)
                {
                    int32_t mixed = (int32_t)output_buffer[i] + render_target[i];
                    output_buffer[i] = clamp_i16(mixed);
                }
            }
            else
            {
                // Standard mixed gain via bit-shift
                for (size_t i = 0; i < num_samples; ++i)
                {
                    int32_t scaled = ((int32_t)render_target[i] * channel->gain) >> 8;
                    int32_t mixed = (int32_t)output_buffer[i] + scaled;
                    output_buffer[i] = clamp_i16(mixed);
                }
            }
        }
    }

    return num_samples;
}