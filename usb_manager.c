#include "usb_manager.h"
#include "tusb.h"
#include "audio.h"

// --- Buffer Management ---
// USB frames come every 1ms. DMA consumes them. 
// We need enough buffers to handle jitter/queueing.
#define USB_FRAME_POOL_COUNT 16
#define MAX_PACKET_SIZE 256 // Safe margin for 48kHz stereo (192 bytes nominal)

// Static pool of buffers to keep data valid while in the DMA queue
static int16_t frame_pool[USB_FRAME_POOL_COUNT][MAX_PACKET_SIZE / 2];
static uint8_t pool_idx = 0;

void usb_manager_init(void) {
    tusb_init();
}

void usb_manager_task(void) {
    tud_task();
}

// --- HID: Volume Control ---

void usb_send_volume(int8_t dir) {
    (void)dir;
#if CFG_TUD_HID
    // TinyUSB HID generic check
    if (!tud_hid_ready()) return;

    // Consumer Control Usage IDs (0x0C page)
    // 0xE9 = Vol+, 0xEA = Vol-, 0xE2 = Mute
    uint16_t cmd = 0;
    
    if (dir > 0) cmd = 0x00E9;      // Vol +
    else if (dir < 0) cmd = 0x00EA; // Vol -
    else cmd = 0x00E2;              // Mute

    // Send Press
    // Report ID must match your descriptor. Usually 0 or 1.
    tud_hid_report(0, &cmd, sizeof(cmd));

    // Send Release (State 0) immediately
    cmd = 0;
    tud_hid_report(0, &cmd, sizeof(cmd));
#endif
}

// --- Audio: Receive Path ---

// Standard ISO RX Callback
#if CFG_TUD_AUDIO && CFG_TUD_AUDIO_ENABLE_EP_OUT
bool tud_audio_rx_done_pre_read_cb(uint8_t rhport, uint16_t n_bytes, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting) {
    (void) rhport;
    (void) func_id;
    (void) ep_out;
    (void) cur_alt_setting;

    if (n_bytes > MAX_PACKET_SIZE) {
        // Discard overshoot (should not happen with compliant host)
        tud_audio_read(frame_pool[pool_idx], n_bytes); 
        return true;
    }

    // 1. Read into the current pool slot
    // Cast to void* for byte-wise read, stored into int16 array
    tud_audio_read(frame_pool[pool_idx], n_bytes);

    // 2. Push this slot to the Audio Engine
    // n_bytes / 2 = number of int16 samples
    // audio_enqueue expects a pointer that remains valid until played.
    // Our pool guarantees validity for 'USB_FRAME_POOL_COUNT' ms.
    bool queued = audio_enqueue(frame_pool[pool_idx], n_bytes / 2);

    if (queued) {
        // Move to next slot only if successful
        pool_idx = (pool_idx + 1) % USB_FRAME_POOL_COUNT;
    } else {
        // Queue full: Drop packet. 
        // We reuse the current slot for the next incoming packet.
    }

    return true;
}

// Stub for required set request callback
bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff) {
    (void) rhport; (void) p_request; (void) pBuff;
    return true; 
}
#endif
