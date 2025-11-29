#include "CompilationEngine.h"

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
