/**
 * @file arinc429.h
 * @brief High-Precision ARINC 429 Avionics Protocol Engine Header
 * @author Amit Bakale (Avionics & Embedded Software Engineer)
 *
 * Conforms to ARINC Specification 429-16 (Digital Information Transfer System - DITS)
 */

#ifndef ARINC429_H
#define ARINC429_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ARINC 429 Bit Position Constants (0-indexed) */
#define ARINC429_LABEL_SHIFT      0
#define ARINC429_LABEL_MASK       0x000000FFU

#define ARINC429_SDI_SHIFT        8
#define ARINC429_SDI_MASK         0x00000300U

#define ARINC429_DATA_SHIFT       10
#define ARINC429_DATA_MASK        0x1FFFFC00U

#define ARINC429_SSM_SHIFT        29
#define ARINC429_SSM_MASK         0x60000000U

#define ARINC429_PARITY_SHIFT     31
#define ARINC429_PARITY_MASK      0x80000000U

/* Sign / Status Matrix (SSM) for BNR (Binary) */
typedef enum {
    ARINC_SSM_BNR_FAILURE_WARNING   = 0x00, /* 00b: Hardware/System Failure */
    ARINC_SSM_BNR_NO_COMPUTED_DATA  = 0x01, /* 01b: Sensor initializing / Inactive */
    ARINC_SSM_BNR_FUNCTIONAL_TEST   = 0x02, /* 10b: Maintenance / Self-test */
    ARINC_SSM_BNR_NORMAL_OPERATION  = 0x03  /* 11b: Data valid and certified */
} arinc_ssm_bnr_t;

/* Sign / Status Matrix (SSM) for BCD (Binary Coded Decimal) */
typedef enum {
    ARINC_SSM_BCD_PLUS_NORTH_EAST_RIGHT = 0x00, /* 00b */
    ARINC_SSM_BCD_NO_COMPUTED_DATA      = 0x01, /* 01b */
    ARINC_SSM_BCD_FUNCTIONAL_TEST       = 0x02, /* 10b */
    ARINC_SSM_BCD_MINUS_SOUTH_WEST_LEFT = 0x03  /* 11b */
} arinc_ssm_bcd_t;

/* Common Standard Avionics Equipment Labels (Represented in Octal) */
#define ARINC_LABEL_SELECTED_ALTITUDE    0203  /* Pressure Altitude / Flight Level */
#define ARINC_LABEL_BARO_ALTITUDE        0204  /* Baro Corrected Altitude */
#define ARINC_LABEL_TRUE_AIRSPEED        0210  /* True Airspeed (TAS) in Knots */
#define ARINC_LABEL_MAGNETIC_HEADING     0320  /* Magnetic Heading in Degrees */
#define ARINC_LABEL_PITCH_ANGLE          0324  /* Aircraft Pitch Angle (deg) */
#define ARINC_LABEL_ROLL_ANGLE           0325  /* Aircraft Roll / Bank Angle (deg) */
#define ARINC_LABEL_BODY_PITCH_RATE      0326  /* Pitch Rate (deg/s) */
#define ARINC_LABEL_BODY_ROLL_RATE       0327  /* Roll Rate (deg/s) */
#define ARINC_LABEL_VERTICAL_SPEED       0365  /* Inertial Vertical Speed (ft/min) */

/**
 * @brief Decoded ARINC 429 telemetry package
 */
typedef struct {
    uint32_t raw_word;             /**< Full 32-bit hex word */
    uint8_t  label_raw;            /**< Raw 8-bit label received on wire */
    uint8_t  label_octal;          /**< Octal decoded label (bit-reversed) */
    uint8_t  sdi;                  /**< Source / Destination Identifier (0-3) */
    uint32_t data_raw;             /**< 19-bit payload (bits 11..29) */
    uint8_t  ssm;                  /**< Sign / Status Matrix (bits 30..31) */
    bool     parity_bit;           /**< Raw 32nd parity bit */
    bool     is_parity_valid;      /**< True if odd parity satisfies specification */
    
    /* Engineering Interpretation */
    double   engineering_value;    /**< Converted physical measurement */
    const char* units;             /**< Unit string (e.g. "feet", "knots", "deg") */
    const char* parameter_name;    /**< Descriptive avionics symbol */
    const char* ssm_status_str;    /**< SSM textual translation */
} arinc_decoded_t;

/**
 * @brief Reverse 8-bit label bit ordering (ARINC 429 wire serialization requirement)
 */
uint8_t arinc429_reverse_label(uint8_t label);

/**
 * @brief Calculate odd parity bit for a 32-bit ARINC 429 word (bits 1..31)
 */
uint8_t arinc429_calculate_odd_parity(uint32_t word_without_parity);

/**
 * @brief Verify that the word satisfies ARINC 429 odd parity
 */
bool arinc429_verify_odd_parity(uint32_t full_word);

/**
 * @brief Encode a physical engineering value into an ARINC 429 BNR 32-bit word
 */
uint32_t arinc429_encode_bnr(uint8_t octal_label, uint8_t sdi, double value, 
                            double max_range, uint8_t sig_bits, arinc_ssm_bnr_t ssm);

/**
 * @brief Decode an incoming 32-bit ARINC 429 word into engineering telemetry
 */
arinc_decoded_t arinc429_decode(uint32_t raw_word);

/**
 * @brief Pretty print decoded ARINC 429 telemetry structure
 */
void arinc429_print_telemetry(const arinc_decoded_t* decoded);

#ifdef __cplusplus
}
#endif

#endif /* ARINC429_H */
