#pragma once

#include <cstdint>
#include <string>

// Instruction currently in flight
struct InstructionInFlight {
    std::int64_t pc;
    std::string type;
    int dst;
    int src1; int src2;
    bool taken;  
};

//
struct PipelineState {
    long long cycle = 0;
    long long last_completion_cycle = 0;
    long long register_ready_cycle[32] = {0};
    long long total_instruction_count = 0;
    long long total_stall_cycles = 0;
    long long stall_cycles_data_hazard = 0;
    long long stall_cycles_branch_flush = 0;
};

void initialize_pipeline(PipelineState& pipeline);
void process_instruction(PipelineState& pipeline, const InstructionInFlight& instr);