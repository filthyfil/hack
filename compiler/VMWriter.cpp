#include "VMWriter.h"

// // TO-DO: 
// // i would like to change the VM types into enums 
// // instead of strings but VM is written to take string

VMWriter::VMWriter(const std::string& vm_file_name) {
    vm_file.open(vm_file_name);
    if (!vm_file.is_open()) {
        std::cerr << "[error] cannot create VM file: " << vm_file_name << " \n";
        std::exit(1);
    }
}
VMWriter::~VMWriter() {
    if (vm_file.is_open()) 
        vm_file.close();
}

void VMWriter::writePush(const std::string& segment, int index) {
    vm_file << "push " << segment << " " << index << '\n';
}

void VMWriter::writePop(const std::string& segment, int index) {
    vm_file << "pop " << segment << " " << index << '\n';
}

void VMWriter::writeArithmetic(const std::string& command) {
    static const std::unordered_set<std::string> arithmetic {
        "add", "sub", "neg",
        "eq",  "gt",  "lt",
        "and", "or",  "not"
    };
    if (arithmetic.find(command) == arithmetic.end()) 
        throw std::runtime_error("VMWriter::writeArithmetic: invalid command '" + command + "'");
    vm_file << command << '\n';
}

void VMWriter::writeLabel(const std::string& label) {
    vm_file << "label " << label << '\n';
}

void VMWriter::writeGoto(const std::string& label) {
    vm_file << "goto " << label << '\n';
}

void VMWriter::writeIf(const std::string& label) {
    vm_file << "if-goto " << label << '\n';
}

void VMWriter::writeCall(const std::string& name, int nArgs) {
    vm_file << "call " << name << " " << nArgs << '\n';
}

void VMWriter::writeFunction(const std::string& name, int nVars) {
    vm_file << "function " << name << " " << nVars << '\n';
}

void VMWriter::writeReturn() {
    vm_file << "return\n";
}

void VMWriter::close() {
    if (vm_file.is_open()) 
        vm_file.close();
}