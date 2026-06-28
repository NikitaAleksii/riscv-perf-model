#include "nlohmann/json.hpp"
#include "stats.hpp"

#include <iostream>
#include <fstream>
#include <sstream>


/**
 * @brief Serializes instruction statistics to a JSON results file.
 *
 * Gets the benchmark name from @p file_norm by stripping the directory
 * prefix and file extension, then writes a JSON object to
 * `<results_dir>/<name>.json` containing the instruction counts and
 * placeholder fields for metrics computed in later pipeline stages
 * (cycles, IPC, mispredictions, stall cycles).
 *
 * @param file_norm    Full path to the normalized trace.
 * @param results_dir  Directory where the JSON file will be written.
 * @param stats        Instruction counts.
 * @param pipeline     Timing model results (cycles, stalls, etc.).
 * @return             0 on success, 1 if the output file could not be opened.
 */

int save_json(const std::string& file_norm, const std::string& results_dir, const Stats& stats, const PipelineState& pipeline)
{
    nlohmann::json results;

    std::string file_name = file_norm.substr(file_norm.find_last_of('/') + 1);
    file_name = file_name.erase(file_name.find_first_of('.'));

    results["trace"] = file_name;
    results["instructions"] = stats.instructions;
    results["categories"] = {
        {"LOAD", stats.load}, {"STORE", stats.store}, {"BRANCH", stats.branch}, {"JAL", stats.jal}, {"JALR", stats.jalr}, {"ALU", stats.alu}};
    results["cycles"] = pipeline.last_completion_cycle;
    results["ipc"] = static_cast<double>(pipeline.total_instruction_count) / pipeline.last_completion_cycle;
    results["branch_mispredictions"] = pipeline.total_branch_mispredict;
    results["branch_mispredictions_rate"] = (pipeline.total_branch_count > 0) ? 100.0 * pipeline.total_branch_mispredict / pipeline.total_branch_count
                                            : 0.0;
    results["total_stall_cycles"] = pipeline.total_stall_cycles;
    results["stall_cycles_data_hazard"] = pipeline.stall_cycles_data_hazard;
    results["dcache_accesses"] = pipeline.dcache->accesses;
    results["dcache_hits"] = pipeline.dcache->hits;
    results["dcache_misses"] = pipeline.dcache->misses;
    results["stall_cycles_cache_miss"] = pipeline.stall_cycles_cache_miss;
    results["dcache_mpki"] = 1000.0 * pipeline.dcache->misses / stats.instructions;
    results["stall_cycles_branch_flush"] = pipeline.stall_cycles_branch_flush;

    std::string dir = results_dir;
    if (!results_dir.empty() && results_dir.back() != '/') dir += '/';

    std::string out_path = dir + file_name + ".json";
    std::ofstream output_file(out_path);
    if (!output_file.is_open())
    {
        std::cerr << "Error: could not open output file " << out_path << "\n";
        return 1;
    }
    output_file << results.dump(4);
    output_file.close();

    return 0;
}