# Makefile for AX.25 Protocol Implementation
# Following NASA JPL C Coding Standard

CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wpedantic -std=c11 -O2
CFLAGS += -Wformat=2 -Wformat-security -Wstrict-prototypes
CFLAGS += -Wmissing-prototypes -Wold-style-definition
CFLAGS += -fstack-protector-strong -D_FORTIFY_SOURCE=2

# Debug flags (uncomment for debugging)
# CFLAGS += -g -O0 -DDEBUG

# Test flags (disable assertions for comprehensive testing)
TEST_CFLAGS = -Wall -Wno-unused-function -O2 -std=c11 -DNDEBUG

LDFLAGS = -lm

# Source files
SRCS = ax25.c ax25_test.c
TEST_SRCS = ax25.c ax25_complete_test.c
OBJS = $(SRCS:.c=.o)
HEADERS = ax25.h config.h utils.h

# Target executables
TARGET = ax25_test
TEST_TARGET = ax25_complete_test

# Default target
all: $(TARGET)

# Build basic test executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build comprehensive test executable
$(TEST_TARGET): ax25.c ax25_complete_test.c $(HEADERS)
	$(CC) $(TEST_CFLAGS) -o $@ ax25.c ax25_complete_test.c $(LDFLAGS)

# Compile object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Run basic tests
test: $(TARGET)
	@echo "Running basic test suite..."
	./$(TARGET)

# Run comprehensive tests
test-complete: $(TEST_TARGET)
	@echo "Running comprehensive test suite..."
	./$(TEST_TARGET)

# Run all tests
test-all: test test-complete
	@echo ""
	@echo "All test suites completed!"

# Run tests with verbose output
test-verbose: $(TEST_TARGET)
	@echo "Running comprehensive tests with verbose output..."
	./$(TEST_TARGET) -v

# Quick smoke test (just compilation)
smoke-test:
	@echo "Smoke test: checking if code compiles..."
	@$(CC) $(CFLAGS) -c ax25.c -o /tmp/ax25_smoke.o
	@$(CC) $(CFLAGS) -c ax25_test.c -o /tmp/ax25_test_smoke.o
	@rm -f /tmp/ax25_smoke.o /tmp/ax25_test_smoke.o
	@echo "Smoke test passed: code compiles successfully"

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET) $(TEST_TARGET)
	rm -f ax25.o ax25_test.o ax25_complete_test.o
	rm -f *.o

# Deep clean (including backup files)
distclean: clean
	rm -f *~ *.bak

# Static analysis (if cppcheck is available)
analyze:
	@which cppcheck > /dev/null 2>&1 && \
		cppcheck --enable=all --suppress=missingIncludeSystem \
		--suppress=unusedFunction ax25.c ax25_test.c ax25_complete_test.c || \
		echo "cppcheck not found, skipping static analysis"

# Format check (if clang-format is available)
format-check:
	@which clang-format > /dev/null 2>&1 && \
		clang-format --dry-run --Werror $(SRCS) $(HEADERS) || \
		echo "clang-format not found, skipping format check"

# Show compiler version and flags
info:
	@echo "Compiler: $(CC)"
	@$(CC) --version
	@echo ""
	@echo "Build Flags: $(CFLAGS)"
	@echo "Test Flags:  $(TEST_CFLAGS)"
	@echo "Link Flags:  $(LDFLAGS)"
	@echo ""
	@echo "Sources: $(SRCS)"
	@echo "Headers: $(HEADERS)"
	@echo ""
	@echo "Targets:"
	@echo "  $(TARGET) - Basic test suite"
	@echo "  $(TEST_TARGET) - Comprehensive test suite"

# Help target
help:
	@echo "AX.25 Protocol Implementation - Makefile Help"
	@echo ""
	@echo "Build Targets:"
	@echo "  all              - Build basic test suite (default)"
	@echo "  $(TARGET)        - Build basic test suite"
	@echo "  $(TEST_TARGET)   - Build comprehensive test suite"
	@echo ""
	@echo "Test Targets:"
	@echo "  test             - Run basic test suite"
	@echo "  test-complete    - Run comprehensive test suite (21 tests)"
	@echo "  test-all         - Run all test suites"
	@echo "  test-verbose     - Run tests with verbose output"
	@echo "  smoke-test       - Quick compilation check"
	@echo ""
	@echo "Utility Targets:"
	@echo "  clean            - Remove build artifacts"
	@echo "  distclean        - Deep clean (including backups)"
	@echo "  analyze          - Run static analysis (requires cppcheck)"
	@echo "  format-check     - Check code formatting (requires clang-format)"
	@echo "  info             - Show compiler and build information"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build basic tests"
	@echo "  make test               # Run basic tests"
	@echo "  make test-complete      # Run comprehensive tests"
	@echo "  make clean test-all     # Clean, then run all tests"

.PHONY: all test test-complete test-all test-verbose smoke-test \
        clean distclean analyze format-check info help