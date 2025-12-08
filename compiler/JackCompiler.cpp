#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "CompilationEngine.h"

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: ./compiler <source> [--xml]\n"
                  << "  where <source> is either:\n"
                  << "    - a single .jack file, or\n"
                  << "    - a directory containing one or more .jack files\n";
        return 1;
    }
    bool emit_xml = false;
    if (argc == 3 && std::string(argv[2]) == "--xml") emit_xml = true;

    std::filesystem::path source_path = argv[1];

    if (!std::filesystem::exists(source_path)) {
        std::cerr << "[error] Path does not exist: " << source_path.string() << "\n";
        return 1;
    }

    std::vector<std::filesystem::path> jack_files;

    if (std::filesystem::is_directory(source_path)) {
        // collect all .jack files in the directory
        for (const auto& entry : std::filesystem::directory_iterator(source_path)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() == ".jack") {
                jack_files.push_back(entry.path());
            }
        }

        if (jack_files.empty()) {
            std::cerr << "[error] No .jack files found in directory: " << source_path.string() << "\n";
            return 1;
        }
    } else {
        // must be a single .jack file
        if (source_path.extension() != ".jack") {
            std::cerr << "[error] Expected a .jack file or a directory, got: " << source_path.string() << "\n";
            return 1;
        }
        jack_files.push_back(source_path);
    }

    // process each Xxx.jack file to Xxx.xml / Xxx.vm in folder
    for (const auto& jack_file : jack_files) {
        try {
            // create tokenizer for this file
            JackTokenizer tokenizer(jack_file, emit_xml);
            // start tokenizer
            tokenizer.advance();

            // create compilation engine (produce base.xml / base.vm)
            CompilationEngine engine(tokenizer, emit_xml);

            // start compilation at root rule
            engine.compile();

            std::filesystem::path output_path = jack_file.parent_path();
            std::cout << "Compilation Successful. Output written to: " << output_path.string() << "\n";
        } 
        catch (const std::exception& e) {
            std::cerr << "[error] While compiling " << jack_file.string() << ":\n"
                      << "  " << e.what() << "\n";
            return 1;
        }
    }

    return 0;
}
