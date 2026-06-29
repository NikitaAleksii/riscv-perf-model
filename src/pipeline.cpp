#include "pipeline.hpp"
#include "predictor.hpp"
#include <algorithm>

const int MISPREDICT_PENALTY = 2;
const int CACHE_MISS_PENALTY = 30;
const int RAS_SIZE = 16;

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

    // Branch and jump control hazard penalty: flush wrongly-fetched instructions
    auto is_link = [](int r){return r == 1 || r == 5;}; // check if this is a link register (x1 or x5)
    int branch_penalty = 0;
    if (instr.type == "BRANCH") {
        bool predict_taken = pipeline.predictor->predict(instr.pc);
        bool actually_taken = instr.taken;
        pipeline.total_branch_count++;
        if (predict_taken != actually_taken) {
            branch_penalty = MISPREDICT_PENALTY;
            pipeline.total_branch_mispredict++;
        }
        pipeline.predictor->update(instr.pc, actually_taken);
    }
    else if (instr.type == "JAL") {
        pipeline.total_jal_count++;
        if (is_link(instr.dst)) {
            if ((int)pipeline.ras.size() == RAS_SIZE) {
                pipeline.ras.erase(pipeline.ras.begin());
            }
            pipeline.ras.push_back(instr.pc + instr.width);
        }
    }
    else if (instr.type == "JALR") {
        pipeline.total_jalr_count++;
        bool ret = (!is_link(instr.dst) && is_link(instr.src1)); // jalr x0, ra  -> return
        bool call = is_link(instr.dst); // jalr ra, rs1 -> indirect call
        if (ret) {
            if (!pipeline.ras.empty()) {
                pipeline.ras.pop_back();
                pipeline.ras_hits++;
            } else {
                branch_penalty = MISPREDICT_PENALTY;
                pipeline.ras_misses++;
            }
        } else if (call) {
            if ((int) pipeline.ras.size() == RAS_SIZE) pipeline.ras.erase(pipeline.ras.begin());
            pipeline.ras.push_back(instr.pc + instr.width);
        }
    }

    // L1 D-cache probe
    int dcache_penalty = 0;
    if (instr.type == "LOAD" || instr.type == "STORE") {
        if (!pipeline.dcache->access(instr.mem_addr)) {
            dcache_penalty = CACHE_MISS_PENALTY;
        }
    }

    // L1 I-cache probe — every instruction is fetched.
    int icache_penalty = 0;
    if (!pipeline.icache->access(instr.pc)) {
        icache_penalty = CACHE_MISS_PENALTY;
    }

    long long ready = earliest + instr_latency + dcache_penalty + icache_penalty;

    // Assign when the result of a register will be ready
    if (instr.dst > 0) pipeline.register_ready_cycle[instr.dst] = ready;

    // Update pipeline state
    pipeline.cycle = earliest + branch_penalty + dcache_penalty + icache_penalty;
    pipeline.last_completion_cycle = ready;
    pipeline.total_instruction_count += 1;
    pipeline.total_stall_cycles += stalls + branch_penalty + dcache_penalty + icache_penalty;
    pipeline.stall_cycles_data_hazard += stalls;
    pipeline.stall_cycles_branch_flush += branch_penalty;
    pipeline.stall_cycles_dcache_miss += dcache_penalty;
    pipeline.stall_cycles_icache_miss += icache_penalty;
}