#pragma once

#include <fstream>
#include <string>
#include <cstddef>

#include "TokenUtils.h"
#include "SymbolTable.h"

class CompilationEngine {
public:
    CompilationEngine(JackTokenizer& jack_tokenizer);
    ~CompilationEngine();

    // entry point
    void compile();

private:
    // modules

    // references the tokenizer, whose outputs is a stream of tokens
    // get next token with advance() method
    JackTokenizer& tokenizer;

    // constructs two instances of SymbolTable
    // (1) class variables (static, field)
    // (2) subroutine variables (local, argument)
    SymbolTable class_symbol_table;
    SymbolTable subroutine_symbol_table;

    std::string class_name;

    // output and state
    std::ofstream xml_file;
    size_t indent_level;

    // xml emit helpers
    void writeIndent();
    void writeOpen(const std::string& tag);
    void writeClose(const std::string& tag);
    void writeToken(const std::string& tag, const std::string& token);
    std::string kindToCategory(Kind k);
    enum class IdentifierUsage { iu_DECLARED, iu_USED };
    enum class IdentifierRole  { ir_VARLIKE, // static/field/arg/var
                                ir_CLASSNAME, ir_SUBROUTINENAME };
    void writeIdentifier(const std::string& name, IdentifierUsage usage, IdentifierRole role);

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