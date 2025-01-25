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
public:
    std::ifstream VM_file;
    std::string current_command;

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
    std::unordered_set<std::string> defined_arithmetic_commands = {
        "add", // addition
        "sub", // subtraction
        "neg", // negative
        "eq",  // equals
        "gt",  // greater than
        "lt",  // less than
        "and", // AND operator
        "or",  // OR operator
        "not"  // NOT operator
    };

    std::string commandType() {
        if (current_command.empty()) {
            return "NULL"; // return NULL for empty commands
        }

        if (defined_arithmetic_commands.find(current_command) != defined_arithmetic_commands.end()){
            return "C_ARITHMETIC"; // return C_ARITHMETIC if iterator doesnt point to end() of set
        }
    }

    // NOTE: operations on the stack follow the reverse polish notation (RPN) in which operators are written
    //       after the operands. i.e. 2 3 add = 5
    //       This notation is unambiguous and does not require parentheses.
    std::string arg1() { // returns arguments like add, sub, etc on the stack

    }

    int arg2() { // returns arguments of the operands on the stack

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
            std::cerr << "(error) There was a problem creating the output assembly file.\n";
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
