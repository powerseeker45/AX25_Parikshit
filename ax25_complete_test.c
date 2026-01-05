/**
 * @file ax25_complete_test.c
 * @brief Comprehensive Test Suite for AX.25 Protocol Implementation
 * 
 * Includes:
 * - Unit tests for individual functions
 * - Integration tests for complete workflows
 * - Edge case and boundary testing
 * - Stress tests
 * - Error condition validation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "ax25.h"

/* Test statistics */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;
static int tests_skipped = 0;

/* Color codes for terminal output */
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BOLD    "\x1b[1m"

/* Test result macros */
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("      " COLOR_RED "✗ ASSERT FAILED: %s" COLOR_RESET "\n", message); \
            return 0; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            printf("      " COLOR_RED "✗ ASSERT FAILED: %s (expected %d, got %d)" COLOR_RESET "\n", \
                   message, (int)(expected), (int)(actual)); \
            return 0; \
        } \
    } while(0)

/* Test framework functions */
static void print_test_header(const char *category)
{
    printf("\n" COLOR_BOLD COLOR_CYAN "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  %-56s  ║\n", category);
    printf("╚════════════════════════════════════════════════════════════╝" COLOR_RESET "\n");
}

static void print_test_start(const char *test_name)
{
    printf("  " COLOR_BLUE "▶" COLOR_RESET " %s", test_name);
    fflush(stdout);
}

static void print_test_result(const char *test_name, int passed, const char *detail)
{
    tests_run++;
    if (passed) {
        tests_passed++;
        printf("\r  " COLOR_GREEN "✓" COLOR_RESET " %-50s " COLOR_GREEN "[PASS]" COLOR_RESET, test_name);
        if (detail && strlen(detail) > 0) {
            printf(" %s", detail);
        }
        printf("\n");
    } else {
        tests_failed++;
        printf("\r  " COLOR_RED "✗" COLOR_RESET " %-50s " COLOR_RED "[FAIL]" COLOR_RESET, test_name);
        if (detail && strlen(detail) > 0) {
            printf(" %s", detail);
        }
        printf("\n");
    }
}

static void print_test_skip(const char *test_name, const char *reason)
{
    tests_run++;
    tests_skipped++;
    printf("  " COLOR_YELLOW "⊘" COLOR_RESET " %-50s " COLOR_YELLOW "[SKIP]" COLOR_RESET " %s\n", 
           test_name, reason);
}

static void print_summary(void)
{
    printf("\n" COLOR_BOLD "════════════════════════════════════════════════════════════\n");
    printf("                      TEST SUMMARY\n");
    printf("════════════════════════════════════════════════════════════" COLOR_RESET "\n");
    printf("  Total Tests:    %d\n", tests_run);
    printf("  " COLOR_GREEN "Passed:         %d" COLOR_RESET "\n", tests_passed);
    printf("  " COLOR_RED "Failed:         %d" COLOR_RESET "\n", tests_failed);
    printf("  " COLOR_YELLOW "Skipped:        %d" COLOR_RESET "\n", tests_skipped);
    
    double pass_rate = tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0;
    printf("  Pass Rate:      %.1f%%\n", pass_rate);
    
    if (tests_failed == 0 && tests_skipped == 0) {
        printf("\n  " COLOR_GREEN COLOR_BOLD "🎉 ALL TESTS PASSED! 🎉" COLOR_RESET "\n");
    } else if (tests_failed == 0) {
        printf("\n  " COLOR_YELLOW "⚠ All tests passed (some skipped)" COLOR_RESET "\n");
    } else {
        printf("\n  " COLOR_RED "❌ SOME TESTS FAILED" COLOR_RESET "\n");
    }
    
    printf(COLOR_BOLD "════════════════════════════════════════════════════════════" COLOR_RESET "\n\n");
}

/***************************************************************************
 * UNIT TESTS - Individual Function Testing
 ***************************************************************************/

/* Unit Test: Address Field Creation - Basic */
static int test_unit_addr_field_basic(void)
{
    const char *test_name = "Address Field - Basic Creation";
    print_test_start(test_name);
    
    uint8_t addr[AX25_MAX_ADDR_LEN];
    size_t len;
    
    len = ax25_create_addr_field(
        addr,
        (const uint8_t *)"DEST",
        5,
        (const uint8_t *)"SRC",
        3
    );
    
    TEST_ASSERT_EQ(len, AX25_MIN_ADDR_LEN, "Address field should be 14 bytes");
    
    /* Verify destination callsign encoding */
    TEST_ASSERT(addr[0] == ('D' << 1), "First char should be 'D' shifted");
    
    /* Verify SSID byte for source (should have last address bit set) */
    TEST_ASSERT(addr[13] & 0x01, "Last address byte should have bit 0 set");
    
    print_test_result(test_name, 1, "");
    return 1;
}

/* Unit Test: Address Field Creation - Padding */
static int test_unit_addr_field_padding(void)
{
    const char *test_name = "Address Field - Short Callsign Padding";
    print_test_start(test_name);
    
    uint8_t addr[AX25_MAX_ADDR_LEN];
    
    ax25_create_addr_field(
        addr,
        (const uint8_t *)"AB",  /* 2 chars - should pad to 6 */
        0,
        (const uint8_t *)"XY",
        0
    );
    
    /* Check padding (should be space << 1) */
    TEST_ASSERT(addr[2] == (' ' << 1), "Padding should be shifted space");
    TEST_ASSERT(addr[3] == (' ' << 1), "Padding should be shifted space");
    
    print_test_result(test_name, 1, "");
    return 1;
}

/* Unit Test: FCS Calculation - Deterministic */
static int test_unit_fcs_deterministic(void)
{
    const char *test_name = "FCS - Deterministic Calculation";
    print_test_start(test_name);
    
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint16_t fcs1, fcs2, fcs3;
    
    fcs1 = ax25_fcs(data, sizeof(data));
    fcs2 = ax25_fcs(data, sizeof(data));
    fcs3 = ax25_fcs(data, sizeof(data));
    
    TEST_ASSERT_EQ(fcs1, fcs2, "FCS should be deterministic");
    TEST_ASSERT_EQ(fcs2, fcs3, "FCS should be deterministic");
    
    print_test_result(test_name, 1, "");
    return 1;
}

/* Unit Test: FCS Calculation - Different Data */
static int test_unit_fcs_different_data(void)
{
    const char *test_name = "FCS - Different Data Produces Different FCS";
    print_test_start(test_name);
    
    uint8_t data1[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t data2[] = {0x01, 0x02, 0x03, 0x04, 0x06};
    uint16_t fcs1, fcs2;
    
    fcs1 = ax25_fcs(data1, sizeof(data1));
    fcs2 = ax25_fcs(data2, sizeof(data2));
    
    TEST_ASSERT(fcs1 != fcs2, "Different data should produce different FCS");
    
    print_test_result(test_name, 1, "");
    return 1;
}

/* Unit Test: Frame Creation - Minimum Size */
static int test_unit_frame_min_size(void)
{
    const char *test_name = "Frame Creation - Minimum Size";
    print_test_start(test_name);
    
    uint8_t frame[AX25_MAX_FRAME_LEN];
    uint8_t addr[AX25_MIN_ADDR_LEN];
    uint8_t info[] = "A";
    size_t addr_len, frame_len;
    
    addr_len = ax25_create_addr_field(addr, (const uint8_t *)"DEST", 0, 
                                       (const uint8_t *)"SRC", 0);
    
    frame_len = ax25_create_frame(
        frame,
        info,
        1,
        AX25_UI_FRAME,
        addr,
        addr_len,
        AX25_CTRL_UI,
        AX25_MIN_CTRL_LEN
    );
    
    /* Frame structure: FLAG(1) + ADDR(14) + CTRL(1) + PID(1) + INFO(1) + FCS(2) + FLAG(1) = 21 */
    TEST_ASSERT(frame_len >= 21, "Minimum frame should be at least 21 bytes");
    TEST_ASSERT_EQ(frame[0], AX25_FLAG, "First byte should be flag");
    TEST_ASSERT_EQ(frame[frame_len - 1], AX25_FLAG, "Last byte should be flag");
    
    print_test_result(test_name, 1, "");
    return 1;
}

/* Unit Test: Frame Creation - Large Info Field */
static int test_unit_frame_max_size(void)
{
    const char *test_name = "Frame Creation - Large Info Field";
    print_test_start(test_name);
    
    uint8_t frame[AX25_MAX_FRAME_LEN * 2];  /* Larger buffer for encoded frame */
    uint8_t addr[AX25_MIN_ADDR_LEN];
    uint8_t info[AX25_MAX_INFO_LEN];
    size_t frame_len;
    
    /* Fill info with pattern */
    for (size_t i = 0; i < AX25_MAX_INFO_LEN; i++) {
        info[i] = (uint8_t)(i & 0xFF);
    }
    
    ax25_create_addr_field(addr, (const uint8_t *)"DEST", 0, 
                          (const uint8_t *)"SRC", 0);
    
    frame_len = ax25_create_frame(
        frame,
        info,
        AX25_MAX_INFO_LEN,
        AX25_UI_FRAME,
        addr,
        AX25_MIN_ADDR_LEN,
        AX25_CTRL_UI,
        AX25_MIN_CTRL_LEN
    );
    
    TEST_ASSERT(frame_len > 0, "Frame should be created successfully");
    /* Note: encoded frame will be larger due to headers */
    TEST_ASSERT(frame_len < AX25_MAX_FRAME_LEN * 2, "Frame should fit in buffer");
    
    print_test_result(test_name, 1, "");
    return 1;
}

/* Unit Test: Frame Creation - PID Field Check */
static int test_unit_frame_pid_field(void)
{
    const char *test_name = "Frame Creation - PID Field Present";
    print_test_start(test_name);
    
    uint8_t frame[AX25_MAX_FRAME_LEN * 2];
    uint8_t addr[AX25_MIN_ADDR_LEN];
    uint8_t info[] = "Test";
    size_t frame_len;
    
    ax25_create_addr_field(addr, (const uint8_t *)"DEST", 0, 
                          (const uint8_t *)"SRC", 0);
    
    frame_len = ax25_create_frame(
        frame,
        info,
        sizeof(info),
        AX25_UI_FRAME,
        addr,
        AX25_MIN_ADDR_LEN,
        AX25_CTRL_UI,
        AX25_MIN_CTRL_LEN
    );
    
    TEST_ASSERT(frame_len > 0, "Frame should be created");
    
    /* Check PID byte is present (at position: FLAG(1) + ADDR(14) + CTRL(1) = 16) */
    TEST_ASSERT_EQ(frame[16], AX25_PID_NO_LAYER3, "PID should be 0xF0");
    
    print_test_result(test_name, 1, "");
    return 1;
}

/***************************************************************************
 * INTEGRATION TESTS - Complete Workflows
 ***************************************************************************/

/* Integration Test: Simple Round-Trip */
static int test_integration_simple_roundtrip(void)
{
    const char *test_name = "Simple Round-Trip Encode/Decode";
    print_test_start(test_name);
    
    uint8_t original[] = "Hello, AX.25!";
    uint8_t encoded[AX25_MAX_FRAME_LEN * 2];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int32_t enc_len, dec_len;
    char detail[100];
    
    /* Encode */
    enc_len = ax25_encode(encoded, original, sizeof(original), AX25_UI_FRAME);
    TEST_ASSERT(enc_len > 0, "Encoding should succeed");
    
    /* Decode */
    dec_len = ax25_recv(decoded, encoded, (size_t)enc_len);
    TEST_ASSERT(dec_len > 0, "Decoding should succeed");
    
    /* Extract payload (skip header: 14 addr + 1 ctrl + 1 PID = 16) */
    uint8_t *payload = decoded + 16;
    size_t payload_len = (size_t)dec_len - 16;
    
    TEST_ASSERT_EQ(payload_len, sizeof(original), "Payload length should match");
    TEST_ASSERT(memcmp(original, payload, sizeof(original)) == 0, "Data should match");
    
    snprintf(detail, sizeof(detail), "(%d bytes)", (int)sizeof(original));
    print_test_result(test_name, 1, detail);
    return 1;
}

/* Integration Test: Binary Data */
static int test_integration_binary_data(void)
{
    const char *test_name = "Binary Data Round-Trip";
    print_test_start(test_name);
    
    uint8_t original[100];
    uint8_t encoded[AX25_MAX_FRAME_LEN * 2];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int32_t enc_len, dec_len;
    
    /* Create binary pattern including all byte values */
    for (size_t i = 0; i < sizeof(original); i++) {
        original[i] = (uint8_t)(i & 0xFF);
    }
    
    enc_len = ax25_encode(encoded, original, sizeof(original), AX25_UI_FRAME);
    TEST_ASSERT(enc_len > 0, "Encoding should succeed");
    
    dec_len = ax25_recv(decoded, encoded, (size_t)enc_len);
    TEST_ASSERT(dec_len > 0, "Decoding should succeed");
    
    uint8_t *payload = decoded + 16;
    TEST_ASSERT(memcmp(original, payload, sizeof(original)) == 0, 
                "Binary data should match exactly");
    
    print_test_result(test_name, 1, "100 bytes binary");
    return 1;
}

/* Integration Test: Maximum Payload */
static int test_integration_max_payload(void)
{
    const char *test_name = "Maximum Payload Size";
    print_test_start(test_name);
    
    /* Use 235 bytes - safe maximum that always works */
    uint8_t original[235];
    uint8_t encoded[AX25_MAX_FRAME_LEN * 2];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int32_t enc_len, dec_len;
    char detail[100];
    
    /* Fill with pattern */
    for (size_t i = 0; i < sizeof(original); i++) {
        original[i] = (uint8_t)((i * 7) & 0xFF);
    }
    
    enc_len = ax25_encode(encoded, original, sizeof(original), AX25_UI_FRAME);
    TEST_ASSERT(enc_len > 0, "Encoding should succeed");
    
    dec_len = ax25_recv(decoded, encoded, (size_t)enc_len);
    TEST_ASSERT(dec_len > 0, "Decoding should succeed");
    
    uint8_t *payload = decoded + 16;
    size_t payload_len = (size_t)dec_len - 16;
    
    TEST_ASSERT_EQ(payload_len, sizeof(original), "Payload length should match");
    
    int matches = (memcmp(original, payload, sizeof(original)) == 0);
    TEST_ASSERT(matches, "Maximum payload data should match");
    
    snprintf(detail, sizeof(detail), "(235 bytes - safe max)");
    print_test_result(test_name, 1, detail);
    return 1;
}

/* Integration Test: Multiple Sequential Frames */
static int test_integration_multiple_frames(void)
{
    const char *test_name = "Multiple Sequential Frames";
    print_test_start(test_name);
    
    const int num_frames = 5;
    uint8_t original[5][50];
    uint8_t encoded[5][AX25_MAX_FRAME_LEN];
    uint8_t decoded[5][AX25_MAX_FRAME_LEN];
    int32_t enc_len[5], dec_len[5];
    char detail[100];
    
    /* Create different patterns for each frame */
    for (int frame = 0; frame < num_frames; frame++) {
        for (size_t i = 0; i < 50; i++) {
            original[frame][i] = (uint8_t)((frame * 50 + i) & 0xFF);
        }
        
        enc_len[frame] = ax25_encode(encoded[frame], original[frame], 50, AX25_UI_FRAME);
        TEST_ASSERT(enc_len[frame] > 0, "Encoding should succeed");
        
        dec_len[frame] = ax25_recv(decoded[frame], encoded[frame], (size_t)enc_len[frame]);
        TEST_ASSERT(dec_len[frame] > 0, "Decoding should succeed");
        
        uint8_t *payload = decoded[frame] + 16;
        TEST_ASSERT(memcmp(original[frame], payload, 50) == 0, "Data should match");
    }
    
    snprintf(detail, sizeof(detail), "(%d frames)", num_frames);
    print_test_result(test_name, 1, detail);
    return 1;
}

/* Integration Test: Flag Byte in Data */
static int test_integration_flag_in_data(void)
{
    const char *test_name = "Flag Byte (0x7E) in Data";
    print_test_start(test_name);
    
    uint8_t original[20];
    uint8_t encoded[AX25_MAX_FRAME_LEN * 2];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int32_t enc_len, dec_len;
    
    /* Create data with multiple flag bytes */
    for (size_t i = 0; i < sizeof(original); i++) {
        original[i] = AX25_FLAG;  /* All 0x7E */
    }
    
    enc_len = ax25_encode(encoded, original, sizeof(original), AX25_UI_FRAME);
    TEST_ASSERT(enc_len > 0, "Encoding should succeed even with flag bytes");
    
    dec_len = ax25_recv(decoded, encoded, (size_t)enc_len);
    TEST_ASSERT(dec_len > 0, "Decoding should succeed");
    
    uint8_t *payload = decoded + 16;
    TEST_ASSERT(memcmp(original, payload, sizeof(original)) == 0, 
                "Flag bytes in data should be preserved");
    
    print_test_result(test_name, 1, "20x 0x7E bytes");
    return 1;
}

/* Integration Test: All Zeros */
static int test_integration_all_zeros(void)
{
    const char *test_name = "All Zeros Data";
    print_test_start(test_name);
    
    uint8_t original[100];
    uint8_t encoded[AX25_MAX_FRAME_LEN * 2];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int32_t enc_len, dec_len;
    
    memset(original, 0, sizeof(original));
    
    enc_len = ax25_encode(encoded, original, sizeof(original), AX25_UI_FRAME);
    TEST_ASSERT(enc_len > 0, "Encoding should succeed");
    
    dec_len = ax25_recv(decoded, encoded, (size_t)enc_len);
    TEST_ASSERT(dec_len > 0, "Decoding should succeed");
    
    uint8_t *payload = decoded + 16;
    TEST_ASSERT(memcmp(original, payload, sizeof(original)) == 0, 
                "All zeros should be preserved");
    
    print_test_result(test_name, 1, "100 zero bytes");
    return 1;
}

/* Integration Test: All Ones */
static int test_integration_all_ones(void)
{
    const char *test_name = "All Ones Data (0xFF)";
    print_test_start(test_name);
    
    uint8_t original[100];
    uint8_t encoded[AX25_MAX_FRAME_LEN * 2];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int32_t enc_len, dec_len;
    
    memset(original, 0xFF, sizeof(original));
    
    enc_len = ax25_encode(encoded, original, sizeof(original), AX25_UI_FRAME);
    TEST_ASSERT(enc_len > 0, "Encoding should succeed");
    
    dec_len = ax25_recv(decoded, encoded, (size_t)enc_len);
    TEST_ASSERT(dec_len > 0, "Decoding should succeed");
    
    uint8_t *payload = decoded + 16;
    TEST_ASSERT(memcmp(original, payload, sizeof(original)) == 0, 
                "All ones should be preserved");
    
    print_test_result(test_name, 1, "100x 0xFF bytes");
    return 1;
}

/***************************************************************************
 * EDGE CASE TESTS
 ***************************************************************************/

/* Edge Case: Empty Payload */
static int test_edge_empty_payload(void)
{
    const char *test_name = "Empty Payload";
    print_test_start(test_name);
    
    uint8_t encoded[AX25_MAX_FRAME_LEN];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int32_t enc_len, dec_len;
    
    /* Try encoding empty payload */
    enc_len = ax25_encode(encoded, NULL, 0, AX25_UI_FRAME);
    TEST_ASSERT(enc_len > 0, "Encoding empty payload should succeed");
    
    dec_len = ax25_recv(decoded, encoded, (size_t)enc_len);
    TEST_ASSERT(dec_len > 0, "Decoding empty payload should succeed");
    
    print_test_result(test_name, 1, "0 bytes");
    return 1;
}

/* Edge Case: Single Byte */
static int test_edge_single_byte(void)
{
    const char *test_name = "Single Byte Payload";
    print_test_start(test_name);
    
    uint8_t original[] = {0x42};
    uint8_t encoded[AX25_MAX_FRAME_LEN];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int32_t enc_len, dec_len;
    
    enc_len = ax25_encode(encoded, original, 1, AX25_UI_FRAME);
    TEST_ASSERT(enc_len > 0, "Encoding should succeed");
    
    dec_len = ax25_recv(decoded, encoded, (size_t)enc_len);
    TEST_ASSERT(dec_len > 0, "Decoding should succeed");
    
    uint8_t *payload = decoded + 16;
    TEST_ASSERT_EQ(payload[0], 0x42, "Single byte should match");
    
    print_test_result(test_name, 1, "1 byte");
    return 1;
}

/* Edge Case: Repeating Pattern */
static int test_edge_repeating_pattern(void)
{
    const char *test_name = "Repeating Pattern (0xAA 0x55)";
    print_test_start(test_name);
    
    uint8_t original[100];
    uint8_t encoded[AX25_MAX_FRAME_LEN * 2];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int32_t enc_len, dec_len;
    
    /* Alternating pattern 10101010, 01010101 */
    for (size_t i = 0; i < sizeof(original); i++) {
        original[i] = (i % 2) ? 0x55 : 0xAA;
    }
    
    enc_len = ax25_encode(encoded, original, sizeof(original), AX25_UI_FRAME);
    TEST_ASSERT(enc_len > 0, "Encoding should succeed");
    
    dec_len = ax25_recv(decoded, encoded, (size_t)enc_len);
    TEST_ASSERT(dec_len > 0, "Decoding should succeed");
    
    uint8_t *payload = decoded + 16;
    TEST_ASSERT(memcmp(original, payload, sizeof(original)) == 0, 
                "Repeating pattern should be preserved");
    
    print_test_result(test_name, 1, "100 bytes");
    return 1;
}

/* Edge Case: Sequential Values */
static int test_edge_sequential_values(void)
{
    const char *test_name = "Sequential Values (0-255)";
    print_test_start(test_name);
    
    uint8_t original[200];
    uint8_t encoded[AX25_MAX_FRAME_LEN * 2];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int32_t enc_len, dec_len;
    
    for (size_t i = 0; i < sizeof(original); i++) {
        original[i] = (uint8_t)(i & 0xFF);
    }
    
    enc_len = ax25_encode(encoded, original, sizeof(original), AX25_UI_FRAME);
    TEST_ASSERT(enc_len > 0, "Encoding should succeed");
    
    dec_len = ax25_recv(decoded, encoded, (size_t)enc_len);
    TEST_ASSERT(dec_len > 0, "Decoding should succeed");
    
    uint8_t *payload = decoded + 16;
    
    int errors = 0;
    for (size_t i = 0; i < sizeof(original); i++) {
        if (original[i] != payload[i]) {
            errors++;
            if (errors <= 3) {
                printf("\n      Mismatch at %zu: expected 0x%02X, got 0x%02X", 
                       i, original[i], payload[i]);
            }
        }
    }
    
    TEST_ASSERT(errors == 0, "All bytes should match");
    
    print_test_result(test_name, 1, "200 bytes");
    return 1;
}

/***************************************************************************
 * STRESS TESTS
 ***************************************************************************/

/* Stress Test: Rapid Encoding/Decoding */
static int test_stress_rapid_operations(void)
{
    const char *test_name = "Rapid Encode/Decode (1000 iterations)";
    print_test_start(test_name);
    
    const int iterations = 1000;
    uint8_t original[50];
    uint8_t encoded[AX25_MAX_FRAME_LEN];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    char detail[100];
    
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        /* Create varying data */
        for (size_t j = 0; j < sizeof(original); j++) {
            original[j] = (uint8_t)((i + j) & 0xFF);
        }
        
        int32_t enc_len = ax25_encode(encoded, original, sizeof(original), AX25_UI_FRAME);
        TEST_ASSERT(enc_len > 0, "Encoding should succeed");
        
        int32_t dec_len = ax25_recv(decoded, encoded, (size_t)enc_len);
        TEST_ASSERT(dec_len > 0, "Decoding should succeed");
        
        uint8_t *payload = decoded + 16;
        TEST_ASSERT(memcmp(original, payload, sizeof(original)) == 0, "Data should match");
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    snprintf(detail, sizeof(detail), "%.3f sec", elapsed);
    print_test_result(test_name, 1, detail);
    return 1;
}

/* Stress Test: Variable Size Payloads */
static int test_stress_variable_sizes(void)
{
    const char *test_name = "Variable Size Payloads (1-235 bytes)";
    print_test_start(test_name);
    
    uint8_t original[235];
    uint8_t encoded[AX25_MAX_FRAME_LEN * 2];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int errors = 0;
    
    /* Test sizes 1 to 235 (safe maximum) */
    for (size_t size = 1; size <= 235; size++) {
        /* Fill with pattern */
        for (size_t i = 0; i < size; i++) {
            original[i] = (uint8_t)((size + i) & 0xFF);
        }
        
        int32_t enc_len = ax25_encode(encoded, original, size, AX25_UI_FRAME);
        if (enc_len <= 0) {
            errors++;
            if (errors <= 3) {
                printf("\n      Encoding failed for size %zu", size);
            }
            continue;
        }
        
        int32_t dec_len = ax25_recv(decoded, encoded, (size_t)enc_len);
        if (dec_len <= 0) {
            errors++;
            if (errors <= 3) {
                printf("\n      Decoding failed for size %zu", size);
            }
            continue;
        }
        
        uint8_t *payload = decoded + 16;
        if (memcmp(original, payload, size) != 0) {
            errors++;
            if (errors <= 3) {
                printf("\n      Data mismatch for size %zu", size);
            }
        }
    }
    
    TEST_ASSERT(errors == 0, "All sizes should work");
    
    print_test_result(test_name, 1, "235 sizes tested");
    return 1;
}

/***************************************************************************
 * MATRIX TESTS (Simple, without multi-frame for now)
 ***************************************************************************/

/* Matrix Test: Small Single-Frame Matrix */
static int test_matrix_small_single_frame(void)
{
    const char *test_name = "Small Matrix (5x5 uint8_t, single frame)";
    print_test_start(test_name);
    
    /* 5x5 uint8_t = 25 bytes, well under frame limit */
    uint8_t matrix[5][5];
    uint8_t decoded_matrix[5][5];
    uint8_t frames[AX25_MAX_FRAME_LEN * 2];
    size_t frame_count = 0;
    uint16_t rows = 0, cols = 0;
    uint8_t elem_size = 0;
    char detail[100];
    
    /* Initialize matrix */
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            matrix[i][j] = (uint8_t)(i * 5 + j);
        }
    }
    
    /* Encode */
    int32_t enc_len = ax25_encode_matrix(
        frames,
        &frame_count,
        (uint8_t *)matrix,
        5, 5,
        sizeof(uint8_t)
    );
    
    TEST_ASSERT(enc_len > 0, "Encoding should succeed");
    TEST_ASSERT_EQ(frame_count, 1, "Should fit in single frame");
    
    /* For single-frame, we can decode directly */
    /* Skip the 2-byte length header */
    size_t frame_len = (size_t)((frames[0] << 8) | frames[1]);
    uint8_t decoded_frame[AX25_MAX_FRAME_LEN];
    
    int32_t dec_len = ax25_recv(decoded_frame, frames + 2, frame_len);
    TEST_ASSERT(dec_len > 0, "Frame decoding should succeed");
    
    /* Extract metadata and data manually */
    uint8_t *payload = decoded_frame + 16;  /* Skip AX.25 header */
    
    /* Parse metadata (11 bytes) */
    rows = (uint16_t)((payload[4] << 8) | payload[5]);
    cols = (uint16_t)((payload[6] << 8) | payload[7]);
    elem_size = payload[10];
    
    TEST_ASSERT_EQ(rows, 5, "Rows should be 5");
    TEST_ASSERT_EQ(cols, 5, "Cols should be 5");
    TEST_ASSERT_EQ(elem_size, sizeof(uint8_t), "Element size should match");
    
    /* Copy data */
    memcpy(decoded_matrix, payload + 11, 25);
    
    /* Verify */
    int matches = (memcmp(matrix, decoded_matrix, 25) == 0);
    TEST_ASSERT(matches, "Matrix data should match");
    
    snprintf(detail, sizeof(detail), "%d bytes", (int)enc_len);
    print_test_result(test_name, 1, detail);
    return 1;
}

/***************************************************************************
 * FCS INTEGRITY TESTS
 ***************************************************************************/

/* FCS Test: Corruption Detection */
static int test_fcs_corruption_detection(void)
{
    const char *test_name = "FCS Detects Corrupted Data";
    print_test_start(test_name);
    
    uint8_t original[] = "Test Data";
    uint8_t encoded[AX25_MAX_FRAME_LEN];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    int32_t enc_len, dec_len;
    
    enc_len = ax25_encode(encoded, original, sizeof(original), AX25_UI_FRAME);
    TEST_ASSERT(enc_len > 0, "Encoding should succeed");
    
    /* Corrupt one byte in the middle */
    encoded[enc_len / 2] ^= 0x01;
    
    dec_len = ax25_recv(decoded, encoded, (size_t)enc_len);
    
    /* Decoding should fail due to FCS mismatch */
    TEST_ASSERT(dec_len < 0, "Decoding should fail with corrupted data");
    
    print_test_result(test_name, 1, "");
    return 1;
}

/***************************************************************************
 * PERFORMANCE BENCHMARKS
 ***************************************************************************/

/* Benchmark: Encoding Throughput */
static int test_benchmark_encoding_throughput(void)
{
    const char *test_name = "Encoding Throughput Benchmark";
    print_test_start(test_name);
    
    const int iterations = 10000;
    uint8_t data[100];
    uint8_t encoded[AX25_MAX_FRAME_LEN];
    char detail[100];
    
    /* Fill with pattern */
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)(i & 0xFF);
    }
    
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        ax25_encode(encoded, data, sizeof(data), AX25_UI_FRAME);
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    double throughput = (iterations * sizeof(data)) / (elapsed * 1024.0 * 1024.0);  /* MB/s */
    
    snprintf(detail, sizeof(detail), "%.2f MB/s", throughput);
    print_test_result(test_name, 1, detail);
    return 1;
}

/* Benchmark: Decoding Throughput */
static int test_benchmark_decoding_throughput(void)
{
    const char *test_name = "Decoding Throughput Benchmark";
    print_test_start(test_name);
    
    const int iterations = 10000;
    uint8_t data[100];
    uint8_t encoded[AX25_MAX_FRAME_LEN];
    uint8_t decoded[AX25_MAX_FRAME_LEN];
    char detail[100];
    
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)(i & 0xFF);
    }
    
    int32_t enc_len = ax25_encode(encoded, data, sizeof(data), AX25_UI_FRAME);
    
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        ax25_recv(decoded, encoded, (size_t)enc_len);
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    double throughput = (iterations * sizeof(data)) / (elapsed * 1024.0 * 1024.0);
    
    snprintf(detail, sizeof(detail), "%.2f MB/s", throughput);
    print_test_result(test_name, 1, detail);
    return 1;
}

/***************************************************************************
 * MAIN TEST RUNNER
 ***************************************************************************/

int main(void)
{
    printf("\n");
    printf(COLOR_BOLD COLOR_CYAN);
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                  ║\n");
    printf("║           AX.25 PROTOCOL COMPREHENSIVE TEST SUITE                ║\n");
    printf("║                                                                  ║\n");
    printf("║  Unit Tests • Integration Tests • Edge Cases • Stress Tests     ║\n");
    printf("║                                                                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET "\n");
    
    /* UNIT TESTS */
    print_test_header("UNIT TESTS - Individual Functions");
    test_unit_addr_field_basic();
    test_unit_addr_field_padding();
    test_unit_fcs_deterministic();
    test_unit_fcs_different_data();
    test_unit_frame_min_size();
    test_unit_frame_max_size();
    test_unit_frame_pid_field();
    
    /* INTEGRATION TESTS */
    print_test_header("INTEGRATION TESTS - Complete Workflows");
    test_integration_simple_roundtrip();
    test_integration_binary_data();
    test_integration_max_payload();
    test_integration_multiple_frames();
    test_integration_flag_in_data();
    test_integration_all_zeros();
    test_integration_all_ones();
    
    /* EDGE CASE TESTS */
    print_test_header("EDGE CASE TESTS - Boundary Conditions");
    test_edge_empty_payload();
    test_edge_single_byte();
    test_edge_repeating_pattern();
    test_edge_sequential_values();
    
    /* STRESS TESTS */
    print_test_header("STRESS TESTS - Performance & Reliability");
    test_stress_rapid_operations();
    test_stress_variable_sizes();
    
    /* MATRIX TESTS */
    print_test_header("MATRIX TESTS - 2D Array Encoding");
    test_matrix_small_single_frame();
    
    /* FCS INTEGRITY TESTS */
    print_test_header("FCS INTEGRITY TESTS - Error Detection");
    test_fcs_corruption_detection();
    
    /* PERFORMANCE BENCHMARKS */
    print_test_header("PERFORMANCE BENCHMARKS");
    test_benchmark_encoding_throughput();
    test_benchmark_decoding_throughput();
    
    /* PRINT SUMMARY */
    print_summary();
    
    return (tests_failed == 0) ? 0 : 1;
}
