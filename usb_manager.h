/**
 * Bocla - usb_manager.h
 * USB management for the Bocla synthesizer using TinyUSB.
 * (c) 2024 Luis Enrique Garcia Ontanon
 * See LICENSE.txt for license details.
 */
#ifndef USB_MANAGER_H
#define USB_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize the USB stack.
 */
void usb_manager_init(void);

/**
 * @brief Main USB task. Call this in your main loop.
 * Wraps tud_task().
 */
void usb_manager_task(void);

/**
 * @brief Send a Consumer Control Volume command.
 * * @param dir Direction: 1 = Vol Up, -1 = Vol Down, 0 = Mute Toggle
 */
void usb_send_volume(int8_t dir);

#endif
