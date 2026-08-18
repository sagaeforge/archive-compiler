#pragma once
#include "kern/support/SourceLocation.h"
#include <string>
#include <vector>
#include <ostream>

namespace kern {

enum class DiagLevel { Error, Warning, Note };

struct Diagnostic {
    DiagLevel      level;
    SourceLocation loc;
    std::string    message;
};

class DiagnosticEngine {
public:
    void report(DiagLevel level, SourceLocation loc, std::string message);
    void error(SourceLocation loc, std::string message);
    void warning(SourceLocation loc, std::string message);
    void note(SourceLocation loc, std::string message);

    bool hasErrors() const { return has_errors_; }
    bool hasWarnings() const {
        for (auto& d : diags_) if (d.level == DiagLevel::Warning) return true;
        return false;
    }
    const std::vector<Diagnostic>& diagnostics() const { return diags_; }
    void setSource(std::string_view source) { source_ = source; }
    void printAll(std::ostream& out) const;

private:
    std::vector<Diagnostic> diags_;
    std::string_view source_;
    bool has_errors_ = false;
};

} // namespace kern
