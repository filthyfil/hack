#include "JackAnalyzer.h"


JackAnalyzer::JackAnalyzer() :
    class_symbol_table{},
    subroutine_symbol_table{} {
    }

void JackAnalyzer::compile() {
    class_symbol_table.reset();
    subroutine_symbol_table.reset();
}