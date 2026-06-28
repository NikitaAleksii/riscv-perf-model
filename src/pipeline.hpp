#pragma once

#include "predictor.hpp"
#include "cache.hpp"
#include <cstdint>
#include <string>
#include <memory>

// Instruction currently in flight
struct InstructionInFlight {
    std::int64_t pc;
    std::string type;
    int dst;
    int src1; int src2;
    bool taken;  
    std::int64_t mem_addr;
};

//
struct PipelineState {
    std::unique_ptr<Predictor> predictor;
    std::unique_ptr<Cache> dcache;

    long long cycle = 0;
    long long last_completion_cycle = 0;
    long long register_ready_cycle[32] = {0};

    long long total_instruction_count = 0;

    long long total_stall_cycles = 0;
    long long stall_cycles_data_hazard = 0;

    long long total_branch_count = 0;
    long long total_branch_mispredict = 0;
    long long stall_cycles_branch_flush = 0;
    long long stall_cycles_cache_miss = 0;

    long long total_jal_count = 0;
    long long total_jalr_count = 0;
};

void initialize_pipeline(PipelineState& pipeline);
void process_instruction(PipelineState& pipeline, const InstructionInFlight& instr);