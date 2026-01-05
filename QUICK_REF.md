# AX.25 Quick Reference Card

## 🚀 Quick Start (30 seconds)

```bash
# Build and test
make clean test

# Run comprehensive tests  
make test-complete

# See all options
make help
```

## 📋 Common Make Commands

| Command | Description | Time |
|---------|-------------|------|
| `make` | Build basic tests | ~1s |
| `make test` | Run basic tests (6 tests) | ~1s |
| `make test-complete` | Run comprehensive tests (21 tests) | ~5s |
| `make test-all` | Run all tests | ~6s |
| `make clean` | Clean build files | <1s |
| `make help` | Show all commands | <1s |
| `make info` | Show build info | <1s |

## 📊 Test Suites

### Basic Suite (`make test`)
```
✓ Simple Frame Encoding/Decoding
✓ Maximum Payload Size  
✓ FCS Calculation
✓ Address Field Creation
```

### Comprehensive Suite (`make test-complete`)
```
✓ 7 Unit Tests
✓ 7 Integration Tests
✓ 4 Edge Case Tests
✓ 2 Stress Tests
✓ 1 Matrix Test
✓ 1 FCS Integrity Test
✓ 2 Performance Benchmarks
═══════════════════════
  21 Total Tests
```

## 💻 Code Usage Examples

### Simple Encoding/Decoding

```c
#include "ax25.h"

uint8_t data[] = "Hello AX.25";
uint8_t encoded[AX25_MAX_FRAME_LEN * 2];
uint8_t decoded[AX25_MAX_FRAME_LEN];

// Encode
int32_t enc_len = ax25_encode(
    encoded, 
    data, 
    sizeof(data), 
    AX25_UI_FRAME
);

// Decode
int32_t dec_len = ax25_recv(
    decoded, 
    encoded, 
    (size_t)enc_len
);

// Extract payload (skip 16-byte header)
uint8_t *payload = decoded + 16;
```

### Maximum Safe Payload

```c
// Use 235 bytes for guaranteed reliability
#define SAFE_PAYLOAD_SIZE 235

uint8_t payload[SAFE_PAYLOAD_SIZE];
// Fill payload...

int32_t len = ax25_encode(
    frame, 
    payload, 
    SAFE_PAYLOAD_SIZE, 
    AX25_UI_FRAME
);
```

### Matrix Encoding (Single Frame)

```c
// Works reliably for data <= ~200 bytes
uint8_t matrix[10][10];  // 100 bytes
uint8_t frames[1000];
size_t frame_count;

// Initialize matrix
for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
        matrix[i][j] = i * 10 + j;
    }
}

// Encode
int32_t len = ax25_encode_matrix(
    frames,
    &frame_count,
    (uint8_t *)matrix,
    10,              // rows
    10,              // cols  
    sizeof(uint8_t)  // element size
);

printf("Encoded into %zu frames\n", frame_count);
```

### Error Checking

```c
int32_t result = ax25_encode(out, in, len, AX25_UI_FRAME);

if (result < 0) {
    // Error codes
    switch (result) {
        case AX25_ERROR:
            fprintf(stderr, "Generic error\n");
            break;
        case AX25_ERROR_INVALID_PARAM:
            fprintf(stderr, "Invalid parameter\n");
            break;
        case AX25_ERROR_BUFFER_OVERFLOW:
            fprintf(stderr, "Buffer overflow\n");
            break;
        case AX25_ERROR_FCS_MISMATCH:
            fprintf(stderr, "FCS mismatch\n");
            break;
    }
    return -1;
}

printf("Success: %d bytes\n", result);
```

## 🔧 Configuration

Edit `config.h`:

```c
// Callsigns
static const char SAT_CALLSIGN[] = "PARSAT";
static const char GRD_CALLSIGN[] = "ABCD";

// SSIDs  
static const uint8_t SAT_SSID = 0;
static const uint8_t GRD_SSID = 0;

// Limits
#define AX25_MAX_FRAME_LEN    256
#define AX25_MAX_INFO_LEN     240
#define MATRIX_CHUNK_SIZE     200
```

## ⚡ Performance

| Operation | Throughput |
|-----------|-----------|
| Encoding  | ~30-35 MB/s |
| Decoding  | ~45-50 MB/s |
| Latency   | <1 ms per frame |

## ⚠️ Known Limitations

| Issue | Impact | Workaround |
|-------|--------|------------|
| Multi-frame matrices | ❌ Fails | Use single frames (<200 bytes) |
| Payloads 238-240 bytes | ⚠️ Unreliable | Limit to 235 bytes |
| Only UI frames | ℹ️ Limited | Future enhancement |

## 📝 File Structure

```
ax25.h              - Main header
ax25.c              - Core implementation
config.h            - Configuration  
utils.h             - CRC tables
ax25_test.c         - Basic tests (6)
ax25_complete_test.c - Comprehensive tests (21)
Makefile            - Build system
README.md           - Full documentation
TESTING.md          - Test documentation
run_tests.sh        - Test runner script
```

## 🐛 Debugging

```bash
# Compile with debug symbols
gcc -g -O0 -Wall ax25.c your_code.c -o debug -lm

# Run with GDB
gdb ./debug
(gdb) break ax25_encode
(gdb) run
(gdb) print enc_len
(gdb) continue

# Or use prints
printf("DEBUG: len=%d\n", len);
```

## 📚 Documentation Files

- `README.md` - Complete documentation
- `TESTING.md` - Testing guide
- `QUICK_REF.md` - This file
- `config.h` - Configuration reference

## 🎯 Best Practices

1. ✅ Always check return values
2. ✅ Use SAFE_PAYLOAD_SIZE (235) for reliability  
3. ✅ Run `make test` before commits
4. ✅ Keep payloads under 235 bytes
5. ⚠️ Avoid multi-frame matrices (known issue)
6. ⚠️ Test edge cases thoroughly

## 📞 Quick Help

```bash
make help           # Show all make targets
make info           # Show build configuration  
make smoke-test     # Quick compilation check
./run_tests.sh all  # Run all test suites
```

## 🔗 Related Commands

```bash
# CI/CD Integration
make clean && make test-all && echo "PASSED" || echo "FAILED"

# Static analysis
make analyze

# Show compiler info
make info

# Run specific test suite
./ax25_test              # Basic
./ax25_complete_test     # Comprehensive
```

## 📖 Learn More

See full documentation in:
- `README.md` for complete API reference
- `TESTING.md` for testing details
- Code comments for implementation details

---
**Version:** 1.0  
**Last Updated:** 2025-01-02  
**Compatibility:** Linux, macOS, POSIX systems
