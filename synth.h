#ifndef SYNTH_H
#define SYNTH_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ============================================================================
// CONFIG & TYPES
// ============================================================================

#ifndef SYNTH_SAMPLE_RATE
#define SYNTH_SAMPLE_RATE 48000
#endif

// Audio data type (Signed 16-bit)
typedef int16_t sample_t;

// --- Oscillator State ---

typedef struct synth_osc_t synth_osc_t;

typedef void (*osc_render_func_t)(synth_osc_t *self, sample_t *buf, size_t len);

struct synth_osc_t
{
    uint32_t phase;
    uint32_t phase_inc;
    const sample_t *table;
    size_t table_mask;
    osc_render_func_t func;
};

// --- ADSR State ---

typedef enum
{
    ADSR_IDLE,
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE
} adsr_state_t;

typedef struct
{
    adsr_state_t state;
    int32_t current_level;
    int32_t attack_rate;
    int32_t decay_rate;
    int32_t sustain_level;
    int32_t release_rate;
    bool gate;
} synth_adsr_t;

// --- The Command Protocol ---

typedef enum
{
    SYNTH_CMD_NOP,   // Do nothing
    SYNTH_CMD_CLEAR, // Zero out a buffer (Silence)
    SYNTH_CMD_OSC,   // Run Oscillator -> Buffer
    SYNTH_CMD_ADSR,  // Run Envelope -> Buffer
    SYNTH_CMD_MIX,   // Buffer A + Buffer B -> Buffer Out
    SYNTH_CMD_AMP,   // Buffer A * Buffer B -> Buffer Out (VCA / Ring Mod)
    SYNTH_CMD_COPY   // Buffer A -> Buffer Out
} synth_cmd_type_t;

typedef struct
{
    synth_cmd_type_t type;
    union
    {
        // SYNTH_CMD_OSC
        struct
        {
            synth_osc_t *state; // Pointer to persistent state
            uint8_t out;        // Output buffer index
        } osc;

        // SYNTH_CMD_ADSR
        struct
        {
            synth_adsr_t *state; // Pointer to persistent state
            uint8_t out;         // Output buffer index
        } adsr;

        // SYNTH_CMD_MIX, SYNTH_CMD_AMP
        struct
        {
            uint8_t in_a; // Input Buffer A index
            uint8_t in_b; // Input Buffer B index
            uint8_t out;  // Output Buffer index
        } mix;

        // SYNTH_CMD_CLEAR, SYNTH_CMD_COPY
        struct
        {
            uint8_t src; // Source (for copy)
            uint8_t dst; // Destination (for clear/copy)
        } buf;
    };
} synth_cmd_t;

// ============================================================================
// API
// ============================================================================

// --- Initialization ---

void synth_osc_init_table(synth_osc_t *osc, const sample_t *table, size_t table_len, float freq_hz);
void synth_osc_set_freq(synth_osc_t *osc, float freq_hz);
void synth_adsr_init(synth_adsr_t *env, float a, float d, float s, float r);
void synth_adsr_gate(synth_adsr_t *env, bool gate);

// --- Primitives ---

void synth_osc_process(synth_osc_t *osc, sample_t *buf, size_t len);
void synth_adsr_process(synth_adsr_t *env, sample_t *buf, size_t len);
void synth_mix(sample_t *out, const sample_t *a, const sample_t *b, size_t len);
void synth_modulate(sample_t *out, const sample_t *carrier, const sample_t *modulator, size_t len);

// --- Batch Processing ---

/**
 * @brief Run a sequence of synthesis commands.
 * * @param cmds Array of commands to execute.
 * @param cmd_count Number of commands.
 * @param buffers Array of pointers to audio buffers (the "register file").
 * @param buffer_count Number of available buffers in the array.
 * @param len Number of samples to process per buffer.
 */
void synth_process_batch(const synth_cmd_t *cmds, size_t cmd_count, sample_t **buffers, size_t buffer_count, size_t len);

#endif