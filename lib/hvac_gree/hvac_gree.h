#pragma once

#include <infrared_transmit.h>
#include <infrared_worker.h>

#include "furi_hal.h"

#define HVAC_GREE_PACKET_SIZE 8

typedef uint8_t* HvacGreePacket;

typedef enum {
    HvacGreeModeCool = 0,
    HvacGreeModeDry,
    HvacGreeModeFan,
    HvacGreeModeHeat,
    HvacGreeModeAuto,
} HvacGreeMode;

typedef enum {
    HvacGreeFanAuto = 0,
    HvacGreeFan1,
    HvacGreeFan2,
    HvacGreeFan3,
} HvacGreeFan;

typedef uint8_t HvacGreeTemperature;
#define HVAC_GREE_TEMPERATURE_MIN 16
#define HVAC_GREE_TEMPERATURE_MAX 30
#define HVAC_GREE_TEMPERATURE_DEFAULT 24

#define HVAC_GREE_TRANSMIT_FREQUENCY  38000
#define HVAC_GREE_TRANSMIT_DUTY_CYCLE 0.33

#define HVAC_GREE_HDR_MARK   9000
#define HVAC_GREE_HDR_SPACE  4500
#define HVAC_GREE_BIT_MARK   620
#define HVAC_GREE_ONE_SPACE  1660
#define HVAC_GREE_ZERO_SPACE 620
#define HVAC_GREE_END_MARK   620
#define HVAC_GREE_FRAME_GAP  20000
#define HVAC_GREE_REPEAT_GAP 40000

/*
 * GREE AC IR Protocol Structure (verified from code.ir):
 * 
 * Each transmission consists of 2 frames:
 * Frame 1: Header (9000+4500us) + 35 bits (4 bytes + 3 padding bits) + End mark + 20ms gap
 * Frame 2: 4 bytes (NO header!) + End mark + 40ms gap
 * 
 * The extra 3 bits in Frame 1 are always [0, 1, 0] (0x02)
 * This entire sequence is sent twice (repeat).
 * The second repeat does NOT end with a repeat gap.
 * Total: 279 timings (140 for first repeat + 139 for second repeat)
 * 
 * Byte 2 controls LED display:
 *   Bit 5 (0x20): LED ON when set, OFF when cleared
 *   Default: LED ON (0x20)
 */

/* Single repeat structure:
 * Frame 1: 2 (header) + 35*2 (bits) + 1 (end) + 1 (gap) = 74
 * Frame 2: 32*2 (bits) + 1 (end) + 1 (gap) = 66
 * Total per repeat: 74 + 66 = 140
 * 
 * Full transmission (2 repeats):
 * First repeat: 140 timings (includes repeat gap)
 * Second repeat: 139 timings (no repeat gap at end)
 * Total: 279 timings
 */
#define HVAC_GREE_TRANSMIT_TIMINGS_PER_FRAME (140 + 139)

/* Frame 1 padding bits - always [0, 1, 0] */
#define HVAC_GREE_FRAME1_PADDING_BITS 3
#define HVAC_GREE_FRAME1_PADDING_VALUE 0x02

#define HVAC_GREE_TRANSMIT_REPEATS_DEFAULT 1

HvacGreePacket hvac_gree_create_packet(void);
void hvac_gree_free_packet(HvacGreePacket packet);

void hvac_gree_set_power(HvacGreePacket packet, bool on);

void hvac_gree_set_mode(HvacGreePacket packet, HvacGreeMode mode);

void hvac_gree_set_temperature(HvacGreePacket packet, HvacGreeTemperature temperature);

void hvac_gree_set_fan(HvacGreePacket packet, HvacGreeFan fan);

void hvac_gree_set_swing(HvacGreePacket packet, bool on);

void hvac_gree_set_light(HvacGreePacket packet, bool on);

void hvac_gree_set_turbo(HvacGreePacket packet, bool on);

void hvac_gree_set_clean(HvacGreePacket packet, bool on);

void hvac_gree_send(const HvacGreePacket packet);
