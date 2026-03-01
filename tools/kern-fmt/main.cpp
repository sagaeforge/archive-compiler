#include "kern/fmt/Formatter.h"
#include "kern/lexer/Lexer.h"
#include "kern/parser/Parser.h"
#include "kern/support/Arena.h"
#include "kern/support/Diagnostic.h"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace kern;

static void printUsage() {
    std::cerr << "Usage: kern-fmt [options] <file.kern>\n"
              << "Options:\n"
              << "  -i            Format in place\n"
              << "  --check       Check if file is formatted (exit 1 if not)\n"
              << "  --indent <n>  Indent width (default: 4)\n"
              << "  --help        Show this help\n";
}

int main(int argc, char** argv) {
    bool in_place = false;
    bool check_only = false;
    uint32_t indent_width = 4;
    const char* input_file = nullptr;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "-i") {
            in_place = true;
        } else if (arg == "--check") {
            check_only = true;
        } else if (arg == "--indent" && i + 1 < argc) {
            indent_width = static_cast<uint32_t>(std::stoi(argv[++i]));
        } else if (arg == "--help") {
            printUsage();
            return 0;
        } else if (arg[0] != '-') {
            input_file = argv[i];
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage();
            return 1;
        }
    }

    if (!input_file) {
        std::cerr << "Error: no input file\n";
        printUsage();
        return 1;
    }

    std::ifstream ifs(input_file);
    if (!ifs) {
        std::cerr << "Error: cannot open " << input_file << "\n";
        return 1;
    }
    std::string source((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
    ifs.close();

    Arena arena;
    DiagnosticEngine diag;
    diag.setSource(source);

    Lexer lexer(source, input_file, diag);
    Parser parser(lexer, arena, diag);
    auto* mod = parser.parseModule();

    if (diag.hasErrors()) {
        diag.printAll(std::cerr);
        return 1;
    }

    FormatOptions opts;
    opts.indent_width = indent_width;

    std::ostringstream formatted;
    Formatter fmt(formatted, opts);
    fmt.formatModule(mod);

    if (check_only) {
        return (formatted.str() == source) ? 0 : 1;
    }

    if (in_place) {
        std::ofstream ofs(input_file);
        ofs << formatted.str();
    } else {
        std::cout << formatted.str();
    }

    return 0;
}
