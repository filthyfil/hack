#pragma once

#include <string>
#include <unordered_map>

#include <stdexcept>

enum class Kind {
    k_STATIC, k_FIELD,
    k_ARG, k_VAR,
    k_NONE
};

struct SymbolInfo {
    std::string type;
    Kind kind;
    int index;
};

class SymbolTable {
public:
    SymbolTable();
    ~SymbolTable();
private:
    int static_index;
    int field_index;
    int arg_index;
    int var_index;
    std::unordered_map<std::string, SymbolInfo> symbol_table;
public:
    void reset();
        // empties the symbol table, and resets the four indices to 0
        // should be called when starting to compie a subroutine declaration

    void define(std::string name, std::string type, Kind kind);
        // defines (adds to the table) a new variable of the given name, type, and kind 
        // assigns to it the index value of that kind, and adds 1 to the index

    int varCount(Kind kind);
        // returns the number of variables of the given kind already defined
        // in the table

    Kind kindOf(std::string name);
        // returns the kind of the named identifier
        // if the identifier is not found, return NONE

    std::string typeOf(std::string name);
        // reutrns the type of the named variable

    int indexOf(std::string name);
        // returns the index of the named variable
};