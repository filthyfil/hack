#include "CompilationEngine.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>

CompilationEngine::CompilationEngine(JackTokenizer& jack_tokenizer, const std::string& base) :
    tokenizer(jack_tokenizer), 
    indent_level{0},
    class_symbol_table{},
    subroutine_symbol_table{},
    class_name{},
    vmwriter(base + ".vm") {
    std::string xml_file_name = base + ".xml";
    // open output file
    xml_file.open(xml_file_name);
    if (!xml_file.is_open()) {
        std::cerr << "[error] cannot create XML file: " << xml_file_name << " \n";
        std::exit(1);
    }
}

CompilationEngine::~CompilationEngine() {
    if (xml_file.is_open())
        xml_file.close();
}

void CompilationEngine::compile() {
    compileClass(); // compileClass(); calls all other compilation processes
}

inline void CompilationEngine::writeIndent() {
    for (size_t i = 0; i < indent_level; ++i)
        xml_file << '\t';
}

inline void CompilationEngine::writeOpen(const std::string& tag) {
    writeIndent();
    xml_file << '<' << tag << '>' << '\n';
    ++indent_level;
}

inline void CompilationEngine::writeClose(const std::string& tag) {
    --indent_level;
    writeIndent();
    xml_file << "</" << tag << ">\n";
}

inline void CompilationEngine::writeToken(const std::string& tag, const std::string& token) {
    writeIndent();
    xml_file << '<' << tag << "> "
            << tokenizer.xml_escape(token)
            << " </" << tag << ">\n";
}

void CompilationEngine::writeIdentifier(const std::string& name, IdentifierUsage usage, IdentifierRole role) {
    std::string category;
    int index = -1;

    if (role == IdentifierRole::ir_VARLIKE) {
        // first look in subroutine scope
        Kind k = subroutine_symbol_table.kindOf(name);
        if (k != Kind::k_NONE) {
            category = kindToCategory(k);
            index = subroutine_symbol_table.indexOf(name);
        } else {
            // then class scope
            k = class_symbol_table.kindOf(name);
            if (k != Kind::k_NONE) {
                category = kindToCategory(k);
                index = class_symbol_table.indexOf(name);
            } else {
                category = "none"; // undeclared or global class name we don't track
                index = -1; // dont update indices
            }
        }
    } 
    else if (role == IdentifierRole::ir_CLASSNAME) {
        category = "class";
        index    = -1;
    } 
    else if (role == IdentifierRole::ir_SUBROUTINENAME) {
        category = "subroutine";
        index    = -1;
    }

    writeIndent();
    xml_file << "<identifier "
             << "name=\""     << tokenizer.xml_escape(name) << "\" "
             << "category=\"" << category << "\" "
             << "index=\""    << index << "\" "
             << "usage=\""    << (usage == IdentifierUsage::iu_DECLARED ? "declared" : "used")
             << "\">"
             << "</identifier>\n";
}

std::string CompilationEngine::kindToCategory(Kind k) {
    switch (k) {
        case Kind::k_STATIC: return "static";
        case Kind::k_FIELD: return "field";
        case Kind::k_ARG: return "arg";
        case Kind::k_VAR: return "var";
        default: return "none"; // not in symbol table
    }
}

std::string CompilationEngine::kindToSegment(Kind k) {
    switch (k) {
        case Kind::k_STATIC: return "static";
        case Kind::k_FIELD:  return "this"; // fields 
        case Kind::k_ARG:    return "argument";
        case Kind::k_VAR:    return "local";
        default: throw std::runtime_error("[error] no match for k in kindToSegment()."); 
    }
}

void CompilationEngine::pushVar(const std::string& name) {
    Kind k = subroutine_symbol_table.kindOf(name);
    int index;

    if (k != Kind::k_NONE) {
        index = subroutine_symbol_table.indexOf(name);
    } 
    else {
        k = class_symbol_table.kindOf(name);
        if (k == Kind::k_NONE) {
            throw std::runtime_error("Unknown variable: " + name);
        }
        index = class_symbol_table.indexOf(name);
    }

    vmwriter.writePush(kindToSegment(k), index);
}

void CompilationEngine::popVar(const std::string& name) {
    Kind k = subroutine_symbol_table.kindOf(name);
    int index;

    if (k != Kind::k_NONE) {
        index = subroutine_symbol_table.indexOf(name);
    } 
    else {
        k = class_symbol_table.kindOf(name);
        if (k == Kind::k_NONE) {
            throw std::runtime_error("Unknown variable: " + name);
        }
        index = class_symbol_table.indexOf(name);
    }

    vmwriter.writePop(kindToSegment(k), index);
}

void CompilationEngine::codeWrite(const std::string& exp) {}

void CompilationEngine::compileClass() {
    // clear symbol table
    class_symbol_table.reset();
    subroutine_symbol_table.reset();
    // 'class' className '{' classVarDec* subroutineDec* '}'
    writeOpen("class");
    // 'class'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_CLASS) {
        throw std::runtime_error("Expected 'class' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    writeToken("keyword", keywordToString(tokenizer.keyWord()));
    tokenizer.advance();

    // className
    if (tokenizer.tokenType() != Type::t_IDENTIFIER)
        throw std::runtime_error("Expected className at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    
    class_name = tokenizer.identifier();
    writeIdentifier(class_name, IdentifierUsage::iu_DECLARED, IdentifierRole::ir_CLASSNAME);
    tokenizer.advance();

    // '{'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
        throw std::runtime_error("Expected '{' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", "{");
    tokenizer.advance();

    // classVarDec*
    while (tokenizer.tokenType() == Type::t_KEYWORD &&
        (tokenizer.keyWord() == KeyWord::kw_STATIC ||
        tokenizer.keyWord() == KeyWord::kw_FIELD)) {
        compileClassVarDec();
    }

    // subroutineDec*
    while (tokenizer.tokenType() == Type::t_KEYWORD &&
        (tokenizer.keyWord() == KeyWord::kw_CONSTRUCTOR ||
        tokenizer.keyWord() == KeyWord::kw_FUNCTION ||
        tokenizer.keyWord() == KeyWord::kw_METHOD)) {
        compileSubroutine();
    }

    // '}'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
        throw std::runtime_error("Expected '}' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", "}");
    tokenizer.advance();
    writeClose("class");
}

void CompilationEngine::compileClassVarDec() {
    // ('static'|'field') type varName(',' varName)* ';'
    writeOpen("classVarDec");
    // static | field
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        (tokenizer.keyWord() != KeyWord::kw_STATIC &&
        tokenizer.keyWord() != KeyWord::kw_FIELD)) {
        throw std::runtime_error("Expected 'static' or 'field' in classVarDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    Kind kind = (tokenizer.keyWord() == KeyWord::kw_STATIC) ? Kind::k_STATIC : Kind::k_FIELD;
    writeToken("keyword", keywordToString(tokenizer.keyWord()));
    tokenizer.advance();

    // type  (keyword type or identifier type)
    std::string type;
    if (tokenizer.tokenType() == Type::t_KEYWORD &&
        (tokenizer.keyWord() == KeyWord::kw_INT ||
        tokenizer.keyWord() == KeyWord::kw_CHAR ||
        tokenizer.keyWord() == KeyWord::kw_BOOLEAN)) {

        // primitive type
        type = keywordToString(tokenizer.keyWord());
        writeToken("keyword", type);
        tokenizer.advance();
    }
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {
        // className type
        type = tokenizer.identifier();
        writeIdentifier(type, IdentifierUsage::iu_USED, IdentifierRole::ir_CLASSNAME);
        tokenizer.advance();
    }
    else {
        throw std::runtime_error("Expected type in classVarDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    // varName
    if (tokenizer.tokenType() != Type::t_IDENTIFIER) {
        throw std::runtime_error("Expected varName in classVarDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    std::string name = tokenizer.identifier();
    class_symbol_table.define(name, type, kind);
    writeIdentifier(tokenizer.identifier(), IdentifierUsage::iu_DECLARED, IdentifierRole::ir_VARLIKE);
    tokenizer.advance();

    // (',' varName)*
    while (tokenizer.tokenType() == Type::t_SYMBOL &&
        tokenizer.symbol() == ',') {

        writeToken("symbol", ",");
        tokenizer.advance();

        if (tokenizer.tokenType() != Type::t_IDENTIFIER)
            throw std::runtime_error("Expected varName after ',' in classVarDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

        name = tokenizer.identifier();
        class_symbol_table.define(name, type, kind);
        writeIdentifier(tokenizer.identifier(), IdentifierUsage::iu_DECLARED, IdentifierRole::ir_VARLIKE);
        tokenizer.advance();
    }

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';')
        throw std::runtime_error("Expected ';' at end of classVarDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", ";");
    tokenizer.advance();
    writeClose("classVarDec");
}

void CompilationEngine::compileSubroutine() {
    subroutine_symbol_table.reset();
    // ('constructor'|'functon'|'method') ('void'|type) subroutineName '(' parameterList ')' subroutineBody
    writeOpen("subroutineDec");
    // ('constructor'|'function'|'method')
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        (tokenizer.keyWord() != KeyWord::kw_CONSTRUCTOR &&
        tokenizer.keyWord() != KeyWord::kw_FUNCTION &&
        tokenizer.keyWord() != KeyWord::kw_METHOD)) {
        throw std::runtime_error("Expected constructor/function/method at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    KeyWord subroutine_kind = tokenizer.keyWord();
    writeToken("keyword", keywordToString(tokenizer.keyWord()));
    tokenizer.advance();

    // implicit 'this' for methods
    if (subroutine_kind == KeyWord::kw_METHOD) {
        // 'this' is ARG 0 of type <class_name>
        subroutine_symbol_table.define("this", class_name, Kind::k_ARG);
    }

    // ('void' | type)
    if (tokenizer.tokenType() == Type::t_KEYWORD &&
        tokenizer.keyWord() == KeyWord::kw_VOID) {

        writeToken("keyword", "void");
        tokenizer.advance();
    }
    else if (tokenizer.tokenType() == Type::t_KEYWORD &&
            (tokenizer.keyWord() == KeyWord::kw_INT ||
            tokenizer.keyWord() == KeyWord::kw_CHAR ||
            tokenizer.keyWord() == KeyWord::kw_BOOLEAN)) {

        writeToken("keyword", keywordToString(tokenizer.keyWord()));
        tokenizer.advance();
    }
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {
        // className type
        writeIdentifier(tokenizer.identifier(), IdentifierUsage::iu_USED, IdentifierRole::ir_CLASSNAME);
        tokenizer.advance();
    }
    else {
        throw std::runtime_error("Expected return type in subroutine at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    // subroutineName
    if (tokenizer.tokenType() != Type::t_IDENTIFIER)
        throw std::runtime_error("Expected subroutineName at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeIdentifier(tokenizer.identifier(), IdentifierUsage::iu_DECLARED, IdentifierRole::ir_SUBROUTINENAME);
    tokenizer.advance();

    // '('
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '(')
        throw std::runtime_error("Expected '(' after subroutineName at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", "(");
    tokenizer.advance();

    // parameterList
    compileParameterList();

    // ')'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ')')
        throw std::runtime_error("Expected ')' after parameterList at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", ")");
    tokenizer.advance();

    // subroutineBody
    compileSubroutineBody();
    writeClose("subroutineDec");
}

void CompilationEngine::compileParameterList() {
    // ((type varName) (',' type varName)*)?
    writeOpen("parameterList");
    // empty?
    if (tokenizer.tokenType() == Type::t_SYMBOL && tokenizer.symbol() == ')') {
        writeClose("parameterList");
        return;
    }

    // type
    std::string type;
    if (tokenizer.tokenType() == Type::t_KEYWORD) {
        type = keywordToString(tokenizer.keyWord());
        writeToken("keyword", type);
        tokenizer.advance();
    } 
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {
        // className type
        type = tokenizer.identifier();
        writeIdentifier(type, IdentifierUsage::iu_USED, IdentifierRole::ir_CLASSNAME);
        tokenizer.advance();
    } 
    else {
        throw std::runtime_error("Expected type in parameterList at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    // varName
    if (tokenizer.tokenType() != Type::t_IDENTIFIER)
        throw std::runtime_error("Expected varName in parameterList at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    std::string name = tokenizer.identifier();
    subroutine_symbol_table.define(name, type, Kind::k_ARG);
    writeIdentifier(name, IdentifierUsage::iu_DECLARED, IdentifierRole::ir_VARLIKE);
    tokenizer.advance();

    // (',' type varName)*
    while (tokenizer.tokenType() == Type::t_SYMBOL && tokenizer.symbol() == ',') {
        writeToken("symbol", ",");
        tokenizer.advance();

        // type
        if (tokenizer.tokenType() == Type::t_KEYWORD &&
            (tokenizer.keyWord() == KeyWord::kw_INT ||
            tokenizer.keyWord() == KeyWord::kw_CHAR ||
            tokenizer.keyWord() == KeyWord::kw_BOOLEAN)) {

            type = keywordToString(tokenizer.keyWord());
            writeToken("keyword", type);
            tokenizer.advance();
        } 
        else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {
            type = tokenizer.identifier();
            writeIdentifier(type, IdentifierUsage::iu_USED, IdentifierRole::ir_CLASSNAME);
            tokenizer.advance();
        } 
        else {
            throw std::runtime_error("Expected type after ',' in parameterList at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
        }

        // varName
        if (tokenizer.tokenType() != Type::t_IDENTIFIER)
            throw std::runtime_error("Expected varName in parameterList at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

        name = tokenizer.identifier();
        subroutine_symbol_table.define(name, type, Kind::k_ARG);
        writeIdentifier(name, IdentifierUsage::iu_DECLARED, IdentifierRole::ir_VARLIKE);
        tokenizer.advance();
    }
    writeClose("parameterList");
}

void CompilationEngine::compileSubroutineBody() {
    // '{' varDec* statements '}'
    writeOpen("subroutineBody");
    // '{'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
        throw std::runtime_error("Expected '{' at start of subroutineBody at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", "{");
    tokenizer.advance();

    // varDec*
    while (tokenizer.tokenType() == Type::t_KEYWORD && tokenizer.keyWord() == KeyWord::kw_VAR) {
        compileVarDec();
    }

    // statements
    compileStatements();

    // '}'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
        throw std::runtime_error("Expected '}' at end of subroutineBody at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", "}");
    tokenizer.advance();
    writeClose("subroutineBody");
}

void CompilationEngine::compileVarDec() {
    // 'var' type varName(',' varName)* ';'
    writeOpen("varDec");
    // 'var'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_VAR)
        throw std::runtime_error("Expected 'var' at start of varDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("keyword", "var");
    tokenizer.advance();

    // type (int | char | boolean | className)
    std::string type;
    if (tokenizer.tokenType() == Type::t_KEYWORD &&
        (tokenizer.keyWord() == KeyWord::kw_INT ||
        tokenizer.keyWord() == KeyWord::kw_CHAR ||
        tokenizer.keyWord() == KeyWord::kw_BOOLEAN)) {
        type = keywordToString(tokenizer.keyWord());
        writeToken("keyword", type);
        tokenizer.advance();
    }
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {
        type = tokenizer.identifier();
        writeIdentifier(type, IdentifierUsage::iu_USED, IdentifierRole::ir_CLASSNAME);
        tokenizer.advance();
    }
    else {
        throw std::runtime_error("Expected type in varDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    // varName
    if (tokenizer.tokenType() != Type::t_IDENTIFIER)
        throw std::runtime_error("Expected varName in varDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    std::string name = tokenizer.identifier();
    subroutine_symbol_table.define(name, type, Kind::k_VAR);
    writeIdentifier(name, IdentifierUsage::iu_DECLARED, IdentifierRole::ir_VARLIKE);
    tokenizer.advance();

    // (',' varName)*
    while (tokenizer.tokenType() == Type::t_SYMBOL &&
        tokenizer.symbol() == ',')
    {
        writeToken("symbol", ",");
        tokenizer.advance();

        if (tokenizer.tokenType() != Type::t_IDENTIFIER)
            throw std::runtime_error("Expected varName after ',' in varDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

        name = tokenizer.identifier();
        subroutine_symbol_table.define(name, type, Kind::k_VAR);
        writeIdentifier(name, IdentifierUsage::iu_DECLARED, IdentifierRole::ir_VARLIKE);
        tokenizer.advance();
    }

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';')
        throw std::runtime_error("Expected ';' at end of varDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", ";");
    tokenizer.advance();
    writeClose("varDec");
}

void CompilationEngine::compileStatements() {
    // statement*
    writeOpen("statements");
    // letStatement|ifStatement|whileStatement|doStatement|returnStatement
    while (tokenizer.tokenType() == Type::t_KEYWORD) {
        switch (tokenizer.keyWord()) {
            case KeyWord::kw_LET:
                compileLet();
                break;

            case KeyWord::kw_IF:
                compileIf();
                break;

            case KeyWord::kw_WHILE:
                compileWhile();
                break;

            case KeyWord::kw_DO:
                compileDo();
                break;

            case KeyWord::kw_RETURN:
                compileReturn();
                break;
        }
    }
    writeClose("statements");
}

void CompilationEngine::compileLet() {
    // 'let' varName('[' expression ']')? '=' expression ';'
    writeOpen("letStatement");
    // 'let'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_LET)
        throw std::runtime_error("Expected 'let' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("keyword", "let");
    tokenizer.advance();

    // varName
    if (tokenizer.tokenType() != Type::t_IDENTIFIER)
        throw std::runtime_error("Expected varName after 'let' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    std::string name = tokenizer.identifier(); 
    writeIdentifier(name, IdentifierUsage::iu_USED, IdentifierRole::ir_VARLIKE);
    tokenizer.advance();

    // ('[' expression ']')?
    if (tokenizer.tokenType() == Type::t_SYMBOL && tokenizer.symbol() == '[') {
        writeToken("symbol", "[");
        tokenizer.advance();

        compileExpression();

        if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ']')
            throw std::runtime_error("Expected ']' in array indexing at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

        writeToken("symbol", "]");
        tokenizer.advance();
    }

    // '='
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '=')
        throw std::runtime_error("Expected '=' in let statement at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", "=");
    tokenizer.advance();

    // expression
    compileExpression();

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';')
        throw std::runtime_error("Expected ';' at end of let statement at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", ";");
    tokenizer.advance();
    writeClose("letStatement");
}

void CompilationEngine::compileIf() {
    // 'if' '(' expression ')' '{' statements '}' ('else' '{' statements '}')?
    writeOpen("ifStatement");
    // 'if'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_IF)
        throw std::runtime_error("Expected 'if' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("keyword", "if");
    tokenizer.advance();

    // '('
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '(')
        throw std::runtime_error("Expected '(' after 'if' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", "(");
    tokenizer.advance();

    // expression
    compileExpression();

    // ')'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ')')
        throw std::runtime_error("Expected ')' after expression in if at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", ")");
    tokenizer.advance();

    // '{'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
        throw std::runtime_error("Expected '{' after ')' in if at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", "{");
    tokenizer.advance();

    // statements
    compileStatements();

    // '}'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
        throw std::runtime_error("Expected '}' after if statements block at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", "}");
    tokenizer.advance();

    // optional else block
    if (tokenizer.tokenType() == Type::t_KEYWORD &&
        tokenizer.keyWord() == KeyWord::kw_ELSE)
    {
        // 'else'
        writeToken("keyword", "else");
        tokenizer.advance();

        // '{'
        if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
            throw std::runtime_error("Expected '{' after 'else' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

        writeToken("symbol", "{");
        tokenizer.advance();

        // statements
        compileStatements();

        // '}'
        if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
            throw std::runtime_error("Expected '}' after else statements block at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

        writeToken("symbol", "}");
        tokenizer.advance();
    }
    writeClose("ifStatement");
}

void CompilationEngine::compileWhile() {
    // 'while' '(' expression ')' '{' statements '}'
    writeOpen("whileStatement");
    // 'while'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_WHILE)
        throw std::runtime_error("Expected 'while' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("keyword", "while");
    tokenizer.advance();

    // '('
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '(')
        throw std::runtime_error("Expected '(' after 'while' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", "(");
    tokenizer.advance();

    // expression
    compileExpression();

    // ')'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ')')
        throw std::runtime_error("Expected ')' after expression in while at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", ")");
    tokenizer.advance();

    // '{'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
        throw std::runtime_error("Expected '{' after ')' in while at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", "{");
    tokenizer.advance();

    // statements
    compileStatements();

    // '}'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
        throw std::runtime_error("Expected '}' after while statements block at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", "}");
    tokenizer.advance();
    writeClose("whileStatement");
}

void CompilationEngine::compileDo() {
    // 'do' subroutineCall ';'
    writeOpen("doStatement");

    // 'do'
    if (tokenizer.tokenType() != Type::t_KEYWORD || tokenizer.keyWord() != KeyWord::kw_DO) {
        throw std::runtime_error(
            "Expected 'do' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    writeToken("keyword", "do");
    tokenizer.advance();

    // trick: parse subroutineCall as if it were an expression
    // compileExpression() will call compileTerm(), which handles subroutineCall.
    compileExpression();

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';') {
        throw std::runtime_error(
            "Expected ';' after do-call at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    writeToken("symbol", ";");
    tokenizer.advance();

    writeClose("doStatement");
}


void CompilationEngine::compileReturn() {
    // 'return' expression? ';'
    writeOpen("returnStatement");
    // 'return'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_RETURN)
        throw std::runtime_error("Expected 'return' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("keyword", "return");
    tokenizer.advance();

    // optional expression
    // if next token is not ';', we must have an expression
    if (!(tokenizer.tokenType() == Type::t_SYMBOL && tokenizer.symbol() == ';')) {
        compileExpression();
    }

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';')
        throw std::runtime_error("Expected ';' after return at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    writeToken("symbol", ";");
    tokenizer.advance();
    writeClose("returnStatement");
}

void CompilationEngine::compileExpression() {
    // term (op term)*
    writeOpen("expression");
    // term
    compileTerm();
    // (op term)*
    while (tokenizer.tokenType() == Type::t_SYMBOL &&
        (tokenizer.symbol() == '+' ||
        tokenizer.symbol() == '-' ||
        tokenizer.symbol() == '*' ||
        tokenizer.symbol() == '/' ||
        tokenizer.symbol() == '&' ||
        tokenizer.symbol() == '|' ||
        tokenizer.symbol() == '<' ||
        tokenizer.symbol() == '>' ||
        tokenizer.symbol() == '=')) {

        // op
        writeToken("symbol", std::string(1, tokenizer.symbol()));
        tokenizer.advance();

        // next term
        compileTerm();
    }
    writeClose("expression");
}

void CompilationEngine::compileTerm() {
    // integerConstant|stringConstant|keywordConstant|varName '[' expression ']'|'(' expression ')'|(unaryOp term)|subroutineCall
    writeOpen("term");
    // integerConstant
    if (tokenizer.tokenType() == Type::t_INT_CONST) {
        writeToken("integerConstant", std::to_string(tokenizer.intVal()));
        tokenizer.advance();
    }

    // stringConstant
    else if (tokenizer.tokenType() == Type::t_STRING_CONST) {
        writeToken("stringConstant", tokenizer.stringVal());
        tokenizer.advance();
    }

    // keywordConstant
    else if (tokenizer.tokenType() == Type::t_KEYWORD &&
    (tokenizer.keyWord() == KeyWord::kw_TRUE ||
        tokenizer.keyWord() == KeyWord::kw_FALSE ||
        tokenizer.keyWord() == KeyWord::kw_NULL ||
        tokenizer.keyWord() == KeyWord::kw_THIS)) {

        writeToken("keyword", keywordToString(tokenizer.keyWord()));
        tokenizer.advance();
    }

    // '(' expression ')'
    else if (tokenizer.tokenType() == Type::t_SYMBOL &&
        tokenizer.symbol() == '(') {

        writeToken("symbol", "(");
        tokenizer.advance();

        compileExpression();

        if (tokenizer.tokenType() != Type::t_SYMBOL ||
            tokenizer.symbol() != ')')
            throw std::runtime_error("Expected ')' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

        writeToken("symbol", ")");
        tokenizer.advance();
    }

    // unaryOp term
    else if (tokenizer.tokenType() == Type::t_SYMBOL &&
    (tokenizer.symbol() == '-' || tokenizer.symbol() == '~')) {

        writeToken("symbol", std::string(1, tokenizer.symbol()));
        tokenizer.advance();
        compileTerm();
    }

    // IDENTIFIER: varName | array access | subroutineCall
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {

        std::string name = tokenizer.identifier();
        tokenizer.advance();

        // Lookahead: '[' vs '(' vs '.' vs plain varName
        if (tokenizer.tokenType() == Type::t_SYMBOL &&
            tokenizer.symbol() == '[') {
            // name '[' expression ']'
            // 'name' must be a var-like identifier
            writeIdentifier(name, IdentifierUsage::iu_USED, IdentifierRole::ir_VARLIKE);

            writeToken("symbol", "[");
            tokenizer.advance();

            compileExpression();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != ']') {
                throw std::runtime_error(
                    "Expected ']' in array indexing at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
            }

            writeToken("symbol", "]");
            tokenizer.advance();
        }
        else if (tokenizer.tokenType() == Type::t_SYMBOL &&
                 tokenizer.symbol() == '(') {
            // subroutineName '(' expressionList ')'
            // 'name' is a subroutineName in the current class
            writeIdentifier(name, IdentifierUsage::iu_USED, IdentifierRole::ir_SUBROUTINENAME);

            writeToken("symbol", "(");
            tokenizer.advance();

            compileExpressionList();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != ')') {
                throw std::runtime_error(
                    "Expected ')' after expressionList at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
            }

            writeToken("symbol", ")");
            tokenizer.advance();
        }
        else if (tokenizer.tokenType() == Type::t_SYMBOL &&
                 tokenizer.symbol() == '.') {
            // (className | varName) '.' subroutineName '(' expressionList ')'

            // decide if 'name' is a className or a var-like symbol
            Kind k = subroutine_symbol_table.kindOf(name);
            if (k == Kind::k_NONE) {
                k = class_symbol_table.kindOf(name);
            }

            if (k == Kind::k_NONE) {
                // treat as className
                writeIdentifier(name, IdentifierUsage::iu_USED, IdentifierRole::ir_CLASSNAME);
            } 
            else {
                // found in a table: var/field/arg/static
                writeIdentifier(name, IdentifierUsage::iu_USED, IdentifierRole::ir_VARLIKE);
            }

            writeToken("symbol", ".");
            tokenizer.advance();

            if (tokenizer.tokenType() != Type::t_IDENTIFIER) {
                throw std::runtime_error(
                    "Expected subroutineName after '.' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
            }

            std::string subName = tokenizer.identifier();
            writeIdentifier(subName, IdentifierUsage::iu_USED, IdentifierRole::ir_SUBROUTINENAME);
            tokenizer.advance();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != '(') {
                throw std::runtime_error(
                    "Expected '(' after subroutineName at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
            }

            writeToken("symbol", "(");
            tokenizer.advance();

            compileExpressionList();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != ')') {
                throw std::runtime_error("Expected ')' after expressionList at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
            }

            writeToken("symbol", ")");
            tokenizer.advance();
        }
        else {
            // plain varName
            writeIdentifier(name, IdentifierUsage::iu_USED, IdentifierRole::ir_VARLIKE);
        }
    }
    else {
        throw std::runtime_error(
            "Invalid term at line " +
            std::to_string(tokenizer.line_number) + ".\n > " +
            tokenizer.current_line + ".\n");
    }

    writeClose("term");
}

int CompilationEngine::compileExpressionList() {
    // (expression(',' expression)*)?
    writeOpen("expressionList");
    size_t count = 0;
    // empty? next token must be ')'
    if (tokenizer.tokenType() == Type::t_SYMBOL &&
        tokenizer.symbol() == ')') {
        writeClose("expressionList");
        return 0;
    }

    // expression
    compileExpression();
    ++count;

    // (',' expression)*
    while (tokenizer.tokenType() == Type::t_SYMBOL &&
        tokenizer.symbol() == ',') {
        writeToken("symbol", ",");
        tokenizer.advance();
        compileExpression();
        ++count;
    }
    writeClose("expressionList");
    return count;
}
