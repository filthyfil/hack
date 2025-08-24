// hackassembler.cpp
// translates Hack Assembly language to Hack machine code.

#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <bitset>

class Parser {
public:
    std::ifstream assembly_file;
    std::string current_instruction;

    explicit Parser(const std::string& file) {
        assembly_file.open(file);
        if (!assembly_file.is_open()) {
            std::cerr << "[Error] Unable to open input file: " << file << "\n";
            exit(1);
        }
    }

    ~Parser() {
        if (assembly_file.is_open()) {
            assembly_file.close();
        }
    }

    // resets the file stream to the beginning for the second pass.
    void reset() {
        if (assembly_file.is_open()) {
            assembly_file.clear();
            assembly_file.seekg(0, std::ios::beg);
        }
        current_instruction.clear();
    }

    // reads the next command, skipping whitespace/comments.
    // returns true if a command was found, false if EOF.
    bool advance() {
        current_instruction.clear();
        while (std::getline(assembly_file, current_instruction)) {
            // remove comments
            auto comment_pos = current_instruction.find("//");
            if (comment_pos != std::string::npos) {
                current_instruction = current_instruction.substr(0, comment_pos);
            }
            // remove whitespace
            current_instruction.erase(std::remove_if(current_instruction.begin(), current_instruction.end(),
                [](unsigned char c) { return std::isspace(c); }),
                current_instruction.end());

            if (!current_instruction.empty()) {
                return true; // found a valid instruction
            }
        }
        return false;
    }

    std::string commandType() const {
        if (current_instruction.empty()) return "NULL";
        if (current_instruction[0] == '@') return "A_COMMAND";
        if (current_instruction[0] == '(') return "L_COMMAND";
        return "C_COMMAND";
    }

    std::string symbol() const {
        if (commandType() == "A_COMMAND") {
            return current_instruction.substr(1);
        }
        if (commandType() == "L_COMMAND") {
            return current_instruction.substr(1, current_instruction.length() - 2);
        }
        return "";
    }

    std::string dest() const {
        auto pos = current_instruction.find('=');
        if (pos != std::string::npos) {
            return current_instruction.substr(0, pos);
        }
        return "NULL";
    }

    std::string comp() const {
        auto eq_pos = current_instruction.find('=');
        auto sc_pos = current_instruction.find(';');
        size_t start = (eq_pos == std::string::npos) ? 0 : eq_pos + 1;
        size_t end = (sc_pos == std::string::npos) ? current_instruction.length() : sc_pos;
        return current_instruction.substr(start, end - start);
    }

    std::string jump() const {
        auto pos = current_instruction.find(';');
        if (pos != std::string::npos) {
            return current_instruction.substr(pos + 1);
        }
        return "NULL";
    }
};

class Coder {
public:
    std::string dest(const std::string& mnemonic) const {
        std::string bits = "000";
        if (mnemonic.find('A') != std::string::npos) bits[0] = '1';
        if (mnemonic.find('D') != std::string::npos) bits[1] = '1';
        if (mnemonic.find('M') != std::string::npos) bits[2] = '1';
        return bits;
    }

    std::string jump(const std::string& mnemonic) const {
        static const std::unordered_map<std::string, std::string> jump_map = {
            {"NULL", "000"}, {"JGT", "001"}, {"JEQ", "010"}, {"JGE", "011"},
            {"JLT", "100"}, {"JNE", "101"}, {"JLE", "110"}, {"JMP", "111"}
        };
        auto it = jump_map.find(mnemonic);
        return it != jump_map.end() ? it->second : "000";
    }

    std::string comp(const std::string& mnemonic) const {
        static const std::unordered_map<std::string, std::string> comp_map = {
            {"0",   "0101010"}, {"1",   "0111111"}, {"-1",  "0111010"},
            {"D",   "0001100"}, {"A",   "0110000"}, {"M",   "1110000"},
            {"!D",  "0001101"}, {"!A",  "0110001"}, {"!M",  "1110001"},
            {"-D",  "0001111"}, {"-A",  "0110011"}, {"-M",  "1110011"},
            {"D+1", "0011111"}, {"A+1", "0110111"}, {"M+1", "1110111"},
            {"D-1", "0001110"}, {"A-1", "0110010"}, {"M-1", "1110010"},
            {"D+A", "0000010"}, {"D+M", "1000010"},
            {"D-A", "0010011"}, {"D-M", "1010011"},
            {"A-D", "0000111"}, {"M-D", "1000111"},
            {"D&A", "0000000"}, {"D&M", "1000000"},
            {"D|A", "0010101"}, {"D|M", "1010101"}
        };
        auto it = comp_map.find(mnemonic);
        if (it != comp_map.end()) {
            return it->second;
        }
        std::cerr << "[Error] Unknown comp mnemonic: " << mnemonic << "\n";
        return "ERROR"; // default error code
    }
};

class SymbolTable {
private:
    std::unordered_map<std::string, unsigned int> table;

public:
    SymbolTable() {
        table = {
            {"SP", 0}, {"LCL", 1}, {"ARG", 2}, {"THIS", 3}, {"THAT", 4},
            {"R0", 0}, {"R1", 1}, {"R2", 2}, {"R3", 3}, {"R4", 4}, {"R5", 5},
            {"R6", 6}, {"R7", 7}, {"R8", 8}, {"R9", 9}, {"R10", 10}, {"R11", 11},
            {"R12", 12}, {"R13", 13}, {"R14", 14}, {"R15", 15},
            {"SCREEN", 16384}, {"KBD", 24576}
        };
    }

    void addSymbol(const std::string& symbol, unsigned int address) {
        table[symbol] = address;
    }

    bool contains(const std::string& symbol) const {
        return table.find(symbol) != table.end();
    }

    unsigned int getAddress(const std::string& symbol) const {
        auto it = table.find(symbol);
        return it != table.end() ? it->second : -1;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.asm> <output.hack>\n";
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];

    Parser parser(input_file);
    Coder coder;
    SymbolTable symbolTable;

    // --- PASS 1: build the symbol table with labels ---
    unsigned int romAddress = 0;
    while (parser.advance()) {
        if (parser.commandType() == "L_COMMAND") {
            if (!symbolTable.contains(parser.symbol())) {
                symbolTable.addSymbol(parser.symbol(), romAddress);
            }
        } else {
            romAddress++; // only increment for A or C commands
        }
    }

    parser.reset(); // reset file for the second pass

    // --- PASS 2: generate code and handle variables ---
    std::ofstream hack_file(output_file);
    if (!hack_file.is_open()) {
        std::cerr << "[Error] Unable to create output file: " << output_file << "\n";
        return 1;
    }

    unsigned int ramAddress = 16; // variables are allocated starting at RAM address 16
    while (parser.advance()) {
        if (parser.commandType() == "A_COMMAND") {
            std::string symbol = parser.symbol();
            unsigned int value = 0;
            bool is_numeric = false;
            try {
                value = std::stoul(symbol);
                is_numeric = true;
            } catch (const std::invalid_argument&) {
                is_numeric = false;
            }

            if (is_numeric) {
                hack_file << "0" << std::bitset<15>(value) << "\n";
            } else { // it's a symbol
                if (!symbolTable.contains(symbol)) {
                    // it's a new variable; assign it the next available RAM address.
                    symbolTable.addSymbol(symbol, ramAddress++);
                }
                hack_file << "0" << std::bitset<15>(symbolTable.getAddress(symbol)) << "\n";
            }
        } else if (parser.commandType() == "C_COMMAND") {
            std::string comp_bits = coder.comp(parser.comp());
            std::string dest_bits = coder.dest(parser.dest());
            std::string jump_bits = coder.jump(parser.jump());
            hack_file << "111" << comp_bits << dest_bits << jump_bits << "\n";
        }
        // L_COMMANDs are ignored in the second pass as they don't generate code.
    }

    hack_file.close();
    std::cout << "Assembly successful. Output written to " << output_file << "\n";
    return 0;
}
