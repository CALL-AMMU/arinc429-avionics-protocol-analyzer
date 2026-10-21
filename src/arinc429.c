/**
 * @file arinc429.c
 * @brief High-Precision ARINC 429 Avionics Protocol Engine Implementation
 * @author Amit Bakale (Avionics & Embedded Software Engineer)
 */

#include "arinc429.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Count set bits (popcount) helper */
static inline int count_set_bits(uint32_t val) {
    int count = 0;
    while (val) {
        count += (val & 1U);
        val >>= 1U;
    }
    return count;
}

uint8_t arinc429_reverse_label(uint8_t label) {
    uint8_t rev = 0;
    for (int i = 0; i < 8; i++) {
        if (label & (1U << i)) {
            rev |= (uint8_t)(1U << (7 - i));
        }
    }
    return rev;
}

uint8_t arinc429_calculate_odd_parity(uint32_t word_without_parity) {
    uint32_t masked = word_without_parity & 0x7FFFFFFFU;
    int ones = count_set_bits(masked);
    /* For odd parity, if count of 1s is even, parity bit must be 1 */
    return (uint8_t)((ones % 2 == 0) ? 1 : 0);
}

bool arinc429_verify_odd_parity(uint32_t full_word) {
    int ones = count_set_bits(full_word);
    return (ones % 2 != 0);
}

uint32_t arinc429_encode_bnr(uint8_t octal_label, uint8_t sdi, double value, 
                            double max_range, uint8_t sig_bits, arinc_ssm_bnr_t ssm) {
    uint32_t word = 0;

    /* 1. Wire label is bit-reversed octal label */
    uint8_t wire_label = arinc429_reverse_label(octal_label);
    word |= ((uint32_t)wire_label & ARINC429_LABEL_MASK);

    /* 2. SDI bits (8..9) */
    word |= (((uint32_t)sdi << ARINC429_SDI_SHIFT) & ARINC429_SDI_MASK);

    /* 3. Scale physical value to two's complement integer */
    if (sig_bits > 19) sig_bits = 19;
    double lsb = max_range / (double)(1U << sig_bits);
    int32_t raw_int = (int32_t)round(value / lsb);
    
    /* Clamp within range */
    int32_t max_val = (1 << sig_bits) - 1;
    int32_t min_val = -(1 << sig_bits);
    if (raw_int > max_val) raw_int = max_val;
    if (raw_int < min_val) raw_int = min_val;

    /* Left align or map within 19-bit data field (bits 10..28) */
    /* In ARINC 429 BNR, bit 28 is MSB/Sign. Bit 28-(sig_bits)+1 is LSB */
    uint32_t data_field = ((uint32_t)raw_int) & 0x7FFFFU;
    word |= ((data_field << ARINC429_DATA_SHIFT) & ARINC429_DATA_MASK);

    /* 4. SSM (bits 29..30) */
    word |= (((uint32_t)ssm << ARINC429_SSM_SHIFT) & ARINC429_SSM_MASK);

    /* 5. Parity bit 31 */
    uint8_t parity = arinc429_calculate_odd_parity(word);
    word |= ((uint32_t)parity << ARINC429_PARITY_SHIFT);

    return word;
}

arinc_decoded_t arinc429_decode(uint32_t raw_word) {
    arinc_decoded_t dec;
    memset(&dec, 0, sizeof(dec));

    dec.raw_word = raw_word;
    dec.label_raw = (uint8_t)(raw_word & ARINC429_LABEL_MASK);
    dec.label_octal = arinc429_reverse_label(dec.label_raw);
    dec.sdi = (uint8_t)((raw_word & ARINC429_SDI_MASK) >> ARINC429_SDI_SHIFT);
    dec.data_raw = (raw_word & ARINC429_DATA_MASK) >> ARINC429_DATA_SHIFT;
    dec.ssm = (uint8_t)((raw_word & ARINC429_SSM_MASK) >> ARINC429_SSM_SHIFT);
    dec.parity_bit = (bool)((raw_word & ARINC429_PARITY_MASK) >> ARINC429_PARITY_SHIFT);
    dec.is_parity_valid = arinc429_verify_odd_parity(raw_word);

    /* Determine SSM textual status */
    switch (dec.ssm) {
        case ARINC_SSM_BNR_FAILURE_WARNING:
            dec.ssm_status_str = "FAILURE WARNING (FW)";
            break;
        case ARINC_SSM_BNR_NO_COMPUTED_DATA:
            dec.ssm_status_str = "NO COMPUTED DATA (NCD)";
            break;
        case ARINC_SSM_BNR_FUNCTIONAL_TEST:
            dec.ssm_status_str = "FUNCTIONAL TEST (FT)";
            break;
        case ARINC_SSM_BNR_NORMAL_OPERATION:
            dec.ssm_status_str = "NORMAL OPERATION (NO)";
            break;
        default:
            dec.ssm_status_str = "UNKNOWN";
            break;
    }

    /* Decode Engineering Values based on Octal Label */
    switch (dec.label_octal) {
        case ARINC_LABEL_SELECTED_ALTITUDE: {
            dec.parameter_name = "Pressure Altitude (FL)";
            dec.units = "feet";
            /* 17 bits signed, LSB = 1.0 ft, Range = 131,072 ft */
            int32_t signed_data = (int32_t)dec.data_raw;
            /* Sign extend from bit 18 (bit 28 of word) */
            if (signed_data & (1 << 18)) {
                signed_data |= ~0x7FFFF;
            }
            dec.engineering_value = (double)signed_data * 1.0;
            break;
        }
        case ARINC_LABEL_BARO_ALTITUDE: {
            dec.parameter_name = "Barometric Altitude";
            dec.units = "feet";
            int32_t signed_data = (int32_t)dec.data_raw;
            if (signed_data & (1 << 18)) {
                signed_data |= ~0x7FFFF;
            }
            dec.engineering_value = (double)signed_data * 1.0;
            break;
        }
        case ARINC_LABEL_TRUE_AIRSPEED: {
            dec.parameter_name = "True Airspeed (TAS)";
            dec.units = "knots";
            /* 14 bits, Range = 2048 knots, LSB = 0.125 knots */
            int32_t signed_data = (int32_t)dec.data_raw;
            if (signed_data & (1 << 18)) {
                signed_data |= ~0x7FFFF;
            }
            dec.engineering_value = (double)signed_data * 0.125;
            break;
        }
        case ARINC_LABEL_MAGNETIC_HEADING: {
            dec.parameter_name = "Magnetic Heading";
            dec.units = "deg";
            /* 12 bits, Range = 180 deg, LSB = 180 / 4096 = 0.043945 deg */
            int32_t signed_data = (int32_t)dec.data_raw;
            if (signed_data & (1 << 18)) {
                signed_data |= ~0x7FFFF;
            }
            dec.engineering_value = (double)signed_data * (180.0 / 4096.0);
            if (dec.engineering_value < 0.0) dec.engineering_value += 360.0;
            break;
        }
        case ARINC_LABEL_PITCH_ANGLE: {
            dec.parameter_name = "Pitch Angle";
            dec.units = "deg";
            /* 14 bits, Range = 180 deg, LSB = 180 / 16384 = 0.010986 deg */
            int32_t signed_data = (int32_t)dec.data_raw;
            if (signed_data & (1 << 18)) {
                signed_data |= ~0x7FFFF;
            }
            dec.engineering_value = (double)signed_data * (180.0 / 16384.0);
            break;
        }
        case ARINC_LABEL_ROLL_ANGLE: {
            dec.parameter_name = "Roll Angle";
            dec.units = "deg";
            int32_t signed_data = (int32_t)dec.data_raw;
            if (signed_data & (1 << 18)) {
                signed_data |= ~0x7FFFF;
            }
            dec.engineering_value = (double)signed_data * (180.0 / 16384.0);
            break;
        }
        case ARINC_LABEL_VERTICAL_SPEED: {
            dec.parameter_name = "Vertical Speed";
            dec.units = "ft/min";
            /* 11 bits, Range = 32768 ft/min, LSB = 16 ft/min */
            int32_t signed_data = (int32_t)dec.data_raw;
            if (signed_data & (1 << 18)) {
                signed_data |= ~0x7FFFF;
            }
            dec.engineering_value = (double)signed_data * 16.0;
            break;
        }
        default: {
            dec.parameter_name = "General Discrete / Payload";
            dec.units = "raw";
            dec.engineering_value = (double)dec.data_raw;
            break;
        }
    }

    return dec;
}

void arinc429_print_telemetry(const arinc_decoded_t* dec) {
    printf("Word: 0x%08X | Octal: %03o (Raw: 0x%02X) | SDI: %u | SSM: %s | Val: %8.2f %-6s | Parity: %s\n",
           dec->raw_word,
           dec->label_octal,
           dec->label_raw,
           dec->sdi,
           dec->ssm_status_str,
           dec->engineering_value,
           dec->units,
           dec->is_parity_valid ? "VALID [ODD]" : "ERROR [FAIL]");
}
