/**
 * Bocla Synthesizer - filter.h
 * (c) 2024 Luis Enrique Garcia Ontanon
 * See LICENSE.txt for license details.
 */

/**
 * @file filter.h
 * @brief Header for audio filters used in the Bocla Synthesizer.
 * This file defines the filter configuration and state structures, as well as the function prototypes for
 * initializing filters and processing audio blocks through various filter types (SVF, 1-pole LPF, DC Blocker, Unskew ASRC).
 * The filter_state_t structure is designed to be flexible, allowing for different filter types and
 * configurations via a union in the filter_config_t structure. Each filter type has its own processing function,
 * and the main filter processing function will dispatch to the appropriate one based on the filter type
 * specified in the configuration. The unskew ASRC implementation uses linear interpolation to
 * resample audio blocks, and it relies on an external callback to provide input samples.
 * This allows it to be used as a simple anti-aliasing filter or for sample rate conversion
 * in a pinch.
 */
#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum
{
    FILTER_TYPE_LOW_PASS = 0,
    FILTER_TYPE_HIGH_PASS = 1,
    FILTER_TYPE_BAND_PASS = 2,
    FILTER_TYPE_BIQUAD = 3,
    FILTER_TYPE_1POLE_LPF = 4,
    FILTER_TYPE_DC_BLOCK = 5,
    FILTER_TYPE_UNSKEW = 6 // Linear Interpolation ASRC
} filter_type_t;

typedef struct filter_config_t
{
    filter_type_t type;
    union
    {
        struct
        {
            uint8_t op1;
            uint8_t op2;
            uint8_t op3;
            uint8_t op4;
        };
        uint32_t combined;
        struct
        {
            uint8_t low;
            uint8_t mid;
            uint8_t high;
            uint8_t reserved;
        } bands;
        struct
        {
            uint16_t cutoff;
            uint8_t resonance;
            uint8_t type;
        } params;
        struct
        {
            int16_t b0, b1, b2, a1, a2;
        } biquad;
        struct
        {
            int16_t alpha;
        } simple;
        void *ptr;
    } data;
} filter_config_t;

typedef struct filter_state_t
{
    const filter_config_t *cfg;
    size_t (*process_block)(struct filter_state_t *state, int16_t *buffer, size_t num_samples);

    int32_t z1;
    int32_t z2;
    void *ext_state; // Pointer to extended state structs (e.g., filter_unskew_state_t)
} filter_state_t;

/**
 * @brief Fetch callback for the Unskew filter
 */
typedef void (*filter_fetch_cb_t)(int16_t *buffer, size_t num_samples, void *user_data);

/**
 * @brief Extended state for the Unskew ASRC filter
 */
typedef struct
{
    filter_fetch_cb_t cb;
    void *cb_data;
    uint32_t in_count;   // Total input samples consumed (read-only tracking)
    uint32_t out_count;  // Total output samples generated (read-only tracking)
    uint32_t phase;      // Q15 fractional position
    uint32_t phase_step; // Q15 rate (32768 = 1.0 = No skew)
    int16_t last_sample; // z^-1
    int16_t curr_sample; // z^0
} filter_unskew_state_t;

void filter_init(filter_state_t *state, const filter_config_t *cfg);

size_t low_pass_process_block(filter_state_t *state, int16_t *buffer, size_t num_samples);
size_t high_pass_process_block(filter_state_t *state, int16_t *buffer, size_t num_samples);
size_t band_pass_process_block(filter_state_t *state, int16_t *buffer, size_t num_samples);
size_t unskew_process_block(filter_state_t *state, int16_t *buffer, size_t num_samples);

void filter_process_block(filter_state_t *state, int16_t *buffer, size_t num_samples);

#endif // FILTER_H