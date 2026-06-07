#pragma once

#include "pipeline.hpp"
#include <string>

/**
 * @brief Instruction counts derived from a normalized trace file.
 *
 * Each field tracks a distinct RISC-V instruction category. All counts
 * default to zero and are incremented as the trace is parsed line by line.
 */
struct Stats {
    long long instructions = 0;
    long long load = 0;       
    long long store = 0;        
    long long branch = 0;      
    long long jal = 0;         
    long long jalr = 0;        
    long long alu = 0;          
};

/**
 * @brief Serializes simulation results to a JSON file.
 *
 * @param file_norm    Full path to the normalized trace.
 * @param results_dir  Directory where the JSON file will be written.
 * @param stats        Instruction counts.
 * @param pipeline     Timing model results (cycles, stalls, etc.).
 * @return             0 on success, 1 if the output file could not be opened.
 */
int save_json(const std::string& file_norm, const std::string& results_dir, const Stats& stats, const PipelineState& pipeline);

