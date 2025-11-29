#include "JackTokenizer.h"
#include "TokenUtils.h"

std::string JackTokenizer::trim(const std::string &s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

static std::unordered_map<std::string, KeyWord> defined_keywords = {
    {"class", KeyWord::kw_CLASS},
    {"method", KeyWord::kw_METHOD},
    {"function", KeyWord::kw_FUNCTION},
    {"constructor", KeyWord::kw_CONSTRUCTOR},
    {"int", KeyWord::kw_INT},
    {"boolean", KeyWord::kw_BOOLEAN},
    {"char", KeyWord::kw_CHAR},
    {"void", KeyWord::kw_VOID},
    {"var", KeyWord::kw_VAR},
    {"static", KeyWord::kw_STATIC},
    {"field", KeyWord::kw_FIELD},
    {"let", KeyWord::kw_LET},
    {"do", KeyWord::kw_DO},
    {"if", KeyWord::kw_IF},
    {"else", KeyWord::kw_ELSE},
    {"while", KeyWord::kw_WHILE},
    {"return", KeyWord::kw_RETURN},
    {"true", KeyWord::kw_TRUE},
    {"false", KeyWord::kw_FALSE},
    {"null", KeyWord::kw_NULL},
    {"this", KeyWord::kw_THIS}
};

JackTokenizer::JackTokenizer(const std::string& file) : jack_file_name(file){ 
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

JackTokenizer::~JackTokenizer() {
    // closing format
    t_xml_file << "</" << "tokens" << '>' << '\n';

    if (jack_file.is_open())
        jack_file.close();
    if (t_xml_file.is_open())
        t_xml_file.close();
}

bool JackTokenizer::hasMoreTokens() { return jack_file.peek() != EOF; }

void JackTokenizer::advance() {
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

std::string JackTokenizer::xml_escape(const std::string& token) {
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

void JackTokenizer::xml_emitter() {
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

bool JackTokenizer::is_delim(const char c) {
    static const std::string delimiters = "{}()[].,;+-*/&|<>~= ";
    return delimiters.find(c) != std::string::npos;
}

bool JackTokenizer::is_symbol(const std::string& s) {
    if (s.size() != 1)
        return false;
    static const std::string symbols = "{}()[].,;+-*/&|<>~=";
    return symbols.find(s[0]) != std::string::npos;
}

bool JackTokenizer::is_keyword(const std::string& s) {
    return defined_keywords.find(s) != defined_keywords.end();
}

bool JackTokenizer::is_int(const std::string& s) {
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

bool JackTokenizer::is_string(const std::string& s) {
    return (s.front() == '"' && s.back() == '"');
}

bool JackTokenizer::is_identifier(const std::string& s) {
    return !isdigit(s.front());
}

void JackTokenizer::processCurrentToken(std::string token) {
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

void JackTokenizer::commentAnalyzer() {
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

    // situation 2: block comment "/* ... */"
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

std::string JackTokenizer::tokenizer() {
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

Type JackTokenizer::tokenType() { return current_type; }

KeyWord JackTokenizer::keyWord() { 
    if (current_type == Type::t_KEYWORD) 
        return current_keyword;
    else throw std::invalid_argument("[error] keyWord() method was called on non-keyword type.\n");
}

char JackTokenizer::symbol() { 
    if (current_type == Type::t_SYMBOL) 
        return current_token[0];
    else throw std::invalid_argument("[error] symbol() method was called on non-symbol type.\n");
}
std::string JackTokenizer::identifier() {
    if (current_type == Type::t_IDENTIFIER)
        return current_token;
    else throw std::invalid_argument("[error] identifer() method was called on non-identifier type.\n");
}

uint16_t JackTokenizer::parse_uint15(const std::string current_token) {
    unsigned long value = std::stoul(current_token);
    constexpr unsigned long LIMIT = 1ul << 15;  // 32768
    if (value > LIMIT)
        throw std::out_of_range("[error] integer " + current_token + " exceeds 2^15 hardware limit.\n");
    return static_cast<uint16_t>(value);
}

int JackTokenizer::intVal() {
    if (current_type == Type::t_INT_CONST)
        return parse_uint15(current_token);
    else throw std::invalid_argument("[error] intVal() method was called on non-int type.\n");
}

std::string JackTokenizer::stringVal() {
    if (current_type == Type::t_STRING_CONST)
        return current_token.substr(1, current_token.size()-2); // removes " "
    else throw std::invalid_argument("[error] stringVal() method was called on non-string type.\n");
}