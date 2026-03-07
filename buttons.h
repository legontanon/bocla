/**
 * Bocla - buttons.h
 * Button management for the Bocla synthesizer using RP2040's PIO for debouncing and chord detection.
 * (c) 2024 Luis Enrique Garcia Ontanon
 * See LICENSE.txt for license details.
 */

/**
 * @file buttons.h
 * @brief Button management for the Bocla synthesizer using RP2040's PIO for debouncing and chord detection.
 * This module provides an interface to initialize a set of 5 buttons connected to contiguous GPIO pins, and to register callbacks for specific button combinations (chords). The implementation uses a PIO state machine to efficiently scan the button states and debounce them, while an alarm-based timer is used to define a "chord window" during which multiple button presses are accumulated into a single bitmask. When the chord window expires, the registered callback for the resulting button combination is invoked. This allows for complex interactions with just 5 physical buttons, enabling up to 32 unique combinations.
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include "cfg.h"

#include <stdint.h>
#include <stdbool.h>
#include "hardware/pio.h"

// 5 Pins = 32 possible combinations
#define BUTTON_COMBOS 32

typedef void (*button_cb_t)(void *user_data);

typedef struct
{
    button_cb_t cb;
    void *user_data;
} button_action_t;

/**
 * @brief Initialize the 5-pin button manager.
 * Uses a PIO state machine to debounce and an internal alarm for chord resolution.
 * * @param pio The PIO instance (pio0 or pio1)
 * @param sm The State Machine index
 * @param base_pin The first of 5 contiguous pins (e.g., 16 means 16-20)
 */
void buttons_init(PIO pio, uint sm, uint base_pin);

/**
 * @brief Register a callback for a specific button combination.
 * * @param mask Bitmask of buttons (1 = pressed). E.g., 0x03 is Button 0 + Button 1.
 * @param cb Function to call.
 * @param user_data Pointer to pass to the callback.
 */
void buttons_register(uint8_t mask, button_cb_t cb, void *user_data);

#endif