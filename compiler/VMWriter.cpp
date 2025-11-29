// #include <string>

// #include "VMWriter.h"

// // TO-DO: 
// // i would like to change the VM types into enums 
// // instead of strings but VM is written to take string
// class VMWriter {
//     public:
//         VMWriter();
//         ~VMWriter();

//     void writePush(std::string segment, int index) {
//         // writes a VM push command
//     }
//     void writePop(std::string segment, int index) {
//         // writes a VM pop command
//     }
//     void writeArithmetic(std::string command) {
//         // writes a VM arithmetic-logical command
//     }
//     void writeLabel(std::string label) {
//         // writes a VM label command
//     }
//     void writeGoto(std::string lable) {
//         // writes a VM goto command
//     }
//     void writeIf(std::string label) {
//         // writes a VM if-goto command
//     }
//     void writeCall(std::string name, int nArgs) {
//         // writes a VM call command
//     }
//     void writeFunction(std::string name, int nArgs) {
//         // writes a VM function command
//     }
//     void writeReturn() {
//         // writes a VM return
//     }
//     void close() {
//         // closes the output file / stream
//     }
// };
