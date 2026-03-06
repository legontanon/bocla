#ifndef FM_SYNTH_H
#define FM_SYNTH_H

#include "cfg.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Fixed-point FM Synthesizer Header
 * Precision: 16-bit signed output.
 * Waveform: Quarter-sine lookup (1024 entries = 4096 full cycle).
 */
#ifndef FM_SINE_QUARTER_SIZE
#error "FM_SINE_QUARTER_SIZE must be defined in cfg.h to specify the resolution of the sine table."
#endif

#ifndef FM_PHASE_SHIFT
#define FM_PHASE_SHIFT 20
#endif

#ifndef FM_OPS_PER_VOICE
#define FM_OPS_PER_VOICE 4
#endif

#ifndef FM_MAX_VOICES
#define FM_MAX_VOICES 32
#endif

typedef enum
{
    FM_ENV_IDLE,
    FM_ENV_ATTACK,
    FM_ENV_DECAY,
    FM_ENV_SUSTAIN,
    FM_ENV_RELEASE
} fm_env_state_t;

/**
 * @brief Static configuration for an operator
 */
typedef struct
{
    uint32_t freq_mult; // Frequency multiplier (Fixed 8.8)
    int16_t attack;     // Envelope Attack rate
    int16_t decay;      // Envelope Decay rate
    int16_t sustain;    // Envelope Sustain level
    int16_t release;    // Envelope Release rate
    int16_t volume;     // Peak operator volume
    uint8_t feedback;   // Self-modulation shift (0-7)
} fm_operator_config_t;

/**
 * @brief Runtime state for a single operator
 */
typedef struct
{
    uint32_t phase;
    uint32_t phase_inc;
    int32_t env_level; // Current envelope amplitude (Fixed 16.16)
    fm_env_state_t env_state;
    int16_t last_out[2]; // Feedback history
} fm_operator_runtime_t;

/**
 * @brief Patch definition (Strict 4-Op to avoid FAM pointer math)
 */
typedef struct
{
    uint8_t algorithm;
    uint8_t lfo_depth;
    uint8_t n_ops;
    fm_operator_config_t ops[FM_OPS_PER_VOICE];
} fm_voice_config_t;

typedef struct
{
    uint32_t sampling_rate;
    uint32_t tick_rate;

    fm_voice_config_t *patches;
    size_t num_patches;
} fm_synth_cfg_t;

/**
 * @brief The Instance - Live state of a playing note
 */
typedef struct _fm_voice_runtime fm_voice_runtime_t;

struct _fm_voice_runtime
{
    const fm_voice_config_t *patch;

    uint32_t base_freq;
    int16_t velocity;
    uint8_t active;

    /* Block Processing State */
    uint32_t tick_counter;
    uint16_t samples_until_tick;
    int16_t *out_ptr;

    uint8_t n_ops;
    fm_operator_runtime_t op_state[FM_OPS_PER_VOICE];

    fm_voice_runtime_t *next; // for use in active and inactive voice linked lists
};

typedef struct
{
    fm_synth_cfg_t cfg;
    fm_voice_runtime_t voices[FM_MAX_VOICES];
} fm_synth_t;

extern const int16_t fm_sine_table[FM_SINE_QUARTER_SIZE];

/**
 * @brief Initialize the FM synthesizer with configuration.
 *
 * @param cfg Pointer to synth configuration structure (will be filled by init).
 * @param sampling_rate Sample rate in Hz (typically 48000).
 * @param num_patches Number of voice patches available.
 * @param patches Array of voice configuration patches.
 * @return Pointer to initialized fm_synth_t, or NULL on failure.
 */
fm_synth_t *fm_synth_init(fm_synth_cfg_t *cfg, uint32_t sampling_rate, size_t num_patches, fm_voice_config_t *patches);

/**
 * @brief Core Synthesis Functions
 */

/**
 * @brief Trigger a note on a voice with the given patch, frequency, and velocity.
 *
 * @param synth The synthesizer instance.
 * @param voice_index The index of the voice to trigger (0 to FM_MAX_VOICES-1).
 * @param patch_idx The index of the patch to use (0 to num_patches-1).
 * @param freq The base frequency in Hz (will be converted to phase increment).
 * @param velocity The MIDI velocity (0-127) to determine volume.
 * @param intensity An additional parameter (0-127) that can be used for modulation depth or other purposes defined by the patch.
 * @param duration_ticks The duration of the note in ticks (for sequencer use), or 0 for infinite until note off.
 */
void fm_synth_note_on(fm_synth_t *synth, uint8_t voice_index, uint8_t patch_idx, uint32_t freq, uint8_t velocity, uint8_t intensity, uint16_t duration_ticks);

/**
 * @brief Trigger a note off (release) on the given voice.
 *
 * @param synth The synthesizer instance.
 * @param voice_index The index of the voice to release (0 to FM_MAX_VOICES-1).
 */
void fm_synth_note_off(fm_synth_t *synth, uint8_t voice_index);

/**
 * @brief Get the index of an idle voice.
 *
 * @param synth The synthesizer instance.
 * @return The index of an idle voice, or -1 if none are available.
 */
int8_t fm_synth_get_idle_voice_index(fm_synth_t *synth);

/**
 * @brief Render a block of audio samples for the given voice.
 * This should be called repeatedly until all voices become inactive.
 *
 * @param synth The synthesizer instance.
 * @param buffer Output buffer to fill with audio samples (interleaved if stereo).
 * @param num_samples The number of samples to render in this block.
 *
 * @return The number of samples unrendered if all voices became inactive during this block, or 0 if fully rendered.
 */
size_t fm_synth_render_block(fm_synth_t *synth, int16_t *buffer, uint16_t num_samples);

#endif // FM_SYNTH_H