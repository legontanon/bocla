#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hardware/pio.h"

// Max queued buffers before audio_enqueue returns false
#define AUDIO_QUEUE_DEPTH 16

typedef struct {
    const int16_t *samples; // Pointer to Stereo Interleaved data (L, R, L, R...)
    size_t len;             // Count of int16_t elements (NOT bytes, NOT frames)
} audio_chunk_t;

/**
 * @brief Configure I2S PIO and DMA.
 * * @param pio The PIO instance (pio0/pio1)
 * @param sm The State Machine index
 * @param pin_data The DIN pin
 * @param pin_bclk The BCLK pin (LRC must be pin_bclk + 1)
 * @param sample_rate Audio frequency (e.g., 44100, 48000)
 */
void audio_init(PIO pio, uint sm, uint pin_data, uint pin_bclk, uint32_t sample_rate);

/**
 * @brief Push a buffer into the playback queue.
 * * @param samples Pointer to the data. MUST remain valid until played.
 * @param len Total number of int16 samples in the buffer.
 * @return true if queued, false if queue is full.
 */
bool audio_enqueue(const int16_t *samples, size_t len);

/**
 * @brief Check if the queue is active.
 * @return The pointer to the currently playing buffer, or NULL if idle.
 */
const int16_t *audio_is_playing(void);

#endif
