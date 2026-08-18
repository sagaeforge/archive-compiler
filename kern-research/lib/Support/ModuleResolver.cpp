#include "kern/support/ModuleResolver.h"
#include <fstream>
#include <filesystem>
#include <functional>
#include <unordered_set>

namespace kern {

namespace fs = std::filesystem;

ModuleResolver::ModuleResolver(DiagnosticEngine& diag) : diag_(diag) {}

void ModuleResolver::addSearchPath(std::string_view path) {
    search_paths_.emplace_back(path);
}

std::string ModuleResolver::modulePathToFilePath(const std::string& module_path) {
    // "math" → "math.kern"
    // "kern.memory.page" → "kern/memory/page.kern"
    std::string result;
    for (char c : module_path) {
        result += (c == '.') ? '/' : c;
    }
    result += ".kern";
    return result;
}

bool ModuleResolver::resolve(const std::string& module_path, std::string& out_file) {
    std::string rel = modulePathToFilePath(module_path);

    for (const auto& dir : search_paths_) {
        fs::path candidate = fs::path(dir) / rel;
        if (fs::exists(candidate)) {
            out_file = candidate.string();
            return true;
        }
    }

    // Also check if module_path is already a file path
    if (fs::exists(module_path)) {
        out_file = module_path;
        return true;
    }

    return false;
}

bool ModuleResolver::scanFileHeader(const std::string& file_path,
                                     std::string& module_name,
                                     std::vector<std::string>& imports) {
    std::ifstream ifs(file_path);
    if (!ifs) return false;

    std::string line;
    while (std::getline(ifs, line)) {
        // Trim leading whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        std::string_view sv(line.data() + start, line.size() - start);

        // module <name>
        if (sv.substr(0, 7) == "module " && sv.size() > 7) {
            auto name_start = sv.find_first_not_of(" \t", 7);
            if (name_start != std::string_view::npos) {
                auto name_end = sv.find_first_of(" \t\n\r", name_start);
                if (name_end == std::string_view::npos) name_end = sv.size();
                module_name = std::string(sv.substr(name_start, name_end - name_start));
            }
            continue;
        }

        // [pub] import <module_path> (...)
        std::string_view check = sv;
        if (check.substr(0, 4) == "pub " && check.size() > 4) {
            check = check.substr(4);
            start = check.find_first_not_of(" \t");
            if (start != std::string_view::npos) check = check.substr(start);
        }
        if (check.substr(0, 7) == "import " && check.size() > 7) {
            auto name_start = check.find_first_not_of(" \t", 7);
            if (name_start != std::string_view::npos) {
                auto name_end = check.find_first_of(" \t(\n\r", name_start);
                if (name_end == std::string_view::npos) name_end = check.size();
                imports.emplace_back(check.substr(name_start, name_end - name_start));
            }
            continue;
        }

        // Stop scanning at first non-module/import/comment/blank line
        // (but allow comments and effect annotations)
        if (sv[0] == '/' && sv.size() > 1 && sv[1] == '/') continue; // line comment
        if (sv.substr(0, 5) == "with ") continue; // effect annotation
        if (sv.substr(0, 3) == "fn " || sv.substr(0, 4) == "pub " ||
            sv.substr(0, 7) == "struct " || sv.substr(0, 5) == "enum " ||
            sv.substr(0, 6) == "union " || sv.substr(0, 6) == "trait " ||
            sv.substr(0, 7) == "static " || sv.substr(0, 5) == "type " ||
            sv.substr(0, 8) == "newtype " || sv.substr(0, 5) == "impl " ||
            sv.substr(0, 7) == "extern ") {
            break;
        }
    }

    return true;
}

bool ModuleResolver::buildDependencyGraph(const std::vector<std::string>& entry_files) {
    // BFS from entry files
    std::vector<std::string> worklist;

    for (const auto& file : entry_files) {
        std::string module_name;
        std::vector<std::string> imports;

        if (!scanFileHeader(file, module_name, imports)) {
            diag_.error({}, std::string("cannot open file '") + file + "'");
            return false;
        }

        // If no module declaration, derive from filename
        if (module_name.empty()) {
            fs::path p(file);
            module_name = p.stem().string();
        }

        ResolvedModule rm;
        rm.module_path = module_name;
        rm.file_path = fs::canonical(fs::path(file)).string();
        rm.imports = std::move(imports);

        worklist.insert(worklist.end(), rm.imports.begin(), rm.imports.end());
        modules_[rm.module_path] = std::move(rm);
    }

    // Process worklist — resolve each imported module
    std::unordered_set<std::string> visited;
    for (const auto& [k, _] : modules_) visited.insert(k);

    while (!worklist.empty()) {
        std::string mod_path = std::move(worklist.back());
        worklist.pop_back();

        if (visited.count(mod_path)) continue;
        visited.insert(mod_path);

        std::string file_path;
        if (!resolve(mod_path, file_path)) {
            diag_.error({}, std::string("module '") + mod_path +
                        "' not found in search paths");
            return false;
        }

        std::string actual_name;
        std::vector<std::string> imports;
        if (!scanFileHeader(file_path, actual_name, imports)) {
            diag_.error({}, std::string("cannot open module file '") + file_path + "'");
            return false;
        }

        if (actual_name.empty()) actual_name = mod_path;

        ResolvedModule rm;
        rm.module_path = actual_name;
        rm.file_path = file_path;
        rm.imports = std::move(imports);

        worklist.insert(worklist.end(), rm.imports.begin(), rm.imports.end());
        modules_[rm.module_path] = std::move(rm);
    }

    return true;
}

bool ModuleResolver::topologicalOrder(std::vector<std::string>& order) {
    // 3-color DFS-based topo sort with cycle detection
    enum Color { White, Gray, Black };
    std::unordered_map<std::string, Color> color;

    for (const auto& [name, _] : modules_) color[name] = White;

    bool has_cycle = false;
    std::vector<std::string> cycle_path;

    std::function<void(const std::string&)> dfs = [&](const std::string& node) {
        if (has_cycle) return;
        color[node] = Gray;
        cycle_path.push_back(node);

        auto it = modules_.find(node);
        if (it != modules_.end()) {
            for (const auto& dep : it->second.imports) {
                auto cit = color.find(dep);
                if (cit == color.end()) continue; // external module, skip
                if (cit->second == Gray) {
                    // Cycle detected — build cycle string
                    has_cycle = true;
                    std::string cycle_str;
                    bool in_cycle = false;
                    for (const auto& p : cycle_path) {
                        if (p == dep) in_cycle = true;
                        if (in_cycle) {
                            if (!cycle_str.empty()) cycle_str += " -> ";
                            cycle_str += p;
                        }
                    }
                    cycle_str += " -> " + dep;
                    diag_.error({}, std::string("circular import detected: ") + cycle_str);
                    return;
                }
                if (cit->second == White) {
                    dfs(dep);
                }
            }
        }

        cycle_path.pop_back();
        color[node] = Black;
        order.push_back(node);
    };

    for (const auto& [name, _] : modules_) {
        if (color[name] == White) {
            dfs(name);
            if (has_cycle) return false;
        }
    }

    return true;
}

const ResolvedModule* ModuleResolver::getModule(const std::string& module_path) const {
    auto it = modules_.find(module_path);
    return it != modules_.end() ? &it->second : nullptr;
}

} // namespace kern
