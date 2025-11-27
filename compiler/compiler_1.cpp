#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <sstream>

#include "compiler_1.h"
#include "TokenUtils.h"

class JackTokenizer {
    private:
        static std::string trim(const std::string &s) {
            auto start = s.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) return "";
            auto end = s.find_last_not_of(" \t\n\r");
            return s.substr(start, end - start + 1);
        }

        std::unordered_map<std::string, KeyWord> defined_keywords = {
            {"class", KeyWord::k_CLASS},
            {"method", KeyWord::k_METHOD},
            {"function", KeyWord::k_FUNCTION},
            {"constructor", KeyWord::k_CONSTRUCTOR},
            {"int", KeyWord::k_INT},
            {"boolean", KeyWord::k_BOOLEAN},
            {"char", KeyWord::k_CHAR},
            {"void", KeyWord::k_VOID},
            {"var", KeyWord::k_VAR},
            {"static", KeyWord::k_STATIC},
            {"field", KeyWord::k_FIELD},
            {"let", KeyWord::k_LET},
            {"do", KeyWord::k_DO},
            {"if", KeyWord::k_IF},
            {"else", KeyWord::k_ELSE},
            {"while", KeyWord::k_WHILE},
            {"return", KeyWord::k_RETURN},
            {"true", KeyWord::k_TRUE},
            {"false", KeyWord::k_FALSE},
            {"null", KeyWord::k_NULL},
            {"this", KeyWord::k_THIS}
        };

    public:
        std::ifstream jack_file;
        std::ofstream t_xml_file;
        std::string jack_file_name;
        std::string current_line;
        std::istringstream token_builder;
        std::string current_token;
        Type current_type;
        KeyWord current_keyword;
        bool in_comment_block = false;

        JackTokenizer(const std::string& file) : jack_file_name(file){ 
            // open input file
            jack_file.open(file);
            if (!jack_file.is_open()) {
                std::cerr << "[error] unable to open input file: " << file << ".\n";
                exit(1);
            }
            size_t dot = jack_file_name.find_last_of('.');
            std::string base = jack_file_name.substr(0, dot);
            std::string t_xml_file_name = base + "T.xml";
            // open output file
            t_xml_file.open(t_xml_file_name);
            if (!t_xml_file.is_open()) {
                std::cerr << "[error] cannot create XML file: " << t_xml_file_name << " \n";
                std::exit(1);
            }
            // opening format
            t_xml_file << '<' << "tokens" << '>' << '\n';
        }
        ~JackTokenizer() {
            // closing format
            t_xml_file << "</" << "tokens" << '>' << '\n';

            if (jack_file.is_open())
                jack_file.close();
            if (t_xml_file.is_open())
                t_xml_file.close();
        }

        bool hasMoreTokens() { return jack_file.peek() != EOF; }

        void advance() {
            while (true) {
                std::string t = tokenizer(); // get raw token
                if (t.empty()) { // skip empty lines / EOF
                    if (jack_file.eof()) return;
                    continue; // skip blank
                }
                current_token = t; // assign valid token
                processCurrentToken(current_token);
                xml_emitter();
                return;
            }
        }

        inline std::string xml_escape(const std::string& token) {
            if (is_symbol(token)) {
                switch (token[0]) {
                    case '<': return "&lt;";
                    case '>': return "&gt;";
                    case '"': return "&quot;";
                    case '&': return "&amp;";
                    default:  return token;
                }
            }
            return token;
        }

        inline void xml_emitter() {
            std::string type = typeToString(current_type);
            std::string keyword = keywordToString(current_keyword);
            // special tokens (xml markup specific)
            // check if keyword to call keyword
            // emitter
            if (current_type == Type::t_KEYWORD)
                t_xml_file << "<keyword> " << keyword << " </keyword>\n";
            else {
                std::string printed = xml_escape(current_token);
                t_xml_file << '<' << type << "> "
                           << printed
                           << " </" << type << ">\n";
            }
        }

        inline bool is_delim(const char c) {
            static const std::string delimiters = "{}()[].,;+-*/&|<>~= ";
            return delimiters.find(c) != std::string::npos;
        }

        inline bool is_symbol(const std::string s) {
            if (s.size() != 1)
                return false;
            static const std::string symbols = "{}()[].,;+-*/&|<>~=";
            return symbols.find(s[0]) != std::string::npos;
        }

        inline bool is_keyword(const std::string s) {
            return defined_keywords.find(s) != defined_keywords.end();
        }

        inline bool is_int(const std::string s) {
            std::string::const_iterator it = s.begin();
            while (it != s.end() && std::isdigit(*it)) ++it;
            return !s.empty() && it == s.end();
        }
        
        inline bool is_string(const std::string s) {
            return (s.front() == '"' && s.back() == '"');
        }

        inline bool is_identifier(const std::string s) {
            return !isdigit(s.front());
        }

        void processCurrentToken(std::string token) {
            // type classification
            if (is_keyword(token)) {  
                current_type = Type::t_KEYWORD;
                current_keyword = defined_keywords[token];
            }
            else if (is_symbol(token))
                current_type = Type::t_SYMBOL;
            else if (is_int(token))
                current_type = Type::t_INT_CONST;
            else if (is_string(token))
                current_type = Type::t_STRING_CONST;
            else if (is_identifier(token))
                current_type = Type::t_IDENTIFIER;
            else throw std::runtime_error("[error] token " + token + " has no valid type.\n");
        }

        void commentAnalyzer() {
            // if we are inside a multi-line block comment
            if (in_comment_block) {
                std::size_t pos_end = current_line.find("*/");
                if (pos_end != std::string::npos) {
                    // end the comment block
                    in_comment_block = false;
                    // remove everything up to and including "*/"
                    current_line = current_line.substr(pos_end+2);
                    if (!current_line.empty()) {
                        commentAnalyzer(); 
                    }
                } 
                else {
                    // whole line is inside a block comment
                    current_line.clear();
                }
                return;
            }

            // situation 1: single line comments "//" 
            if (auto pos = current_line.find("//"); pos != std::string::npos) {
                current_line = current_line.substr(0, pos);
                return;
            }

            // situation 2: inline block comment "/* ... */"
            std::size_t pos_start = current_line.find("/*");
            std::size_t pos_end   = current_line.find("*/");
            if (pos_start != std::string::npos &&
                pos_end   != std::string::npos &&
                pos_end > pos_start) {
                current_line = current_line.erase(pos_start, (pos_end - pos_start + 2));
                if (!current_line.empty()) {
                    commentAnalyzer();
                }
                return;
            }

            // situation 3: start of multi-line block comment
            if (pos_start != std::string::npos) {
                current_line = current_line.substr(0, pos_start);
                // entering comment_block
                in_comment_block = true;
                if (!current_line.empty()) {
                    commentAnalyzer();
                }
                return;
            }
        }

        std::string tokenizer() {
            // load a non-empty line
            while (current_line.empty()) {
                if (!std::getline(jack_file, current_line))
                    return {}; // EOF
                token_builder.clear();
                current_line = trim(current_line);
                commentAnalyzer();
                token_builder.str(current_line);
            }
            int ic = token_builder.get();
            // if end of line, return nothing
            if (ic == EOF) {
                current_line.clear();
                return tokenizer();
            }
            char c = static_cast<char>(ic);
            std::string token;
            if (c == '"') {
                token.push_back(c); // opening quote
                while (true) {
                    ic = token_builder.get();
                    if (ic == EOF) {
                        throw std::runtime_error("Unterminated string constant");
                    }
                    c = static_cast<char>(ic);
                    token.push_back(c);
                    if (c == '"') {
                        break; // closing quote
                    }
                }

                return token;
            }

            // if delimiter, return the symbol
            if (is_delim(c)) {
                if (c == ' ') {
                    return tokenizer(); // skip whitespace
                }
                return std::string(1, c);
            }
            token.push_back(c);
            while (true) {
                if (is_delim(token_builder.peek())) // check if next token is delimiter
                    break;
                ic = token_builder.get();
                if (ic == EOF) {
                    current_line.clear();
                    break;
                }
                c = static_cast<char>(ic);
                token.push_back(c);
            }
            return token;
        }

        inline Type tokenType() { return current_type; }

        inline KeyWord keyWord() { 
            if (current_type == Type::t_KEYWORD) 
                return current_keyword;
            else throw std::invalid_argument("[error] keyWord() method was called on non-keyword type.\n");
        }

        inline char symbol() { 
            if (current_type == Type::t_SYMBOL) 
                return current_token[0];
            else throw std::invalid_argument("[error] symbol() method was called on non-symbol type.\n");
        }
        inline std::string identifier() {
            if (current_type == Type::t_IDENTIFIER)
                return current_token;
            else throw std::invalid_argument("[error] identifer() method was called on non-identifier type.\n");
        }

        inline uint16_t parse_uint15(const std::string current_token) {
            unsigned long value = std::stoul(current_token);
            constexpr unsigned long LIMIT = 1ul << 15;  // 32768
            if (value > LIMIT)
                throw std::out_of_range("[error] integer " + current_token + " exceeds 2^15 hardware limit.\n");
            return static_cast<uint16_t>(value);
        }

        inline int intVal() {
            if (current_type == Type::t_INT_CONST)
                return parse_uint15(current_token);
            else throw std::invalid_argument("[error] intVal() method was called on non-int type.\n");
        }

        inline std::string stringVal() {
            if (current_type == Type::t_STRING_CONST)
                return current_token.substr(1, current_token.size()-2); // removes " "
            else throw std::invalid_argument("[error] stringVal() method was called on non-string type.\n");
        }
};


class CompilationEngine {        
    public:
        std::ofstream xml_file;
        JackTokenizer& tokenizer;
        size_t indent_level = 0;

        CompilationEngine(JackTokenizer& jack_tokenizer) : tokenizer(jack_tokenizer) {
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
        ~CompilationEngine() {
            if (xml_file.is_open())
                xml_file.close();
        }

        void compile() {
            compileClass(); // compileClass(); calls all other compilation processes
        }

    private:
        inline void writeIndent() {
            for (size_t i = 0; i < indent_level; ++i)
                xml_file << '\t';
        }

        inline void writeOpen(const std::string& tag) {
            writeIndent();
            xml_file << '<' << tag << '>' << '\n';
            ++indent_level;
        }

        inline void writeClose(const std::string& tag) {
            --indent_level;
            writeIndent();
            xml_file << "</" << tag << ">\n";
        }

        inline void writeToken(const std::string& tag, const std::string& token) {
            writeIndent();
            xml_file << '<' << tag << "> "
                     << tokenizer.xml_escape(token)
                     << " </" << tag << ">\n";
        }

        void compileClass() {
            // 'class' className '{' classVarDec* subroutineDec* '}'
            writeOpen("class");
            // 'class'
            if (tokenizer.tokenType() != Type::t_KEYWORD ||
                tokenizer.keyWord() != KeyWord::k_CLASS) {
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
                (tokenizer.keyWord() == KeyWord::k_STATIC ||
                tokenizer.keyWord() == KeyWord::k_FIELD)) {
                compileClassVarDec();
            }

            // subroutineDec*
            while (tokenizer.tokenType() == Type::t_KEYWORD &&
                (tokenizer.keyWord() == KeyWord::k_CONSTRUCTOR ||
                tokenizer.keyWord() == KeyWord::k_FUNCTION ||
                tokenizer.keyWord() == KeyWord::k_METHOD)) {
                compileSubroutine();
            }

            // '}'
            if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '}')
                throw std::runtime_error("Expected '}'");

            writeToken("symbol", "}");
            tokenizer.advance();
            writeClose("class");
        }

        void compileClassVarDec() {
            // ('static'|'field') type varName(',' varName)* ';'
            writeOpen("classVarDec");
            // static | field
            if (tokenizer.tokenType() != Type::t_KEYWORD ||
                (tokenizer.keyWord() != KeyWord::k_STATIC &&
                tokenizer.keyWord() != KeyWord::k_FIELD)) {
                throw std::runtime_error("Expected 'static' or 'field' in classVarDec");
            }

            writeToken("keyword", keywordToString(tokenizer.keyWord()));
            tokenizer.advance();

            // type  (keyword type or identifier type)
            if (tokenizer.tokenType() == Type::t_KEYWORD &&
                (tokenizer.keyWord() == KeyWord::k_INT ||
                tokenizer.keyWord() == KeyWord::k_CHAR ||
                tokenizer.keyWord() == KeyWord::k_BOOLEAN)) {

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

        void compileSubroutine() {
            // ('constructor'|'functon'|'method') ('void'|type) subroutineName '(' parameterList ')' subroutineBody
            writeOpen("subroutineDec");
            // ('constructor'|'function'|'method')
            if (tokenizer.tokenType() != Type::t_KEYWORD ||
                (tokenizer.keyWord() != KeyWord::k_CONSTRUCTOR &&
                tokenizer.keyWord() != KeyWord::k_FUNCTION &&
                tokenizer.keyWord() != KeyWord::k_METHOD)) {
                throw std::runtime_error("Expected constructor/function/method");
            }

            writeToken("keyword", keywordToString(tokenizer.keyWord()));
            tokenizer.advance();

            // ('void' | type)
            if (tokenizer.tokenType() == Type::t_KEYWORD &&
                tokenizer.keyWord() == KeyWord::k_VOID) {

                writeToken("keyword", "void");
                tokenizer.advance();
            }
            else if (tokenizer.tokenType() == Type::t_KEYWORD &&
                    (tokenizer.keyWord() == KeyWord::k_INT ||
                    tokenizer.keyWord() == KeyWord::k_CHAR ||
                    tokenizer.keyWord() == KeyWord::k_BOOLEAN)) {

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

        void compileParameterList() {
            // ((type varName) (',' type varName)*)?
            writeOpen("parameterList");
            // empty?
            if (!(
                (tokenizer.tokenType() == Type::t_KEYWORD &&
                (tokenizer.keyWord() == KeyWord::k_INT ||
                tokenizer.keyWord() == KeyWord::k_CHAR ||
                tokenizer.keyWord() == KeyWord::k_BOOLEAN))
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
                    (tokenizer.keyWord() == KeyWord::k_INT ||
                    tokenizer.keyWord() == KeyWord::k_CHAR ||
                    tokenizer.keyWord() == KeyWord::k_BOOLEAN)) {

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

        void compileSubroutineBody() {
            // '{' varDec* statements '}'
            writeOpen("subroutineBody");
            // '{'
            if (tokenizer.tokenType() != Type::t_SYMBOL || tokenizer.symbol() != '{')
                throw std::runtime_error("Expected '{' at start of subroutineBody");

            writeToken("symbol", "{");
            tokenizer.advance();

            // varDec*
            while (tokenizer.tokenType() == Type::t_KEYWORD &&
                tokenizer.keyWord() == KeyWord::k_VAR) {
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

        void compileVarDec() {
            // 'var' type varName(',' varName)* ';'
            writeOpen("varDec");
            // 'var'
            if (tokenizer.tokenType() != Type::t_KEYWORD ||
                tokenizer.keyWord() != KeyWord::k_VAR)
                throw std::runtime_error("Expected 'var' at start of varDec");

            writeToken("keyword", "var");
            tokenizer.advance();

            // type (int | char | boolean | className)
            if (tokenizer.tokenType() == Type::t_KEYWORD &&
            (tokenizer.keyWord() == KeyWord::k_INT ||
                tokenizer.keyWord() == KeyWord::k_CHAR ||
                tokenizer.keyWord() == KeyWord::k_BOOLEAN))
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

        void compileStatements() {
            // statement*
            writeOpen("statements");
            // letStatement|ifStatement|whileStatement|doStatement|returnStatement
            while (tokenizer.tokenType() == Type::t_KEYWORD) {
                switch (tokenizer.keyWord()) {
                    case KeyWord::k_LET:
                        compileLet();
                        break;

                    case KeyWord::k_IF:
                        compileIf();
                        break;

                    case KeyWord::k_WHILE:
                        compileWhile();
                        break;

                    case KeyWord::k_DO:
                        compileDo();
                        break;

                    case KeyWord::k_RETURN:
                        compileReturn();
                        break;
                }
            }
            writeClose("statements");
        }

        void compileLet() {
            // 'let' varName('[' expression ']')? '=' expression ';'
            writeOpen("letStatement");
            // 'let'
            if (tokenizer.tokenType() != Type::t_KEYWORD ||
                tokenizer.keyWord() != KeyWord::k_LET)
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

        void compileIf() {
            // 'if' '(' expression ')' '{' statements '}' ('else' '{' statements '}')?
            writeOpen("ifStatement");
            // 'if'
            if (tokenizer.tokenType() != Type::t_KEYWORD ||
                tokenizer.keyWord() != KeyWord::k_IF)
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
                tokenizer.keyWord() == KeyWord::k_ELSE)
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

        void compileWhile() {
            // 'while' '(' expression ')' '{' statements '}'
            writeOpen("whileStatement");
            // 'while'
            if (tokenizer.tokenType() != Type::t_KEYWORD ||
                tokenizer.keyWord() != KeyWord::k_WHILE)
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

        void compileDo() {
            // 'do' subroutineCall ';'
            writeOpen("doStatement");
            // 'do'
            if (tokenizer.tokenType() != Type::t_KEYWORD ||
                tokenizer.keyWord() != KeyWord::k_DO)
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
            (tokenizer.keyWord() == KeyWord::k_TRUE ||
                tokenizer.keyWord() == KeyWord::k_FALSE ||
                tokenizer.keyWord() == KeyWord::k_NULL ||
                tokenizer.keyWord() == KeyWord::k_THIS)) {

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
            (tokenizer.keyWord() == KeyWord::k_TRUE ||
                tokenizer.keyWord() == KeyWord::k_FALSE ||
                tokenizer.keyWord() == KeyWord::k_NULL ||
                tokenizer.keyWord() == KeyWord::k_THIS)) {

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

        void compileReturn() {
            // 'return' expression? ';'
            writeOpen("returnStatement");
            // 'return'
            if (tokenizer.tokenType() != Type::t_KEYWORD ||
                tokenizer.keyWord() != KeyWord::k_RETURN)
                throw std::runtime_error("Expected 'return'");

            writeToken("keyword", "return");
            tokenizer.advance();

            // optional expression
            // If next token is not ';', we must have an expression
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

        void compileExpression() {
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
    
        void compileTerm() {
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
            (tokenizer.keyWord() == KeyWord::k_TRUE ||
                tokenizer.keyWord() == KeyWord::k_FALSE ||
                tokenizer.keyWord() == KeyWord::k_NULL ||
                tokenizer.keyWord() == KeyWord::k_THIS)) {

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

        int compileExpressionList() {
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
};

// main program that sets up and invokes the other modules
// class JackAnalyzer {};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./compiler <file.jack>\n";
        return 1;
    }

    // create tokenizer
    JackTokenizer tokenizer(argv[1]);

    // start tokenizer
    tokenizer.advance();

    // create compilation engine
    CompilationEngine engine(tokenizer);

    // start compilation at the grammar's root rule
    engine.compile();

    return 0;
}
