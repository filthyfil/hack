#pragma once

#include <fstream>
#include <string>
#include <cstddef>

#include "TokenUtils.h"

class CompilationEngine {
public:
    CompilationEngine(JackTokenizer& jack_tokenizer);
    ~CompilationEngine();

    // entry point
    void compile();

private:
    // output and state
    std::ofstream xml_file;
    JackTokenizer& tokenizer;
    size_t indent_level;

    // low level emit helpers
    void writeIndent();
    void writeOpen(const std::string& tag);
    void writeClose(const std::string& tag);
    void writeToken(const std::string& tag, const std::string& token);

    // compilation routines 
    void compileClass();
    void compileClassVarDec();
    void compileSubroutine();
    void compileParameterList();
    void compileSubroutineBody();
    void compileVarDec();
    void compileStatements();
    void compileLet();
    void compileIf();
    void compileWhile();
    void compileDo();
    void compileReturn();
    void compileExpression();
    void compileTerm();
    int compileExpressionList();
};