#include "kern/pkg/Manifest.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace kern {

// Minimal TOML-subset parser for kern.toml
// Supports: [package], [dependencies], key = "value", key = ["a", "b"]
static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string stripQuotes(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

static std::vector<std::string> parseArray(const std::string& val) {
    std::vector<std::string> result;
    // Simple array parsing: ["a", "b", "c"]
    auto start = val.find('[');
    auto end = val.rfind(']');
    if (start == std::string::npos || end == std::string::npos) return result;
    std::string inner = val.substr(start + 1, end - start - 1);
    std::istringstream ss(inner);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = trim(item);
        if (!item.empty()) {
            result.push_back(stripQuotes(item));
        }
    }
    return result;
}

Manifest Manifest::parse(const std::string& content) {
    Manifest m;
    std::string section;
    std::istringstream ss(content);
    std::string line;

    while (std::getline(ss, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line[0] == '[') {
            auto end = line.find(']');
            section = line.substr(1, end - 1);
            continue;
        }

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));

        if (section == "package") {
            if (key == "name") m.name = stripQuotes(val);
            else if (key == "version") m.version = stripQuotes(val);
            else if (key == "entry") m.entry = stripQuotes(val);
            else if (key == "sources") m.sources = parseArray(val);
        } else if (section == "dependencies") {
            Dependency dep;
            dep.name = key;
            // Simple: dep = "path/to/dep" or dep = { path = "..." }
            if (val[0] == '"') {
                dep.path = stripQuotes(val);
            }
            m.dependencies.push_back(dep);
        }
    }

    return m;
}

Manifest Manifest::fromFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) {
        throw std::runtime_error("Cannot open manifest: " + path);
    }
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    return parse(content);
}

BuildPlan BuildPlan::create(const Manifest& manifest) {
    BuildPlan plan;

    // For now: simple single-file compilation
    if (!manifest.entry.empty()) {
        plan.compile_order.push_back(manifest.entry);
    }
    for (const auto& src : manifest.sources) {
        if (src != manifest.entry) {
            plan.compile_order.push_back(src);
        }
    }

    plan.output = manifest.name.empty() ? "a.out" : manifest.name;
    return plan;
}

} // namespace kern
