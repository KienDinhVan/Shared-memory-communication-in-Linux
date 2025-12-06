# Makefile for Shared Memory Communication Project
# Author: Kien
# Description: Build all shared memory examples

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = -lrt -lpthread

# Directories
SRC_DIR = src
BIN_DIR = bin
BENCH_DIR = benchmarks
CHALLENGE_DIR = challenges

# Source files
SYSV_SRC = $(SRC_DIR)/sysv_shm_basic.c
POSIX_SRC = $(SRC_DIR)/posix_shm_basic.c
SYNC_SRC = $(SRC_DIR)/synchronized_shm.c
IMAGE_SRC = $(SRC_DIR)/image_sharing.c
BENCH_SRC = $(BENCH_DIR)/shm_benchmark.c
RACE_DEMO_SRC = $(CHALLENGE_DIR)/race_condition_demo.c
RACE_FIXED_SRC = $(CHALLENGE_DIR)/race_condition_fixed.c
DEADLOCK_SRC = $(CHALLENGE_DIR)/deadlock_demo.c

# Binary targets
SYSV_BIN = $(BIN_DIR)/sysv_shm_basic
POSIX_BIN = $(BIN_DIR)/posix_shm_basic
SYNC_BIN = $(BIN_DIR)/synchronized_shm
IMAGE_BIN = $(BIN_DIR)/image_sharing
BENCH_BIN = $(BIN_DIR)/shm_benchmark
RACE_DEMO_BIN = $(BIN_DIR)/race_condition_demo
RACE_FIXED_BIN = $(BIN_DIR)/race_condition_fixed
DEADLOCK_BIN = $(BIN_DIR)/deadlock_demo

# All targets
ALL_BINS = $(SYSV_BIN) $(POSIX_BIN) $(SYNC_BIN) $(IMAGE_BIN) $(BENCH_BIN)
CHALLENGE_BINS = $(RACE_DEMO_BIN) $(RACE_FIXED_BIN) $(DEADLOCK_BIN)

# Default target
.PHONY: all
all: $(BIN_DIR) $(ALL_BINS) $(CHALLENGE_BINS)
	@echo "========================================="
	@echo "Build completed successfully!"
	@echo "========================================="
	@echo "Main programs:"
	@echo "  - $(SYSV_BIN)"
	@echo "  - $(POSIX_BIN)"
	@echo "  - $(SYNC_BIN)"
	@echo "  - $(IMAGE_BIN)"
	@echo "  - $(BENCH_BIN)"
	@echo ""
	@echo "Challenge demonstrations:"
	@echo "  - $(RACE_DEMO_BIN)"
	@echo "  - $(RACE_FIXED_BIN)"
	@echo "  - $(DEADLOCK_BIN)"
	@echo "========================================="

# Create bin directory
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Build individual programs
$(SYSV_BIN): $(SYSV_SRC)
	@echo "Compiling System V shared memory example..."
	$(CC) $(CFLAGS) -o $@ $<

$(POSIX_BIN): $(POSIX_SRC)
	@echo "Compiling POSIX shared memory example..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(SYNC_BIN): $(SYNC_SRC)
	@echo "Compiling synchronized shared memory (producer-consumer)..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(IMAGE_BIN): $(IMAGE_SRC)
	@echo "Compiling image sharing example..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BENCH_BIN): $(BENCH_SRC)
	@echo "Compiling benchmark..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Challenge demonstrations
$(RACE_DEMO_BIN): $(RACE_DEMO_SRC)
	@echo "Compiling race condition demo (buggy)..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(RACE_FIXED_BIN): $(RACE_FIXED_SRC)
	@echo "Compiling race condition fixed..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(DEADLOCK_BIN): $(DEADLOCK_SRC)
	@echo "Compiling deadlock demo..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Individual build targets
.PHONY: sysv posix sync image benchmark challenges
sysv: $(BIN_DIR) $(SYSV_BIN)
posix: $(BIN_DIR) $(POSIX_BIN)
sync: $(BIN_DIR) $(SYNC_BIN)
image: $(BIN_DIR) $(IMAGE_BIN)
benchmark: $(BIN_DIR) $(BENCH_BIN)
challenges: $(BIN_DIR) $(CHALLENGE_BINS)

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BIN_DIR)
	rm -f $(SRC_DIR)/*.o
	@echo "Clean completed!"

# Clean shared memory resources
.PHONY: clean-shm
clean-shm:
	@echo "Cleaning shared memory resources..."
	-ipcs -m | grep '^0x' | awk '{print $$2}' | xargs -r ipcrm -m
	-rm -f /dev/shm/my_shared_memory
	-rm -f /dev/shm/sync_shm
	-rm -f /dev/shm/image_shm
	-rm -f /dev/shm/shm_*
	-rm -f /dev/shm/sem.*
	@echo "Shared memory cleanup completed!"

# Help target
.PHONY: help
help:
	@echo "========================================="
	@echo "Shared Memory Communication - Makefile"
	@echo "========================================="
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build all programs (default)"
	@echo "  sysv        - Build System V example only"
	@echo "  posix       - Build POSIX example only"
	@echo "  sync        - Build synchronized example only"
	@echo "  image       - Build image sharing example only"
	@echo "  benchmark   - Build benchmark only"
	@echo "  clean       - Remove compiled binaries"
	@echo "  clean-shm   - Remove shared memory resources"
	@echo "  help        - Show this help message"
	@echo "========================================="

# Rebuild everything
.PHONY: rebuild
rebuild: clean all
