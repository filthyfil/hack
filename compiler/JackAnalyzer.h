#pragma once

class JackAnalyzer {
public:
    JackAnalyzer();
    ~JackAnalyzer();

    void compile();
    // compiles the xxx.jack program
private:
    void constructSymbolTable();
    // constructs two instances of SymbolTable
    // (1) class variables (static, field)
    // (2) subroutine variables (local, argument)
};