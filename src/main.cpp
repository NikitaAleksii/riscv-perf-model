#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <processed_trace_file>\n";
        return 1;
    }

    // Open the file in a reading more and check if it was successful
    std::ifstream in_file(argv[1]);
    if (!in_file.is_open())
    {
        std::cerr << "Error: could not open " << argv[1] << "\n";
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

        ss >> tok;                    // skip PC      
        if (!(ss >> tok)) continue;   // read TYPE

        counter += 1;
        
        if (tok == "LOAD") load+=1;
        else if (tok == "STORE") store+=1;
        else if (tok == "BRANCH") branch+=1;
        else if (tok == "JAL") jal+=1;
        else if (tok == "JALR") jalr+=1;
        else if (tok == "ALU") alu+=1;
    }

    if (counter == 0) {
        std::cerr << "No valid instructions found in trace.\n";
        return 1;
    }

    std::cout << "There are " << counter << " instructions." << "\n";
    std::cout << "There are " << load << " load instructions. " << (static_cast<double>(load * 100) / counter) << "%\n";
    std::cout << "There are " << store << " store instructions. " << (static_cast<double>(store * 100) / counter) << "%\n";
    std::cout << "There are " << branch << " branch instructions. " << (static_cast<double>(branch * 100) / counter) << "%\n";
    std::cout << "There are " << jal << " jal instructions. " << (static_cast<double>(jal * 100) / counter) << "%\n";
    std::cout << "There are " << jalr << " jalr instructions. " << (static_cast<double>(jalr * 100) / counter) << "%\n";
    std::cout << "There are " << alu << " alu instructions. " << (static_cast<double>(alu * 100) / counter) << "%\n";
    return 0;
}