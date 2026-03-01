#pragma once
#include <string>
#include <vector>

namespace kern {

struct Dependency {
    std::string name;
    std::string version;
    std::string path;  // local path (remote registry not yet supported)
};

struct Manifest {
    std::string name;
    std::string version;
    std::string entry;   // main source file
    std::vector<std::string> sources;
    std::vector<Dependency> dependencies;

    static Manifest parse(const std::string& toml_content);
    static Manifest fromFile(const std::string& path);
};

struct BuildPlan {
    std::vector<std::string> compile_order;  // topological order of .kern files
    std::string output;

    static BuildPlan create(const Manifest& manifest);
};

} // namespace kern
