#include "CompilationEngine.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>

CompilationEngine::CompilationEngine(JackTokenizer& jack_tokenizer) :
    tokenizer(jack_tokenizer), indent_level(0) {
    std::string jack_file_name = jack_tokenizer.jack_file_name;
    // strip .jack extension
    size_t dot = jack_file_name.find_last_of('.');
    std::string base = jack_file_name.substr(0, dot);
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

void CompilationEngine::compileClass() {
    // 'class' className '{' classVarDec* subroutineDec* '}'
    writeOpen("class");
    // 'class'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_CLASS) {
        throw std::runtime_error("Expected 'class' at beginning of class");
    }

    writeToken("keyword", keywordToString(tokenizer.keyWord()));
    tokenizer.advance();

    // className
    if (tokenizer.tokenType() != Type::t_IDENTIFIER)
        throw std::runtime_error("Expected className");

    writeToken("identifier", tokenizer.identifier());
    tokenizer.advance();

    // '{'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
        throw std::runtime_error("Expected '{'");

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
        throw std::runtime_error("Expected '}'");

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
        throw std::runtime_error("Expected 'static' or 'field' in classVarDec");
    }

    writeToken("keyword", keywordToString(tokenizer.keyWord()));
    tokenizer.advance();

    // type  (keyword type or identifier type)
    if (tokenizer.tokenType() == Type::t_KEYWORD &&
        (tokenizer.keyWord() == KeyWord::kw_INT ||
        tokenizer.keyWord() == KeyWord::kw_CHAR ||
        tokenizer.keyWord() == KeyWord::kw_BOOLEAN)) {

        // primitive type
        writeToken("keyword", keywordToString(tokenizer.keyWord()));
        tokenizer.advance();
    }
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {
        // className type
        writeToken("identifier", tokenizer.identifier());
        tokenizer.advance();
    }
    else {
        throw std::runtime_error("Expected type in classVarDec");
    }

    // varName
    if (tokenizer.tokenType() != Type::t_IDENTIFIER) {
        throw std::runtime_error("Expected varName in classVarDec");
    }

    writeToken("identifier", tokenizer.identifier());
    tokenizer.advance();

    // (',' varName)*
    while (tokenizer.tokenType() == Type::t_SYMBOL &&
        tokenizer.symbol() == ',') {

        writeToken("symbol", ",");
        tokenizer.advance();

        if (tokenizer.tokenType() != Type::t_IDENTIFIER)
            throw std::runtime_error("Expected varName after ',' in classVarDec");

        writeToken("identifier", tokenizer.identifier());
        tokenizer.advance();
    }

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';')
        throw std::runtime_error("Expected ';' at end of classVarDec");

    writeToken("symbol", ";");
    tokenizer.advance();
    writeClose("classVarDec");
}

void CompilationEngine::compileSubroutine() {
    // ('constructor'|'functon'|'method') ('void'|type) subroutineName '(' parameterList ')' subroutineBody
    writeOpen("subroutineDec");
    // ('constructor'|'function'|'method')
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        (tokenizer.keyWord() != KeyWord::kw_CONSTRUCTOR &&
        tokenizer.keyWord() != KeyWord::kw_FUNCTION &&
        tokenizer.keyWord() != KeyWord::kw_METHOD)) {
        throw std::runtime_error("Expected constructor/function/method");
    }

    writeToken("keyword", keywordToString(tokenizer.keyWord()));
    tokenizer.advance();

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
        writeToken("identifier", tokenizer.identifier());
        tokenizer.advance();
    }
    else {
        throw std::runtime_error("Expected return type in subroutine");
    }

    // subroutineName
    if (tokenizer.tokenType() != Type::t_IDENTIFIER)
        throw std::runtime_error("Expected subroutineName");

    writeToken("identifier", tokenizer.identifier());
    tokenizer.advance();

    // '('
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '(')
        throw std::runtime_error("Expected '(' after subroutineName");

    writeToken("symbol", "(");
    tokenizer.advance();

    // parameterList
    compileParameterList();

    // ')'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ')')
        throw std::runtime_error("Expected ')' after parameterList");

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
    if (!(
        (tokenizer.tokenType() == Type::t_KEYWORD &&
        (tokenizer.keyWord() == KeyWord::kw_INT ||
        tokenizer.keyWord() == KeyWord::kw_CHAR ||
        tokenizer.keyWord() == KeyWord::kw_BOOLEAN))
        ||
        tokenizer.tokenType() == Type::t_IDENTIFIER)) {
        writeClose("parameterList");
        return;
    }

    // type
    if (tokenizer.tokenType() == Type::t_KEYWORD) {
        writeToken("keyword", keywordToString(tokenizer.keyWord()));
        tokenizer.advance();
    } else {
        writeToken("identifier", tokenizer.identifier());
        tokenizer.advance();
    }

    // varName
    if (tokenizer.tokenType() != Type::t_IDENTIFIER)
        throw std::runtime_error("Expected varName in parameterList");

    writeToken("identifier", tokenizer.identifier());
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

            writeToken("keyword", keywordToString(tokenizer.keyWord()));
            tokenizer.advance();
        } else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {
            writeToken("identifier", tokenizer.identifier());
            tokenizer.advance();
        } else {
            throw std::runtime_error("Expected type after ',' in parameterList");
        }

        // varName
        if (tokenizer.tokenType() != Type::t_IDENTIFIER)
            throw std::runtime_error("Expected varName in parameterList");

        writeToken("identifier", tokenizer.identifier());
        tokenizer.advance();
    }
    writeClose("parameterList");
}

void CompilationEngine::compileSubroutineBody() {
    // '{' varDec* statements '}'
    writeOpen("subroutineBody");
    // '{'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
        throw std::runtime_error("Expected '{' at start of subroutineBody");

    writeToken("symbol", "{");
    tokenizer.advance();

    // varDec*
    while (tokenizer.tokenType() == Type::t_KEYWORD &&
        tokenizer.keyWord() == KeyWord::kw_VAR) {
        compileVarDec();
    }

    // statements
    compileStatements();

    // '}'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
        throw std::runtime_error("Expected '}' at end of subroutineBody");

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
        throw std::runtime_error("Expected 'var' at start of varDec");

    writeToken("keyword", "var");
    tokenizer.advance();

    // type (int | char | boolean | className)
    if (tokenizer.tokenType() == Type::t_KEYWORD &&
    (tokenizer.keyWord() == KeyWord::kw_INT ||
        tokenizer.keyWord() == KeyWord::kw_CHAR ||
        tokenizer.keyWord() == KeyWord::kw_BOOLEAN))
    {
        writeToken("keyword", keywordToString(tokenizer.keyWord()));
        tokenizer.advance();
    }
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {
        writeToken("identifier", tokenizer.identifier());
        tokenizer.advance();
    }
    else {
        throw std::runtime_error("Expected type in varDec");
    }

    // varName
    if (tokenizer.tokenType() != Type::t_IDENTIFIER)
        throw std::runtime_error("Expected varName in varDec");

    writeToken("identifier", tokenizer.identifier());
    tokenizer.advance();

    // (',' varName)*
    while (tokenizer.tokenType() == Type::t_SYMBOL &&
        tokenizer.symbol() == ',')
    {
        writeToken("symbol", ",");
        tokenizer.advance();

        if (tokenizer.tokenType() != Type::t_IDENTIFIER)
            throw std::runtime_error("Expected varName after ',' in varDec");

        writeToken("identifier", tokenizer.identifier());
        tokenizer.advance();
    }

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';')
        throw std::runtime_error("Expected ';' at end of varDec");

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
        throw std::runtime_error("Expected 'let'");

    writeToken("keyword", "let");
    tokenizer.advance();

    // varName
    if (tokenizer.tokenType() != Type::t_IDENTIFIER)
        throw std::runtime_error("Expected varName after 'let'");

    writeToken("identifier", tokenizer.identifier());
    tokenizer.advance();

    // ('[' expression ']')?
    if (tokenizer.tokenType() == Type::t_SYMBOL && tokenizer.symbol() == '[') {
        writeToken("symbol", "[");
        tokenizer.advance();

        compileExpression();

        if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ']')
            throw std::runtime_error("Expected ']' in array indexing");

        writeToken("symbol", "]");
        tokenizer.advance();
    }

    // '='
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '=')
        throw std::runtime_error("Expected '=' in let statement");

    writeToken("symbol", "=");
    tokenizer.advance();

    // expression
    compileExpression();

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';')
        throw std::runtime_error("Expected ';' at end of let statement");

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
        throw std::runtime_error("Expected 'if'");

    writeToken("keyword", "if");
    tokenizer.advance();

    // '('
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '(')
        throw std::runtime_error("Expected '(' after 'if'");

    writeToken("symbol", "(");
    tokenizer.advance();

    // expression
    compileExpression();

    // ')'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ')')
        throw std::runtime_error("Expected ')' after expression in if");

    writeToken("symbol", ")");
    tokenizer.advance();

    // '{'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
        throw std::runtime_error("Expected '{' after ')' in if");

    writeToken("symbol", "{");
    tokenizer.advance();

    // statements
    compileStatements();

    // '}'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
        throw std::runtime_error("Expected '}' after if statements block");

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
            throw std::runtime_error("Expected '{' after 'else'");

        writeToken("symbol", "{");
        tokenizer.advance();

        // statements
        compileStatements();

        // '}'
        if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
            throw std::runtime_error("Expected '}' after else statements block");

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
        throw std::runtime_error("Expected 'while'");

    writeToken("keyword", "while");
    tokenizer.advance();

    // '('
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '(')
        throw std::runtime_error("Expected '(' after 'while'");

    writeToken("symbol", "(");
    tokenizer.advance();

    // expression
    compileExpression();

    // ')'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ')')
        throw std::runtime_error("Expected ')' after expression in while");

    writeToken("symbol", ")");
    tokenizer.advance();

    // '{'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
        throw std::runtime_error("Expected '{' after ')' in while");

    writeToken("symbol", "{");
    tokenizer.advance();

    // statements
    compileStatements();

    // '}'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
        throw std::runtime_error("Expected '}' after while statements block");

    writeToken("symbol", "}");
    tokenizer.advance();
    writeClose("whileStatement");
}

void CompilationEngine::compileDo() {
    // 'do' subroutineCall ';'
    writeOpen("doStatement");
    // 'do'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_DO)
        throw std::runtime_error("Expected 'do'");

    writeToken("keyword", "do");
    tokenizer.advance();

    // treat subroutineCall as an expression term
    // EXPRESSION
    // term
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
            throw std::runtime_error("Expected ')'");

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

    // IDENTIFIER: varName, array access, or subroutine call
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {

        std::string name = tokenizer.identifier();
        writeToken("identifier", name);
        tokenizer.advance();

        // array access: name '[' expression ']'
        if (tokenizer.tokenType() == Type::t_SYMBOL &&
            tokenizer.symbol() == '[') {

            writeToken("symbol", "[");
            tokenizer.advance();

            compileExpression();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != ']')
                throw std::runtime_error("Expected ']'");

            writeToken("symbol", "]");
            tokenizer.advance();
        }

        // subroutine call: name '(' ...
        else if (tokenizer.tokenType() == Type::t_SYMBOL &&
            tokenizer.symbol() == '(') {

            writeToken("symbol", "(");
            tokenizer.advance();

            compileExpressionList();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != ')')
                throw std::runtime_error("Expected ')'");

            writeToken("symbol", ")");
            tokenizer.advance();
        }

        // subroutine call: name '.' name '(' ...
        else if (tokenizer.tokenType() == Type::t_SYMBOL &&
            tokenizer.symbol() == '.') {

            writeToken("symbol", ".");
            tokenizer.advance();

            if (tokenizer.tokenType() != Type::t_IDENTIFIER)
                throw std::runtime_error("Expected subroutineName after '.'");

            writeToken("identifier", tokenizer.identifier());
            tokenizer.advance();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != '(')
                throw std::runtime_error("Expected '('");

            writeToken("symbol", "(");
            tokenizer.advance();

            compileExpressionList();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != ')')
                throw std::runtime_error("Expected ')'");

            writeToken("symbol", ")");
            tokenizer.advance();
        }

        // plain varName
    }
    else throw std::runtime_error("Invalid term");
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
            throw std::runtime_error("Expected ')'");

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

    // IDENTIFIER: varName, array access, or subroutine call
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {

        std::string name = tokenizer.identifier();
        writeToken("identifier", name);
        tokenizer.advance();

        // array access: name '[' expression ']'
        if (tokenizer.tokenType() == Type::t_SYMBOL &&
            tokenizer.symbol() == '[') {

            writeToken("symbol", "[");
            tokenizer.advance();

            compileExpression();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != ']')
                throw std::runtime_error("Expected ']'");

            writeToken("symbol", "]");
            tokenizer.advance();
        }

        // subroutine call: name '(' ...
        else if (tokenizer.tokenType() == Type::t_SYMBOL &&
            tokenizer.symbol() == '(') {

            writeToken("symbol", "(");
            tokenizer.advance();

            compileExpressionList();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != ')')
                throw std::runtime_error("Expected ')'");

            writeToken("symbol", ")");
            tokenizer.advance();
        }

        // subroutine call: name '.' name '(' ...
        else if (tokenizer.tokenType() == Type::t_SYMBOL &&
            tokenizer.symbol() == '.') {

            writeToken("symbol", ".");
            tokenizer.advance();

            if (tokenizer.tokenType() != Type::t_IDENTIFIER)
                throw std::runtime_error("Expected subroutineName after '.'");

            writeToken("identifier", tokenizer.identifier());
            tokenizer.advance();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != '(')
                throw std::runtime_error("Expected '('");

            writeToken("symbol", "(");
            tokenizer.advance();

            compileExpressionList();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != ')')
                throw std::runtime_error("Expected ')'");

            writeToken("symbol", ")");
            tokenizer.advance();
        }

        // plain varName
    }
    else throw std::runtime_error("Invalid term");
    }

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';')
        throw std::runtime_error("Expected ';' after do-call");

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
        throw std::runtime_error("Expected 'return'");

    writeToken("keyword", "return");
    tokenizer.advance();

    // optional expression
    // if next token is not ';', we must have an expression
    if (!(tokenizer.tokenType() == Type::t_SYMBOL && tokenizer.symbol() == ';')) {
        compileExpression();
    }

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';')
        throw std::runtime_error("Expected ';' after return");

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
            throw std::runtime_error("Expected ')'");

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

    // IDENTIFIER: varName, array access, or subroutine call
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {

        std::string name = tokenizer.identifier();
        writeToken("identifier", name);
        tokenizer.advance();

        // array access: name '[' expression ']'
        if (tokenizer.tokenType() == Type::t_SYMBOL &&
            tokenizer.symbol() == '[') {

            writeToken("symbol", "[");
            tokenizer.advance();

            compileExpression();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != ']')
                throw std::runtime_error("Expected ']'");

            writeToken("symbol", "]");
            tokenizer.advance();
        }

        // subroutine call: name '(' ...
        else if (tokenizer.tokenType() == Type::t_SYMBOL &&
            tokenizer.symbol() == '(') {

            writeToken("symbol", "(");
            tokenizer.advance();

            compileExpressionList();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != ')')
                throw std::runtime_error("Expected ')'");

            writeToken("symbol", ")");
            tokenizer.advance();
        }

        // subroutine call: name '.' name '(' ...
        else if (tokenizer.tokenType() == Type::t_SYMBOL &&
            tokenizer.symbol() == '.') {

            writeToken("symbol", ".");
            tokenizer.advance();

            if (tokenizer.tokenType() != Type::t_IDENTIFIER)
                throw std::runtime_error("Expected subroutineName after '.'");

            writeToken("identifier", tokenizer.identifier());
            tokenizer.advance();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != '(')
                throw std::runtime_error("Expected '('");

            writeToken("symbol", "(");
            tokenizer.advance();

            compileExpressionList();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != ')')
                throw std::runtime_error("Expected ')'");

            writeToken("symbol", ")");
            tokenizer.advance();
        }

        // plain varName
    }
    else throw std::runtime_error("Invalid term");
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
