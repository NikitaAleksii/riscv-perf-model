#include "pipeline.hpp"
#include <algorithm>

void initialize_pipeline(PipelineState& pipeline) {
    pipeline.cycle = 0;
    for (int i = 0; i < 32; i++) pipeline.register_ready_cycle[i] = 0;  // ready cycle for each register
    pipeline.total_instruction_count = 0;
    pipeline.total_stall_cycles = 0;
}

void process_instruction(PipelineState& pipeline, const InstructionInFlight& instr) {
    // Compute earliest issue cycle
    long long earliest = pipeline.cycle+1;
    
    // Check if the sources are ready to be taken
    if (instr.src1 >= 0) earliest = std::max(earliest, pipeline.register_ready_cycle[instr.src1]);
    if (instr.src2 >= 0) earliest = std::max(earliest, pipeline.register_ready_cycle[instr.src2]);

    // Compute stalls
    long long stalls = earliest - (pipeline.cycle + 1);

    // Compute when the instruction is ready
    int instr_latency = (instr.type == "LOAD") ? 2 : 1;
    long long ready = earliest + instr_latency;

    // Assign when the result of a register will be ready
    if (instr.dst > 0) pipeline.register_ready_cycle[instr.dst] = ready;
    
    // Branch/jump control hazard penalty: flush wrongly-fetched instructions.
    // BRANCH (taken): 2-cycle penalty (flush IF + ID).
    // JAL: 1-cycle penalty (target resolved at decode).
    // JALR: 2-cycle penalty (target resolved at execute).
    int branch_penalty = 0;
    if (instr.type == "BRANCH" && instr.taken)
        branch_penalty = 2;
    else if (instr.type == "JAL")
        branch_penalty = 1;
    else if (instr.type == "JALR")
        branch_penalty = 2;

    // Update pipeline state
    pipeline.cycle = earliest + branch_penalty;
    pipeline.last_completion_cycle = ready;
    pipeline.total_instruction_count += 1;
    pipeline.total_stall_cycles += stalls + branch_penalty;
    pipeline.stall_cycles_data_hazard += stalls;
    pipeline.stall_cycles_branch_flush += branch_penalty;
}