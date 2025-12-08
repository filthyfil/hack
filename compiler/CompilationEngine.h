#pragma once

#include <fstream>
#include <string>
#include <cstddef>
#include <filesystem>

#include "TokenUtils.h"
#include "SymbolTable.h"
#include "VMWriter.h"

class CompilationEngine {
public:
    CompilationEngine(JackTokenizer& jack_tokenizer, bool emit_xml = false);
    ~CompilationEngine();

    // entry point
    void compile();

private:
    bool emit_xml_flag;
    // modules

    // references the tokenizer, whose outputs is a stream of tokens
    // get next token with advance() method
    JackTokenizer& tokenizer;

    // constructs two instances of SymbolTable
    // (1) class variables (static, field)
    // (2) subroutine variables (local, argument)
    SymbolTable class_symbol_table;
    SymbolTable subroutine_symbol_table;

    // track important info 
    std::string class_name;
    KeyWord current_subroutine_keyword;
    std::string current_subroutine_name;

    // writes VM code
    VMWriter vmwriter;

    // output and state
    std::ofstream xml_file;
    size_t indent_level;

    // xml emit helpers
    void emitIndent();
    void emitOpen(const std::string& tag);
    void emitClose(const std::string& tag);
    void emitToken(const std::string& tag, const std::string& token);
    std::string kindToCategory(Kind k);
    enum class IdentifierUsage { iu_DECLARED, iu_USED };
    enum class IdentifierRole  { ir_VARLIKE, // static/field/arg/var
                                 ir_CLASSNAME, ir_SUBROUTINENAME };
    void emitIdentifier(const std::string& name, IdentifierUsage usage, IdentifierRole role);

    // vm writer helpers
    std::string kindToSegment(Kind k);
    void writeOp(char op);
    void writeUnaryOp(char op);
    void pushVar(const std::string& name);
    void popVar(const std::string& name);
    void codeWrite(const std::string& exp);

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