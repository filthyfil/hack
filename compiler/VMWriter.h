#pragma once

#include <string>
#include <fstream>
#include <iostream>
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
    std::fstream vm_file;
public:
    VMWriter(const std::string& vm_file_name);
    
    ~VMWriter();

    // writes a VM push command
    void writePush(const std::string& segment, int index);

    // writes a VM pop command
    void writePop(const std::string& segment, int index);

    // writes a VM arithmetic-logical command
    void writeArithmetic(const std::string& command);

    // writes a VM label command
    void writeLabel(const std::string& label);

    // writes a VM goto command
    void writeGoto(const std::string& lable);

    // writes a VM if-goto command
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