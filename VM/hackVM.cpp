// hackVM.cpp : This file contains the 'main' function. Program execution begins and ends there.
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
    // this class unpacks a VM command into its components such that it is accessible and readable by the CodeWriter class
private:
    std::string current_command;
    std::ifstream VM_file;

public:
    Parser(const std::string& file) {
        VM_file.open(file);
        if (!VM_file.is_open()) {
            std::cerr << "[error] There was a problem with opening the input file.\n";
            exit(1);
        }
    }

    ~Parser() {
        if (VM_file.is_open()) {
            VM_file.close();
        }
    }

    bool hasMoreCommands() {
        return (not VM_file.eof());
    }

    void advance() {
        current_command.clear();
        while (std::getline(VM_file, current_command)) {
            auto comment_pos = current_command.find("//");
            if (comment_pos != std::string::npos) {
                current_command = current_command.substr(0, comment_pos);
            }

            current_command.erase(std::remove_if(current_command.begin(), current_command.end(),
                [](unsigned char c) { return std::isspace(c); }),
                current_command.end());

            if (!current_command.empty()) {
                return; 
            }
        }
    }

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

    std::string commandType() {
        if (current_command.empty()) {
            return "NULL"; // return NULL for empty commands
        }
        
        try
        {
            defined_command_types[current_command];
        }
        catch(...)
        {
            std::cerr << "[error] Something went wrong with the defined_command_types mapping.";
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

    std::unordered_set<std::string> commands_with_args = {
        "C_PUSH",   
        "C_POP",
        "C_FUNCTION",
        "C_CALL"
    };

    int arg2() { // returns the second argument of the command
        // iterate through the dictionary of defined commands
        if (commands_with_args.find(commandType()) != commands_with_args.end()) {
            const char delimiter = ' ';
            size_t delimiter_position = current_command.rfind(delimiter);
            std::string argument_index = current_command.substr(delimiter_position + 1, current_command.size());
            return std::stoi(argument_index);
        }
       
        else {
            std::cerr << "[error] there was an error finding the correct command type.\n";
        }
    }
};

class CodeWriter {
private:
    std::ofstream assembly_file;

    // helper functions for vm to asm translation
    void local_vm_to_asm(const int & index) {
        assembly_file << "@LCL\n"
                      << "A=M+" << index << '\n';
    }

    void argument_vm_to_asm(const int & index) {
        assembly_file << "@ARG\n"
                      << "A=M+" << index << '\n';
    }

    void this_vm_to_asm(const int & index) {
        assembly_file << "@THIS\n"
                      << "A=M+" << index << '\n';
    }

    void that_vm_to_asm(const int & index) {
        assembly_file << "@THAT\n"
                      << "A=M+" << index << '\n';
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
        assembly_file << "D=" << index << '\n';
    }

    void pointer_vm_to_asm(const int & index) {
        // pointer 0 holds the memory address of THIS in RAM[3]
        // pointer 1 holds the memory address of THAT in RAM[4] 
        // this offset is obviously 3, hence:
        const int location_shift = 3;
        assembly_file << "@" << (index + location_shift) << '\n';
    }

    void static_vm_to_asm(const int & index) {
        // static is located on RAM locations 16-255 (239)
        if (0 <= index && index < 239) {
            unsigned int location_shift = 16;
            assembly_file << "@" << (location_shift + index) << '\n';
        }
        else{
            std::cerr << "[error] accessing memory out of range in static segment.\n";
        }
    }

    using vm_to_asm = void (CodeWriter::*)(const int &);
    std::unordered_map<std::string, vm_to_asm> memorySegmentator {
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
                "M=M+D\n"},

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
               "(END_EQ)\n"}, 

        {"GT", "@SP\n"
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
               "(END_GT)\n"}, 
               
        {"LT", "@SP\n"
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
               "(END_LT)\n"},  

        {"and", "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "A=A-1\n"
                "M=M&D\n"}, 

        {"or", "@SP\n"
                "AM=M-1\n"
                "D=M\n"
                "A=A-1\n"
                "M=M|D\n"},

        {"not", "@SP\n"
                "A=M-1\n"
                "M=!M\n"}        
    };

public:
    void push(const std::string & segment, const int & index){
        // pushes a value from segment[index] into the stack
        try {
            (this->*memorySegmentator[segment])(index); // @<segment + index> 
            if ((segment == "constant") || (segment == "static") || (segment == "pointer")) {
                assembly_file << "D=M\n"
                            << "@SP\n"
                            << "AM=M+1\n"
                            << "M=D\n";
            }
        }
        catch(...) {
            std::cerr << "[error] something went wrong while pushing to the stack.";
        }
        // all these just access respective memory segment for now:
        // <local, argument, this, that> 
        /* 
           > @BASE_VALUE + INDEX (this already in the .asm program via the translation functions above ^-^)
        */

        // just an accessor for now:
        // <temp>
        /*
          -> @MEM_VALUE (this already in the .asm program via the translation functions above ^-^)
           > 
        */
    }
    
    void pop(const std::string & segment, const int & index){
        // pops a value off the stack into segment[index]
        try {
            if ((segment == "constant") || (segment == "static") || (segment == "pointer")) {
                assembly_file << "@SP\n"
                            << "@D=M\n";
                (this->*memorySegmentator[segment])(index); // @<segment + index> 
                assembly_file << "M=D\n"
                            << "@SP\n"
                            << "M=M-1\n";
            }
        }
        catch(...) {
            "[error] something went wrong while popping from the stack.";
        }
    }

public:
    // this class reads relevant info from the parser and instantiates respective hack assembly instructions
    CodeWriter(const std::string & file) {
        try {
            assembly_file.open(file);
            if (!assembly_file.is_open()) {
                throw std::ios_base::failure("[error] Unable to open output file.\n");
            }
        } 
        catch (const std::ios_base::failure & e) {
            std::cerr << e.what() << '\n';
        }
    }

    void writeArithmetic(const std::string& command) {
        // pop 2 two elements off the stack and add them.
        // note: SP starts at 256
        try
        {
            arithmetic_dictionary[command];
        }
        catch(...)
        {
            std::cerr << "[error] Something went wrong with the arithmetic dictionary.";
        }
    }

    void writePushPop (const std::string & command, const std::string & segment, const int & index) {
        if (command == "push"){
            push(segment, index);
            return;
        }

        if (command == "pop"){
            pop(segment, index);
            return;
        }
    }

    // more methods will be implemented

    void close() {

    }
};


int main()
{
    const std::string input_file = "VM_test_commands.txt";
    std::cout << "Hello World!\n";
    Parser parser(input_file);
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
