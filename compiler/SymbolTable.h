#pragma once

enum class Kind {
    s_STATIC, s_FIELD,
    s_ARG, s_VAR,
    s_NONE
};

class SymbolTable;