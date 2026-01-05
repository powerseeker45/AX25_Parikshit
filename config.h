/**
 * @file config.h
 * @brief Configuration parameters for AX.25 protocol
 * 
 * Defines callsigns, SSIDs, and protocol constants
 * Following NASA JPL C Coding Standard
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* Satellite and Ground Station Identifiers */
static const char SAT_CALLSIGN[] = "PARSAT";
static const uint8_t SAT_SSID = 0U;

static const char GRD_CALLSIGN[] = "ABCD";
static const uint8_t GRD_SSID = 0U;

/* AX.25 Frame Length Constraints */
#define AX25_MAX_FRAME_LEN          256U    /* Maximum frame size in bytes */
#define AX25_MAX_INFO_LEN           240U    /* Maximum info field size */
#define AX25_MIN_FRAME_LEN          18U     /* Minimum valid frame size */

/* Address Field Lengths */
#define AX25_MAX_ADDR_LEN           28U     /* Maximum address field (2 addresses + digipeaters) */
#define AX25_MIN_ADDR_LEN           14U     /* Minimum address field (dest + src) */
#define AX25_CALLSIGN_MAX_LEN       6U      /* Maximum callsign length */
#define AX25_CALLSIGN_MIN_LEN       2U      /* Minimum callsign length */

/* Control Field Lengths */
#define AX25_MIN_CTRL_LEN           1U      /* 1-byte control field */
#define AX25_MAX_CTRL_LEN           2U      /* 2-byte control field */

/* Flag and Protocol Constants */
#define AX25_FLAG                   0x7EU   /* Frame delimiter flag (01111110) */
#define AX25_PID_NO_LAYER3          0xF0U   /* PID: No layer 3 protocol */

/* Bit Stuffing Constraints */
#define AX25_MAX_CONSECUTIVE_ONES   5U      /* Maximum consecutive 1s before stuffing */
#define AX25_ABORT_SEQUENCE_LEN     7U      /* 7 consecutive 1s = abort */
#define AX25_MAX_STUFFED_BITS       14U     /* Maximum consecutive 1s allowed total */

/* CRC Parameters */
#define AX25_FCS_LEN                2U      /* FCS field length in bytes */
#define AX25_FCS_INIT               0xFFFFU /* FCS initial value */
#define AX25_FCS_XOR                0xFFFFU /* FCS final XOR value */

/* Preamble and Postamble (for HDLC framing) */
#define AX25_PREAMBLE_LEN           16U     /* Preamble length in bytes */
#define AX25_POSTAMBLE_LEN          16U     /* Postamble length in bytes */

/* Control Field Values */
#define AX25_CTRL_UI                0x03U   /* Unnumbered Information frame */
#define AX25_CTRL_I_FRAME_MASK      0x01U   /* I-frame identifier mask */
#define AX25_CTRL_S_FRAME_MASK      0x03U   /* S-frame identifier mask */

/* SSID and Address Byte Masks */
#define AX25_SSID_MASK              0x0FU   /* SSID field mask (bits 1-4) */
#define AX25_RESERVED_BITS          0x60U   /* Reserved bits in SSID byte */
#define AX25_LAST_ADDR_BIT          0x01U   /* Last address field indicator */
#define AX25_CALLSIGN_SHIFT         1U      /* Shift amount for callsign encoding */

/* Matrix Encoding Parameters */
#define MATRIX_CHUNK_SIZE           200U    /* Bytes per matrix chunk */
#define MATRIX_MAX_ROWS             1000U   /* Maximum matrix rows */
#define MATRIX_MAX_COLS             1000U   /* Maximum matrix columns */

/* Return Status Codes */
#define AX25_SUCCESS                0       /* Operation successful */
#define AX25_ERROR                  (-1)    /* Generic error */
#define AX25_ERROR_INVALID_PARAM    (-2)    /* Invalid parameter */
#define AX25_ERROR_BUFFER_OVERFLOW  (-3)    /* Buffer overflow */
#define AX25_ERROR_FCS_MISMATCH     (-4)    /* FCS verification failed */

#endif /* CONFIG_H */
