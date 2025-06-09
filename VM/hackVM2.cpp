// hackVM2.cpp : Beginning implementation for Chapter 8 part 2 of the VM.
// This file extends the part 1 translator with program flow and function commands.

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Parser {
private:
    std::unordered_set<std::string> commands_with_args {
        "C_PUSH", "C_POP", "C_FUNCTION", "C_CALL"
    };
    std::unordered_map<std::string, std::string> defined_command_types {
        {"add", "C_ARITHMETIC"}, {"sub", "C_ARITHMETIC"}, {"neg", "C_ARITHMETIC"},
        {"eq", "C_ARITHMETIC"},  {"gt", "C_ARITHMETIC"},  {"lt", "C_ARITHMETIC"},
        {"and", "C_ARITHMETIC"}, {"or", "C_ARITHMETIC"},  {"not", "C_ARITHMETIC"},
        {"push", "C_PUSH"},       {"pop", "C_POP"},         {"label", "C_LABEL"},
        {"goto", "C_GOTO"},       {"if-goto", "C_IF"},      {"function", "C_FUNCTION"},
        {"call", "C_CALL"},       {"return", "C_RETURN"}
    };

public:
    std::string current_command;
    std::ifstream vm_file;

    explicit Parser(const std::string &file) {
        vm_file.open(file);
        if (!vm_file.is_open()) {
            throw std::runtime_error("[error] unable to open input VM file");
        }
    }

    bool hasMoreCommands() { return !vm_file.eof(); }

    static std::string trim(const std::string &s) {
        auto start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        auto end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    }

    void advance() {
        current_command.clear();
        while (std::getline(vm_file, current_command)) {
            auto pos = current_command.find("//");
            if (pos != std::string::npos)
                current_command = current_command.substr(0, pos);
            current_command = trim(current_command);
            if (!current_command.empty()) return;
        }
    }

    std::string commandTokenizer() {
        auto pos = current_command.find(' ');
        return (pos == std::string::npos) ? current_command
                                          : current_command.substr(0, pos);
    }

    std::string commandType() {
        if (current_command.empty()) return "NULL";
        return defined_command_types.at(commandTokenizer());
    }

    std::string arg1() {
        if (commandType() == "C_ARITHMETIC") return commandTokenizer();
        if (commandType() != "C_RETURN") {
            auto pos = current_command.find(' ');
            std::string rest = current_command.substr(pos + 1);
            pos = rest.find(' ');
            return rest.substr(0, pos);
        }
        throw std::invalid_argument("arg1() called on return command");
    }

    int arg2() {
        if (commands_with_args.count(commandType())) {
            auto pos = current_command.rfind(' ');
            return std::stoi(current_command.substr(pos + 1));
        }
        throw std::invalid_argument("arg2() called on command without arg");
    }
};

class CodeWriter {
    std::string file_name;
    std::ofstream out;
    unsigned int label_counter = 0;

    void fileNameTrim(std::string &name) {
        auto pos = name.find('.');
        if (pos != std::string::npos) name = name.substr(0, pos + 1);
    }

    void push(const std::string &segment, int index) {
        static const std::unordered_map<std::string, std::string> base {
            {"local", "LCL"}, {"argument", "ARG"}, {"this", "THIS"}, {"that", "THAT"}
        };
        if (segment == "constant") {
            out << "@" << index << "\nD=A\n";
        } else if (segment == "temp") {
            out << "@" << 5 + index << "\nD=M\n";
        } else if (segment == "pointer") {
            out << "@" << 3 + index << "\nD=M\n";
        } else if (segment == "static") {
            out << "@" << file_name << index << "\nD=M\n";
        } else {
            out << "@" << base.at(segment) << "\nD=M\n@" << index << "\nA=D+A\nD=M\n";
        }
        out << "@SP\nA=M\nM=D\n@SP\nM=M+1\n";
    }

    void pop(const std::string &segment, int index) {
        static const std::unordered_map<std::string, std::string> base {
            {"local", "LCL"}, {"argument", "ARG"}, {"this", "THIS"}, {"that", "THAT"}
        };
        if (segment == "temp") {
            out << "@SP\nAM=M-1\nD=M\n@" << 5 + index << "\nM=D\n";
        } else if (segment == "pointer") {
            out << "@SP\nAM=M-1\nD=M\n@" << 3 + index << "\nM=D\n";
        } else if (segment == "static") {
            out << "@SP\nAM=M-1\nD=M\n@" << file_name << index << "\nM=D\n";
        } else {
            out << "@" << base.at(segment) << "\nD=M\n@" << index
                << "\nD=D+A\n@R13\nM=D\n@SP\nAM=M-1\nD=M\n@R13\nA=M\nM=D\n";
        }
    }

public:
    CodeWriter(const std::string &vm, const std::string &asm_file) {
        out.open(asm_file);
        file_name = vm;
        fileNameTrim(file_name);
    }
    ~CodeWriter() { if (out.is_open()) out.close(); }

    void writeArithmetic(const std::string &cmd) {
        static const std::unordered_map<std::string, std::string> ops {
            {"add", "@SP\nAM=M-1\nD=M\nA=A-1\nM=D+M\n"},
            {"sub", "@SP\nAM=M-1\nD=M\nA=A-1\nM=M-D\n"},
            {"neg", "@SP\nA=M-1\nM=-M\n"},
            {"and", "@SP\nAM=M-1\nD=M\nA=A-1\nM=D&M\n"},
            {"or",  "@SP\nAM=M-1\nD=M\nA=A-1\nM=D|M\n"},
            {"not", "@SP\nA=M-1\nM=!M\n"}
        };
        if (cmd == "eq" || cmd == "gt" || cmd == "lt") {
            std::string jmp = (cmd == "eq") ? "JEQ" : (cmd == "gt" ? "JGT" : "JLT");
            out << "@SP\nAM=M-1\nD=M\nA=A-1\nD=M-D\n@BOOL_" << label_counter
                << "\nD;" << jmp << "\n@SP\nA=M-1\nM=0\n@END_" << label_counter
                << "\n0;JMP\n(BOOL_" << label_counter << ")\n@SP\nA=M-1\nM=-1\n(END_"
                << label_counter << ")\n";
            ++label_counter;
        } else {
            out << ops.at(cmd);
        }
    }

    void writePushPop(const std::string &type, const std::string &seg, int idx) {
        if (type == "C_PUSH") push(seg, idx);
        else if (type == "C_POP") pop(seg, idx);
    }

    void writeLabel(const std::string &label) { out << "(" << label << ")\n"; }

    void writeGoto(const std::string &label) {
        out << "@" << label << "\n0;JMP\n";
    }

    void writeIf(const std::string &label) {
        out << "@SP\nAM=M-1\nD=M\n@" << label << "\nD;JNE\n";
    }

    void writeFunction(const std::string &name, int nLocals) {
        out << "(" << name << ")\n";
        for (int i = 0; i < nLocals; ++i) {
            out << "@0\nD=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n";
        }
    }

    void writeCall(const std::string &name, int nArgs) {
        std::string ret = "RET_" + std::to_string(label_counter++);
        out << "@" << ret << "\nD=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"; // push return
        out << "@LCL\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n";           // push LCL
        out << "@ARG\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n";           // push ARG
        out << "@THIS\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n";         // push THIS
        out << "@THAT\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n";         // push THAT
        out << "@SP\nD=M\n@" << nArgs + 5 << "\nD=D-A\n@ARG\nM=D\n";  // ARG=SP-n-5
        out << "@SP\nD=M\n@LCL\nM=D\n";                               // LCL=SP
        out << "@" << name << "\n0;JMP\n(" << ret << ")\n";             // goto function
    }

    void writeReturn() {
        out << "@LCL\nD=M\n@R13\nM=D\n";               // FRAME = LCL
        out << "@5\nA=D-A\nD=M\n@R14\nM=D\n";         // RET = *(FRAME-5)
        out << "@SP\nAM=M-1\nD=M\n@ARG\nA=M\nM=D\n";   // *ARG = pop()
        out << "@ARG\nD=M+1\n@SP\nM=D\n";               // SP = ARG+1
        out << "@R13\nAM=M-1\nD=M\n@THAT\nM=D\n";       // THAT = *(FRAME-1)
        out << "@R13\nAM=M-1\nD=M\n@THIS\nM=D\n";       // THIS = *(FRAME-2)
        out << "@R13\nAM=M-1\nD=M\n@ARG\nM=D\n";        // ARG = *(FRAME-3)
        out << "@R13\nAM=M-1\nD=M\n@LCL\nM=D\n";        // LCL = *(FRAME-4)
        out << "@R14\nA=M\n0;JMP\n";                     // goto RET
    }

    void close() { if (out.is_open()) out.close(); }

    void code(Parser &parser) {
        while (parser.hasMoreCommands()) {
            parser.advance();
            if (parser.current_command.empty()) continue;
            std::string type = parser.commandType();
            if (type == "C_ARITHMETIC") {
                writeArithmetic(parser.arg1());
            } else if (type == "C_PUSH" || type == "C_POP") {
                writePushPop(type, parser.arg1(), parser.arg2());
            } else if (type == "C_LABEL") {
                writeLabel(parser.arg1());
            } else if (type == "C_GOTO") {
                writeGoto(parser.arg1());
            } else if (type == "C_IF") {
                writeIf(parser.arg1());
            } else if (type == "C_FUNCTION") {
                writeFunction(parser.arg1(), parser.arg2());
            } else if (type == "C_CALL") {
                writeCall(parser.arg1(), parser.arg2());
            } else if (type == "C_RETURN") {
                writeReturn();
            }
        }
        close();
    }
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>\n";
        return 1;
    }

    try {
        Parser parser(argv[1]);
        CodeWriter writer(argv[1], argv[2]);
        writer.code(parser);
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}

