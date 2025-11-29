#include "TokenUtils.h"

std::string keywordToString(KeyWord kw) {
    switch (kw) {
        case KeyWord::kw_CLASS:       return "class";
        case KeyWord::kw_METHOD:      return "method";
        case KeyWord::kw_FUNCTION:    return "function";
        case KeyWord::kw_CONSTRUCTOR: return "constructor";
        case KeyWord::kw_INT:         return "int";
        case KeyWord::kw_BOOLEAN:     return "boolean";
        case KeyWord::kw_CHAR:        return "char";
        case KeyWord::kw_VOID:        return "void";
        case KeyWord::kw_VAR:         return "var";
        case KeyWord::kw_STATIC:      return "static";
        case KeyWord::kw_FIELD:       return "field";
        case KeyWord::kw_LET:         return "let";
        case KeyWord::kw_DO:          return "do";
        case KeyWord::kw_IF:          return "if";
        case KeyWord::kw_ELSE:        return "else";
        case KeyWord::kw_WHILE:       return "while";
        case KeyWord::kw_RETURN:      return "return";
        case KeyWord::kw_TRUE:        return "true";
        case KeyWord::kw_FALSE:       return "false";
        case KeyWord::kw_NULL:        return "null";
        case KeyWord::kw_THIS:        return "this";
        default:                      return "invalid";
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
