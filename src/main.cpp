#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "nlohmann/json.hpp"

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
 * @brief Serializes instruction statistics to a JSON results file.
 *
 * Gets the benchmark name from @p file_norm by stripping the directory
 * prefix and file extension, then writes a JSON object to
 * `../../results/<name>.json` containing the instruction counts and
 * placeholder fields for metrics computed in later pipeline stages
 * (cycles, IPC, mispredictions, stall cycles).
 *
 * @param file_norm  Path to the normalized trace file (used to derive the
 *                   output file name).
 * @param stats      Instruction statistics.
 * @return           0 on success, 1 if the output file could not be opened.
 */
int save_json(std::string file_norm, Stats stats)
{
    nlohmann::json results;

    std::string file_name = file_norm.substr(file_norm.find_last_of('/') + 1);
    file_name = file_name.erase(file_name.find_first_of('.'));

    results["trace"] = file_name;
    results["instructions"] = stats.instructions;
    results["categories"] = {
        {"LOAD", stats.load}, {"STORE", stats.store}, {"BRANCH", stats.branch}, {"JAL", stats.jal}, {"JALR", stats.jalr}, {"ALU", stats.alu}};
    results["cycles"] = "";
    results["ipc"] = "";
    results["branch_mispredictions"] = "";
    results["stall_cycles_data_hazard"] = "";
    results["stall_cycles_cache_miss"] = "";
    results["stall_cycles_branch_flush"] = "";

    std::ofstream output_file("../../results/" + file_name + ".json");
    if (!output_file.is_open())
    {
        std::cerr << "Error: could not open output file ../../results/" << file_name << ".json\n";
        return 1;
    }
    output_file << results.dump(4);
    output_file.close();

    return 0;
}

/**
 * @brief Parses a normalized RISC-V trace and emits statistics.
 *
 * Reads the normalized trace file at `../../traces/<argv[1]>`, counts
 * instructions by category, prints breakdown, and
 * writes a JSON results file via save_json().
 *
 * @param argc  Argument count; must be at least 2.
 * @param argv  argv[1] is the normalized trace file name (basename only).
 * @return      0 on success, 1 on usage error, missing file, or empty trace.
 */
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <processed_trace_file_name>\n";
        return 1;
    }

    std::string file_norm = "../../traces/" + static_cast<std::string>(argv[1]);

    // Open the file in a reading more and check if it was successful
    std::ifstream in_file(file_norm);
    if (!in_file.is_open())
    {
        std::cerr << "Error: could not open " << file_norm << "\n";
        return 1;
    }

    Stats stats;
    std::string line;

    // Parse instructions in the file
    while (getline(in_file, line))
    {
        std::stringstream ss(line);
        std::string tok;

        ss >> tok;      // skip PC
        if (!(ss >> tok))
            continue;   // read TYPE

        stats.instructions += 1;

        if (tok == "LOAD")
            stats.load += 1;
        else if (tok == "STORE")
            stats.store += 1;
        else if (tok == "BRANCH")
            stats.branch += 1;
        else if (tok == "JAL")
            stats.jal += 1;
        else if (tok == "JALR")
            stats.jalr += 1;
        else if (tok == "ALU")
            stats.alu += 1;
    }

    // Check whether the program has any valid instructions
    if (stats.instructions == 0)
    {
        std::cerr << "No valid instructions found in trace.\n";
        return 1;
    }

    // Output file statistics into a json file
    if (save_json(file_norm, stats) == 1) return 1;

    std::cout << "There are " << stats.instructions << " instructions." << "\n";
    std::cout << "There are " << stats.load << " load instructions. " << (static_cast<double>(stats.load * 100) / stats.instructions) << "%\n";
    std::cout << "There are " << stats.store << " store instructions. " << (static_cast<double>(stats.store * 100) / stats.instructions) << "%\n";
    std::cout << "There are " << stats.branch << " branch instructions. " << (static_cast<double>(stats.branch * 100) / stats.instructions) << "%\n";
    std::cout << "There are " << stats.jal << " jal instructions. " << (static_cast<double>(stats.jal * 100) / stats.instructions) << "%\n";
    std::cout << "There are " << stats.jalr << " jalr instructions. " << (static_cast<double>(stats.jalr * 100) / stats.instructions) << "%\n";
    std::cout << "There are " << stats.alu << " alu instructions. " << (static_cast<double>(stats.alu * 100) / stats.instructions) << "%\n";
    return 0;
}