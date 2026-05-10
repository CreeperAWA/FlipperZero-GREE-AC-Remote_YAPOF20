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
 *   Bits 4-5: Fan speed (2 bits)
 *               00 (0x00): Auto
 *               01 (0x10): Fan speed 1
 *               10 (0x20): Fan speed 2
 *               11 (0x30): Fan speed 3
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
    packet[5] = 0x40;
    packet[6] = 0x00;
    packet[7] = 0x70;

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
    uint8_t fan = packet[0] & 0x30;
    uint8_t swing = packet[0] & 0x40;
    uint8_t current_temp = packet[1];

    packet[0] = base | fan | swing;
    packet[1] = current_temp;
}

static uint8_t gree_encode_byte7(HvacGreeMode mode, HvacGreeTemperature temp) {
    switch(mode) {
    case HvacGreeModeHeat:
        return 0xC8 + (temp - 18) * 0x10;
    case HvacGreeModeCool:
    case HvacGreeModeDry:
    case HvacGreeModeFan:
        return 0x90 + (temp - 18) * 0x10;
    case HvacGreeModeAuto:
        return 0xF0;
    default:
        return 0x70;
    }
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
    packet[7] = gree_encode_byte7(mode_enum, temperature);
}

void hvac_gree_set_fan(HvacGreePacket packet, HvacGreeFan fan) {
    furi_assert(packet);

    // Clear fan speed bits (Bit4 and Bit5, values 0x10 and 0x20)
    // Fan speed is encoded in 2 bits:
    //   00 (0x00): Auto
    //   01 (0x10): Fan speed 1
    //   10 (0x20): Fan speed 2
    //   11 (0x30): Fan speed 3
    packet[0] &= ~0x30;

    switch(fan) {
    case HvacGreeFan1:
        // Fan speed 1: set Bit4 only (0x10)
        packet[0] |= 0x10;
        break;
    case HvacGreeFan2:
        // Fan speed 2: set Bit5 only (0x20)
        packet[0] |= 0x20;
        break;
    case HvacGreeFan3:
        // Fan speed 3: set both Bit4 and Bit5 (0x30)
        packet[0] |= 0x30;
        break;
    case HvacGreeFanAuto:
    default:
        // Auto: both bits cleared (0x00)
        break;
    }
}

void hvac_gree_set_swing(HvacGreePacket packet, bool on) {
    furi_assert(packet);

    if(on) {
        packet[0] |= 0x40;
        packet[4] = 0x01;
    } else {
        packet[0] &= ~0x40;
        packet[4] = 0x00;
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

static void hvac_gree_send_bits(uint32_t* timings, size_t* idx, const uint8_t* data, size_t byte_count) {
    for(size_t byte_idx = 0; byte_idx < byte_count; byte_idx++) {
        uint8_t byte = data[byte_idx];
        for(uint8_t bit = 0; bit < 8; bit++) {
            timings[(*idx)++] = HVAC_GREE_BIT_MARK;
            if(byte & (1 << bit)) {
                timings[(*idx)++] = HVAC_GREE_ONE_SPACE;
            } else {
                timings[(*idx)++] = HVAC_GREE_ZERO_SPACE;
            }
        }
    }
}

static void hvac_gree_send_padding_bits(uint32_t* timings, size_t* idx) {
    // Frame 1 has 3 extra padding bits: [0, 1, 0] = 0x02
    uint8_t padding = HVAC_GREE_FRAME1_PADDING_VALUE;
    for(uint8_t bit = 0; bit < HVAC_GREE_FRAME1_PADDING_BITS; bit++) {
        timings[(*idx)++] = HVAC_GREE_BIT_MARK;
        if(padding & (1 << bit)) {
            timings[(*idx)++] = HVAC_GREE_ONE_SPACE;
        } else {
            timings[(*idx)++] = HVAC_GREE_ZERO_SPACE;
        }
    }
}

static void hvac_gree_send_raw(const HvacGreePacket packet) {
    /*
     * GREE AC IR signal structure (verified from code.ir):
     * Frame 1: Header + 4 bytes + 3 padding bits + End mark + 20ms gap
     * Frame 2: 4 bytes (no header!) + End mark + 40ms gap
     * 
     * This is sent twice (repeat), with the second pair separated by timing
     * Note: The last repeat does NOT end with a repeat gap
     */
    size_t timings_len = HVAC_GREE_TRANSMIT_TIMINGS_PER_FRAME;
    uint32_t* timings = malloc(sizeof(uint32_t) * timings_len);
    furi_assert(timings);

    size_t idx = 0;
    uint8_t mode_base = packet[0] & 0x0F;
    uint8_t fan_bits = packet[0] & 0x30;

    // Send two identical transmissions (repeat)
    for(int repeat = 0; repeat < 2; repeat++) {
        // Frame 1: Header + 4 bytes + 3 padding bits + End mark + 20ms gap
        timings[idx++] = HVAC_GREE_HDR_MARK;
        timings[idx++] = HVAC_GREE_HDR_SPACE;
        
        if(repeat == 1) {
            uint8_t modified_frame1[4];
            for(int i = 0; i < 4; i++) {
                modified_frame1[i] = packet[i];
            }
            
            // Byte 3: set bit 5 (0x20) in second transmission
            modified_frame1[3] |= 0x20;

            hvac_gree_send_bits(timings, &idx, modified_frame1, 4);
        } else {
            hvac_gree_send_bits(timings, &idx, packet, 4);
        }
        
        hvac_gree_send_padding_bits(timings, &idx);
        timings[idx++] = HVAC_GREE_END_MARK;
        timings[idx++] = HVAC_GREE_FRAME_GAP;

        // Frame 2: next 4 bytes (NO header) + End mark + 40ms gap (except for last repeat)
        if(repeat == 1) {
            uint8_t modified_frame2[4];
            for(int i = 0; i < 4; i++) {
                modified_frame2[i] = packet[4 + i];
            }
            
            // Byte 0: clear swing bit
            modified_frame2[0] &= ~0x01;
            
            // Byte 1: clear bit 6 (0x40)
            modified_frame2[1] &= ~0x40;
            
            // Byte 2: set bit 5 if fan is not auto (fan 2 or 3)
            if(fan_bits != 0x00) {
                modified_frame2[2] |= 0x20;
            }
            
            // Byte 3: depends on mode, fan speed, and temperature (bit5)
            uint8_t byte3 = packet[7];
            uint8_t clear_mask;
            uint8_t set_bits = 0x00;
            if(mode_base == 0x0C) { // Heat mode
                if(fan_bits == 0x00) { // Auto fan
                    clear_mask = 0x48; // Clear bits 3 and 6
                } else { // Non-auto fan
                    if(byte3 & 0x20) { // bit5 set (higher temp)
                        clear_mask = 0x28; // Clear bits 3 and 5
                        set_bits = 0x00;
                    } else { // bit5 not set (lower temp)
                        clear_mask = 0x48; // Clear bits 3 and 6
                        set_bits = 0x20; // Set bit 5
                    }
                }
            } else if(mode_base == 0x09) { // Cool/Dry/Fan mode
                if(fan_bits == 0x00) { // Auto fan
                    clear_mask = 0x80; // Clear bit 7
                    set_bits = 0x40; // Set bit 6
                } else { // Non-auto fan
                    if(byte3 & 0x20) { // bit5 set (higher temp)
                        clear_mask = 0x20; // Clear bit 5
                    } else { // bit5 not set (lower temp)
                        clear_mask = 0x80; // Clear bit 7
                        set_bits = 0x60; // Set bits 5 and 6
                    }
                }
            } else { // Auto mode or Power off
                if(fan_bits == 0x00) { // Auto fan
                    clear_mask = 0x40; // Clear bit 6
                } else { // Non-auto fan
                    clear_mask = 0x20; // Clear bit 5
                }
            }
            modified_frame2[3] = (byte3 & ~clear_mask) | set_bits;
            
            hvac_gree_send_bits(timings, &idx, modified_frame2, 4);
        } else {
            hvac_gree_send_bits(timings, &idx, packet + 4, 4);
        }
        
        timings[idx++] = HVAC_GREE_END_MARK;
        
        // Only add repeat gap if this is not the last repeat
        if(repeat < 1) {
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
