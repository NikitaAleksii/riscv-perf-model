#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <sstream>

enum class Category
{
    LOAD,
    STORE,
    BRANCH,
    JAL,
    JALR,
    ALU
};

// Returns a category for a given instruction encoding 
Category categorize_by_opcode(uint32_t encoding)
{
    uint32_t opcode = encoding & 0b1111111;
    switch (opcode)
    {
    case 0b0000011:
        return Category::LOAD;
    case 0b0100011:
        return Category::STORE;
    case 0b1100011:
        return Category::BRANCH;
    case 0b1101111:
        return Category::JAL;
    case 0b1100111:
        return Category::JALR;
    default:
        return Category::ALU;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <trace_file>\n";
        return 1;
    }

    // Open the file in a reading more and check if it was successful
    std::ifstream in_file(argv[1]);
    if (!in_file.is_open())
    {
        std::cerr << "Error: could not open " << argv[0] << "\n";
        return 1;
    }

    long long counter{0};
    long long load{0}, store{0}, branch{0}, jal{0}, jalr{0}, alu{0};
    std::string line;

    // Parse instructions in the file
    while (getline(in_file, line))
    {
        std::stringstream ss(line);
        std::string tok;

        // Skip first 4 entries -- core 0: 3 0x0000000000001000
        for (int i = 0; i < 4; i++)
        {
            ss >> tok;
        }

        // If the read failed, skip to the next iteration 
        if (!(ss >> tok))
            continue;

        // Strip the parens (0x00000297) -> 0x00000297
        std::string hex_str = tok.substr(1, tok.length() - 2);

        // Convert hex string into integer
        uint32_t encoding = std::stoul(hex_str, nullptr, 16);

        // Categorize the instruction and increment category counter
        Category cat = categorize_by_opcode(encoding);
        switch (cat)
        {
        case Category::LOAD:
            load+=1; break;
        case Category::STORE:
            store+=1; break;
        case Category::BRANCH:
            branch+=1; break;
        case Category::JAL:
            jal+=1; break;
        case Category::JALR:
            jalr+=1; break;
        default:
            alu+=1; break;
        }
        counter += 1;
    }

    std::cout << "There are " << counter << " instructions." << "\n";
    std::cout << "There are " << load << " load instructions. " << (static_cast<double>(load*100)/counter) << "%\n";
    std::cout << "There are " << store << " store instructions. " << (static_cast<double>(store*100)/counter) << "%\n";
    std::cout << "There are " << branch << " branch instructions. " << (static_cast<double>(branch*100)/counter) << "%\n";
    std::cout << "There are " << jal << " jal instructions. " << (static_cast<double>(jal*100)/counter) <<  "%\n";
    std::cout << "There are " << jalr << " jalr instructions. " << (static_cast<double>(jalr*100)/counter) << "%\n";
    std::cout << "There are " << alu << " alu instructions. " << (static_cast<double>(alu*100)/counter) << "%\n";
    return 0;
}