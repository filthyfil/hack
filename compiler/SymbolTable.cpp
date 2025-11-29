#include <string>

#include "SymbolTable.h"

class SymbolTable {
    public:
        SymbolTable();
        ~SymbolTable();
    void reset() {}
    void define(std::string name, std::string type, Kind kind) {}
    int varCount(std::string name) {}
    Kind kindOf(std::string name) {}
    // should this use the enum??? V
    std::string typeOf(std::string name) {}
    int indexOf(std::string name) {}
};