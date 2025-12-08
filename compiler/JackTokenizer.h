#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <sstream>

enum class Type {
    t_KEYWORD, t_SYMBOL, t_IDENTIFIER,
    t_INT_CONST, t_STRING_CONST
};

enum class KeyWord {
    kw_CLASS, kw_METHOD, kw_FUNCTION, kw_CONSTRUCTOR,
    kw_INT, kw_BOOLEAN, kw_CHAR, kw_VOID,
    kw_VAR, kw_STATIC, kw_FIELD,
    kw_LET, kw_DO, kw_IF, kw_ELSE,
    kw_WHILE, kw_RETURN,
    kw_TRUE, kw_FALSE, kw_NULL, kw_THIS
};

class JackTokenizer {
public:
    JackTokenizer(std::filesystem::path file, bool emit_xml = false);
    ~JackTokenizer();

    bool hasMoreTokens();
    void advance();
    std::string xml_escape(const std::string& token);
    void xml_emitter();
    
    std::string tokenizer();
    // token accessors
    Type tokenType();
    KeyWord keyWord();
    char symbol();
    std::string identifier();
    int intVal();
    std::string stringVal();

    std::ifstream jack_file;
    std::ofstream t_xml_file;
    std::filesystem::path path;

    std::string current_line;
    unsigned int line_number;
private:
    std::istringstream token_builder;
    std::string current_token;
    Type current_type;
    KeyWord current_keyword;
    bool in_comment_block = false;
    bool emit_xml_flag = false;
    static std::string trim(const std::string &s);

    void commentAnalyzer();
    void processCurrentToken(std::string token);

    bool is_delim(char c);
    bool is_symbol(const std::string& s);
    bool is_keyword(const std::string& s);
    bool is_int(const std::string& s);
    bool is_string(const std::string& s);
    bool is_identifier(const std::string& s);

    uint16_t parse_uint15(const std::string current_token);
};