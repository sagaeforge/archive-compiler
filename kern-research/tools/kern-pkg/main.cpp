#include "kern/pkg/Manifest.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace kern;

static void printUsage() {
    std::cerr << "Usage: kern-pkg <command>\n"
              << "Commands:\n"
              << "  build     Build the project\n"
              << "  init      Initialize a new kern.toml\n"
              << "  --help    Show this help\n";
}

static int cmdInit() {
    if (std::filesystem::exists("kern.toml")) {
        std::cerr << "Error: kern.toml already exists\n";
        return 1;
    }
    std::string dirname = std::filesystem::current_path().filename().string();
    std::ofstream ofs("kern.toml");
    ofs << "[package]\n"
        << "name = \"" << dirname << "\"\n"
        << "version = \"0.1.0\"\n"
        << "entry = \"src/main.kern\"\n\n"
        << "[dependencies]\n";
    std::cout << "Created kern.toml\n";
    return 0;
}

static int cmdBuild() {
    if (!std::filesystem::exists("kern.toml")) {
        std::cerr << "Error: kern.toml not found\n";
        return 1;
    }

    auto manifest = Manifest::fromFile("kern.toml");
    auto plan = BuildPlan::create(manifest);

    if (plan.compile_order.empty()) {
        std::cerr << "Error: no source files to compile\n";
        return 1;
    }

    // Build each source file, then link
    std::string kernc = "kernc";  // assume in PATH
    std::string cmd = kernc + " -o " + plan.output;
    for (const auto& src : plan.compile_order) {
        cmd += " " + src;
    }

    // For now, only single-file compilation is supported
    cmd = kernc + " " + plan.compile_order[0] + " -o " + plan.output;

    std::cout << "Building " << manifest.name << " v" << manifest.version << "...\n";
    std::cout << "  " << cmd << "\n";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "Build failed\n";
        return 1;
    }
    std::cout << "Built: " << plan.output << "\n";
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string_view cmd = argv[1];
    if (cmd == "build") return cmdBuild();
    if (cmd == "init") return cmdInit();
    if (cmd == "--help") { printUsage(); return 0; }

    std::cerr << "Unknown command: " << cmd << "\n";
    printUsage();
    return 1;
}
