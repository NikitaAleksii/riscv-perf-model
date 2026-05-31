# riscv-perf-model

A trace-driven cycle-level performance model for an in-order RISC-V core.

This project models how a small RV64 in-order pipeline would execute real programs — predicting cycle counts, IPC, and stall causes (data hazards, cache misses, branch mispredictions) — and validates those predictions against ground-truth references. It targets the same scoping discipline as academic microarchitecture simulators (gem5, ChampSim) but at a size one person can reason about end-to-end.

**Status:** in development. 

## Project structure

```
riscv-perf-model/
├── src/                    C++ simulator source
├── scripts/                Python tooling (trace normalizer, harness)
├── tests/                  small RISC-V test programs
├── traces/                 raw and normalized traces 
├── configs/                simulator configurations 
├── results/                JSON stats and plots 
├── benchmarks/embench-iot/ Embench-IoT (git submodule, GPL-3.0)
```

## Setup

After cloning:

```bash
git submodule update --init
mkdir build && cd build && cmake ..
make
```

Requires the RISC-V GNU toolchain (`riscv64-unknown-elf-gcc`) and Spike (`riscv-isa-sim`). On macOS:

```bash
brew tap riscv-software-src/riscv
brew install riscv-tools
```

## How to use

### Cross-compile a small C program for RISC-V

```bash
riscv64-unknown-elf-gcc -march=rv64imac -mabi=lp64 -o name.elf name.c
```

The cross-toolchain ships as `riscv64-*` only on macOS; emitting RV32 binaries would require building from source. Spike runs RV64 by default, so `rv64imac` works here. The softcore validation in Week 6 will use `-march=rv32i`.

### Run a binary in Spike

```bash
spike pk name.elf
```

### Generate an execution trace

```bash
spike --log-commits pk name.elf 2> name.log
```

The trace is Spike's commit log — one line per retired instruction with PC and encoding.

### Compile an Embench benchmark for Spike

Embench's build system targets specific embedded boards. To compile a single benchmark for Spike + pk, bypass the build system and stub the board/chip layer:

```bash
riscv64-unknown-elf-gcc \
    -march=rv64imac -mabi=lp64 \
    -O2 \
    -DCPU_MHZ=1 \
    -DWARMUP_HEAT=1 \
    -DGLOBAL_SCALE_FACTOR=1 \
    -Isupport \
    -Isrc/aha-mont64 \
    src/aha-mont64/*.c \
    support/beebsc.c \
    spike_harness.c \
    -o aha-mont64.elf
```

See `tests/embench_main.c` for the minimal main + empty board-function stubs that replace Embench's normal harness.

### Normalize a trace

```bash
python3 scripts/normalize_trace.py traces/aha-mont64.log traces/aha-mont64.norm
```

Traces include C-runtime initialization (`__libc_init_array`, newlib
setup, atexit handlers) before `main()` runs — typically ~140k instructions.
The normalizer does not strip this; the runtime is part of how programs
actually execute. For Embench benchmarks, dilute this fixed overhead by
increasing `GLOBAL_SCALE_FACTOR` at compile time (e.g., `-DGLOBAL_SCALE_FACTOR=10`),
which makes the benchmark's hot path repeat enough times that the runtime
becomes a small fraction of the total trace.

Output format: one line per user-space instruction with fields `PC TYPE OPCODE DST SRC1 SRC2 TAKEN MEM_ADDR`. Kernel-space (pk) instructions are filtered.

## Benchmarks

This project uses [Embench-IoT](https://github.com/embench/embench-iot) (GPL-3.0) as a git submodule under `benchmarks/embench-iot/`. The submodule contains Embench's source code under its own license; this project's code remains MIT.

Run `bash normalize_all.sh` to generate and normalize some of the embench traces.