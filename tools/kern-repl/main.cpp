#include "kern/pipeline/CompilerPipeline.h"
#include "kern/support/CompilationContext.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>

// kern-repl: Read-Eval-Print Loop for Kern.
// Each expression is compiled as a full program and executed.
// Full incremental compilation requires Arena generations (future work).

using namespace kern;

namespace {

uint32_t expr_counter = 0;

int evalExpression(const std::string& expr) {
    std::string source = "fn main() -> i64 {\n    " + expr + "\n}\n";

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
        if (!err.empty()) std::cerr << err;
        std::filesystem::remove(tmp_kern);
        return -1;
    }

    // Execute the binary
    int exec_rc = std::system(tmp_bin.c_str());
    int exit_code = WEXITSTATUS(exec_rc);

    // Cleanup
    std::filesystem::remove(tmp_kern);
    std::filesystem::remove(tmp_bin);
    // Remove intermediate files
    std::filesystem::remove(tmp_base + ".asm");
    std::filesystem::remove(tmp_base + ".o");

    return exit_code;
}

} // anonymous namespace

int main() {
    std::cout << "kern-repl v0.1.0\n"
              << "Type expressions to evaluate. Results shown as exit codes (0-255).\n"
              << "Type :quit to exit.\n\n";

    std::string line;
    while (true) {
        std::cout << "kern> ";
        if (!std::getline(std::cin, line)) break;

        if (line.empty()) continue;
        if (line == ":quit" || line == ":q") break;
        if (line == ":help") {
            std::cout << "  :quit    Exit REPL\n"
                      << "  :help    Show this help\n"
                      << "  <expr>   Evaluate expression (result = exit code)\n";
            continue;
        }

        int result = evalExpression(line);
        if (result >= 0) {
            std::cout << "= " << result << "\n";
        }
    }

    return 0;
}
