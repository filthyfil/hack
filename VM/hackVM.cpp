// hackvm.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <unordered_map>
#include <bitset>
#include <functional>
#include <unordered_set>


// TODO:
// Implement test cases.


class Parser {
// this class unpacks a vm command into its components such that it is accessible and readable by the CodeWriter class
private:
    std::unordered_set<std::string> commands_with_args = {
            "C_PUSH",   
            "C_POP",
            "C_FUNCTION",
            "C_CALL"
        };

    // commands on the stack are only (as of part 1) of types:
    //      arithmetic, push / pop, 
    //      label, goto, if, 
    //      function, return, call
    std::unordered_map<std::string, std::string> defined_command_types = {
        {"add",      "C_ARITHMETIC"},  // addition
        {"sub",      "C_ARITHMETIC"},  // subtraction
        {"neg",      "C_ARITHMETIC"},  // negative
        {"eq",       "C_ARITHMETIC"},  // equals
        {"gt",       "C_ARITHMETIC"},  // greater than
        {"lt",       "C_ARITHMETIC"},  // less than
        {"and",      "C_ARITHMETIC"},  // AND operator
        {"or",       "C_ARITHMETIC"},  // OR operator
        {"not",      "C_ARITHMETIC"},  // NOT operator 

        {"push",     "C_PUSH"      },
        {"pop",      "C_POP"       },
        {"label",    "C_LABEL"     },
        {"goto",     "C_GOTO"      },
        {"if-goto",  "C_IF"        },
        {"function", "C_FUNCTION"  },
        {"call",     "C_CALL"      },
        {"return",   "C_RETURN"    }
    };

public:
    std::string current_command;
    std::ifstream vm_file;

    Parser(const std::string & file) {
        vm_file.open(file);
        if (!vm_file.is_open()) {
            std::cerr << "[error] there was a problem with opening the input file.\n";
            exit(1);
        }
    }

    ~Parser() {
        if (vm_file.is_open()) {
            vm_file.close();
        }
    }

    bool hasMoreCommands() {
        return (not vm_file.eof());
    }


    // helper function that trims leading and trailing whitespace.
    static std::string trim(const std::string &s) {
        // find first non-whitespace character.
        auto start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            return ""; // string is all whitespace.
        }
        // find last non-whitespace character.
        auto end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    }
    void advance() {
        current_command.clear();
        while (std::getline(vm_file, current_command)) {
            // remove inline comments.
            auto comment_pos = current_command.find("//");
            if (comment_pos != std::string::npos) {
                current_command = current_command.substr(0, comment_pos);
            }
            // trim (remove leading/trailing whitespace)
            current_command = trim(current_command);
            if (!current_command.empty()) {
                return;
            }
        }
    }

    std::string commandTokenizer(){
        size_t delimiter_position = current_command.find(' ');
        std::string command_token = (delimiter_position == std::string::npos) ? current_command : current_command.substr(0, delimiter_position);
        return command_token;
    }

    std::string commandType() {
        if (current_command.empty()) {
            return "NULL"; // return NULL for empty commands
        }
        
        std::string command_token = commandTokenizer();

        try {
            return defined_command_types.at(command_token); // using at() is safer (it throws if not found)
        }
        catch(const std::out_of_range& e) {
            std::cerr << "[error] Command type not found for command: " << current_command << "\n";
            throw std::invalid_argument("[error] Unknown command type.");
        }
    }   

    // NOTE: operations on the stack follow the reverse polish notation (RPN) in which operators are written
    //       after the operands. i.e. 2 3 add = 5
    //       this notation is unambiguous and does not require parentheses.
    std::string arg1() { // returns arguments like add, sub, etc
        const char delimiter = ' ';
        size_t delimiter_position = 0;
        if (commandType() == "C_ARITHMETIC") {
            return current_command; // the command itself is the argument
        }
        if (commandType() != "C_RETURN"){ 
            // must return in the case of:
            // >   "push constant 3040"
            // return constant (which is the segment index)                  
            delimiter_position = current_command.find(delimiter);
            std::string segment_index = current_command.substr(delimiter_position + 1, current_command.size());
            // >   "push constant 3040"
            // becomes
            // >   "constant 3040"
            delimiter_position = segment_index.find(delimiter);
            return segment_index.substr(0, delimiter_position);
            // >   "constant 3040"
            // becomes
            // >   "constant"
        }
        throw std::invalid_argument("[error] arg1() should not be called for C_RETURN commands\n");
    }

    int arg2() { // returns the second argument of the command
        if (commands_with_args.find(commandType()) != commands_with_args.end()) {
            size_t delimiter_position = current_command.rfind(' ');
            if (delimiter_position == std::string::npos) {
                throw std::invalid_argument("[error] Invalid command format: no delimiter found.");
            }
            std::string index = current_command.substr(delimiter_position + 1);
            try {
                return std::stoi(index);
            } catch(const std::exception &e) {
                throw std::invalid_argument(std::string("[error] Conversion to int failed: ") + e.what());
            }
        }
        // if the command type does not support a second argument, throw an error.
        throw std::invalid_argument("[error] arg2() should not be called for commands without a second argument.");
    }

    void reset() {
        if (vm_file.is_open()) {
            vm_file.clear();
            vm_file.seekg(0, std::ios::beg);
        }
        current_command.clear();
    }

    void parse() {
        while (hasMoreCommands()) {
            advance();
            if (!current_command.empty()) {
                std::cout << current_command << '\n';
                std::cout << std::string(60, ' ') << '\n';

                std::cout << commandType() << '\n';
                bool command_with_arg = commands_with_args.find(commandType()) != commands_with_args.end();
                std::cout << arg1() << " --- " << (command_with_arg ? std::to_string(arg2()) : "NULL") << '\n';
                std::cout << std::string(60, '-') << '\n';
            }
        }
        reset();
    }
};

class CodeWriter {
private:
    std::string vm_file_name;
    std::ofstream assembly_file;

    // for static variables, since they are of the form @Foo.(index)
    void vmFileNameTrimmer(std::string & file_name) {
        size_t dot_position = file_name.find('.');
        if (dot_position != std::string::npos) {
            file_name = file_name.substr(0, dot_position + 1);
        }
    }

    // helper functions for vm to asm translation
    void local_vm_to_asm(const int & index) {
        assembly_file << "@LCL\n"
                      << "D=M\n"
                      << "@" << index << '\n'
                      << "D=D+A\n"
                      << "A=D\n";
    }

    void argument_vm_to_asm(const int & index) {
        assembly_file << "@ARG\n"
                      << "D=M\n"
                      << "@" << index << '\n'
                      << "D=D+A\n"
                      << "A=D\n";
    }

    void this_vm_to_asm(const int & index) {
        assembly_file << "@THIS\n"
                      << "D=M\n"
                      << "@" << index << '\n'
                      << "D=D+A\n"
                      << "A=D\n";
    }

    void that_vm_to_asm(const int & index) {
        assembly_file << "@THAT\n"
                      << "D=M\n"
                      << "@" << index << '\n'
                      << "D=D+A\n"
                      << "A=D\n";
    }

    void temp_vm_to_asm(const int & index) {
        // TEMP is located on RAM locations 5-12
        if (0 <= index && index < 8) {
            const int location_shift = 5;
            assembly_file << "@" << (index + location_shift) << '\n';
        }
        else { 
            std::cerr << "[error] accessing memory out of range in temp segment.\n";
        }
    }

    void constant_vm_to_asm(const int & index) {
        assembly_file << "@" << index << '\n'
                      << "D=A\n";
    }

    void pointer_vm_to_asm(const int & index) {
        // pointer 0 holds the memory address of THIS in RAM[3]
        // pointer 1 holds the memory address of THAT in RAM[4] 
        // this offset is obviously 3, hence:
        const int location_shift = 3;
        assembly_file << "@" << (index + location_shift) << '\n'
                      << "D=M\n";
    }

    void static_vm_to_asm(const int & index) {
        // static is located on RAM locations 16-255 (239)
        if (0 <= index && index < 239) {
            unsigned int location_shift = 16;
            assembly_file << "@" << vm_file_name << index << '\n'
                          << "D=M\n";
        }
        else {
            std::cerr << "[error] accessing memory out of range in static segment.\n";
        }
    }

    std::unordered_map<std::string, void (CodeWriter::*)(const int &)> memorySegmentator {
        {"constant", &CodeWriter::constant_vm_to_asm},
        {"local",    &CodeWriter::local_vm_to_asm},
        {"argument", &CodeWriter::argument_vm_to_asm},
        {"this",     &CodeWriter::this_vm_to_asm},
        {"that",     &CodeWriter::that_vm_to_asm},
        {"pointer",  &CodeWriter::pointer_vm_to_asm},
        {"temp",     &CodeWriter::temp_vm_to_asm},
        {"static",   &CodeWriter::static_vm_to_asm}
    };

    // arithmetic map
    std::unordered_map<std::string, std::string> arithmetic_dictionary {
        {"add", "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "A=A-1\n"
                "M=D+M\n"},

        {"sub", "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "A=A-1\n"
                "M=M-D\n"},  

        {"neg", "@SP\n"
                "A=M-1\n"
                "M=-M\n"},         

        {"eq", "@SP\n"
               "AM=M-1\n"
               "D=M\n"
               "A=A-1\n"
               "D=M-D\n"
               "M=0\n"
               "@END_EQ\n"
               "D;JNE\n"
               "@SP\n"
               "A=M-1\n"
               "M=-1\n"
               "(END_EQ_"}, // relationalOperationCounter() method adds > "count)\n"

        {"gt", "@SP\n"
               "AM=M-1\n"
               "D=M\n"
               "A=A-1\n"
               "D=M-D\n"
               "M=0\n"
               "@END_GT\n"
               "D;JLE\n"
               "@SP\n"
               "A=M-1\n"
               "M=-1\n"
               "(END_GT_"}, // relationalOperationCounter() method adds > "count)\n"
               
        {"lt", "@SP\n"
               "AM=M-1\n"
               "D=M\n"
               "A=A-1\n"
               "D=M-D\n"
               "M=0\n"
               "@END_LT\n"
               "D;JGE\n"
               "@SP\n"
               "A=M-1\n"
               "M=-1\n"
               "(END_LT_"}, // relationalOperationCounter() method adds > "count)\n"

        {"and", "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "A=A-1\n"
                "M=D&M\n"}, 

        {"or", "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "A=A-1\n"
                "M=D|M\n"},

        {"not", "@SP\n"
                "A=M-1\n"
                "M=!M\n"}        
    };

    unsigned int EQ_COUNTER = 0;
    unsigned int GT_COUNTER = 0;
    unsigned int LT_COUNTER = 0;
    void relationalOperationCounter(Parser & parser){
        std::string command = parser.commandTokenizer();
        if (command == "eq") {
            assembly_file << EQ_COUNTER << ")\n";
            ++EQ_COUNTER;
        }
        if (command == "gt") {
            assembly_file << GT_COUNTER << ")\n";
            ++GT_COUNTER;
        }
        if (command == "lt") {
            assembly_file << LT_COUNTER << ")\n";
            ++LT_COUNTER;        
        }
    }

    void push(const std::string & segment, const int & index){
        // pushes a value from segment[index] into the stack
        try {
            (this->*memorySegmentator[segment])(index); // @<segment + index> 
            assembly_file << "@SP\n"
                          << "A=M\n"
                          << "M=D\n"
                          << "@SP\n"
                          << "M=M+1\n";
        }
        catch(...) {
            std::cerr << "[error] something went wrong while pushing to the stack.";
        }
    }
    
    void pop(const std::string & segment, const int & index){
        // pops a value off the stack into segment[index]
        try {
            assembly_file << "@SP\n"
                          << "AM=M-1\n"
                          << "D=M\n";
            (this->*memorySegmentator[segment])(index); // @<segment + index> 
            assembly_file << "M=D\n";
            
        }
        catch(...) {
            "[error] something went wrong while popping from the stack.";
        }
    }

public:
    // this class reads relevant info from the parser and instantiates respective hack assembly instructions
    CodeWriter(const std::string & vm_file, const std::string & asm_file) {
        try {
            assembly_file.open(asm_file);
            if (!assembly_file.is_open()) {
                throw std::ios_base::failure("[error] unable to open output file.\n");
            }
            vm_file_name = vm_file;
            vmFileNameTrimmer(vm_file_name);
        } 
        catch (const std::ios_base::failure & e) {
            std::cerr << e.what() << '\n';
        }
    }

    ~CodeWriter() {
        if (assembly_file.is_open()) {
            assembly_file.close();
        }
    }

    void writeArithmetic(Parser & parser, const std::string & command) {
        // pop 2 two elements off the stack and add them.
        // note: SP starts at 256
        try
        {
            assembly_file << arithmetic_dictionary[command];
            relationalOperationCounter(parser);
        }
        catch(...)
        {
            std::cerr << "[error] something went wrong while referencing the arithmetic dictionary.";
        }
    }

    void writePushPop (const std::string & command, const std::string & segment, const int & index) {
        if (command == "C_PUSH"){
            push(segment, index);
            return;
        }

        if (command == "C_POP"){
            pop(segment, index);
            return;
        }
    }

    void close() {
        if (assembly_file.is_open()) {
            assembly_file.close();
        }
    }

    void code(Parser & parser){
        while (parser.hasMoreCommands()) {
            parser.advance();
            if (!parser.current_command.empty()) {
                if (parser.commandType() == "C_ARITHMETIC"){
                    try { 
                        writeArithmetic(parser, parser.arg1());
                    }
                    catch(...) {
                        std::cerr << "[error] there was an issue writing the current arithmetic command.";
                    }
                }

                else {
                    try {
                        writePushPop(parser.commandType(), parser.arg1(), parser.arg2());
                    }
                    catch(...) {
                        std::cerr << "[error] there was an issue writing the current command.";
                    }
                }
            }
        }
        close();
    }
};

int main()
{
    std::string vm_file = "VM_test_commands.txt";
    std::string asm_file = "asm_test_commands.txt";

    Parser parser(vm_file);
    CodeWriter coder(vm_file, asm_file);

    parser.parse();
    coder.code(parser);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
