# Shared Memory Communication in Linux

**Operating Systems Project**
**Author:** Kien
**Topic:** Shared memory communication implementation and challenges

---

## Quick Start

```bash
# 1. Build tất cả programs
./build.sh

# 2. Demo Race Condition (xem vấn đề thực tế)
./bin/race_condition_demo       # Shows 1285 lost updates 
./bin/race_condition_fixed      # Shows 0 lost updates 

# 3. Demo Deadlock
timeout 10s ./bin/deadlock_demo buggy   # Will hang (deadlock)
./bin/deadlock_demo fixed              # Works perfectly

# 4. Benchmark performance
./bin/shm_benchmark

# 5. Hoặc chạy interactive demo
./demo.sh
```

**Expected Results:**
- Race condition demo: ~2.5% lost updates without synchronization
- Fixed version: 100% accurate with semaphores
- Deadlock demo: Shows problem and solution clearly

---

## Overview

This project demonstrates various approaches to shared memory communication in Linux, including:
- System V shared memory API
- POSIX shared memory API
- Synchronized producer-consumer with semaphores
- High-performance image data sharing
- Performance benchmarking and comparison

---

## Project Structure

```
OS_prj/
├── src/
│   ├── sysv_shm_basic.c          # System V shared memory example
│   ├── posix_shm_basic.c         # POSIX shared memory example
│   ├── synchronized_shm.c        # Producer-consumer with synchronization
│   └── image_sharing.c           # Image buffer sharing with threads
├── benchmarks/
│   └── shm_benchmark.c           # Performance comparison
├── challenges/                   # **NEW!** Challenge demonstrations
│   ├── race_condition_demo.c     # Race condition problem (buggy)
│   ├── race_condition_fixed.c    # Race condition solution (fixed)
│   └── deadlock_demo.c           # Deadlock demo and prevention
├── bin/                          # Compiled binaries (created after build)
├── build.sh                      # Build script (alternative to Makefile)
├── demo.sh                       # Interactive demo script
├── Makefile                      # Build automation (requires make)
└── README.md                     # This file
```

---

## Building the Project

### Option 1: Using build.sh script 

```bash
chmod +x build.sh
./build.sh
```

### Option 2: Using Makefile

```bash
make all
```

### Option 3: Manual compilation

```bash
mkdir -p bin

# System V example
gcc -Wall -Wextra -O2 -g -o bin/sysv_shm_basic src/sysv_shm_basic.c

# POSIX example
gcc -Wall -Wextra -O2 -g -o bin/posix_shm_basic src/posix_shm_basic.c -lrt -lpthread

# Synchronized producer-consumer
gcc -Wall -Wextra -O2 -g -o bin/synchronized_shm src/synchronized_shm.c -lrt -lpthread

# Image sharing
gcc -Wall -Wextra -O2 -g -o bin/image_sharing src/image_sharing.c -lrt -lpthread

# Benchmark
gcc -Wall -Wextra -O2 -g -o bin/shm_benchmark benchmarks/shm_benchmark.c -lrt -lpthread
```

---

## Running the Examples

### Interactive Demo 
```bash
chmod +x demo.sh
./demo.sh
```

This will present an interactive menu to run any example.

### Manual Execution

#### 1. System V Shared Memory

```bash
# Terminal 1 - Writer
./bin/sysv_shm_basic writer

# Terminal 2 - Reader
./bin/sysv_shm_basic reader
```

#### 2. POSIX Shared Memory

```bash
# Terminal 1 - Writer
./bin/posix_shm_basic writer

# Terminal 2 - Reader
./bin/posix_shm_basic reader
```

#### 3. Synchronized Producer-Consumer

```bash
# Initialize shared memory and semaphores
./bin/synchronized_shm init

# Terminal 1 - Producer
./bin/synchronized_shm producer 20

# Terminal 2 - Consumer
./bin/synchronized_shm consumer 20

# Cleanup when done
./bin/synchronized_shm cleanup
```

#### 4. Image Sharing (Single process with threads)

```bash
./bin/image_sharing
# Press Ctrl+C after observing the output
```

#### 5. Performance Benchmark

```bash
./bin/shm_benchmark
```

---

## Key Features Demonstrated

### 1. System V Shared Memory (`sysv_shm_basic.c`)
- Uses `shmget()`, `shmat()`, `shmdt()`, `shmctl()`
- Key-based identification
- Legacy API but widely supported
- Demonstrates read-only attachment with `SHM_RDONLY`

### 2. POSIX Shared Memory (`posix_shm_basic.c`)
- Uses `shm_open()`, `mmap()`, `munmap()`, `shm_unlink()`
- Name-based identification (filesystem-like)
- Modern, more portable API
- Better integration with `mmap()`

### 3. Synchronized Producer-Consumer (`synchronized_shm.c`)
- Circular buffer implementation
- Semaphore synchronization (mutex, items, spaces)
- Demonstrates proper inter-process synchronization
- Prevents race conditions

### 4. Image Data Sharing (`image_sharing.c`)
- Simulates camera capture and image processing
- Large data structure sharing (640x480x3 RGB images)
- Thread-based with pthread mutex and condition variables
- Measures processing latency

### 5. Performance Benchmark (`shm_benchmark.c`)
- Compares System V vs POSIX performance
- Measures creation, attachment, read/write throughput
- 1MB data transfer over 1000 iterations
- Provides timing breakdown

---

## Challenge Demonstrations

We've added practical demonstrations of common shared memory challenges:

### 1. Race Condition Demo

**Buggy Version** (`race_condition_demo.c`):
```bash
./bin/race_condition_demo
```
Demonstrates ACTUAL race conditions where multiple processes lose updates.
Expected: 50,000 increments → Actual: ~48,000 (lost ~2,000 updates!)

**Fixed Version** (`race_condition_fixed.c`):
```bash
./bin/race_condition_fixed
```
Shows the SOLUTION using semaphores. Result: 50,000/50,000 (100% accurate)

### 2. Deadlock Demo

**Buggy Version**:
```bash
./bin/deadlock_demo buggy
# Press Ctrl+C to terminate when it deadlocks
```
Demonstrates how deadlock occurs with improper lock ordering.

**Fixed Version**:
```bash
./bin/deadlock_demo fixed
```
Shows deadlock prevention using consistent lock ordering.

### Key Learnings:
- **Race conditions** cause silent data corruption (lost updates)
- **Proper synchronization** (semaphores/mutexes) prevents race conditions
- **Lock ordering** prevents deadlocks
- These are REAL problems that happen without proper synchronization!

---

## Challenges and Solutions

### Challenge 1: Race Conditions
**Problem:** Multiple processes accessing shared data simultaneously can corrupt data.

**Solution:**
- Use semaphores for mutual exclusion
- Implement proper locking mechanisms (mutex)
- See `synchronized_shm.c` for implementation

**Live Demo:**
- Run `./bin/race_condition_demo` to see the problem
- Run `./bin/race_condition_fixed` to see the solution
- Compare results: buggy version loses ~2,000 updates

### Challenge 2: Deadlock
**Problem:** Two processes waiting for each other's locks = system hangs forever.

**Solution:**
- Always acquire locks in the same order
- Use timeout mechanisms
- Deadlock detection algorithms

**Live Demo:**
- Run `./bin/deadlock_demo buggy` to see deadlock (press Ctrl+C to stop)
- Run `./bin/deadlock_demo fixed` to see prevention with lock ordering

### Challenge 3: Synchronization Overhead
**Problem:** Excessive locking degrades performance.

**Solution:**
- Minimize critical section size
- Use condition variables for efficient waiting
- Lock only when necessary

### Challenge 4: Resource Cleanup
**Problem:** Shared memory persists after process termination.

**Solution:**
- Explicit cleanup with `shmctl(IPC_RMID)` or `shm_unlink()`
- Signal handlers for graceful shutdown
- Cleanup script provided (`demo.sh` includes cleanup)

### Challenge 5: Memory Coherency
**Problem:** CPU caching can cause inconsistent views of shared memory.

**Solution:**
- Use `MAP_SHARED` flag with `mmap()`
- Memory barriers with `volatile` keyword where needed
- Proper synchronization primitives

### Challenge 6: Permission Management
**Problem:** Different processes may have different access rights.

**Solution:**
- Set appropriate permissions (0666 for read/write)
- Use `SHM_RDONLY` for read-only access
- Check permissions before attachment

---

## Cleanup

To remove all shared memory segments:

```bash
# Using demo script
./demo.sh
# Select option 7: "Cleanup Shared Memory"

# Or manually:
# Remove POSIX shared memory
rm -f /dev/shm/my_shared_memory
rm -f /dev/shm/sync_shm
rm -f /dev/shm/image_shm
rm -f /dev/shm/sem.*

# Remove System V shared memory
ipcs -m | grep "^0x" | awk '{print $2}' | xargs -r ipcrm -m
```

---

## Requirements

- Linux operating system
- GCC compiler
- POSIX threads library (pthread)
- Real-time library (rt)

Install dependencies on Ubuntu/Debian:
```bash
sudo apt-get install build-essential
```

---

## Testing Checklist

- [x] System V basic read/write works
- [x] POSIX basic read/write works
- [x] Producer-consumer synchronization correct
- [x] Image sharing without data corruption
- [x] Benchmark runs and produces results
- [x] Proper cleanup of resources
- [x] Error handling works correctly

---

## Future Enhancements

1. Add more complex synchronization patterns (reader-writer locks)
2. Implement shared memory with message queues
3. Add memory-mapped file sharing examples
4. Create multi-process benchmarks (not just multi-threaded)
5. Add visualization of shared memory state

---

## References

- `man shm_overview` - Shared memory overview
- `man shmget` - System V shared memory
- `man shm_open` - POSIX shared memory
- `man sem_overview` - POSIX semaphores
- Stevens & Rago, "Advanced Programming in the UNIX Environment"

