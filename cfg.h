// cfg.h

#ifndef CFG_H
#define CFG_H

#ifdef NDEBUG
#define D(...) ((void)0)
#else
#define D(...) printf(__VA_ARGS__)
#endif

// USB Device Descriptor
#define USB_VENDOR_ID 0xCafe
#define USB_PRODUCT_ID 0xB0C1
#define USB_DEVICE_VERSION 0x0100

#define USB_MANUFACTURER "Lego"
#define USB_PRODUCT "Bocla"
#define USB_SERIAL "0001"

#define USB_HID_REPORT_ID_VOLUME 0x01
#define USB_HID_REPORT_ID_PLAY_PAUSE 0x02

#define USB_HID_USAGE_VOLUME_UP 0xE9
#define USB_HID_USAGE_VOLUME_DOWN 0xEA
#define USB_HID_USAGE_MUTE 0xE2
#define USB_HID_USAGE_PLAY_PAUSE 0xCD

#define USB_HID_REPORT_SIZE 2      // Report ID + Usage
#define USB_HID_REPORT_INTERVAL 10 // ms
#define USB_HID_REPORT_TIMEOUT 100 // ms

// Audio Config
#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_CHANNELS 2
#define AUDIO_BITS_PER_SAMPLE 16
#define AUDIO_BYTES_PER_SAMPLE (AUDIO_BITS_PER_SAMPLE / 8)
#define AUDIO_BUFFER_SIZE (AUDIO_SAMPLE_RATE * AUDIO_CHANNELS * AUDIO_BYTES_PER_SAMPLE / 100) // 10ms buffer
#define AUDIO_DMA_CHANNEL 0

// Synth Config
#define FM_SINE_QUARTER_SIZE 1024 // 1024 entries for full 4096 step cycle (12-bit phase resolution)
#define FM_PHASE_SHIFT 20
#define FM_OPS_PER_VOICE 4
#define FM_MAX_VOICES 32

// LED Config
#define LED_COUNT 1
#define LED_BRIGHTNESS 50 // 0-255

#endif // CFG_H
