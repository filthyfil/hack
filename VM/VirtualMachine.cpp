// VirtualMachine.cpp
// Translates Hack VM files to Hack Assembly code.
// Handles both single .vm files and directories containing multiple .vm files.

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

class Parser {
private:
    std::unordered_set<std::string> commands_with_args {
        "C_PUSH", "C_POP", "C_FUNCTION", "C_CALL"
    };

    std::unordered_map<std::string, std::string> defined_command_types {
        {"add", "C_ARITHMETIC"},  {"sub", "C_ARITHMETIC"},  {"neg", "C_ARITHMETIC"},
        {"eq", "C_ARITHMETIC"},   {"gt", "C_ARITHMETIC"},   {"lt", "C_ARITHMETIC"},
        {"and", "C_ARITHMETIC"},  {"or", "C_ARITHMETIC"},   {"not", "C_ARITHMETIC"},
        {"push", "C_PUSH"},       {"pop", "C_POP"},         {"label", "C_LABEL"},
        {"goto", "C_GOTO"},       {"if-goto", "C_IF"},      {"function", "C_FUNCTION"},
        {"call", "C_CALL"},       {"return", "C_RETURN"}
    };

    static std::string trim(const std::string &s) {
        auto start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        auto end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    }

public:
    std::string current_command;
    std::ifstream vm_file;

    explicit Parser(const std::string &file) {
        vm_file.open(file);
        if (!vm_file.is_open()) {
            throw std::runtime_error("[error] unable to open input VM file: " + file);
        }
    }

    ~Parser() {
        if (vm_file.is_open())
            vm_file.close();
    }

    bool hasMoreCommands() {
        return vm_file.peek() != EOF;
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
        current_command.clear();
    }

    std::string commandTokenizer() {
        auto pos = current_command.find(' ');
        return (pos == std::string::npos) ? current_command
                                          : current_command.substr(0, pos);
    }

    std::string commandType() {
        if (current_command.empty()) return "NULL";
        try {
            return defined_command_types.at(commandTokenizer());
        } catch (const std::out_of_range&) {
            std::cerr << "Unknown command: " << commandTokenizer() << std::endl;
            return "NULL";
        }
    }

    std::string arg1() {
        std::string type = commandType();
        if (type == "C_ARITHMETIC") {
            return commandTokenizer();
        } else if (type != "C_RETURN") {
            std::istringstream iss(current_command);
            std::string cmd, arg;
            iss >> cmd >> arg;
            if (!iss || arg.empty()) {
                throw std::runtime_error("arg1(): failed to parse command: " + current_command);
            }
            return arg;
        }
        throw std::invalid_argument("arg1() called on return or invalid command");
    }

    int arg2() {
        if (commands_with_args.count(commandType())) {
            std::istringstream iss(current_command);
            std::string cmd, arg1;
            int arg2;
            iss >> cmd >> arg1 >> arg2;
            if (!iss) {
                throw std::runtime_error("arg2(): failed to parse command: " + current_command);
            }
            return arg2;
        }
        throw std::invalid_argument("arg2() called on command without arg");
    }
};

class CodeWriter {
    std::string file_name_base; // Stores base name like "Sys" for static variables
    std::string current_function_name; // Stores current function for labels
    std::ofstream out;
    unsigned int label_counter = 0;

    void push(const std::string &segment, int index) {
        // #region agent log
        static int push_count = 0;
        push_count++;
        std::ofstream log("/home/filthyfil/Code/hack/.cursor/debug.log", std::ios::app);
        if (log.is_open() && push_count % 100 == 0) {
            log << "{\"sessionId\":\"debug-session\",\"runId\":\"run1\",\"hypothesisId\":\"B\",\"location\":\"VirtualMachine.cpp:123\",\"message\":\"push operation\",\"data\":{\"segment\":\"" << segment << "\",\"index\":" << index << ",\"total_pushes\":\"" << push_count << "\"},\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "}\n";
            log.close();
        }
        // #endregion
        static const std::unordered_map<std::string, std::string> base {
            {"local", "LCL"}, {"argument", "ARG"}, {"this", "THIS"}, {"that", "THAT"}
        };

        if (segment == "constant") {
            out << "@" << index << "\n"
                << "D=A\n";
        } else if (segment == "temp") {
            out << "@" << 5 + index << "\n"
                << "D=M\n";
        } else if (segment == "pointer") {
            out << "@" << 3 + index << "\n"
                << "D=M\n";
        } else if (segment == "static") {
            out << "@" << file_name_base << "." << index << "\n"
                << "D=M\n";
        } else {
            out << "@" << base.at(segment) << "\n"
                << "D=M\n"
                << "@" << index << "\n"
                << "A=D+A\n"
                << "D=M\n";
        }

        out << "@SP\n"
            << "A=M\n"
            << "M=D\n"
            << "@SP\n"
            << "M=M+1\n";
    }

    void pop(const std::string &segment, int index) {
        static const std::unordered_map<std::string, std::string> base {
            {"local", "LCL"}, {"argument", "ARG"}, {"this", "THIS"}, {"that", "THAT"}
        };

        if (segment == "temp" || segment == "pointer" || segment == "static") {
            std::string symbol = (segment == "temp")    ? std::to_string(5 + index) :
                                 (segment == "pointer") ? std::to_string(3 + index) :
                                                           (file_name_base + "." + std::to_string(index));
            out << "@SP\n"
                << "AM=M-1\n"
                << "D=M\n"
                << "@" << symbol << "\n"
                << "M=D\n";
        } else {
            out << "@" << base.at(segment) << "\n"
                << "D=M\n"
                << "@" << index << "\n"
                << "D=D+A\n"
                << "@R13\n"
                << "M=D\n"
                << "@SP\n"
                << "AM=M-1\n"
                << "D=M\n"
                << "@R13\n"
                << "A=M\n"
                << "M=D\n";
        }
    }

public:
    explicit CodeWriter(const std::string &asm_file) {
        out.open(asm_file);
        if (!out.is_open()) {
            throw std::runtime_error("[error] unable to create output asm file");
        }
    }

    ~CodeWriter() {
        if (out.is_open())
            out.close();
    }

    void setFileName(const std::string& vm_filepath) {
        namespace fs = std::filesystem;
        fs::path p(vm_filepath);
        this->file_name_base = p.stem().string();
    }

    void writeInit() {
        out << "// Bootstrap Code\n"
            << "@256\n"
            << "D=A\n"
            << "@SP\n"
            << "M=D\n";
        writeCall("Sys.init", 0);
    }

    void writeArithmetic(const std::string &cmd) {
        if (cmd == "add") {
            out << "@SP\n"
                << "AM=M-1\n"
                << "D=M\n"
                << "A=A-1\n"
                << "M=D+M\n";
        } else if (cmd == "sub") {
            out << "@SP\n"
                << "AM=M-1\n"
                << "D=M\n"
                << "A=A-1\n"
                << "M=M-D\n";
        } else if (cmd == "neg") {
            out << "@SP\n"
                << "A=M-1\n"
                << "M=-M\n";
        } else if (cmd == "and") {
            out << "@SP\n"
                << "AM=M-1\n"
                << "D=M\n"
                << "A=A-1\n"
                << "M=D&M\n";
        } else if (cmd == "or") {
            out << "@SP\n"
                << "AM=M-1\n"
                << "D=M\n"
                << "A=A-1\n"
                << "M=D|M\n";
        } else if (cmd == "not") {
            out << "@SP\n"
                << "A=M-1\n"
                << "M=!M\n";
        } else if (cmd == "eq" || cmd == "gt" || cmd == "lt") {
            std::string jmp = (cmd == "eq") ? "JEQ" : (cmd == "gt" ? "JGT" : "JLT");
            std::string label_true = "BOOL_TRUE_" + std::to_string(label_counter);
            std::string label_end  = "BOOL_END_"  + std::to_string(label_counter);
            label_counter++;

            out << "@SP\n"
                << "AM=M-1\n"
                << "D=M\n"
                << "A=A-1\n"
                << "D=M-D\n"
                << "@" << label_true << "\n"
                << "D;" << jmp << "\n"
                << "@SP\n"
                << "A=M-1\n"
                << "M=0\n" // false
                << "@" << label_end << "\n"
                << "0;JMP\n"
                << "(" << label_true << ")\n"
                << "@SP\n"
                << "A=M-1\n"
                << "M=-1\n" // true
                << "(" << label_end << ")\n";
        }
    }

    void writePushPop(const std::string &type, const std::string &seg, int idx) {
        out << "// " << (type == "C_PUSH" ? "push" : "pop") << " " << seg << " " << idx << "\n";
        if (type == "C_PUSH") {
            push(seg, idx);
        } else if (type == "C_POP") {
            pop(seg, idx);
        }
    }

    void writeLabel(const std::string &label) {
        out << "(" << current_function_name << "$" << label << ")\n";
    }

    void writeGoto(const std::string &label) {
        out << "@" << current_function_name << "$" << label << "\n"
            << "0;JMP\n";
    }

    void writeIf(const std::string &label) {
        out << "@SP\n"
            << "AM=M-1\n"
            << "D=M\n"
            << "@" << current_function_name << "$" << label << "\n"
            << "D;JNE\n";
    }

    void writeFunction(const std::string &name, int nLocals) {
        current_function_name = name;
        out << "(" << name << ")\n";
        for (int i = 0; i < nLocals; ++i) {
            push("constant", 0);
        }
    }

    void writeCall(const std::string &name, int nArgs) {
        std::string ret_label = name + "$ret." + std::to_string(label_counter++);
        out << "// call " << name << " " << nArgs << "\n";

        // push return-address
        out << "@" << ret_label << "\n"
            << "D=A\n"
            << "@SP\n"
            << "A=M\n"
            << "M=D\n"
            << "@SP\n"
            << "M=M+1\n";

        // push LCL, ARG, THIS, THAT
        for (const char* seg : {"LCL", "ARG", "THIS", "THAT"}) {
            out << "@" << seg << "\n"
                << "D=M\n"
                << "@SP\n"
                << "A=M\n"
                << "M=D\n"
                << "@SP\n"
                << "M=M+1\n";
        }

        // ARG = SP - nArgs - 5
        out << "@SP\n"
            << "D=M\n"
            << "@" << (nArgs + 5) << "\n"
            << "D=D-A\n"
            << "@ARG\n"
            << "M=D\n";

        // LCL = SP
        out << "@SP\n"
            << "D=M\n"
            << "@LCL\n"
            << "M=D\n";

        // goto function
        out << "@" << name << "\n"
            << "0;JMP\n"
            << "(" << ret_label << ")\n";
    }

    void writeReturn() {
        out << "// return\n";
        // FRAME = LCL (R13)
        out << "@LCL\n"
            << "D=M\n"
            << "@R13\n"
            << "M=D\n";

        // RET = *(FRAME-5) (R14)
        out << "@5\n"
            << "A=D-A\n"
            << "D=M\n"
            << "@R14\n"
            << "M=D\n";

        // *ARG = pop()
        out << "@SP\n"
            << "AM=M-1\n"
            << "D=M\n"
            << "@ARG\n"
            << "A=M\n"
            << "M=D\n";

        // SP = ARG+1
        out << "@ARG\n"
            << "D=M+1\n"
            << "@SP\n"
            << "M=D\n";

        // Restore THAT, THIS, ARG, LCL
        for (const char* seg : {"THAT", "THIS", "ARG", "LCL"}) {
            out << "@R13\n"
                << "AM=M-1\n"
                << "D=M\n"
                << "@" << seg << "\n"
                << "M=D\n";
        }

        // goto RET
        out << "@R14\n"
            << "A=M\n"
            << "0;JMP\n";
    }

    void close() {
        if (out.is_open())
            out.close();
    }

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
    }
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file.vm | input_directory>\n";
        return 1;
    }

    namespace fs = std::filesystem;
    fs::path input_path(argv[1]);
    fs::path output_path;
    std::vector<fs::path> vm_files;

    if (fs::is_directory(input_path)) {
        output_path = input_path / (input_path.filename().string() + ".asm");
        for (const auto &entry : fs::directory_iterator(input_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".vm") {
                vm_files.push_back(entry.path());
            }
        }
    } else if (fs::is_regular_file(input_path) && input_path.extension() == ".vm") {
        output_path = input_path;
        output_path.replace_extension(".asm");
        vm_files.push_back(input_path);
    } else {
        std::cerr << "[Error] Input must be a .vm file or a directory.\n";
        return 1;
    }

    if (vm_files.empty()) {
        std::cerr << "[Error] No .vm files found to translate.\n";
        return 1;
    }

    // Optional: sort for deterministic processing order
    std::sort(vm_files.begin(), vm_files.end());

    // Per spec: if translating more than one .vm file, emit bootstrap.
    bool write_bootstrap = (vm_files.size() > 1);

    try {
        CodeWriter writer(output_path.string());

        if (write_bootstrap) {
            writer.writeInit();
        }

        for (const auto &vm_file : vm_files) {
            std::cout << "Translating: " << vm_file.string() << std::endl;
            writer.setFileName(vm_file.string());
            Parser parser(vm_file.string());
            writer.code(parser);
        }

        writer.close();
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    std::cout << "Translation successful. Output written to: " << output_path.string() << std::endl;
    return 0;
}
