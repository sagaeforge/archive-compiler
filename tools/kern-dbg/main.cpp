#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <csignal>
#include <cstdlib>

// kern-dbg: Minimal debugger for Kern programs.
// Phase 7 skeleton — breakpoint + continue + print.
// Full implementation requires DWARF generation (Phase 7a prerequisite).

namespace {

struct Breakpoint {
    std::string file;
    uint32_t line;
    bool enabled = true;
};

std::vector<Breakpoint> breakpoints;

void printUsage() {
    std::cerr << "Usage: kern-dbg <executable>\n"
              << "\nCommands:\n"
              << "  break <file>:<line>  Set breakpoint\n"
              << "  continue (c)         Continue execution\n"
              << "  print <expr> (p)     Print expression value\n"
              << "  info break           List breakpoints\n"
              << "  quit (q)             Exit debugger\n"
              << "  help                 Show this help\n";
}

void cmdBreak(const std::string& loc) {
    auto colon = loc.find(':');
    if (colon == std::string::npos) {
        std::cerr << "Usage: break <file>:<line>\n";
        return;
    }
    Breakpoint bp;
    bp.file = loc.substr(0, colon);
    bp.line = static_cast<uint32_t>(std::stoi(loc.substr(colon + 1)));
    breakpoints.push_back(bp);
    std::cout << "Breakpoint " << breakpoints.size()
              << " at " << bp.file << ":" << bp.line << "\n";
}

void cmdInfoBreak() {
    if (breakpoints.empty()) {
        std::cout << "No breakpoints.\n";
        return;
    }
    for (size_t i = 0; i < breakpoints.size(); ++i) {
        std::cout << "  " << (i + 1) << ": "
                  << breakpoints[i].file << ":" << breakpoints[i].line
                  << (breakpoints[i].enabled ? "" : " (disabled)") << "\n";
    }
}

} // anonymous namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string executable = argv[1];
    std::cout << "kern-dbg: debugging " << executable << "\n";
    std::cout << "Note: Full debugging requires DWARF info generation (not yet implemented).\n";
    std::cout << "Type 'help' for commands.\n\n";

    std::string line;
    while (true) {
        std::cout << "(kern-dbg) ";
        if (!std::getline(std::cin, line)) break;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd.empty()) continue;
        if (cmd == "quit" || cmd == "q") break;
        if (cmd == "help") { printUsage(); continue; }
        if (cmd == "break" || cmd == "b") {
            std::string loc;
            iss >> loc;
            cmdBreak(loc);
            continue;
        }
        if (cmd == "info") {
            std::string what;
            iss >> what;
            if (what == "break") cmdInfoBreak();
            continue;
        }
        if (cmd == "continue" || cmd == "c") {
            std::cout << "Continue: not yet implemented (requires process control)\n";
            continue;
        }
        if (cmd == "print" || cmd == "p") {
            std::string expr;
            std::getline(iss, expr);
            std::cout << "Print: not yet implemented (requires DWARF variable info)\n";
            continue;
        }
        if (cmd == "run" || cmd == "r") {
            std::cout << "Running " << executable << "...\n";
            int rc = std::system(executable.c_str());
            std::cout << "Process exited with code " << WEXITSTATUS(rc) << "\n";
            continue;
        }
        std::cerr << "Unknown command: " << cmd << "\n";
    }

    return 0;
}
