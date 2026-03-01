#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
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

// Resolves transitive dependencies from a root manifest.
// Detects circular dependencies and produces a topological compile order.
class DependencyResolver {
public:
    struct ResolvedDep {
        std::string name;
        std::string path;     // resolved filesystem path
        Manifest manifest;    // parsed manifest of this dep
    };

    struct Result {
        bool success;
        std::string error;
        std::vector<ResolvedDep> resolved;  // topological order (deps before dependents)
    };

    // Resolve all transitive dependencies starting from root manifest.
    // base_dir is the directory containing the root manifest.
    static Result resolve(const Manifest& root, const std::string& base_dir);

private:
    static bool visit(const std::string& name, const std::string& dep_path,
                      const std::string& base_dir,
                      std::unordered_map<std::string, ResolvedDep>& resolved,
                      std::unordered_set<std::string>& visiting,
                      std::unordered_set<std::string>& visited,
                      std::vector<ResolvedDep>& order,
                      std::string& error);
};

} // namespace kern
