// hackVM.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <unordered_map>
#include <bitset>
//#include <functional>
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
            std::cerr << "[error] There was a problem with opening the file.\n";
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
        
        // iterate through the dictionary of defined commands
        auto it = defined_command_types.find(current_command);
        if (it != defined_command_types.end()) { // find the key
                return it->second;               // return its respective value
        }
        else {
            std::cerr << "[error] there was an error finding the correct command type.\n";
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
        throw std::invalid_argument("[error] arg1() should not be called for C_RETURN commands");
    }

    std::unordered_set<std::string> defined_command_arguments = {
        "C_PUSH",   
        "C_POP",
        "C_FUNCTION",
        "C_CALL"
    };

    int arg2() { // returns the second argument of the command
        // iterate through the dictionary of defined commands
        if (defined_command_arguments.find(commandType()) != defined_command_arguments.end()) {
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

public:
    // this class reads relevant info from the parser and instantiates respective hack assembly instructions
    CodeWriter(const std::string& file) {
        try {
            std::ofstream assembly_file(file);
        }
        catch (...) {
            std::cerr << "[error] there was a problem creating the output assembly file.\n";
        }
    }

    void writeArithmetic(std::string& command) {
        
    }

    void writePushPop(std::string& command, std::string& segment, int& index) {

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
