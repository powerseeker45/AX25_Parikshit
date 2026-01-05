/**
 * @file ax25.h
 * @brief AX.25 Protocol Implementation Header
 * 
 * Provides functions for encoding and decoding AX.25 frames
 * Supports large 2D matrix transmission with automatic chunking
 * 
 * Follows NASA JPL C Coding Standard:
 * - Rule 1: Restrict to simple control flow
 * - Rule 2: Fixed upper bound on loop iterations
 * - Rule 3: No dynamic memory allocation after initialization
 * - Rule 4: Function length <= 60 lines
 * - Rule 5: Assertion density >= 2 per function
 * - Rule 6: Data objects declared at smallest scope
 * - Rule 7: Check return values
 * - Rule 8: Limit preprocessor use
 * - Rule 9: Limit pointer use
 * - Rule 10: Compile with all warnings enabled
 */

#ifndef AX25_H
#define AX25_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "config.h"
#include "utils.h"

/**
 * @brief AX.25 Frame Types
 */
typedef enum {
    AX25_I_FRAME = 0,   /**< Information frame */
    AX25_S_FRAME,       /**< Supervisory frame */
    AX25_U_FRAME,       /**< Unnumbered frame */
    AX25_UI_FRAME       /**< Unnumbered Information frame */
} ax25_frame_type_t;

/**
 * @brief AX.25 Encoding Status
 */
typedef enum {
    AX25_ENC_FAIL = 0,  /**< Encoding failed */
    AX25_ENC_OK         /**< Encoding successful */
} ax25_encode_status_t;

/**
 * @brief AX.25 Decoding Status
 */
typedef enum {
    AX25_DEC_FAIL = 0,  /**< Decoding failed */
    AX25_DEC_OK         /**< Decoding successful */
} ax25_decode_status_t;

/**
 * @brief Matrix Metadata Structure
 */
typedef struct {
    uint16_t total_chunks;      /**< Total number of chunks */
    uint16_t chunk_index;       /**< Current chunk index (0-based) */
    uint16_t rows;              /**< Matrix rows */
    uint16_t cols;              /**< Matrix columns */
    uint16_t data_len;          /**< Data length in this chunk */
    uint8_t element_size;       /**< Size of each matrix element in bytes */
} matrix_metadata_t;

/**
 * @brief AX.25 Sync Flag Pattern (binary representation)
 */
extern const uint8_t AX25_SYNC_FLAG_MAP_BIN[8];

/**
 * @brief Creates AX.25 address field
 * 
 * @param[out] out Output buffer (min AX25_MIN_ADDR_LEN bytes)
 * @param[in] dest_addr Destination callsign
 * @param[in] dest_ssid Destination SSID (0-15)
 * @param[in] src_addr Source callsign
 * @param[in] src_ssid Source SSID (0-15)
 * @return Number of bytes written
 * 
 * @pre out != NULL
 * @pre dest_addr != NULL
 * @pre src_addr != NULL
 * @pre dest_ssid <= 15
 * @pre src_ssid <= 15
 */
size_t ax25_create_addr_field(
    uint8_t *out,
    const uint8_t *dest_addr,
    uint8_t dest_ssid,
    const uint8_t *src_addr,
    uint8_t src_ssid
);

/**
 * @brief Calculates Frame Check Sequence (FCS)
 * 
 * @param[in] buffer Data buffer
 * @param[in] len Buffer length
 * @return Calculated FCS value
 * 
 * @pre buffer != NULL
 * @pre len > 0
 */
uint16_t ax25_fcs(const uint8_t *buffer, size_t len);

/**
 * @brief Creates complete AX.25 frame
 * 
 * @param[out] out Output buffer (min AX25_MAX_FRAME_LEN bytes)
 * @param[in] info Information field data
 * @param[in] info_len Length of information field
 * @param[in] type Frame type
 * @param[in] addr Address field
 * @param[in] addr_len Address field length
 * @param[in] ctrl Control field value
 * @param[in] ctrl_len Control field length
 * @return Frame length in bytes, 0 on error
 * 
 * @pre out != NULL
 * @pre addr != NULL
 * @pre info_len <= AX25_MAX_INFO_LEN
 * @pre addr_len == AX25_MIN_ADDR_LEN || addr_len == AX25_MAX_ADDR_LEN
 */
size_t ax25_create_frame(
    uint8_t *out,
    const uint8_t *info,
    size_t info_len,
    ax25_frame_type_t type,
    const uint8_t *addr,
    size_t addr_len,
    uint16_t ctrl,
    size_t ctrl_len
);

/**
 * @brief Performs bit stuffing on frame
 * 
 * @param[out] out Output bit buffer
 * @param[out] out_len Output length in bits
 * @param[in] buffer Input frame buffer
 * @param[in] buffer_len Input frame length
 * @return Encoding status
 * 
 * @pre out != NULL
 * @pre out_len != NULL
 * @pre buffer != NULL
 * @pre buffer_len > 2
 */
ax25_encode_status_t ax25_bit_stuffing(
    uint8_t *out,
    size_t *out_len,
    const uint8_t *buffer,
    size_t buffer_len
);

/**
 * @brief Encodes data into AX.25 frame
 * 
 * @param[out] out Output buffer for encoded frame
 * @param[in] in Input data
 * @param[in] inlen Input data length
 * @param[in] type Frame type
 * @return Encoded frame length in bytes, negative on error
 * 
 * @pre out != NULL
 * @pre in != NULL
 * @pre inlen <= AX25_MAX_INFO_LEN
 */
int32_t ax25_encode(
    uint8_t *out,
    const uint8_t *in,
    size_t inlen,
    ax25_frame_type_t type
);

/**
 * @brief Decodes AX.25 frame (bit-level)
 * 
 * @param[out] out Decoded output buffer
 * @param[out] out_len Decoded data length
 * @param[in] ax25_frame Input frame (bit array)
 * @param[in] len Frame length in bits
 * @return Decoding status
 * 
 * @pre out != NULL
 * @pre out_len != NULL
 * @pre ax25_frame != NULL
 */
ax25_decode_status_t ax25_decode(
    uint8_t *out,
    size_t *out_len,
    const uint8_t *ax25_frame,
    size_t len
);

/**
 * @brief Receives and decodes AX.25 frame
 * 
 * @param[out] out Decoded output buffer
 * @param[in] in Input frame buffer
 * @param[in] len Input length in bytes
 * @return Decoded length in bytes, negative on error
 * 
 * @pre out != NULL
 * @pre in != NULL
 */
int32_t ax25_recv(
    uint8_t *out,
    const uint8_t *in,
    size_t len
);

/**
 * @brief Encodes 2D matrix into multiple AX.25 frames
 * 
 * @param[out] frames Output buffer for encoded frames
 * @param[out] frame_count Number of frames created
 * @param[in] matrix Input matrix data
 * @param[in] rows Number of rows
 * @param[in] cols Number of columns
 * @param[in] element_size Size of each element in bytes
 * @return Total bytes written, negative on error
 * 
 * @pre frames != NULL
 * @pre frame_count != NULL
 * @pre matrix != NULL
 * @pre rows > 0 && rows <= MATRIX_MAX_ROWS
 * @pre cols > 0 && cols <= MATRIX_MAX_COLS
 * @pre element_size > 0
 */
int32_t ax25_encode_matrix(
    uint8_t *frames,
    size_t *frame_count,
    const uint8_t *matrix,
    uint16_t rows,
    uint16_t cols,
    uint8_t element_size
);

/**
 * @brief Decodes multiple AX.25 frames into 2D matrix
 * 
 * @param[out] matrix Output matrix buffer
 * @param[out] rows Number of rows decoded
 * @param[out] cols Number of columns decoded
 * @param[out] element_size Element size in bytes
 * @param[in] frames Input encoded frames
 * @param[in] frame_count Number of frames
 * @return Total bytes decoded, negative on error
 * 
 * @pre matrix != NULL
 * @pre rows != NULL
 * @pre cols != NULL
 * @pre element_size != NULL
 * @pre frames != NULL
 * @pre frame_count > 0
 */
int32_t ax25_decode_matrix(
    uint8_t *matrix,
    uint16_t *rows,
    uint16_t *cols,
    uint8_t *element_size,
    const uint8_t *frames,
    size_t frame_count
);

#endif /* AX25_H */
