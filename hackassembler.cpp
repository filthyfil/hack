
// hackassembler.cpp : This file contains the 'main' function. Program execution begins and ends there.


// this project is meant for the development of the assembler for the hack assembly language
// the hack assembly language adheres to the binary bit code:
// i - 11 - a - cccccc - ddd - jjj
// INSTRUCTION - FIXED BITS - ADDRESSING - COMPUTATION - DESTINATION - JUMP
// or just:
// i11accccccdddjjj
// which corresponds to the 16-bit system

#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <unordered_map>
#include <bitset>
#include <functional>


class Parser {
public:
    std::ifstream assembly_file;
    std::string current_instruction;

    Parser(const std::string& file) {
        assembly_file.open(file);
        if (!assembly_file.is_open()) {
            std::cerr << "[error] There was a problem with opening the file.\n";
            exit(1);
        }
    }

    ~Parser() {
        if (assembly_file.is_open()) {
            assembly_file.close();
        }
    }

    bool hasMoreCommands() { 
        return (!assembly_file.eof());
    }

    void advance() {
        current_instruction.clear();
        while (std::getline(assembly_file, current_instruction)) {
            auto comment_pos = current_instruction.find("//");
            if (comment_pos != std::string::npos) {
                current_instruction = current_instruction.substr(0, comment_pos);
            }

            current_instruction.erase(std::remove_if(current_instruction.begin(), current_instruction.end(),
                [](unsigned char c) { return std::isspace(c); }),
                current_instruction.end());

            if (!current_instruction.empty()) {
                return;
            }
        }
    }

    std::string commandType() {
        if (current_instruction.empty()) {
            return "NULL"; // return a default value for empty instructions
        }

        if (current_instruction[0] == '@') {
            return "A_COMMAND";
        }
        else if (current_instruction[0] == '(' && current_instruction.back() == ')') {
            return "L_COMMAND";
        }
        else {
            return "C_COMMAND";
        }
    }

    std::string symbol() { 
        if (commandType() == "A_COMMAND") {
            return current_instruction.substr(1);
        }
        else if (commandType() == "L_COMMAND") {
            return current_instruction.substr(1, current_instruction.size() - 2); 
        }
        return "NULL";
    }

    std::string dest() { 
        if (commandType() != "C_COMMAND") {
            return "NULL";
        }
        auto pos_equals = current_instruction.find('=');
        if (pos_equals == std::string::npos) {
            return "NULL";
        }
        return current_instruction.substr(0, pos_equals);
    }

    std::string comp() {
        if (commandType() != "C_COMMAND") {
            return "NULL";
        }
        auto pos_equals = current_instruction.find('=');
        auto pos_semicolon = current_instruction.find(';');

        size_t comp_start = (pos_equals != std::string::npos) ? pos_equals + 1 : 0;
        size_t comp_end = (pos_semicolon != std::string::npos) ? pos_semicolon : current_instruction.size();

        return current_instruction.substr(comp_start, (comp_end - comp_start));
    }

    std::string jump() { 
        if (commandType() != "C_COMMAND") {
            return "NULL";
        }
        auto pos_semicolon = current_instruction.find(';');
        if (pos_semicolon != std::string::npos) {
            return current_instruction.substr(pos_semicolon + 1);
        }
        else {
            return "NULL";
        }
    }

    void reset() {
        if (assembly_file.is_open()) {
            assembly_file.clear();
            assembly_file.seekg(0, std::ios::beg);
        }
        current_instruction.clear();
    }
};

class Coder {
public:
    std::string dest(const std::string& dest_mnemonic) {
        // dest_mnumonic takes the form: NULL, M, D, MD, A, AM, AD, AMD
        auto A = dest_mnemonic.find('A') != std::string::npos ? 1 : 0;
        auto D = dest_mnemonic.find('D') != std::string::npos ? 1 : 0;
        auto M = dest_mnemonic.find('M') != std::string::npos ? 1 : 0;

        return std::to_string(A) + std::to_string(D) + std::to_string(M);
    }

    std::string comp(const std::string& comp_mnemonic) {
        // comp_mnumonic takes the form: NULL, M, D, MD, A, AM, AD, AMD
     
        const std::unordered_map<std::string, std::string> comp_map = {
            {"NULL", ""},
            // Map for A=0 (computations involving register A)
            {"0", "0101010"}, 
            {"1", "0111111"}, 
            {"-1", "0111010"},
            {"D", "0001100"}, 
            {"A", "0110000"}, 
            {"!D", "0001101"},
            {"!A", "0110001"}, 
            {"-D", "0001111"}, 
            {"-A", "0110011"},
            {"D+1", "0011111"}, 
            {"A+1", "0110111"}, 
            {"D-1", "0001110"},
            {"A-1", "0110010"}, 
            {"D+A", "0000010"}, 
            {"D-A", "0010011"},
            {"A-D", "0000111"}, 
            {"D&A", "0000000"}, 
            {"D|A", "0010101"},
            // Map for A=1 (computations involving memory)
            {"0", "1101010"}, 
            {"1", "1111111"}, 
            {"-1", "1111010"},
            {"D", "1001100"}, 
            {"M", "1110000"}, 
            {"!D", "1001101"},
            {"!M", "1110001"}, 
            {"-D", "1001111"}, 
            {"-M", "1110011"},
            {"D+1", "1011111"}, 
            {"M+1", "1110111"}, 
            {"D-1", "1001110"},
            {"M-1", "1110010"}, 
            {"D+M", "1000010"}, 
            {"D-M", "1010011"},
            {"M-D", "1000111"}, 
            {"D&M", "1000000"}, 
            {"D|M", "1010101"}
        };
        auto it = comp_map.find(comp_mnemonic);
        if (it != comp_map.end()) {
            return it->second;
        }
        else {
            std::cerr << "[error] Unknown comp mnemonic: " << comp_mnemonic << "\n";
            return "ERROR";
        }
    }

    std::string jump(const std::string& jump_mnemonic) {
        // jump_mnumonic takes the form: NULL, JGT, JEQ, JGE, JLT, JNE, JLE, JMP
        const std::unordered_map<std::string, std::string> jump_map = {
            {"NULL", "000"},
            {"JGT", "001"},
            {"JEQ", "010"},
            {"JGE", "011"},
            {"JLT", "100"},
            {"JNE", "101"},
            {"JLE", "110"},
            {"JMP", "111"}
        };
        auto it = jump_map.find(jump_mnemonic);
        if (it != jump_map.end()) {
            return it->second;
        }
        else {
            std::cerr << "[error] Unknown jump mnemonic: " << jump_mnemonic << "\n";
            return "ERROR";
        }
    }
};

class Symbol {
private:
    std::unordered_map<std::string, std::bitset<16>> hash_table;

    // predefined symbols table
    const std::unordered_map<std::string, std::bitset<16>> predefined_symbols = {
        {"R0",   std::bitset<16>("0000000000000000")},
        {"SP",   std::bitset<16>("0000000000000000")},
        {"R1",   std::bitset<16>("0000000000000001")},
        {"LCL",  std::bitset<16>("0000000000000001")},
        {"R2",   std::bitset<16>("0000000000000010")},
        {"ARG",  std::bitset<16>("0000000000000010")},
        {"R3",   std::bitset<16>("0000000000000011")},
        {"THIS", std::bitset<16>("0000000000000011")},
        {"R4",   std::bitset<16>("0000000000000100")},
        {"THAT", std::bitset<16>("0000000000000100")},
        {"R5",   std::bitset<16>("0000000000000101")},
        {"R6",   std::bitset<16>("0000000000000110")},
        {"R7",   std::bitset<16>("0000000000000111")},
        {"R8",   std::bitset<16>("0000000000001000")},
        {"R9",   std::bitset<16>("0000000000001001")},
        {"R10",  std::bitset<16>("0000000000001010")},
        {"R11",  std::bitset<16>("0000000000001011")},
        {"R12",  std::bitset<16>("0000000000001100")},
        {"R13",  std::bitset<16>("0000000000001101")},
        {"R14",  std::bitset<16>("0000000000001110")},
        {"R15",  std::bitset<16>("0000000000001111")}
    };

public: 
    Symbol() {
        // copy predefined symbols
        hash_table = predefined_symbols;
    }

    std::bitset<16> process(const std::string& symbol) {
        std::bitset<16> binary;
        for (char c : symbol) {
            binary <<= 1;
            binary |= (c & 1);
        }
        return binary;
    }

    void addSymbol(const std::string& symbol, const std::bitset<16>& value) {
        hash_table[symbol] = value;
    }

    int hash(int number_of_memory_addresses) {
        const int memorable_location = 16;
        int hash_value = memorable_location + number_of_memory_addresses;
        return hash_value;
    }

    bool contains(const std::string& symbol) const {
        return hash_table.find(symbol) != hash_table.end();
    }
        
    void define(const std::string& symbol, const std::bitset<16>& value) {
        // check if the symbol is not predefined and not already in hash_table
        if (predefined_symbols.find(symbol) == predefined_symbols.end() && !contains(symbol)) {
            addSymbol(symbol, value);
        }
    }

    std::string find(const std::string& symbol) const {
        auto it = hash_table.find(symbol);
        if (it != hash_table.end()) {
            return (it->second).to_string(); // symbol found, return its value
        }
        else {
            std::cerr << "[error] Symbol '" << symbol << "' not found in table.\n";
            return "NOT FOUND"; 
        }
    }

    void overwrite(std::string symbol, int new_key) {
        auto it = hash_table.find(symbol);
            if (it != hash_table.end())
                it->second = new_key;
    }
};

void parse(const std::string& file, Parser& parser, Symbol& encoder) {
    const int width = 15;

    while (parser.hasMoreCommands()) {
        parser.advance();
        if (!parser.current_instruction.empty()) {

            std::cout << parser.current_instruction << "\n";
            std::cout << std::string(60, ' ') << "\n";


            std::string command_type = parser.commandType();
            std::string symbol = parser.symbol();
            std::string location = "NULL";
            std::string dest = parser.dest();
            std::string comp = parser.comp();
            std::string jump = parser.jump();

            if (command_type == "A_COMMAND") {
                if (encoder.contains(symbol)) {
                    location = encoder.find(symbol);
                }
                else {
                    try {
                        location = std::to_string(std::stoi(symbol));
                    }
                    catch (const std::invalid_argument&) {
                        location = "NULL"; // Symbol is not a number.
                    }
                    catch (const std::out_of_range&) {
                        location = "OUT_OF_RANGE"; // Number too large.
                    }
                }
            }

            std::cout << std::left;
            std::cout << std::setw(width) << "COMMAND TYPE:" << command_type << '\n';
            std::cout << std::setw(width) << "SYMBOL:" << symbol << '\n';
            std::cout << std::setw(width) << "LOCATION:" << location << '\n';
            std::cout << std::setw(width) << "DEST:" << dest << '\n';
            std::cout << std::setw(width) << "COMP:" << comp << '\n';
            std::cout << std::setw(width) << "JUMP:" << jump << '\n';

            std::cout << std::string(60, '=') << "\n";
        }
    }
    parser.reset();
}

void assemble(const std::string& input_file, const std::string& output_file, Parser& parser, Coder& coder, Symbol& encoder) {
    std::ofstream hack_file(output_file);

    while (parser.hasMoreCommands()) {
        parser.advance();
        if (parser.commandType() == "A_COMMAND") {
            // try if integer (@number)
            try {
                int value = std::stoi(parser.symbol());
                std::cout << "0" << std::bitset<15>(value) << "\n";
                hack_file << "0" << std::bitset<15>(value) << "\n";
            }
            // if stoi is impossible (because of symbol)@symbol), add into hash table
            catch (const std::invalid_argument&) {
                if (encoder.contains(parser.symbol())) {
                    std::string binary = encoder.find(parser.symbol());
                    std::cout << "" << binary << "\n";
                    hack_file << "" << binary << "\n";
                }
            }
        }
        else if (parser.commandType() == "C_COMMAND") {
            std::string comp_bits = coder.comp(parser.comp());
            std::string dest_bits = coder.dest(parser.dest());
            std::string jump_bits = coder.jump(parser.jump());
            std::cout << "111" << comp_bits << dest_bits << jump_bits << "\n";
            hack_file << "111" << comp_bits << dest_bits << jump_bits << "\n";
        }
    }
    std::cout << std::string(60, '-') << "\n";

    hack_file.close();
}

void defineLabels(Parser& parser, Symbol& encoder, int& int_rom_address) {
    while (parser.hasMoreCommands()) {
        parser.advance();
        if (parser.commandType() == "L_COMMAND") {
            std::string label = parser.symbol();
            std::bitset<16> binary_rom_address(int_rom_address);
            encoder.define(label, binary_rom_address);
        }
        else {
            int_rom_address++;
        }
    }
    std::cout << std::string(60, '=') << "\n";
    parser.reset();
}

void defineSymbols(Parser& parser, Symbol& encoder, int& number_of_memory_addresses) {
    while (parser.hasMoreCommands()) {
        parser.advance();
        if (parser.commandType() == "A_COMMAND") {
            try {
                int value = std::stoi(parser.symbol());
            }
            // if stoi is impossible (because of symbol)@symbol), add into hash table
            catch (const std::invalid_argument&) {
                if (encoder.contains(parser.symbol()) == false) {
                    int int_memory_location = encoder.hash(number_of_memory_addresses);
                    number_of_memory_addresses++;
                    std::bitset<16> binary_memory_location(int_memory_location);
                    encoder.define(parser.symbol(), binary_memory_location);
                    std::cout << "Created address for " << parser.symbol() << " at address " << encoder.find(parser.symbol()) << ".\n";
                }
            }
        }
    }
    std::cout << std::string(60, '=') << "\n";
    parser.reset();
}

int main() {
    const std::string input_file = "assembly_code.txt";
    const std::string output_file = "output.txt";

    int rom_address = 0;
    int random_memory_address = 0;

    Parser parser(input_file);
    Coder coder;
    Symbol encoder;
    
    defineLabels(parser, encoder, rom_address);
    defineSymbols(parser, encoder, random_memory_address);
    parse(input_file, parser, encoder);
    assemble(input_file, output_file, parser, coder, encoder);

    std::cout << "Done. Output written to " << output_file << "\n";
    return 0;
}