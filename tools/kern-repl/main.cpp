#include "kern/pipeline/CompilerPipeline.h"
#include "kern/support/CompilationContext.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>

// kern-repl: Read-Eval-Print Loop for Kern.
// Each expression/statement is compiled as a full program and executed.
// Function/struct/union/enum definitions persist across evaluations.
// Full incremental compilation requires Arena generations (future work).

using namespace kern;

namespace {

uint32_t expr_counter = 0;

// Persistent definitions accumulated across REPL interactions
std::string accumulated_defs;

bool isDefinition(const std::string& input) {
    // Check if input starts with fn, struct, enum, union, trait, impl, type, newtype
    auto pos = input.find_first_not_of(" \t\n");
    if (pos == std::string::npos) return false;
    auto rest = input.substr(pos);
    return rest.rfind("fn ", 0) == 0 ||
           rest.rfind("struct ", 0) == 0 ||
           rest.rfind("enum ", 0) == 0 ||
           rest.rfind("union ", 0) == 0 ||
           rest.rfind("trait ", 0) == 0 ||
           rest.rfind("impl ", 0) == 0 ||
           rest.rfind("type ", 0) == 0 ||
           rest.rfind("newtype ", 0) == 0 ||
           rest.rfind("@", 0) == 0;  // annotations like @interrupt
}

bool isStatement(const std::string& input) {
    // val/var declarations are statements, not expressions
    auto pos = input.find_first_not_of(" \t\n");
    if (pos == std::string::npos) return false;
    auto rest = input.substr(pos);
    return rest.rfind("val ", 0) == 0 ||
           rest.rfind("var ", 0) == 0;
}

bool isComplete(const std::string& input) {
    // Simple heuristic: count braces to detect multi-line input
    int braces = 0;
    bool in_string = false;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '"' && (i == 0 || input[i - 1] != '\\')) {
            in_string = !in_string;
        }
        if (!in_string) {
            if (c == '{') braces++;
            else if (c == '}') braces--;
        }
    }
    return braces <= 0;
}

int evalSource(const std::string& source, bool quiet = false) {
    std::string tmp_base = "/tmp/kern_repl_" + std::to_string(expr_counter);
    std::string tmp_kern = tmp_base + ".kern";
    std::string tmp_bin = tmp_base;
    ++expr_counter;

    {
        std::ofstream ofs(tmp_kern);
        ofs << source;
    }

    CompilationContext ctx;
    CompilerPipeline pipeline(ctx);

    CompileOptions opts;
    opts.input_file = tmp_kern;
    opts.output_file = tmp_bin;

    std::ostringstream out_stream, err_stream;
    int rc = pipeline.run(source, opts, out_stream, err_stream);

    if (rc != 0) {
        std::string err = err_stream.str();
        if (!err.empty() && !quiet) std::cerr << err;
        std::filesystem::remove(tmp_kern);
        return -1;
    }

    // Execute the binary
    int exec_rc = std::system(tmp_bin.c_str());
    int exit_code = WEXITSTATUS(exec_rc);

    // Cleanup
    std::filesystem::remove(tmp_kern);
    std::filesystem::remove(tmp_bin);
    std::filesystem::remove(tmp_base + ".asm");
    std::filesystem::remove(tmp_base + ".o");

    return exit_code;
}

int evalExpression(const std::string& expr) {
    std::string source = accumulated_defs +
        "fn main() -> i64 {\n    " + expr + "\n}\n";
    return evalSource(source);
}

int evalStatement(const std::string& stmt) {
    // Statements like val/var: wrap in main that returns 0
    std::string source = accumulated_defs +
        "fn main() -> i64 {\n    " + stmt + "\n    0\n}\n";
    return evalSource(source);
}

bool evalDefinition(const std::string& def) {
    // Try compiling with the new definition to check for errors
    std::string test_source = accumulated_defs + def + "\n" +
        "fn main() -> i64 { 0 }\n";

    int rc = evalSource(test_source, true);
    if (rc < 0) {
        // Re-run to show errors
        evalSource(test_source, false);
        return false;
    }

    // Definition is valid — add to persistent state
    accumulated_defs += def + "\n\n";
    return true;
}

void printDefs() {
    if (accumulated_defs.empty()) {
        std::cout << "(no definitions)\n";
    } else {
        std::cout << accumulated_defs;
    }
}

} // anonymous namespace

int main() {
    std::cout << "kern-repl v0.2.0\n"
              << "Type expressions to evaluate (result = exit code 0-255).\n"
              << "Function/struct/union/enum definitions persist across lines.\n"
              << "Commands: :quit :help :defs :clear\n\n";

    std::string line;
    std::string buffer;
    bool in_multiline = false;

    while (true) {
        std::cout << (in_multiline ? "...   " : "kern> ");
        if (!std::getline(std::cin, line)) break;

        if (!in_multiline && line.empty()) continue;

        // Commands (only at top level)
        if (!in_multiline) {
            if (line == ":quit" || line == ":q") break;
            if (line == ":help") {
                std::cout << "  :quit    Exit REPL\n"
                          << "  :defs    Show accumulated definitions\n"
                          << "  :clear   Clear all definitions\n"
                          << "  :help    Show this help\n"
                          << "  fn/struct/enum/union ...  Define (persists)\n"
                          << "  <expr>   Evaluate expression\n";
                continue;
            }
            if (line == ":defs") {
                printDefs();
                continue;
            }
            if (line == ":clear") {
                accumulated_defs.clear();
                std::cout << "Definitions cleared.\n";
                continue;
            }
        }

        buffer += line + "\n";

        if (!isComplete(buffer)) {
            in_multiline = true;
            continue;
        }
        in_multiline = false;

        // Trim trailing whitespace
        auto input = buffer;
        while (!input.empty() && (input.back() == '\n' || input.back() == ' '))
            input.pop_back();

        // Determine if this is a definition, statement, or expression
        if (isDefinition(input)) {
            if (evalDefinition(input)) {
                std::cout << "defined\n";
            }
        } else if (isStatement(input)) {
            int result = evalStatement(input);
            if (result >= 0) {
                std::cout << "ok\n";
            }
        } else {
            int result = evalExpression(input);
            if (result >= 0) {
                std::cout << "= " << result << "\n";
            }
        }
        buffer.clear();
    }

    return 0;
}
