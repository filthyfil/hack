#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <unordered_set>
#include <cstdlib>
#include <stdexcept>

// // refer to TO-DO in .cpp
// // enum class Segment {
// //     s_CONSTANT,
// //     s_ARGUMENT,
// //     s_LOCAL,
// //     s_STATIC,
// //     s_THIS,
// //     s_THAT,
// //     s_POINTER,
// //     s_TEMP
// // };

class VMWriter {
    std::ofstream vm_file;
    unsigned int label_count;
    
public:
    VMWriter(std::filesystem::path vm_file_path);
    
    ~VMWriter();

    // writes a VM push command
    void writePush(const std::string& segment, int index);

    // writes a VM pop command
    void writePop(const std::string& segment, int index);

    // writes a VM arithmetic-logical command
    void writeArithmetic(const std::string& command);

    // writes a VM label command
    void writeLabel(const std::string& label);

    // get a unique label from the VM writer
    std::string getLabel();

    // writes a VM goto command, unconditiional
    void writeGoto(const std::string& label);

    // writes a VM if-goto command, if top is non-zero, control jumps to the given label
    void writeIf(const std::string& label);

    // writes a VM call command
    void writeCall(const std::string& name, int nArgs);

    // writes a VM function command
    void writeFunction(const std::string& name, int nArgs);

    // writes a VM return
    void writeReturn();

    // closes the output file / stream
    void close();
    
};