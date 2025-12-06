#!/bin/bash
# Build script for Shared Memory Communication Project
# This script compiles all programs manually (alternative to make)

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "Building Shared Memory Communication Project"
echo "========================================="

# Create bin directory
mkdir -p bin

# Compiler settings
CC="gcc"
CFLAGS="-Wall -Wextra -O2 -g"
LDFLAGS="-lrt -lpthread"

# Build function
build() {
    local src=$1
    local out=$2
    local libs=$3

    echo -n "Compiling $(basename $out)... "
    if $CC $CFLAGS -o $out $src $libs 2>/dev/null; then
        echo -e "${GREEN}✓${NC}"
        return 0
    else
        echo -e "${RED}✗${NC}"
        $CC $CFLAGS -o $out $src $libs
        return 1
    fi
}

# Build main programs
echo "Building main examples..."
build "src/sysv_shm_basic.c" "bin/sysv_shm_basic" ""
build "src/posix_shm_basic.c" "bin/posix_shm_basic" "$LDFLAGS"
build "src/synchronized_shm.c" "bin/synchronized_shm" "$LDFLAGS"
build "src/image_sharing.c" "bin/image_sharing" "$LDFLAGS"
build "benchmarks/shm_benchmark.c" "bin/shm_benchmark" "$LDFLAGS"

# Build challenge demonstrations
echo ""
echo "Building challenge demonstrations..."
build "challenges/race_condition_demo.c" "bin/race_condition_demo" "$LDFLAGS"
build "challenges/race_condition_fixed.c" "bin/race_condition_fixed" "$LDFLAGS"
build "challenges/deadlock_demo.c" "bin/deadlock_demo" "$LDFLAGS"

echo "========================================="
echo -e "${GREEN}Build completed!${NC}"
echo "========================================="
echo "Available programs:"
ls -1 bin/
echo "========================================="
echo "Run './demo.sh' to see examples"
echo "========================================="
