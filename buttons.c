/**
 * Bocla - buttons.c
 * Button handling with chord detection for the Bocla synthesizer.
 * Uses PIO for efficient button scanning and a timer for chord resolution.
 * (c) 2024 Luis Enrique Garcia Ontanon
 * See LICENSE.txt for license details.
 */

#include "buttons.h"
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "buttons.pio.h"

// --- Config ---
#define CHORD_WINDOW_MS 60 // Window to wait for other buttons to join
#define DEBOUNCE_DIV 2500  // Clock divider for PIO (slow down sampling)

// --- Globals ---
static struct
{
    PIO pio;
    uint sm;
    alarm_id_t timer_id;
    uint8_t current_mask; // What is currently physically held
    uint8_t latched_mask; // What we are accumulating for the chord
    bool gating;          // True if we are inside the chord window

    // Callback Table (Index = Mask)
    button_action_t actions[BUTTON_COMBOS];
} btn_ctx;

// --- Chord Resolution Timer ---
static int64_t chord_timeout_cb(alarm_id_t id, void *user_data)
{
    // 1. Logic: The window is closed.
    btn_ctx.gating = false;
    btn_ctx.timer_id = 0;

    // 2. Dispatch
    // We only fire if the mask is non-zero (valid press)
    if (btn_ctx.latched_mask > 0)
    {
        // Strict Match: We invoke the callback for the EXACT mask.
        // If A+B are pressed, we look at index 3 (0x03).
        // We do NOT look at index 1 or 2.
        button_action_t *action = &btn_ctx.actions[btn_ctx.latched_mask];

        if (action->cb)
        {
            action->cb(action->user_data);
        }
    }

    // 3. Note: We don't clear latched_mask here.
    // We effectively ignore new presses until the physical buttons are released (handled in ISR).
    return 0; // Don't repeat
}

// --- PIO Interrupt Handler ---
static void __not_in_flash_func(buttons_isr)()
{
    // Drain FIFO (should only have 1 or 2 items usually)
    while (pio_sm_get_rx_fifo_level(btn_ctx.pio, btn_ctx.sm) > 0)
    {
        // Read raw state (0 = Pressed because Pull-Up)
        uint32_t raw = pio_sm_get(btn_ctx.pio, btn_ctx.sm);

        // Invert so 1=Pressed, mask to 5 bits
        uint8_t state = (~raw) & 0x1F;

        // --- State Machine ---

        if (state == 0)
        {
            // ALL RELEASED
            // Cancel any pending timer to prevent firing ghost presses
            if (btn_ctx.gating && btn_ctx.timer_id > 0)
            {
                cancel_alarm(btn_ctx.timer_id);
                btn_ctx.timer_id = 0;
            }
            btn_ctx.gating = false;
            btn_ctx.latched_mask = 0;
        }
        else if (state > 0 && !btn_ctx.gating && btn_ctx.latched_mask == 0)
        {
            // NEW PRESS (Start of a sequence)
            btn_ctx.latched_mask = state;
            btn_ctx.gating = true;

            // Start the "Chord Window"
            btn_ctx.timer_id = add_alarm_in_ms(CHORD_WINDOW_MS, chord_timeout_cb, NULL, true);
        }
        else if (btn_ctx.gating)
        {
            // CONTINUED PRESS (During Window)
            // Accumulate bits (if A is held, and B comes in 10ms later)
            btn_ctx.latched_mask |= state;
        }
        else
        {
            // Late arrival? (Pressed C while holding A+B long after window closed)
            // Currently ignored to prevent accidental triggering.
            // User must release all to reset.
        }

        btn_ctx.current_mask = state;
    }

    // Clear IRQ flag
    pio_interrupt_clear(btn_ctx.pio, 0);
}

// --- Public API ---

void buttons_init(PIO pio, uint sm, uint base_pin)
{
    btn_ctx.pio = pio;
    btn_ctx.sm = sm;
    btn_ctx.latched_mask = 0;
    btn_ctx.gating = false;

    // 1. Load PIO Program
    uint offset = pio_add_program(pio, &button_input_program);

    // 2. Configure State Machine
    pio_sm_config c = button_input_program_get_default_config(offset);

    // Map 5 pins to IN
    sm_config_set_in_pins(&c, base_pin);
    // Jump pin (for the `mov y, isr` trick, we don't use a specific pin, standard mapping is fine)

    // Init GPIO
    for (int i = 0; i < 5; i++)
    {
        pio_gpio_init(pio, base_pin + i);
        gpio_pull_up(base_pin + i); // CRITICAL: Pull-ups
    }
    pio_sm_set_consecutive_pindirs(pio, sm, base_pin, 5, false);

    // Shifting: Right, Autopush OFF (PIO program does explicit push)
    sm_config_set_in_shift(&c, false, false, 32);

    // Clock Divider: Slow it down to filter glitchy mechanical noise (simple debounce)
    // 125MHz / 2500 = 50kHz sampling. Fast enough for humans, slow enough for basic filter.
    sm_config_set_clkdiv(&c, DEBOUNCE_DIV);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    // 3. Interrupt Setup
    // Use PIO IRQ0
    pio_set_irq0_source_enabled(pio, pis_sm0_rx_fifo_not_empty + sm, true); // Map specific SM FIFO
    irq_set_exclusive_handler(PIO0_IRQ_0, buttons_isr);                     // Assuming PIO0 used. Check `pio` arg in real usage!
    irq_set_enabled(PIO0_IRQ_0, true);
}

void buttons_register(uint8_t mask, button_cb_t cb, void *user_data)
{
    if (mask >= BUTTON_COMBOS)
        return;

    btn_ctx.actions[mask].cb = cb;
    btn_ctx.actions[mask].user_data = user_data;
}
