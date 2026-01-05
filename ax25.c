/**
 * @file ax25.c
 * @brief AX.25 Protocol Implementation
 * 
 * Implementation of AX.25 frame encoding/decoding with support
 * for large 2D matrix transmission
 * 
 * Follows NASA JPL C Coding Standard
 */

#define _POSIX_C_SOURCE 200809L

#include "ax25.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* AX.25 Sync Flag Binary Pattern */
const uint8_t AX25_SYNC_FLAG_MAP_BIN[8] = {0, 1, 1, 1, 1, 1, 1, 0};

/**
 * @brief Creates the address field of the AX.25 frame
 */
size_t ax25_create_addr_field(
    uint8_t *out,
    const uint8_t *dest_addr,
    uint8_t dest_ssid,
    const uint8_t *src_addr,
    uint8_t src_ssid)
{
    uint16_t i = 0U;
    size_t dest_len = 0U;
    size_t src_len = 0U;

    /* Preconditions */
    assert(out != NULL);
    assert(dest_addr != NULL);
    assert(src_addr != NULL);
    assert(dest_ssid <= 15U);
    assert(src_ssid <= 15U);

    /* Get string lengths safely */
    dest_len = strnlen((const char *)dest_addr, AX25_CALLSIGN_MAX_LEN);
    src_len = strnlen((const char *)src_addr, AX25_CALLSIGN_MAX_LEN);

    /* Encode destination callsign */
    for (i = 0U; i < dest_len && i < AX25_CALLSIGN_MAX_LEN; i++) {
        *out = dest_addr[i] << AX25_CALLSIGN_SHIFT;
        out++;
    }

    /* Pad with spaces if necessary */
    for (; i < AX25_CALLSIGN_MAX_LEN; i++) {
        *out = (uint8_t)(' ' << AX25_CALLSIGN_SHIFT);
        out++;
    }

    /* Add SSID byte for destination (C bit = 0) */
    *out = (uint8_t)(((dest_ssid & AX25_SSID_MASK) << AX25_CALLSIGN_SHIFT) | 
                     AX25_RESERVED_BITS);
    out++;

    /* Encode source callsign */
    for (i = 0U; i < src_len && i < AX25_CALLSIGN_MAX_LEN; i++) {
        *out = src_addr[i] << AX25_CALLSIGN_SHIFT;
        out++;
    }

    /* Pad with spaces if necessary */
    for (; i < AX25_CALLSIGN_MAX_LEN; i++) {
        *out = (uint8_t)(' ' << AX25_CALLSIGN_SHIFT);
        out++;
    }

    /* Add SSID byte for source (last address, so set bit 0) */
    *out = (uint8_t)(((src_ssid & AX25_SSID_MASK) << AX25_CALLSIGN_SHIFT) | 
                     AX25_RESERVED_BITS | AX25_LAST_ADDR_BIT);

    return AX25_MIN_ADDR_LEN;
}

/**
 * @brief Calculates the FCS of the AX25 frame
 */
uint16_t ax25_fcs(const uint8_t *buffer, size_t len)
{
    uint16_t fcs = AX25_FCS_INIT;
    size_t i = 0U;

    /* Preconditions */
    assert(buffer != NULL);
    assert(len > 0U);

    /* Calculate CRC using lookup table */
    for (i = 0U; i < len; i++) {
        fcs = (fcs >> 8) ^ 
              crc16_ccitt_table_reverse[(fcs ^ buffer[i]) & 0xFFU];
    }

    return fcs ^ AX25_FCS_XOR;
}

/**
 * @brief Creates complete AX.25 frame
 */
size_t ax25_create_frame(
    uint8_t *out,
    const uint8_t *info,
    size_t info_len,
    ax25_frame_type_t type,
    const uint8_t *addr,
    size_t addr_len,
    uint16_t ctrl,
    size_t ctrl_len)
{
    size_t idx = 0U;
    size_t j = 0U;
    uint16_t fcs = 0U;

    /* Preconditions */
    assert(out != NULL);
    assert(addr != NULL);
    assert(info_len <= AX25_MAX_INFO_LEN);
    assert((addr_len == AX25_MIN_ADDR_LEN) || (addr_len == AX25_MAX_ADDR_LEN));
    assert((ctrl_len == AX25_MIN_CTRL_LEN) || (ctrl_len == AX25_MAX_CTRL_LEN));

    /* Validate info length */
    if (info_len > AX25_MAX_INFO_LEN) {
        return 0U;
    }

    /* Add initial flag */
    out[idx] = AX25_FLAG;
    idx++;

    /* Add address field */
    for (j = 0U; j < addr_len; j++) {
        out[idx] = addr[j];
        idx++;
    }

    /* Add control field */
    if (ctrl_len == AX25_MIN_CTRL_LEN) {
        out[idx] = (uint8_t)(ctrl & 0xFFU);
        idx++;
    } else if (ctrl_len == AX25_MAX_CTRL_LEN) {
        out[idx] = (uint8_t)(ctrl & 0xFFU);
        idx++;
        out[idx] = (uint8_t)((ctrl >> 8) & 0xFFU);
        idx++;
    } else {
        return 0U;
    }

    /* Add PID field for I and UI frames */
    if ((type == AX25_I_FRAME) || (type == AX25_UI_FRAME)) {
        out[idx] = AX25_PID_NO_LAYER3;
        idx++;
    }

    /* Add info field */
    if (info != NULL && info_len > 0U) {
        for (j = 0U; j < info_len; j++) {
            out[idx] = info[j];
            idx++;
        }
    }

    /* Compute FCS (exclude first flag byte) */
    fcs = ax25_fcs(out + 1U, idx - 1U);

    /* Add FCS (MSB first) */
    out[idx] = (uint8_t)((fcs >> 8) & 0xFFU);
    idx++;
    out[idx] = (uint8_t)(fcs & 0xFFU);
    idx++;

    /* Add trailing flag */
    out[idx] = AX25_FLAG;
    idx++;

    return idx;
}

/**
 * @brief Performs bit stuffing on AX.25 frame
 */
ax25_encode_status_t ax25_bit_stuffing(
    uint8_t *out,
    size_t *out_len,
    const uint8_t *buffer,
    size_t buffer_len)
{
    uint8_t bit = 0U;
    size_t out_idx = 0U;
    size_t cont_1 = 0U;
    size_t i = 0U;
    size_t j = 0U;
    const uint8_t *data_ptr = NULL;

    /* Preconditions */
    assert(out != NULL);
    assert(out_len != NULL);
    assert(buffer != NULL);
    assert(buffer_len > 2U);

    /* Add leading FLAG (no bit stuffing) */
    for (j = 0U; j < 8U; j++) {
        out[out_idx] = AX25_SYNC_FLAG_MAP_BIN[j];
        out_idx++;
    }

    /* Skip leading and trailing FLAG bytes */
    data_ptr = buffer + 1U;

    /* Process each bit */
    for (i = 0U; i < (8U * (buffer_len - 2U)); i++) {
        /* Extract bit */
        bit = (data_ptr[i / 8U] >> (i % 8U)) & 0x1U;

        /* Check if we need to stuff a bit BEFORE outputting current bit */
        if (cont_1 >= AX25_MAX_CONSECUTIVE_ONES) {
            /* Insert a 0 */
            out[out_idx] = 0U;
            out_idx++;
            cont_1 = 0U;
        }

        /* Output the current bit */
        out[out_idx] = bit;
        out_idx++;

        /* Update consecutive 1s counter */
        if (bit == 1U) {
            cont_1++;
            /* Check for abort condition (too many 1s) */
            if (cont_1 > AX25_MAX_STUFFED_BITS) {
                return AX25_ENC_FAIL;
            }
        } else {
            cont_1 = 0U;
        }
    }

    /* Add trailing FLAG (no bit stuffing) */
    for (j = 0U; j < 8U; j++) {
        out[out_idx] = AX25_SYNC_FLAG_MAP_BIN[j];
        out_idx++;
    }

    *out_len = out_idx;
    return AX25_ENC_OK;
}

/**
 * @brief Main encoding function
 */
int32_t ax25_encode(
    uint8_t *out,
    const uint8_t *in,
    size_t inlen,
    ax25_frame_type_t type)
{
    uint8_t addr[AX25_MAX_ADDR_LEN];
    uint8_t interm_buffer[AX25_MAX_FRAME_LEN];
    uint8_t tmp_send_buf[AX25_MAX_FRAME_LEN * 8U + AX25_MAX_FRAME_LEN];
    size_t addrlen = 0U;
    size_t framelen = 0U;
    size_t ret_len = 0U;
    size_t pad_bits = 0U;
    size_t i = 0U;
    uint8_t ctrl = 0U;
    size_t ctrllen = 0U;
    ax25_encode_status_t status = AX25_ENC_FAIL;

    /* Preconditions */
    assert(out != NULL);
    assert(in != NULL);
    assert(inlen <= AX25_MAX_INFO_LEN);

    /* Initialize buffers */
    (void)memset(addr, 0, sizeof(addr));
    (void)memset(interm_buffer, 0, sizeof(interm_buffer));
    (void)memset(tmp_send_buf, 0, sizeof(tmp_send_buf));

    /* Create address field */
    addrlen = ax25_create_addr_field(
        addr,
        (const uint8_t *)GRD_CALLSIGN,
        GRD_SSID,
        (const uint8_t *)SAT_CALLSIGN,
        SAT_SSID
    );

    /* Set control field based on frame type */
    if (type == AX25_UI_FRAME) {
        ctrl = AX25_CTRL_UI;
        ctrllen = AX25_MIN_CTRL_LEN;
    } else {
        return AX25_ERROR_INVALID_PARAM;
    }

    /* Create frame */
    framelen = ax25_create_frame(
        interm_buffer,
        in,
        inlen,
        type,
        addr,
        addrlen,
        ctrl,
        ctrllen
    );

    if (framelen == 0U) {
        return AX25_ERROR;
    }

    /* Perform bit stuffing */
    status = ax25_bit_stuffing(tmp_send_buf, &ret_len, interm_buffer, framelen);
    if (status != AX25_ENC_OK) {
        return AX25_ERROR;
    }

    /* Pack bits into bytes */
    (void)memset(out, 0, (ret_len / 8U + 1U) * sizeof(uint8_t));
    for (i = 0U; i < ret_len; i++) {
        out[i / 8U] |= (uint8_t)(tmp_send_buf[i] << (7U - (i % 8U)));
    }

    /* Add padding */
    pad_bits = 8U - (ret_len % 8U);
    if (pad_bits < 8U) {
        ret_len += pad_bits;
        out[ret_len / 8U] &= (uint8_t)(0xFFU << pad_bits);
    }

    return (int32_t)(ret_len / 8U);
}

/**
 * @brief Decodes AX.25 frame from bit stream
 */
ax25_decode_status_t ax25_decode(
    uint8_t *out,
    size_t *out_len,
    const uint8_t *ax25_frame,
    size_t len)
{
    size_t i = 0U;
    size_t frame_start = UINT_MAX;
    size_t frame_stop = UINT_MAX;
    uint8_t res = 0U;
    size_t cont_1 = 0U;
    size_t received_bytes = 0U;
    size_t bit_cnt = 0U;
    uint8_t decoded_byte = 0x0U;
    uint16_t fcs = 0U;
    uint16_t recv_fcs = 0U;

    /* Preconditions */
    assert(out != NULL);
    assert(out_len != NULL);
    assert(ax25_frame != NULL);

    /* Search for start flag */
    for (i = 0U; i < (len - 8U); i++) {
        res = (AX25_SYNC_FLAG_MAP_BIN[0] ^ ax25_frame[i]) |
              (AX25_SYNC_FLAG_MAP_BIN[1] ^ ax25_frame[i + 1U]) |
              (AX25_SYNC_FLAG_MAP_BIN[2] ^ ax25_frame[i + 2U]) |
              (AX25_SYNC_FLAG_MAP_BIN[3] ^ ax25_frame[i + 3U]) |
              (AX25_SYNC_FLAG_MAP_BIN[4] ^ ax25_frame[i + 4U]) |
              (AX25_SYNC_FLAG_MAP_BIN[5] ^ ax25_frame[i + 5U]) |
              (AX25_SYNC_FLAG_MAP_BIN[6] ^ ax25_frame[i + 6U]) |
              (AX25_SYNC_FLAG_MAP_BIN[7] ^ ax25_frame[i + 7U]);

        if (res == 0U) {
            frame_start = i;
            break;
        }
    }

    /* Check if start flag found */
    if (frame_start == UINT_MAX) {
        return AX25_DEC_FAIL;
    }

    /* Decode frame content */
    cont_1 = 0U;
    for (i = frame_start + 8U; i < (len - 7U); i++) {
        /* Check for end flag */
        res = (AX25_SYNC_FLAG_MAP_BIN[0] ^ ax25_frame[i]) |
              (AX25_SYNC_FLAG_MAP_BIN[1] ^ ax25_frame[i + 1U]) |
              (AX25_SYNC_FLAG_MAP_BIN[2] ^ ax25_frame[i + 2U]) |
              (AX25_SYNC_FLAG_MAP_BIN[3] ^ ax25_frame[i + 3U]) |
              (AX25_SYNC_FLAG_MAP_BIN[4] ^ ax25_frame[i + 4U]) |
              (AX25_SYNC_FLAG_MAP_BIN[5] ^ ax25_frame[i + 5U]) |
              (AX25_SYNC_FLAG_MAP_BIN[6] ^ ax25_frame[i + 6U]) |
              (AX25_SYNC_FLAG_MAP_BIN[7] ^ ax25_frame[i + 7U]);

        if (res == 0U) {
            frame_stop = i;
            break;
        }

        /* Process bit */
        if (ax25_frame[i] == 1U) {
            cont_1++;

            /* Check if this is a stuffed bit (after 5 consecutive 1s) */
            if (cont_1 > AX25_MAX_CONSECUTIVE_ONES) {
                /* This should be a stuffed 0, but we got a 1 - error! */
                return AX25_DEC_FAIL;
            }

            /* Add bit to decoded byte */
            decoded_byte |= (uint8_t)(1U << bit_cnt);
            bit_cnt++;
        } else {
            /* Bit is 0 */
            if (cont_1 >= AX25_MAX_CONSECUTIVE_ONES) {
                /* This is a stuffed bit, skip it */
                cont_1 = 0U;
            } else {
                /* Normal 0 bit */
                bit_cnt++;
                cont_1 = 0U;
            }
        }

        /* Store complete byte */
        if (bit_cnt == 8U) {
            out[received_bytes] = decoded_byte;
            received_bytes++;
            bit_cnt = 0U;
            decoded_byte = 0x0U;
        }
    }

    /* Validate frame */
    if ((frame_stop == UINT_MAX) || (received_bytes < AX25_MIN_ADDR_LEN)) {
        return AX25_DEC_FAIL;
    }

    /* Verify FCS */
    fcs = ax25_fcs(out, received_bytes - AX25_FCS_LEN);
    recv_fcs = (uint16_t)(((uint16_t)out[received_bytes - 2U] << 8) | 
                          out[received_bytes - 1U]);

    if (fcs != recv_fcs) {
        return AX25_DEC_FAIL;
    }

    *out_len = received_bytes - AX25_FCS_LEN;
    return AX25_DEC_OK;
}

/**
 * @brief Receives and decodes AX.25 frame
 */
int32_t ax25_recv(
    uint8_t *out,
    const uint8_t *in,
    size_t len)
{
    size_t i = 0U;
    size_t decode_len = 0U;
    uint8_t tmp_recv_buf[AX25_MAX_FRAME_LEN * 8U + AX25_MAX_FRAME_LEN];
    ax25_decode_status_t status = AX25_DEC_FAIL;

    /* Preconditions */
    assert(out != NULL);
    assert(in != NULL);

    /* Initialize buffer */
    (void)memset(tmp_recv_buf, 0, sizeof(tmp_recv_buf));

    /* Convert to one bit per byte */
    for (i = 0U; i < len; i++) {
        tmp_recv_buf[8U * i] = (in[i] >> 7) & 0x1U;
        tmp_recv_buf[8U * i + 1U] = (in[i] >> 6) & 0x1U;
        tmp_recv_buf[8U * i + 2U] = (in[i] >> 5) & 0x1U;
        tmp_recv_buf[8U * i + 3U] = (in[i] >> 4) & 0x1U;
        tmp_recv_buf[8U * i + 4U] = (in[i] >> 3) & 0x1U;
        tmp_recv_buf[8U * i + 5U] = (in[i] >> 2) & 0x1U;
        tmp_recv_buf[8U * i + 6U] = (in[i] >> 1) & 0x1U;
        tmp_recv_buf[8U * i + 7U] = in[i] & 0x1U;
    }

    /* Decode */
    status = ax25_decode(out, &decode_len, tmp_recv_buf, len * 8U);
    if (status != AX25_DEC_OK) {
        return AX25_ERROR;
    }

    return (int32_t)decode_len;
}

/**
 * @brief Encodes matrix metadata into buffer
 */
static size_t encode_matrix_metadata(
    uint8_t *buffer,
    const matrix_metadata_t *meta)
{
    size_t idx = 0U;

    assert(buffer != NULL);
    assert(meta != NULL);

    buffer[idx++] = (uint8_t)((meta->total_chunks >> 8) & 0xFFU);
    buffer[idx++] = (uint8_t)(meta->total_chunks & 0xFFU);
    buffer[idx++] = (uint8_t)((meta->chunk_index >> 8) & 0xFFU);
    buffer[idx++] = (uint8_t)(meta->chunk_index & 0xFFU);
    buffer[idx++] = (uint8_t)((meta->rows >> 8) & 0xFFU);
    buffer[idx++] = (uint8_t)(meta->rows & 0xFFU);
    buffer[idx++] = (uint8_t)((meta->cols >> 8) & 0xFFU);
    buffer[idx++] = (uint8_t)(meta->cols & 0xFFU);
    buffer[idx++] = (uint8_t)((meta->data_len >> 8) & 0xFFU);
    buffer[idx++] = (uint8_t)(meta->data_len & 0xFFU);
    buffer[idx++] = meta->element_size;

    return idx;
}

/**
 * @brief Decodes matrix metadata from buffer
 */
static size_t decode_matrix_metadata(
    matrix_metadata_t *meta,
    const uint8_t *buffer)
{
    size_t idx = 0U;

    assert(meta != NULL);
    assert(buffer != NULL);

    meta->total_chunks = (uint16_t)((buffer[idx] << 8) | buffer[idx + 1U]);
    idx += 2U;
    meta->chunk_index = (uint16_t)((buffer[idx] << 8) | buffer[idx + 1U]);
    idx += 2U;
    meta->rows = (uint16_t)((buffer[idx] << 8) | buffer[idx + 1U]);
    idx += 2U;
    meta->cols = (uint16_t)((buffer[idx] << 8) | buffer[idx + 1U]);
    idx += 2U;
    meta->data_len = (uint16_t)((buffer[idx] << 8) | buffer[idx + 1U]);
    idx += 2U;
    meta->element_size = buffer[idx];
    idx++;

    return idx;
}

/**
 * @brief Encodes 2D matrix into multiple AX.25 frames
 * Format: [frame_len (2 bytes)][frame_data][frame_len][frame_data]...
 */
int32_t ax25_encode_matrix(
    uint8_t *frames,
    size_t *frame_count,
    const uint8_t *matrix,
    uint16_t rows,
    uint16_t cols,
    uint8_t element_size)
{
    size_t total_size = 0U;
    size_t chunk_data_size = 0U;
    size_t metadata_size = 0U;
    size_t chunks = 0U;
    size_t i = 0U;
    size_t offset = 0U;
    size_t remaining = 0U;
    size_t to_encode = 0U;
    int32_t encoded_len = 0;
    size_t total_bytes = 0U;
    uint8_t chunk_buffer[AX25_MAX_INFO_LEN];
    uint8_t temp_frame[AX25_MAX_FRAME_LEN * 2];
    matrix_metadata_t meta;

    /* Preconditions */
    assert(frames != NULL);
    assert(frame_count != NULL);
    assert(matrix != NULL);
    assert(rows > 0U && rows <= MATRIX_MAX_ROWS);
    assert(cols > 0U && cols <= MATRIX_MAX_COLS);
    assert(element_size > 0U);

    /* Calculate sizes */
    total_size = (size_t)rows * (size_t)cols * (size_t)element_size;
    metadata_size = sizeof(matrix_metadata_t);
    chunk_data_size = MATRIX_CHUNK_SIZE;

    if (chunk_data_size > (AX25_MAX_INFO_LEN - metadata_size)) {
        chunk_data_size = AX25_MAX_INFO_LEN - metadata_size;
    }

    chunks = (total_size + chunk_data_size - 1U) / chunk_data_size;

    /* Encode each chunk */
    for (i = 0U; i < chunks; i++) {
        (void)memset(chunk_buffer, 0, sizeof(chunk_buffer));
        (void)memset(temp_frame, 0, sizeof(temp_frame));

        remaining = total_size - offset;
        to_encode = (remaining < chunk_data_size) ? remaining : chunk_data_size;

        /* Prepare metadata */
        meta.total_chunks = (uint16_t)chunks;
        meta.chunk_index = (uint16_t)i;
        meta.rows = rows;
        meta.cols = cols;
        meta.data_len = (uint16_t)to_encode;
        meta.element_size = element_size;

        /* Encode metadata */
        metadata_size = encode_matrix_metadata(chunk_buffer, &meta);

        /* Copy data */
        (void)memcpy(chunk_buffer + metadata_size, matrix + offset, to_encode);

        /* Encode frame */
        encoded_len = ax25_encode(
            temp_frame,
            chunk_buffer,
            metadata_size + to_encode,
            AX25_UI_FRAME
        );

        if (encoded_len < 0) {
            return AX25_ERROR;
        }

        /* Store frame length (2 bytes, big-endian) */
        frames[total_bytes] = (uint8_t)((encoded_len >> 8) & 0xFFU);
        total_bytes++;
        frames[total_bytes] = (uint8_t)(encoded_len & 0xFFU);
        total_bytes++;

        /* Copy frame data */
        (void)memcpy(frames + total_bytes, temp_frame, (size_t)encoded_len);
        total_bytes += (size_t)encoded_len;

        offset += to_encode;
    }

    *frame_count = chunks;
    return (int32_t)total_bytes;
}

/**
 * @brief Decodes multiple AX.25 frames into 2D matrix
 * Format: [frame_len (2 bytes)][frame_data][frame_len][frame_data]...
 */
int32_t ax25_decode_matrix(
    uint8_t *matrix,
    uint16_t *rows,
    uint16_t *cols,
    uint8_t *element_size,
    const uint8_t *frames,
    size_t frame_count)
{
    size_t i = 0U;
    int32_t decoded_len = 0;
    size_t offset = 0U;
    size_t frame_offset = 0U;
    size_t metadata_size = 0U;
    size_t current_frame_len = 0U;
    uint8_t decode_buffer[AX25_MAX_FRAME_LEN];
    matrix_metadata_t meta;

    assert(matrix != NULL);
    assert(rows != NULL);
    assert(cols != NULL);
    assert(element_size != NULL);
    assert(frames != NULL);
    assert(frame_count > 0U);

    for (i = 0U; i < frame_count; i++) {
        memset(decode_buffer, 0, sizeof(decode_buffer));

        // Read frame length (2 bytes, big-endian)
        if (frame_offset + 2U > 50000U) {  // Safety check
            return AX25_ERROR;
        }
        
        current_frame_len = (size_t)((frames[frame_offset] << 8) | 
                                      frames[frame_offset + 1U]);
        frame_offset += 2U;

        // Validate frame length
        if (current_frame_len == 0U || current_frame_len > 500U) {
            return AX25_ERROR;
        }

        // Decode frame
        decoded_len = ax25_recv(
            decode_buffer,
            frames + frame_offset,
            current_frame_len
        );

        if (decoded_len < (int32_t)(AX25_MIN_ADDR_LEN + 2U)) {
            return AX25_ERROR;
        }

        // Extract metadata (skip AX.25 header: 14 addr + 1 ctrl + 1 PID = 16)
        metadata_size = decode_matrix_metadata(&meta, decode_buffer + 16U);

        if (i == 0U) {
            *rows = meta.rows;
            *cols = meta.cols;
            *element_size = meta.element_size;
        }

        // Copy data
        memcpy(matrix + offset, 
               decode_buffer + 16U + metadata_size,
               meta.data_len);

        offset += meta.data_len;
        frame_offset += current_frame_len;
    }

    return (int32_t)offset;
}
