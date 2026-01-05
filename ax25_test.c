/**
 * @file ax25_test.c
 * @brief Test suite for AX.25 protocol implementation
 * 
 * Tests encoding/decoding of:
 * - Simple frames
 * - Large payloads
 * - 2D matrices
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ax25.h"

/* Test result tracking */
static int tests_run = 0;
static int tests_passed = 0;

/* Color codes for output */
#define COLOR_GREEN "\x1b[32m"
#define COLOR_RED "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_RESET "\x1b[0m"

/**
 * @brief Print test result
 */
static void print_test_result(const char *test_name, int passed)
{
    tests_run++;
    if (passed) {
        tests_passed++;
        printf(COLOR_GREEN "[PASS]" COLOR_RESET " %s\n", test_name);
    } else {
        printf(COLOR_RED "[FAIL]" COLOR_RESET " %s\n", test_name);
    }
}

/**
 * @brief Test 1: Simple frame encoding/decoding
 */
static int test_simple_frame(void)
{
    const char *test_name = "Simple Frame Encoding/Decoding";
    uint8_t payload[] = "Hello AX.25";
    size_t payload_len = strlen((const char *)payload);
    uint8_t encoded[AX25_MAX_FRAME_LEN * 2];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int32_t encoded_len = 0;
    int32_t decoded_len = 0;
    int result = 0;

    printf("\n" COLOR_BLUE "Running: %s" COLOR_RESET "\n", test_name);

    /* Encode */
    encoded_len = ax25_encode(encoded, payload, payload_len, AX25_UI_FRAME);
    printf("  Payload length: %zu bytes\n", payload_len);
    printf("  Encoded length: %d bytes\n", encoded_len);
    printf("  Payload: ");
    for (int i=0;i<encoded_len;i++)
    {
        printf("%d,",encoded[i]);
    }
    printf("\n");

    if (encoded_len <= 0) {
        printf("  " COLOR_RED "Encoding failed" COLOR_RESET "\n");
        print_test_result(test_name, 0);
        return 0;
    }

    /* Decode */
    decoded_len = ax25_recv(decoded, encoded, (size_t)encoded_len);
    printf("  Decoded length: %d bytes\n", decoded_len);

    if (decoded_len <= 0) {
        printf("  " COLOR_RED "Decoding failed" COLOR_RESET "\n");
        print_test_result(test_name, 0);
        return 0;
    }

    /* Extract info field (skip address, control, PID) */
    size_t info_offset = AX25_MIN_ADDR_LEN + 1 + 1; /* addr + ctrl + PID */
    size_t info_len = decoded_len - info_offset;

    printf("  Info field length: %zu bytes\n", info_len);
    printf("  Original: \"%s\"\n", payload);
    printf("  Decoded:  \"");
    for (size_t i = 0; i < info_len && i < payload_len; i++) {
        printf("%c", decoded[info_offset + i]);
    }
    printf("\"\n");

    /* Compare */
    result = (memcmp(payload, decoded + info_offset, payload_len) == 0);
    print_test_result(test_name, result);
    return result;
}

/**
 * @brief Test 2: Maximum payload size
 */
static int test_max_payload(void)
{
    const char *test_name = "Maximum Payload Size";
    uint8_t payload[AX25_MAX_INFO_LEN];
    uint8_t encoded[AX25_MAX_FRAME_LEN * 2];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int32_t encoded_len = 0;
    int32_t decoded_len = 0;
    size_t i = 0;
    int result = 0;

    printf("\n" COLOR_BLUE "Running: %s" COLOR_RESET "\n", test_name);

    /* Create pattern payload */
    for (i = 0; i < AX25_MAX_INFO_LEN; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }

    printf("  Payload length: %u bytes\n", AX25_MAX_INFO_LEN);

    /* Encode */
    encoded_len = ax25_encode(encoded, payload, AX25_MAX_INFO_LEN, AX25_UI_FRAME);
    printf("  Encoded length: %d bytes\n", encoded_len);

    if (encoded_len <= 0) {
        printf("  " COLOR_RED "Encoding failed" COLOR_RESET "\n");
        print_test_result(test_name, 0);
        return 0;
    }

    /* Decode */
    decoded_len = ax25_recv(decoded, encoded, (size_t)encoded_len);
    printf("  Decoded length: %d bytes\n", decoded_len);

    if (decoded_len <= 0) {
        printf("  " COLOR_RED "Decoding failed" COLOR_RESET "\n");
        print_test_result(test_name, 0);
        return 0;
    }

    /* Compare */
    size_t info_offset = AX25_MIN_ADDR_LEN + 1 + 1;
    result = (memcmp(payload, decoded + info_offset, AX25_MAX_INFO_LEN) == 0);
    print_test_result(test_name, result);
    return result;
}

/**
 * @brief Test 3: Small 2D matrix encoding/decoding
 */
static int test_small_matrix(void)
{
    const char *test_name = "Small 2D Matrix (10x10)";
    const uint16_t rows = 10;
    const uint16_t cols = 10;
    const uint8_t elem_size = sizeof(int32_t);
    int32_t matrix[10][10];
    int32_t decoded_matrix[10][10];
    uint8_t *matrix_bytes = (uint8_t *)matrix;
    uint8_t *decoded_bytes = (uint8_t *)decoded_matrix;
    uint8_t frames[AX25_MAX_FRAME_LEN * 10];
    size_t frame_count = 0;
    uint16_t dec_rows = 0;
    uint16_t dec_cols = 0;
    uint8_t dec_elem_size = 0;
    int32_t enc_len = 0;
    int32_t dec_len = 0;
    int result = 1;

    printf("\n" COLOR_BLUE "Running: %s" COLOR_RESET "\n", test_name);

    /* Initialize matrix with test pattern */
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = i * cols + j;
        }
    }

    printf("  Matrix size: %dx%d, element size: %d bytes\n", rows, cols, elem_size);
    printf("  Total data: %zu bytes\n", (size_t)rows * cols * elem_size);

    /* Encode */
    enc_len = ax25_encode_matrix(
        frames,
        &frame_count,
        matrix_bytes,
        rows,
        cols,
        elem_size
    );

    printf("  Encoded into %zu frames, %d bytes total\n", frame_count, enc_len);

    if (enc_len <= 0) {
        printf("  " COLOR_RED "Encoding failed" COLOR_RESET "\n");
        print_test_result(test_name, 0);
        return 0;
    }

    /* Decode */
    memset(decoded_matrix, 0, sizeof(decoded_matrix));
    dec_len = ax25_decode_matrix(
        decoded_bytes,
        &dec_rows,
        &dec_cols,
        &dec_elem_size,
        frames,
        frame_count
    );

    printf("  Decoded: %dx%d matrix, element size: %d, %d bytes total\n",
           dec_rows, dec_cols, dec_elem_size, dec_len);

    if (dec_len <= 0) {
        printf("  " COLOR_RED "Decoding failed" COLOR_RESET "\n");
        print_test_result(test_name, 0);
        return 0;
    }

    /* Verify dimensions */
    if (dec_rows != rows || dec_cols != cols || dec_elem_size != elem_size) {
        printf("  " COLOR_RED "Dimension mismatch" COLOR_RESET "\n");
        result = 0;
    }

    /* Verify data */
    for (int i = 0; i < rows && result; i++) {
        for (int j = 0; j < cols && result; j++) {
            if (matrix[i][j] != decoded_matrix[i][j]) {
                printf("  " COLOR_RED "Data mismatch at [%d][%d]: %d != %d" COLOR_RESET "\n",
                       i, j, matrix[i][j], decoded_matrix[i][j]);
                result = 0;
            }
        }
    }

    if (result) {
        printf("  All %d elements verified successfully\n", rows * cols);
    }

    print_test_result(test_name, result);
    return result;
}

/**
 * @brief Test 4: Large 2D matrix encoding/decoding
 */
static int test_large_matrix(void)
{
    const char *test_name = "Large 2D Matrix (100x100)";
    const uint16_t rows = 100;
    const uint16_t cols = 100;
    const uint8_t elem_size = sizeof(float);
    float *matrix = NULL;
    float *decoded_matrix = NULL;
    uint8_t *frames = NULL;
    size_t frame_count = 0;
    uint16_t dec_rows = 0;
    uint16_t dec_cols = 0;
    uint8_t dec_elem_size = 0;
    int32_t enc_len = 0;
    int32_t dec_len = 0;
    int result = 1;
    size_t total_size = 0;
    size_t errors = 0;

    printf("\n" COLOR_BLUE "Running: %s" COLOR_RESET "\n", test_name);

    total_size = (size_t)rows * cols * elem_size;

    /* Allocate memory */
    matrix = (float *)malloc(total_size);
    decoded_matrix = (float *)malloc(total_size);
    frames = (uint8_t *)malloc(AX25_MAX_FRAME_LEN * 200); /* Generous allocation */

    if (!matrix || !decoded_matrix || !frames) {
        printf("  " COLOR_RED "Memory allocation failed" COLOR_RESET "\n");
        free(matrix);
        free(decoded_matrix);
        free(frames);
        print_test_result(test_name, 0);
        return 0;
    }

    /* Initialize matrix */
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i * cols + j] = (float)(i * cols + j) * 0.5f;
        }
    }

    printf("  Matrix size: %dx%d, element size: %d bytes\n", rows, cols, elem_size);
    printf("  Total data: %zu bytes\n", total_size);

    /* Encode */
    enc_len = ax25_encode_matrix(
        frames,
        &frame_count,
        (uint8_t *)matrix,
        rows,
        cols,
        elem_size
    );

    printf("  Encoded into %zu frames, %d bytes total\n", frame_count, enc_len);

    if (enc_len <= 0) {
        printf("  " COLOR_RED "Encoding failed" COLOR_RESET "\n");
        result = 0;
        goto cleanup;
    }

    /* Decode */
    memset(decoded_matrix, 0, total_size);
    dec_len = ax25_decode_matrix(
        (uint8_t *)decoded_matrix,
        &dec_rows,
        &dec_cols,
        &dec_elem_size,
        frames,
        frame_count
    );

    printf("  Decoded: %dx%d matrix, element size: %d, %d bytes total\n",
           dec_rows, dec_cols, dec_elem_size, dec_len);

    if (dec_len <= 0) {
        printf("  " COLOR_RED "Decoding failed" COLOR_RESET "\n");
        result = 0;
        goto cleanup;
    }

    /* Verify dimensions */
    if (dec_rows != rows || dec_cols != cols || dec_elem_size != elem_size) {
        printf("  " COLOR_RED "Dimension mismatch" COLOR_RESET "\n");
        result = 0;
        goto cleanup;
    }

    /* Verify data (sample check) */
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (matrix[i * cols + j] != decoded_matrix[i * cols + j]) {
                errors++;
                if (errors <= 5) {
                    printf("  " COLOR_RED "Data mismatch at [%d][%d]: %.2f != %.2f" COLOR_RESET "\n",
                           i, j, matrix[i * cols + j], decoded_matrix[i * cols + j]);
                }
            }
        }
    }

    if (errors > 0) {
        printf("  " COLOR_RED "Total errors: %zu" COLOR_RESET "\n", errors);
        result = 0;
    } else {
        printf("  All %d elements verified successfully\n", rows * cols);
    }

cleanup:
    free(matrix);
    free(decoded_matrix);
    free(frames);
    print_test_result(test_name, result);
    return result;
}

/**
 * @brief Test 5: FCS calculation
 */
static int test_fcs_calculation(void)
{
    const char *test_name = "FCS Calculation";
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint16_t fcs1, fcs2;
    int result = 0;

    printf("\n" COLOR_BLUE "Running: %s" COLOR_RESET "\n", test_name);

    /* Calculate FCS twice */
    fcs1 = ax25_fcs(test_data, sizeof(test_data));
    fcs2 = ax25_fcs(test_data, sizeof(test_data));

    printf("  FCS 1: 0x%04X\n", fcs1);
    printf("  FCS 2: 0x%04X\n", fcs2);

    /* Should be deterministic */
    result = (fcs1 == fcs2);

    print_test_result(test_name, result);
    return result;
}

/**
 * @brief Test 6: Address field creation
 */
static int test_address_field(void)
{
    const char *test_name = "Address Field Creation";
    uint8_t addr[AX25_MAX_ADDR_LEN];
    size_t len = 0;
    int result = 1;

    printf("\n" COLOR_BLUE "Running: %s" COLOR_RESET "\n", test_name);

    len = ax25_create_addr_field(
        addr,
        (const uint8_t *)"DEST",
        5,
        (const uint8_t *)"SRC",
        3
    );

    printf("  Address field length: %zu bytes\n", len);
    printf("  Expected: %d bytes\n", AX25_MIN_ADDR_LEN);

    result = (len == AX25_MIN_ADDR_LEN);

    print_test_result(test_name, result);
    return result;
}

/**
 * @brief Main test runner
 */
int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("  AX.25 Protocol Test Suite\n");
    printf("========================================\n");

    /* Run all tests */
    test_simple_frame();
    test_max_payload();
    test_fcs_calculation();
    test_address_field();
    test_small_matrix();
    test_large_matrix();

    /* Print summary */
    printf("\n");
    printf("========================================\n");
    printf("  Test Summary\n");
    printf("========================================\n");
    printf("  Total tests: %d\n", tests_run);
    printf("  Passed:      " COLOR_GREEN "%d" COLOR_RESET "\n", tests_passed);
    printf("  Failed:      " COLOR_RED "%d" COLOR_RESET "\n", tests_run - tests_passed);
    printf("========================================\n\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
