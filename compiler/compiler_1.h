#pragma once

enum class Type {
    t_KEYWORD, t_SYMBOL, t_IDENTIFIER,
    t_INT_CONST, t_STRING_CONST
};

enum class KeyWord {
    k_CLASS, k_METHOD, k_FUNCTION, k_CONSTRUCTOR,
    k_INT, k_BOOLEAN, k_CHAR, k_VOID,
    k_VAR, k_STATIC, k_FIELD,
    k_LET, k_DO, k_IF, k_ELSE,
    k_WHILE, k_RETURN,
    k_TRUE, k_FALSE, k_NULL, k_THIS
};

class JackTokenizer;
class CompilationEngine;