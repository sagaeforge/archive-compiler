#pragma once
#include "kern/support/Diagnostic.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace kern {

struct ResolvedModule {
    std::string module_path;     // "math" or "kern.memory.page"
    std::string file_path;       // "/path/to/math.kern"
    std::vector<std::string> imports;  // module paths this depends on
};

class ModuleResolver {
public:
    explicit ModuleResolver(DiagnosticEngine& diag);

    // Add a directory to search for modules
    void addSearchPath(std::string_view path);

    // Resolve a module path to a file path. Returns true on success.
    bool resolve(const std::string& module_path, std::string& out_file);

    // Build the full dependency DAG starting from entry files.
    // Scans each file for module/import declarations.
    // Returns true if no errors (no cycles, all modules found).
    bool buildDependencyGraph(const std::vector<std::string>& entry_files);

    // Get topological order (leaves first). Must call buildDependencyGraph first.
    // Returns true on success.
    bool topologicalOrder(std::vector<std::string>& order);

    // Get resolved module info by module path
    const ResolvedModule* getModule(const std::string& module_path) const;

    // Get all resolved modules
    const std::unordered_map<std::string, ResolvedModule>& modules() const { return modules_; }

private:
    DiagnosticEngine& diag_;
    std::vector<std::string> search_paths_;
    std::unordered_map<std::string, ResolvedModule> modules_;

    // Scan a file for module declaration and imports (lightweight, no full parse)
    bool scanFileHeader(const std::string& file_path,
                        std::string& module_name,
                        std::vector<std::string>& imports);

    // Convert module.path to directory/path
    static std::string modulePathToFilePath(const std::string& module_path);
};

} // namespace kern
