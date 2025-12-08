#include "CompilationEngine.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>

CompilationEngine::CompilationEngine(JackTokenizer& jack_tokenizer, bool emit_xml) :
    tokenizer(jack_tokenizer), 
    class_symbol_table{},
    subroutine_symbol_table{},
    vmwriter(jack_tokenizer.path),
    emit_xml_flag{emit_xml},
    indent_level{0},
    class_name{} {

    if (emit_xml) {
        std::filesystem::path xml_file_path = jack_tokenizer.path;
        xml_file_path.replace_extension("xml");
        // open output file
        xml_file.open(xml_file_path);
        if (!xml_file.is_open()) {
            std::cerr << "[error] cannot create XML file: " << xml_file_path << " \n";
            std::exit(1);
        }
    }
}

CompilationEngine::~CompilationEngine() {
    if (xml_file.is_open())
        xml_file.close();
    if (emit_xml_flag && xml_file.is_open())
        xml_file.close();
}

void CompilationEngine::compile() {
    compileClass(); // compileClass(); calls all other compilation processes
}

inline void CompilationEngine::emitIndent() {
    if (!emit_xml_flag) return;
    for (size_t i = 0; i < indent_level; ++i)
        xml_file << '\t';
}

inline void CompilationEngine::emitOpen(const std::string& tag) {
    if (!emit_xml_flag) {++indent_level; return;}
    emitIndent();
    xml_file << '<' << tag << '>' << '\n';
    ++indent_level;
}

inline void CompilationEngine::emitClose(const std::string& tag) {
    --indent_level;
    if (!emit_xml_flag) return;
    emitIndent();
    xml_file << "</" << tag << ">\n";
}

inline void CompilationEngine::emitToken(const std::string& tag, const std::string& token) {
    if (!emit_xml_flag) return;
    emitIndent();
    xml_file << '<' << tag << "> "
            << tokenizer.xml_escape(token)
            << " </" << tag << ">\n";
}

void CompilationEngine::emitIdentifier(const std::string& name, IdentifierUsage usage, IdentifierRole role) {
    std::string category;
    int index = -1;
    if (!emit_xml_flag) return;

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
        index = -1;
    } 
    else if (role == IdentifierRole::ir_SUBROUTINENAME) {
        category = "subroutine";
        index = -1;
    }

    emitIndent();
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
        default: throw std::runtime_error("[error] no match for k in kindToCategory()."); // not in symbol table
    }
}

std::string CompilationEngine::kindToSegment(Kind k) {
    switch (k) {
        case Kind::k_STATIC: return "static";
        case Kind::k_FIELD: return "this"; // fields 
        case Kind::k_ARG: return "argument";
        case Kind::k_VAR: return "local";
        default: throw std::runtime_error("[error] no match for k in kindToSegment()."); 
    }
}

static bool isOpChar(char c) {
    switch (c) {
        case '+': case '-': case '*': case '/':
        case '&': case '|': case '<': case '>': case '=':
            return true;
        default:
            return false;
    }
}

void CompilationEngine::writeOp(char op) {
    switch (op) {
        case '+': vmwriter.VMWriter::writeArithmetic("add"); break;
        case '-': vmwriter.VMWriter::writeArithmetic("sub"); break;
        case '&': vmwriter.VMWriter::writeArithmetic("and"); break;
        case '|': vmwriter.VMWriter::writeArithmetic("or"); break;
        case '<': vmwriter.VMWriter::writeArithmetic("lt"); break;
        case '>': vmwriter.VMWriter::writeArithmetic("gt"); break;
        case '=': vmwriter.VMWriter::writeArithmetic("eq"); break;
        case '*': vmwriter.VMWriter::writeCall("Math.multiply", 2); break; // OS call
        case '/': vmwriter.VMWriter::writeCall("Math.divide", 2); break; // OS call
        default:
            throw std::runtime_error("Unknown binary op at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }
}

void CompilationEngine::writeUnaryOp(char op) {
    switch (op) {
        case '-': vmwriter.writeArithmetic("neg"); break;
        case '~': vmwriter.writeArithmetic("not"); break;
        default:
            throw std::runtime_error("writeUnaryOp: unknown op at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
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
            throw std::runtime_error("[error] Unknown variable: " + name + " at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
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
            throw std::runtime_error("[error] Unknown variable: " + name + " at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
        }
        index = class_symbol_table.indexOf(name);
    }

    vmwriter.writePop(kindToSegment(k), index);
}

void CompilationEngine::codeWrite(const std::string& exp) {
    
}

void CompilationEngine::compileClass() {
    // clear symbol table
    class_symbol_table.reset();
    subroutine_symbol_table.reset();
    // 'class' className '{' classVarDec* subroutineDec* '}'
    emitOpen("class");
    // 'class'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_CLASS) {

        throw std::runtime_error("Expected 'class' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    emitToken("keyword", keywordToString(tokenizer.keyWord()));
    tokenizer.advance();

    // className
    if (tokenizer.tokenType() != Type::t_IDENTIFIER)
        throw std::runtime_error("Expected className at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    
    class_name = tokenizer.identifier();
    emitIdentifier(class_name, IdentifierUsage::iu_DECLARED, IdentifierRole::ir_CLASSNAME);
    tokenizer.advance();

    // '{'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
        throw std::runtime_error("Expected '{' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", "{");
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

    emitToken("symbol", "}");
    tokenizer.advance();
    emitClose("class");
}

void CompilationEngine::compileClassVarDec() {
    // ('static'|'field') type varName(',' varName)* ';'
    emitOpen("classVarDec");
    // static | field
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
       (tokenizer.keyWord() != KeyWord::kw_STATIC &&
        tokenizer.keyWord() != KeyWord::kw_FIELD)) {

        throw std::runtime_error("Expected 'static' or 'field' in classVarDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    Kind kind = (tokenizer.keyWord() == KeyWord::kw_STATIC) ? Kind::k_STATIC : Kind::k_FIELD;
    emitToken("keyword", keywordToString(tokenizer.keyWord()));
    tokenizer.advance();

    // type  (keyword type or identifier type)
    std::string type;
    if (tokenizer.tokenType() == Type::t_KEYWORD &&
       (tokenizer.keyWord() == KeyWord::kw_INT ||
        tokenizer.keyWord() == KeyWord::kw_CHAR ||
        tokenizer.keyWord() == KeyWord::kw_BOOLEAN)) {

        // primitive type
        type = keywordToString(tokenizer.keyWord());
        emitToken("keyword", type);
        tokenizer.advance();
    }
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {
        // className type
        type = tokenizer.identifier();
        emitIdentifier(type, IdentifierUsage::iu_USED, IdentifierRole::ir_CLASSNAME);
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
    emitIdentifier(tokenizer.identifier(), IdentifierUsage::iu_DECLARED, IdentifierRole::ir_VARLIKE);
    tokenizer.advance();

    // (',' varName)*
    while (tokenizer.tokenType() == Type::t_SYMBOL &&
        tokenizer.symbol() == ',') {

        emitToken("symbol", ",");
        tokenizer.advance();

        if (tokenizer.tokenType() != Type::t_IDENTIFIER)
            throw std::runtime_error("Expected varName after ',' in classVarDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

        name = tokenizer.identifier();
        class_symbol_table.define(name, type, kind);
        emitIdentifier(tokenizer.identifier(), IdentifierUsage::iu_DECLARED, IdentifierRole::ir_VARLIKE);
        tokenizer.advance();
    }

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';')
        throw std::runtime_error("Expected ';' at end of classVarDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", ";");
    tokenizer.advance();
    emitClose("classVarDec");
}

void CompilationEngine::compileSubroutine() {
    subroutine_symbol_table.reset();
    // ('constructor'|'functon'|'method') ('void'|type) subroutineName '(' parameterList ')' subroutineBody
    emitOpen("subroutineDec");
    // ('constructor'|'function'|'method')
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
       (tokenizer.keyWord() != KeyWord::kw_CONSTRUCTOR &&
        tokenizer.keyWord() != KeyWord::kw_FUNCTION &&
        tokenizer.keyWord() != KeyWord::kw_METHOD)) {

        throw std::runtime_error("Expected constructor|function|method at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    current_subroutine_keyword = tokenizer.keyWord();
    emitToken("keyword", keywordToString(current_subroutine_keyword));
    tokenizer.advance();

    // implicit 'this' for methods
    if (current_subroutine_keyword == KeyWord::kw_METHOD) {
        // 'this' is ARG 0 of type <class_name>
        subroutine_symbol_table.define("this", class_name, Kind::k_ARG);
    }

    // ('void' | type)
    if (tokenizer.tokenType() == Type::t_KEYWORD &&
        tokenizer.keyWord() == KeyWord::kw_VOID) {

        emitToken("keyword", "void");
        tokenizer.advance();
    }
    else if (tokenizer.tokenType() == Type::t_KEYWORD &&
            (tokenizer.keyWord() == KeyWord::kw_INT ||
             tokenizer.keyWord() == KeyWord::kw_CHAR ||
             tokenizer.keyWord() == KeyWord::kw_BOOLEAN)) {

        emitToken("keyword", keywordToString(tokenizer.keyWord()));
        tokenizer.advance();
    }
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {
        // className type
        emitIdentifier(tokenizer.identifier(), IdentifierUsage::iu_USED, IdentifierRole::ir_CLASSNAME);
        tokenizer.advance();
    }
    else {
        throw std::runtime_error("Expected return type in subroutine at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    // subroutineName
    if (tokenizer.tokenType() != Type::t_IDENTIFIER)
        throw std::runtime_error("Expected subroutineName at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    current_subroutine_name = tokenizer.identifier();

    emitIdentifier(tokenizer.identifier(), IdentifierUsage::iu_DECLARED, IdentifierRole::ir_SUBROUTINENAME);
    tokenizer.advance();

    // '('
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '(')
        throw std::runtime_error("Expected '(' after subroutineName at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", "(");
    tokenizer.advance();

    // parameterList
    compileParameterList();

    // ')'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ')')
        throw std::runtime_error("Expected ')' after parameterList at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", ")");
    tokenizer.advance();

    // subroutineBody
    compileSubroutineBody();
    emitClose("subroutineDec");
}

void CompilationEngine::compileParameterList() {
    // ((type varName) (',' type varName)*)?
    emitOpen("parameterList");
    // empty?
    if (tokenizer.tokenType() == Type::t_SYMBOL && tokenizer.symbol() == ')') {
        emitClose("parameterList");
        return;
    }

    // type
    std::string type;
    if (tokenizer.tokenType() == Type::t_KEYWORD) {
        type = keywordToString(tokenizer.keyWord());
        emitToken("keyword", type);
        tokenizer.advance();
    } 
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {
        // className type
        type = tokenizer.identifier();
        emitIdentifier(type, IdentifierUsage::iu_USED, IdentifierRole::ir_CLASSNAME);
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
    emitIdentifier(name, IdentifierUsage::iu_DECLARED, IdentifierRole::ir_VARLIKE);
    tokenizer.advance();

    // (',' type varName)*
    while (tokenizer.tokenType() == Type::t_SYMBOL && tokenizer.symbol() == ',') {
        emitToken("symbol", ",");
        tokenizer.advance();

        // type
        if (tokenizer.tokenType() == Type::t_KEYWORD &&
           (tokenizer.keyWord() == KeyWord::kw_INT ||
            tokenizer.keyWord() == KeyWord::kw_CHAR ||
            tokenizer.keyWord() == KeyWord::kw_BOOLEAN)) {

            type = keywordToString(tokenizer.keyWord());
            emitToken("keyword", type);
            tokenizer.advance();
        } 
        else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {
            type = tokenizer.identifier();
            emitIdentifier(type, IdentifierUsage::iu_USED, IdentifierRole::ir_CLASSNAME);
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
        emitIdentifier(name, IdentifierUsage::iu_DECLARED, IdentifierRole::ir_VARLIKE);
        tokenizer.advance();
    }
    emitClose("parameterList");
}

void CompilationEngine::compileSubroutineBody() {
    // '{' varDec* statements '}'
    emitOpen("subroutineBody");
    // '{'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
        throw std::runtime_error("Expected '{' at start of subroutineBody at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", "{");
    tokenizer.advance();

    // varDec*
    while (tokenizer.tokenType() == Type::t_KEYWORD && tokenizer.keyWord() == KeyWord::kw_VAR) {
        compileVarDec();
    }

    int nVars = subroutine_symbol_table.varCount(Kind::k_VAR);
    vmwriter.writeFunction(class_name + "." + current_subroutine_name, nVars);

    // prologue depending on keyword
    if (current_subroutine_keyword == KeyWord::kw_CONSTRUCTOR) {
        // constructor: allocate memory for this object
        int nFields = class_symbol_table.varCount(Kind::k_FIELD);

        vmwriter.writePush("constant", nFields); // size of object
        vmwriter.writeCall("Memory.alloc", 1); // returns base address
        vmwriter.writePop("pointer", 0); // this = base addr
    } 
    else if (current_subroutine_keyword == KeyWord::kw_METHOD) {
        // method: set 'this' to argument 0 (the object)
        vmwriter.writePush("argument", 0);
        vmwriter.writePop("pointer", 0);
    }
    
    // statements
    compileStatements();

    // '}'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
        throw std::runtime_error("Expected '}' at end of subroutineBody at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", "}");
    tokenizer.advance();
    emitClose("subroutineBody");
}

void CompilationEngine::compileVarDec() {
    // 'var' type varName(',' varName)* ';'
    emitOpen("varDec");
    // 'var'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_VAR)

        throw std::runtime_error("Expected 'var' at start of varDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("keyword", "var");
    tokenizer.advance();

    // type (int | char | boolean | className)
    std::string type;
    if (tokenizer.tokenType() == Type::t_KEYWORD &&
       (tokenizer.keyWord() == KeyWord::kw_INT ||
        tokenizer.keyWord() == KeyWord::kw_CHAR ||
        tokenizer.keyWord() == KeyWord::kw_BOOLEAN)) {

        type = keywordToString(tokenizer.keyWord());
        emitToken("keyword", type);
        tokenizer.advance();
    }
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {
        type = tokenizer.identifier();
        emitIdentifier(type, IdentifierUsage::iu_USED, IdentifierRole::ir_CLASSNAME);
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
    emitIdentifier(name, IdentifierUsage::iu_DECLARED, IdentifierRole::ir_VARLIKE);
    tokenizer.advance();

    // (',' varName)*
    while (tokenizer.tokenType() == Type::t_SYMBOL &&
        tokenizer.symbol() == ',')
    {
        emitToken("symbol", ",");
        tokenizer.advance();

        if (tokenizer.tokenType() != Type::t_IDENTIFIER)
            throw std::runtime_error("Expected varName after ',' in varDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

        name = tokenizer.identifier();
        subroutine_symbol_table.define(name, type, Kind::k_VAR);
        emitIdentifier(name, IdentifierUsage::iu_DECLARED, IdentifierRole::ir_VARLIKE);
        tokenizer.advance();
    }

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';')
        throw std::runtime_error("Expected ';' at end of varDec at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", ";");
    tokenizer.advance();
    emitClose("varDec");
}

void CompilationEngine::compileStatements() {
    // statement*
    emitOpen("statements");
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
    emitClose("statements");
}

void CompilationEngine::compileLet() {
    // 'let' varName('[' expression ']')? '=' expression ';'
    emitOpen("letStatement");
    // 'let'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_LET)

        throw std::runtime_error("Expected 'let' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("keyword", "let");
    tokenizer.advance();

    // varName
    if (tokenizer.tokenType() != Type::t_IDENTIFIER)
        throw std::runtime_error("Expected varName after 'let' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    std::string name = tokenizer.identifier(); 
    emitIdentifier(name, IdentifierUsage::iu_USED, IdentifierRole::ir_VARLIKE);
    tokenizer.advance();

    // ('[' expression ']')?
    bool isArray = false;
    if (tokenizer.tokenType() == Type::t_SYMBOL && tokenizer.symbol() == '[') {
        isArray = true;
        emitToken("symbol", "[");
        tokenizer.advance();

        pushVar(name);
        compileExpression();
        vmwriter.writeArithmetic("add");

        if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ']')
            throw std::runtime_error("Expected ']' in array indexing at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

        emitToken("symbol", "]");
        tokenizer.advance();
    }

    // '='
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '=')
        throw std::runtime_error("Expected '=' in let statement at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", "=");
    tokenizer.advance();

    // expression
    compileExpression();
    if (isArray) {
        vmwriter.writePop("temp", 0);
        vmwriter.writePop("pointer", 1);
        vmwriter.writePush("temp", 0);
        vmwriter.writePop("that", 0);
    }
    else { popVar(name); }

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';')
        throw std::runtime_error("Expected ';' at end of let statement at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", ";");
    tokenizer.advance();
    emitClose("letStatement");
}

void CompilationEngine::compileIf() {
    // 'if' '(' expression ')' '{' statements '}' ('else' '{' statements '}')?
    emitOpen("ifStatement");
    // 'if'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_IF)

        throw std::runtime_error("Expected 'if' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("keyword", "if");
    tokenizer.advance();

    // '('
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '(')
        throw std::runtime_error("Expected '(' after 'if' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", "(");
    tokenizer.advance();

    // expression
    compileExpression();
    // logical 'NOT', so as to negate the expression's value
    vmwriter.writeArithmetic("not");
    // create needed labels 
    std::string L1 = vmwriter.getLabel();
    std::string L2 = vmwriter.getLabel();
    // if top is non-zero, jump L1
    vmwriter.writeIf(L1);
    // ... statements ...

    // ')'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ')')
        throw std::runtime_error("Expected ')' after expression in if at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", ")");
    tokenizer.advance();

    // '{'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
        throw std::runtime_error("Expected '{' after ')' in if at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", "{");
    tokenizer.advance();

    // statements
    compileStatements();

    // '}'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
        throw std::runtime_error("Expected '}' after if statements block at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", "}");
    tokenizer.advance();

    // optional else block
    if (tokenizer.tokenType() == Type::t_KEYWORD &&
        tokenizer.keyWord() == KeyWord::kw_ELSE) {

        // goto L2
        vmwriter.writeGoto(L2);
        // label L1
        vmwriter.writeLabel(L1);
        // ... statements ...

        // 'else'
        emitToken("keyword", "else");
        tokenizer.advance();

        // '{'
        if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
            throw std::runtime_error("Expected '{' after 'else' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

        emitToken("symbol", "{");
        tokenizer.advance();

        // statements
        compileStatements();
        vmwriter.writeLabel(L2);

        // '}'
        if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
            throw std::runtime_error("Expected '}' after else statements block at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

        emitToken("symbol", "}");
        tokenizer.advance();
    }
    else {
        // no else block L1 is just the end
        vmwriter.writeLabel(L1);
    }

    emitClose("ifStatement");
}

void CompilationEngine::compileWhile() {
    // 'while' '(' expression ')' '{' statements '}'
    emitOpen("whileStatement");
    // 'while'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_WHILE)

        throw std::runtime_error("Expected 'while' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("keyword", "while");
    tokenizer.advance();
    
    std::string L1 = vmwriter.getLabel();
    std::string L2 = vmwriter.getLabel();
    vmwriter.writeLabel(L1);

    // '('
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '(')
        throw std::runtime_error("Expected '(' after 'while' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", "(");
    tokenizer.advance();

    // expression
    compileExpression();
    vmwriter.writeArithmetic("not");
    vmwriter.writeIf(L2);
    // ... statements ...

    // ')'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ')')
        throw std::runtime_error("Expected ')' after expression in while at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", ")");
    tokenizer.advance();

    // '{'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
        throw std::runtime_error("Expected '{' after ')' in while at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", "{");
    tokenizer.advance();

    // statements
    compileStatements();
    vmwriter.writeGoto(L1);
    vmwriter.writeLabel(L2);

    // '}'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
        throw std::runtime_error("Expected '}' after while statements block at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", "}");
    tokenizer.advance();
    emitClose("whileStatement");
}

void CompilationEngine::compileDo() {
    // 'do' subroutineCall ';'
    emitOpen("doStatement");

    // 'do'
    if (tokenizer.tokenType() != Type::t_KEYWORD || tokenizer.keyWord() != KeyWord::kw_DO) {
        throw std::runtime_error(
            "Expected 'do' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    emitToken("keyword", "do");
    tokenizer.advance();

    // trick: parse subroutineCall as if it were an expression
    // compileExpression() will call compileTerm(), which handles subroutineCall.
    compileExpression();

    // get rid of the top most element since we just want side effects
    vmwriter.writePop("temp", 0);

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';') {
        throw std::runtime_error(
            "Expected ';' after do-call at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    emitToken("symbol", ";");
    tokenizer.advance();

    emitClose("doStatement");
}


void CompilationEngine::compileReturn() {
    // 'return' expression? ';'
    emitOpen("returnStatement");
    // 'return'
    if (tokenizer.tokenType() != Type::t_KEYWORD ||
        tokenizer.keyWord() != KeyWord::kw_RETURN)

        throw std::runtime_error("Expected 'return' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("keyword", "return");
    tokenizer.advance();

    // optional expression
    // if next token is not ';', we must have an expression
    if (!(tokenizer.tokenType() == Type::t_SYMBOL && tokenizer.symbol() == ';')) {
        compileExpression(); // value on the stack
    }
    else { // void return
        vmwriter.writePush("constant", 0); // checkit
    }
    vmwriter.writeReturn();

    // ';'
    if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ';')
        throw std::runtime_error("Expected ';' after return at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

    emitToken("symbol", ";");
    tokenizer.advance();
    emitClose("returnStatement");
}

void CompilationEngine::compileExpression() {
    // term (op term)*
    emitOpen("expression");
    // term
    compileTerm();
    // (op term)*
    while (tokenizer.tokenType() == Type::t_SYMBOL && isOpChar(tokenizer.symbol())) {
        // op
        char op = tokenizer.symbol();
        emitToken("symbol", std::string(1, op));
        tokenizer.advance();

        // next term
        compileTerm();
        writeOp(op);
    }
    emitClose("expression");
}

void CompilationEngine::compileTerm() {
    // integerConstant|stringConstant|keywordConstant|varName '[' expression ']'|'(' expression ')'|(unaryOp term)|subroutineCall
    emitOpen("term");
    // integerConstant
    if (tokenizer.tokenType() == Type::t_INT_CONST) {

        int value = tokenizer.intVal();
        emitToken("integerConstant", std::to_string(value));
        vmwriter.writePush("constant", value);
        tokenizer.advance();
    }

    // stringConstant
    else if (tokenizer.tokenType() == Type::t_STRING_CONST) {

        std::string s = tokenizer.stringVal();
        emitToken("stringConstant", s);
        tokenizer.advance();

        // on vm side construct string with OS functions
        vmwriter.VMWriter::writePush("constant", static_cast<int>(s.size())); // size of string
        vmwriter.VMWriter::writeCall("String.new", 1); // OS call with 1 argument (size of string)
        for (char c : s) {
            vmwriter.VMWriter::writePush("constant", static_cast<int>(static_cast<unsigned char>(c)));
            vmwriter.VMWriter::writeCall("String.appendChar", 2);
        }
    }

    // keywordConstant
    else if (tokenizer.tokenType() == Type::t_KEYWORD &&
            (tokenizer.keyWord() == KeyWord::kw_TRUE ||
             tokenizer.keyWord() == KeyWord::kw_FALSE ||
             tokenizer.keyWord() == KeyWord::kw_NULL ||
             tokenizer.keyWord() == KeyWord::kw_THIS)) {
 
        KeyWord kw = tokenizer.keyWord();
        emitToken("keyword", keywordToString(kw));
        tokenizer.advance();

        switch (kw) {
            case KeyWord::kw_TRUE:
                // true is -1
                vmwriter.VMWriter::writePush("constant", 0);
                vmwriter.VMWriter::writeArithmetic("not");
                break;
            case KeyWord::kw_FALSE: vmwriter.VMWriter::writePush("constant", 0); break;
            case KeyWord::kw_NULL: vmwriter.VMWriter::writePush("constant", 0); break;
            case KeyWord::kw_THIS: vmwriter.VMWriter::writePush("pointer", 0); break;
            default: break;
        }
    }

    // '(' expression ')'
    else if (tokenizer.tokenType() == Type::t_SYMBOL &&
             tokenizer.symbol() == '(') {

        emitToken("symbol", "(");
        tokenizer.advance();
        compileExpression();

        if (tokenizer.tokenType() != Type::t_SYMBOL ||
            tokenizer.symbol() != ')')
            throw std::runtime_error("Expected ')' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");

        emitToken("symbol", ")");
        tokenizer.advance();
    }

    // unaryOp term
    else if (tokenizer.tokenType() == Type::t_SYMBOL &&
            (tokenizer.symbol() == '-' || tokenizer.symbol() == '~')) {

        char op = tokenizer.symbol();
        emitToken("symbol", std::string(1, op));
        tokenizer.advance();
        compileTerm();
        writeUnaryOp(op);
    }

    // varName | array access | subroutineCall
    else if (tokenizer.tokenType() == Type::t_IDENTIFIER) {

        std::string name = tokenizer.identifier();
        tokenizer.advance();

        if (tokenizer.tokenType() == Type::t_SYMBOL &&
            tokenizer.symbol() == '[') {

            // name '[' expression ']'
            // 'name' must be a var-like identifier
            emitIdentifier(name, IdentifierUsage::iu_USED, IdentifierRole::ir_VARLIKE);
            emitToken("symbol", "[");
            tokenizer.advance();
            // base address
            pushVar(name);
            // index
            compileExpression();
            // base + index
            vmwriter.VMWriter::writeArithmetic("add");

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
            tokenizer.symbol() != ']') {
                throw std::runtime_error(
                    "Expected ']' in array indexing at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
            }

            emitToken("symbol", "]");
            tokenizer.advance();
            // *that 0
            vmwriter.VMWriter::writePop("pointer", 1);
            vmwriter.VMWriter::writePush("that", 0);
        }
        else if (tokenizer.tokenType() == Type::t_SYMBOL &&
                 tokenizer.symbol() == '(') {

            // subroutineName '(' expressionList ')'
            emitIdentifier(name, IdentifierUsage::iu_USED, IdentifierRole::ir_SUBROUTINENAME);
            emitToken("symbol", "(");
            tokenizer.advance();
            // push this
            vmwriter.VMWriter::writePush("pointer", 0);
            int nArgs = compileExpressionList();

            if (tokenizer.tokenType() != Type::t_SYMBOL ||
                tokenizer.symbol() != ')') {

                throw std::runtime_error(
                    "Expected ')' after expressionList at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
            }

            emitToken("symbol", ")");
            tokenizer.advance();
            vmwriter.VMWriter::writeCall(class_name + "." + name, nArgs + 1);
        }
        else if (tokenizer.tokenType() == Type::t_SYMBOL &&
                 tokenizer.symbol() == '.') {
            // (className | varName) '.' subroutineName '(' expressionList ')'
            // decide if 'name' is a className or a var-like symbol
            Kind k = subroutine_symbol_table.kindOf(name);
            bool isVar = (k != Kind::k_NONE);
            if (!isVar)
                k = class_symbol_table.kindOf(name);

            if (!isVar && k == Kind::k_NONE) {
                // treat as className
                emitIdentifier(name, IdentifierUsage::iu_USED, IdentifierRole::ir_CLASSNAME);
            } 
            else {
                emitIdentifier(name, IdentifierUsage::iu_USED, IdentifierRole::ir_VARLIKE);
                // push object reference
                pushVar(name);
            }

            emitToken("symbol", ".");
            tokenizer.advance();

            if (tokenizer.tokenType() != Type::t_IDENTIFIER) {
                throw std::runtime_error(
                    "Expected subroutineName after '.' at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
            }

            std::string subName = tokenizer.identifier();
            emitIdentifier(subName, IdentifierUsage::iu_USED, IdentifierRole::ir_SUBROUTINENAME);
            tokenizer.advance();

            if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '(') {
                throw std::runtime_error(
                    "Expected '(' after subroutineName at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
            }

            emitToken("symbol", "(");
            tokenizer.advance();
            int nArgs = compileExpressionList();

            if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != ')') {
                throw std::runtime_error("Expected ')' after expressionList at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
            }

            emitToken("symbol", ")");
            tokenizer.advance();
            int totalArgs = nArgs;
            std::string fullName;

            if (!isVar && k == Kind::k_NONE) {
                // class call
                fullName = name + "." + subName;
            } 
            else {
                // method call on object var
                std::string type = (subroutine_symbol_table.kindOf(name) != Kind::k_NONE)
                    ? subroutine_symbol_table.typeOf(name) : class_symbol_table.typeOf(name);

                fullName = type + "." + subName;
                totalArgs = nArgs + 1; // implicit object ref already pushed
            }

            vmwriter.VMWriter::writeCall(fullName, totalArgs);
        }
        else {
            // plain varName
            emitIdentifier(name, IdentifierUsage::iu_USED, IdentifierRole::ir_VARLIKE);
            pushVar(name);
        }
    }
    else {
        throw std::runtime_error( "Invalid term at line " + std::to_string(tokenizer.line_number) + ".\n > " + tokenizer.current_line + ".\n");
    }

    emitClose("term");
}

int CompilationEngine::compileExpressionList() {
    // (expression(',' expression)*)?
    emitOpen("expressionList");
    size_t count = 0;
    // empty? next token must be ')'
    if (tokenizer.tokenType() == Type::t_SYMBOL &&
        tokenizer.symbol() == ')') {
        emitClose("expressionList");
        return 0;
    }

    // expression
    compileExpression();
    ++count;

    // (',' expression)*
    while (tokenizer.tokenType() == Type::t_SYMBOL &&
        tokenizer.symbol() == ',') {
        emitToken("symbol", ",");
        tokenizer.advance();
        compileExpression();
        ++count;
    }
    emitClose("expressionList");
    return count;
}
