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

    bool hasErrors() const { return has_errors_; }
    const std::vector<Diagnostic>& diagnostics() const { return diags_; }
    void printAll(std::ostream& out) const;

private:
    std::vector<Diagnostic> diags_;
    bool has_errors_ = false;
};

} // namespace kern
