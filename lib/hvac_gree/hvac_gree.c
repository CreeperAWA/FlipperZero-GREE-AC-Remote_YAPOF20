#include "hvac_gree.h"

/*
 * GREE AC IR Protocol - verified against captured data from code.ir
 *
 * Packet structure (8 bytes):
 *   Byte 0: Mode + Fan + Swing
 *   Byte 1: Temperature
 *   Byte 2: 0x20 (constant)
 *   Byte 3: 0x50 (constant)
 *
 * Byte 0 layout:
 *   Bits 0-3: Base mode value (Cool/Dry/Fan=0x09, Heat=0x0C, Auto=0x08)
 *   Bit 4:    UNUSED (always 0 in captured data)
 *   Bit 5:    Fan speed 2 (1=on, used for Fan2)
 *   Bit 6:    Swing (1=on)
 *   Bit 7:    UNUSED
 *
 * Byte 1 layout:
 *   0x09: Auto mode / Power off
 *   Other: Temperature value = (temp - 16)
 *     Both Heating and Cooling use the same encoding: (temp - 16)
 *     Example: 20C -> 0x04, 18C -> 0x02
 *
 * Power on/off:
 *   Power ON: The packet with mode/temp/fan settings IS the power-on command
 *   Power OFF: Byte0=0x00, Byte1=0x09 (clears mode bits, keeps constants)
 *   Note: No power bit is needed - verified against real captured data
 *
 * Timing parameters (verified from code.ir):
 *   Header: 9000us mark + 4500us space
 *   Bit 0: 620us mark + 620us space
 *   Bit 1: 620us mark + 1660us space
 *   Frame gap: 20000us between two frames
 *   Repeat gap: 40000us after second frame
 */

static const uint8_t gree_mode_base[] = {
    [HvacGreeModeCool] = 0x09,
    [HvacGreeModeDry] = 0x09,
    [HvacGreeModeFan] = 0x09,
    [HvacGreeModeHeat] = 0x0C,
    [HvacGreeModeAuto] = 0x08,
};

static uint8_t gree_encode_temperature(HvacGreeMode mode, HvacGreeTemperature temp) {
    if(mode == HvacGreeModeAuto || mode == HvacGreeModeFan) {
        return 0x09;
    }

    if(temp < HVAC_GREE_TEMPERATURE_MIN) {
        temp = HVAC_GREE_TEMPERATURE_MIN;
    }
    if(temp > HVAC_GREE_TEMPERATURE_MAX) {
        temp = HVAC_GREE_TEMPERATURE_MAX;
    }

    return (uint8_t)(temp - 16);
}

HvacGreePacket hvac_gree_create_packet(void) {
    HvacGreePacket packet = (HvacGreePacket)malloc(sizeof(uint8_t) * HVAC_GREE_PACKET_SIZE);
    furi_assert(packet);

    packet[0] = 0x00;
    packet[1] = 0x09;
    packet[2] = 0x20;
    packet[3] = 0x50;
    packet[4] = 0x00;
    packet[5] = 0x00;
    packet[6] = 0x00;
    packet[7] = 0x00;

    return packet;
}

void hvac_gree_free_packet(HvacGreePacket packet) {
    furi_assert(packet);
    free(packet);
}

void hvac_gree_set_power(HvacGreePacket packet, bool on) {
    furi_assert(packet);

    if(on) {
        // Verified against captured data: power-on commands do NOT set bit 4
        // The packet with mode/temp/fan settings IS the power-on command
        // No additional power bit needed
    } else {
        packet[0] &= ~0x0F;
        packet[1] = 0x09;
    }
}

void hvac_gree_set_mode(HvacGreePacket packet, HvacGreeMode mode) {
    furi_assert(packet);

    uint8_t base = gree_mode_base[mode];
    uint8_t fan = packet[0] & 0x20;
    uint8_t swing = packet[0] & 0x40;

    packet[0] = base | fan | swing;
    packet[1] = gree_encode_temperature(mode, HVAC_GREE_TEMPERATURE_DEFAULT);
}

void hvac_gree_set_temperature(HvacGreePacket packet, HvacGreeTemperature temperature) {
    furi_assert(packet);

    uint8_t base = packet[0] & 0x0F;
    HvacGreeMode mode_enum;

    if(base == 0x0C) {
        mode_enum = HvacGreeModeHeat;
    } else if(base == 0x09) {
        mode_enum = HvacGreeModeCool;
    } else if(base == 0x08) {
        mode_enum = HvacGreeModeAuto;
    } else {
        mode_enum = HvacGreeModeCool;
    }

    packet[1] = gree_encode_temperature(mode_enum, temperature);
}

void hvac_gree_set_fan(HvacGreePacket packet, HvacGreeFan fan) {
    furi_assert(packet);

    packet[0] &= ~0x20;
    switch(fan) {
    case HvacGreeFan2:
        packet[0] |= 0x20;
        break;
    case HvacGreeFanAuto:
    case HvacGreeFan1:
    case HvacGreeFan3:
    default:
        break;
    }
}

void hvac_gree_set_swing(HvacGreePacket packet, bool on) {
    furi_assert(packet);

    if(on) {
        packet[0] |= 0x40;
    } else {
        packet[0] &= ~0x40;
    }
}

void hvac_gree_set_light(HvacGreePacket packet, bool on) {
    UNUSED(packet);
    UNUSED(on);
}

void hvac_gree_set_turbo(HvacGreePacket packet, bool on) {
    UNUSED(packet);
    UNUSED(on);
}

void hvac_gree_set_clean(HvacGreePacket packet, bool on) {
    UNUSED(packet);
    UNUSED(on);
}

static void hvac_gree_send_raw(const HvacGreePacket packet) {
    size_t timings_len = HVAC_GREE_TRANSMIT_TIMINGS_PER_FRAME * 2;
    uint32_t* timings = malloc(sizeof(uint32_t) * timings_len);
    furi_assert(timings);

    size_t idx = 0;

    for(int frame = 0; frame < 2; frame++) {
        timings[idx++] = HVAC_GREE_HDR_MARK;
        timings[idx++] = HVAC_GREE_HDR_SPACE;

        for(uint8_t byte_idx = 0; byte_idx < HVAC_GREE_PACKET_SIZE; byte_idx++) {
            uint8_t byte = packet[byte_idx];
            for(uint8_t bit = 0; bit < 8; bit++) {
                timings[idx++] = HVAC_GREE_BIT_MARK;
                if(byte & (1 << bit)) {
                    timings[idx++] = HVAC_GREE_ONE_SPACE;
                } else {
                    timings[idx++] = HVAC_GREE_ZERO_SPACE;
                }
            }
        }

        timings[idx++] = HVAC_GREE_END_MARK;

        if(frame == 0) {
            timings[idx++] = HVAC_GREE_FRAME_GAP;
        } else {
            timings[idx++] = HVAC_GREE_REPEAT_GAP;
        }
    }

    infrared_send_raw_ext(
        timings,
        idx,
        true,
        HVAC_GREE_TRANSMIT_FREQUENCY,
        HVAC_GREE_TRANSMIT_DUTY_CYCLE);

    free(timings);
}

void hvac_gree_send(const HvacGreePacket packet) {
    for(uint8_t i = 0; i < HVAC_GREE_TRANSMIT_REPEATS_DEFAULT; i++) {
        hvac_gree_send_raw(packet);
    }
}
