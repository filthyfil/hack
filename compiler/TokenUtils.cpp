#include "TokenUtils.h"

std::string keywordToString(KeyWord kw) {
    switch (kw) {
        case KeyWord::k_CLASS:       return "class";
        case KeyWord::k_METHOD:      return "method";
        case KeyWord::k_FUNCTION:    return "function";
        case KeyWord::k_CONSTRUCTOR: return "constructor";
        case KeyWord::k_INT:         return "int";
        case KeyWord::k_BOOLEAN:     return "boolean";
        case KeyWord::k_CHAR:        return "char";
        case KeyWord::k_VOID:        return "void";
        case KeyWord::k_VAR:         return "var";
        case KeyWord::k_STATIC:      return "static";
        case KeyWord::k_FIELD:       return "field";
        case KeyWord::k_LET:         return "let";
        case KeyWord::k_DO:          return "do";
        case KeyWord::k_IF:          return "if";
        case KeyWord::k_ELSE:        return "else";
        case KeyWord::k_WHILE:       return "while";
        case KeyWord::k_RETURN:      return "return";
        case KeyWord::k_TRUE:        return "true";
        case KeyWord::k_FALSE:       return "false";
        case KeyWord::k_NULL:        return "null";
        case KeyWord::k_THIS:        return "this";
        default:                     return "invalid";
    }
}

std::string typeToString(Type t) {
    switch (t) {
        case Type::t_KEYWORD:       return "keyword";
        case Type::t_SYMBOL:        return "symbol";
        case Type::t_IDENTIFIER:    return "identifier";
        case Type::t_INT_CONST:     return "integerConstant";
        case Type::t_STRING_CONST:  return "stringConstant";
        default:                    return "invalid";
    }
}
