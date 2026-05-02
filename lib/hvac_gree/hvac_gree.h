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
#define HVAC_GREE_ZERO_SPACE 560
#define HVAC_GREE_END_MARK   620
#define HVAC_GREE_FRAME_GAP  20000
#define HVAC_GREE_REPEAT_GAP 40000

#define HVAC_GREE_TRANSMIT_TIMINGS_PER_FRAME \
    (2 + 2 * HVAC_GREE_PACKET_SIZE * 8 + 1)

#define HVAC_GREE_TRANSMIT_REPEATS_DEFAULT 2

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
