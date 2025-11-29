#include "SymbolTable.h"

SymbolTable::SymbolTable(const std::string& scope) : 
    scope{scope},
    static_index{0},
    field_index{0},
    arg_index{0},
    var_index{0} {
}

SymbolTable::~SymbolTable() = default;

void SymbolTable::reset() {
    symbol_table.clear();
    static_index = 0;
    field_index = 0;
    arg_index = 0;
    var_index = 0;
}

void SymbolTable::define(std::string name, std::string type, Kind kind){
    int index = varCount(kind);
    SymbolInfo symbol{type, kind, index};
    symbol_table[name] = symbol;

    switch (kind) {
        case Kind::k_STATIC: ++static_index; break;
        case Kind::k_FIELD: ++field_index; break;
        case Kind::k_ARG: ++arg_index; break;
        case Kind::k_VAR: ++var_index; break;
        default: break;
    }
}

int SymbolTable::varCount(Kind kind){
    switch (kind) {
        case Kind::k_STATIC: return static_index; break;
        case Kind::k_FIELD: return field_index; break;
        case Kind::k_ARG: return arg_index; break;
        case Kind::k_VAR: return var_index; break;
        default: break;
    }
    throw std::runtime_error("[error] invalid kind for method varCount()\n");
}

Kind SymbolTable::kindOf(std::string name){
    if (auto it = symbol_table.find(name); it != symbol_table.end()) {
        return it->second.kind;
    }
    else return Kind::k_NONE; 
}

std::string SymbolTable::typeOf(std::string name){
    if (auto it = symbol_table.find(name); it != symbol_table.end()) {
        return it->second.type;
    }
    else throw std::runtime_error("[error] symbol has no type: " + name + '\n');
}

int SymbolTable::indexOf(std::string name){
    if (auto it = symbol_table.find(name); it != symbol_table.end()) {
        return it->second.index;
    }
    else throw std::runtime_error("[error] symbol has no index: " + name + '\n');
}