#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "pipeline.hpp"
#include "stats.hpp"

std::unique_ptr<Predictor> set_predictor(const std::string& predictor_name) {
    if (predictor_name == "not_taken") return std::make_unique<AlwaysNotTakenPredictor>();
    if (predictor_name == "bimodal") return std::make_unique<BimodalPredictor>();
    if (predictor_name == "gshare") return std::make_unique<GsharePredictor>();
    return nullptr;
}

/**
 * @brief Parses a normalized RISC-V trace and emits statistics.
 *
 * Reads the normalized trace file at @p argv[1], counts instructions by
 * category, prints a breakdown, and writes a JSON results file via save_json().
 *
 * @param argc  Argument count; must be at least 3.
 * @param argv  argv[1] is the full path to the normalized trace file;
 *              argv[2] is the directory where results will be written.
 * @return      0 on success, 1 on usage error, missing file, or empty trace.
 */
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <trace_path> <results_dir> [predictor: not_taken|bimodal|gshare]\n";
        return 1;
    }

    std::string file_norm = argv[1];
    std::string results_dir = argv[2];
    std::string predictor_name = (argc >= 4) ? argv[3] : "gshare";

    // Open the file in a reading more and check if it was successful
    std::ifstream in_file(file_norm);
    if (!in_file.is_open())
    {
        std::cerr << "Error: could not open " << file_norm << "\n";
        return 1;
    }

    Stats stats;
    PipelineState pipeline;
    initialize_pipeline(pipeline);

    pipeline.predictor = set_predictor(predictor_name);
    if (pipeline.predictor == nullptr) {
        std::cerr << "Error: predictor not supported \n";
        return 1;
    }

    // Parse instructions in the file
    std::string line;
    while (getline(in_file, line))
    {
        std::stringstream ss(line);

        std::string pc_str, type, opcode_str;
        std::string dst_str, src1_str, src2_str, taken_str, mem_str;
        if (!(ss >> pc_str >> type >> opcode_str >> dst_str >> src1_str >> src2_str >> taken_str >> mem_str)) continue;

        // Classify instruction type
        stats.instructions += 1;
        if (type == "LOAD")
            stats.load += 1;
        else if (type == "STORE")
            stats.store += 1;
        else if (type == "BRANCH")
            stats.branch += 1;
        else if (type == "JAL")
            stats.jal += 1;
        else if (type == "JALR")
            stats.jalr += 1;
        else if (type == "ALU")
            stats.alu += 1;

        // Build InstructionInFlight and execute the timing model
        InstructionInFlight inst;
        inst.pc = std::stoull(pc_str, nullptr, 16);
        inst.type = type;
        inst.dst = (dst_str == "-") ? -1 : std::stoi(dst_str);
        inst.src1 = (src1_str == "-") ? -1 : std::stoi(src1_str);
        inst.src2 = (src2_str == "-") ? -1 : std::stoi(src2_str);
        inst.taken = (taken_str == "Y");

        process_instruction(pipeline, inst);
    }

    // Check whether the program has any valid instructions
    if (stats.instructions == 0)
    {
        std::cerr << "No valid instructions found in trace.\n";
        return 1;
    }

    // Output file statistics into a json file
    if (save_json(file_norm, results_dir, stats, pipeline) == 1) return 1;

    return 0;
}