#!/bin/bash
set -e

BENCHMARKS=(
    "aha-mont64"
    "crc32"
    "edn"
)

# Embench root
EMBENCH=../benchmarks/embench-iot

for BENCH in "${BENCHMARKS[@]}"; do
    echo "=== ${BENCH} ==="

    # Compile
    riscv64-unknown-elf-gcc \
        -march=rv64imac \
        -mabi=lp64 \
        -O2 \
        -DCPU_MHZ=1 \
        -DWARMUP_HEAT=1 \
        -DGLOBAL_SCALE_FACTOR=1 \
        -I${EMBENCH}/support \
        -I${EMBENCH}/src/${BENCH} \
        ${EMBENCH}/src/${BENCH}/*.c \
        ${EMBENCH}/support/beebsc.c \
        ../tests/embench_main.c \
        -o ../tests/${BENCH}.elf
    
    # Trace
    spike --log-commits pk ../tests/${BENCH}.elf 2> ../traces/${BENCH}.log
    
    # Normalize
    python3 normalize_trace.py ../traces/${BENCH}.log ../traces/${BENCH}.norm
    
    echo "  $(wc -l < ../traces/${BENCH}.norm) instructions normalized"
done

echo "All benchmarks processed."