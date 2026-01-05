#!/bin/bash
# Run AX.25 Test Suites
# Usage: ./run_tests.sh [basic|complete|all]

set -e

COLOR_GREEN='\033[0;32m'
COLOR_RED='\033[0;31m'
COLOR_BLUE='\033[0;34m'
COLOR_RESET='\033[0m'

echo -e "${COLOR_BLUE}═══════════════════════════════════════════════════${COLOR_RESET}"
echo -e "${COLOR_BLUE}  AX.25 Protocol Test Runner${COLOR_RESET}"
echo -e "${COLOR_BLUE}═══════════════════════════════════════════════════${COLOR_RESET}"
echo ""

# Clean previous builds
echo "Cleaning previous builds..."
make clean > /dev/null 2>&1 || true
rm -f ax25_complete_test

# Determine which tests to run
TEST_MODE=${1:-all}

run_basic_tests() {
    echo -e "\n${COLOR_BLUE}Building basic test suite...${COLOR_RESET}"
    make test
}

run_complete_tests() {
    echo -e "\n${COLOR_BLUE}Building comprehensive test suite...${COLOR_RESET}"
    # Build with assertions disabled for comprehensive testing
    gcc -Wall -Wno-unused-function -O2 -std=c11 -DNDEBUG \
        ax25.c ax25_complete_test.c -o ax25_complete_test -lm
    
    echo -e "${COLOR_BLUE}Running comprehensive tests...${COLOR_RESET}\n"
    ./ax25_complete_test
}

case "$TEST_MODE" in
    basic)
        run_basic_tests
        ;;
    complete)
        run_complete_tests
        ;;
    all)
        run_basic_tests
        echo ""
        run_complete_tests
        ;;
    *)
        echo "Usage: $0 [basic|complete|all]"
        echo "  basic    - Run basic test suite (ax25_test)"
        echo "  complete - Run comprehensive test suite (ax25_complete_test)"
        echo "  all      - Run both (default)"
        exit 1
        ;;
esac

echo ""
echo -e "${COLOR_GREEN}═══════════════════════════════════════════════════${COLOR_RESET}"
echo -e "${COLOR_GREEN}  Test execution complete${COLOR_RESET}"
echo -e "${COLOR_GREEN}═══════════════════════════════════════════════════${COLOR_RESET}"
