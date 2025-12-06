#!/bin/bash
# Demo script for Shared Memory Communication Project
# This script demonstrates all the shared memory examples

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Cleanup function
cleanup() {
    echo -e "${YELLOW}Cleaning up shared memory resources...${NC}"
    # Clean POSIX shared memory
    rm -f /dev/shm/my_shared_memory 2>/dev/null
    rm -f /dev/shm/sync_shm 2>/dev/null
    rm -f /dev/shm/image_shm 2>/dev/null
    rm -f /dev/shm/shm_* 2>/dev/null
    rm -f /dev/shm/sem.* 2>/dev/null

    # Clean System V shared memory
    ipcs -m | grep "^0x" | awk '{print $2}' | xargs -r ipcrm -m 2>/dev/null
    echo -e "${GREEN}Cleanup done!${NC}\n"
}

# Check if binaries exist
if [ ! -d "bin" ] || [ -z "$(ls -A bin 2>/dev/null)" ]; then
    echo -e "${RED}Error: Binaries not found. Please run ./build.sh first${NC}"
    exit 1
fi

echo "========================================="
echo "Shared Memory Communication - Demo"
echo "========================================="
echo ""

# Menu
PS3='Select a demo to run (or 0 to exit): '
options=(
    "System V Shared Memory (Basic)"
    "POSIX Shared Memory (Basic)"
    "Synchronized Producer-Consumer"
    "Image Sharing with Threads"
    "Performance Benchmark"
    "Run All Examples"
    "Cleanup Shared Memory"
    "Exit"
)

select opt in "${options[@]}"
do
    case $opt in
        "System V Shared Memory (Basic)")
            echo -e "\n${BLUE}=== System V Shared Memory Demo ===${NC}"
            echo "Starting writer in background..."
            ./bin/sysv_shm_basic writer &
            WRITER_PID=$!
            sleep 2
            echo "Starting reader..."
            ./bin/sysv_shm_basic reader
            wait $WRITER_PID
            echo -e "${GREEN}Demo completed!${NC}\n"
            ;;

        "POSIX Shared Memory (Basic)")
            echo -e "\n${BLUE}=== POSIX Shared Memory Demo ===${NC}"
            echo "Starting writer in background..."
            ./bin/posix_shm_basic writer &
            WRITER_PID=$!
            sleep 2
            echo "Starting reader..."
            ./bin/posix_shm_basic reader
            wait $WRITER_PID
            echo -e "${GREEN}Demo completed!${NC}\n"
            ;;

        "Synchronized Producer-Consumer")
            echo -e "\n${BLUE}=== Synchronized Producer-Consumer Demo ===${NC}"
            echo "Initializing shared memory and semaphores..."
            ./bin/synchronized_shm init
            sleep 1
            echo "Starting producer in background..."
            ./bin/synchronized_shm producer 15 &
            PRODUCER_PID=$!
            sleep 1
            echo "Starting consumer..."
            ./bin/synchronized_shm consumer 15
            wait $PRODUCER_PID
            ./bin/synchronized_shm cleanup
            echo -e "${GREEN}Demo completed!${NC}\n"
            ;;

        "Image Sharing with Threads")
            echo -e "\n${BLUE}=== Image Sharing Demo ===${NC}"
            echo "Running image producer/consumer simulation..."
            timeout 15s ./bin/image_sharing || true
            echo -e "${GREEN}Demo completed!${NC}\n"
            ;;

        "Performance Benchmark")
            echo -e "\n${BLUE}=== Performance Benchmark ===${NC}"
            echo "Running benchmark (this may take a few seconds)..."
            ./bin/shm_benchmark
            echo -e "${GREEN}Benchmark completed!${NC}\n"
            ;;

        "Run All Examples")
            echo -e "\n${YELLOW}Running all examples...${NC}\n"

            # 1. System V
            echo -e "${BLUE}[1/5] System V Shared Memory${NC}"
            ./bin/sysv_shm_basic writer &
            WRITER_PID=$!
            sleep 2
            timeout 15s ./bin/sysv_shm_basic reader || true
            wait $WRITER_PID 2>/dev/null
            sleep 1

            # 2. POSIX
            echo -e "\n${BLUE}[2/5] POSIX Shared Memory${NC}"
            ./bin/posix_shm_basic writer &
            WRITER_PID=$!
            sleep 2
            timeout 15s ./bin/posix_shm_basic reader || true
            wait $WRITER_PID 2>/dev/null
            sleep 1

            # 3. Synchronized
            echo -e "\n${BLUE}[3/5] Synchronized Producer-Consumer${NC}"
            ./bin/synchronized_shm init
            ./bin/synchronized_shm producer 10 &
            PRODUCER_PID=$!
            sleep 1
            timeout 15s ./bin/synchronized_shm consumer 10 || true
            wait $PRODUCER_PID 2>/dev/null
            ./bin/synchronized_shm cleanup
            sleep 1

            # 4. Image
            echo -e "\n${BLUE}[4/5] Image Sharing${NC}"
            timeout 10s ./bin/image_sharing || true
            sleep 1

            # 5. Benchmark
            echo -e "\n${BLUE}[5/5] Performance Benchmark${NC}"
            ./bin/shm_benchmark

            echo -e "\n${GREEN}All demos completed!${NC}\n"
            cleanup
            ;;

        "Cleanup Shared Memory")
            cleanup
            ;;

        "Exit")
            echo "Exiting..."
            cleanup
            break
            ;;

        *)
            echo -e "${RED}Invalid option $REPLY${NC}"
            ;;
    esac
done
