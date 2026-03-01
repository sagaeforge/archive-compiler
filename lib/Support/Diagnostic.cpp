#include "kern/support/Diagnostic.h"
#include <iostream>

namespace kern {

void DiagnosticEngine::report(DiagLevel level, SourceLocation loc, std::string message) {
    if (level == DiagLevel::Error) {
        has_errors_ = true;
    }
    diags_.push_back({level, loc, std::move(message)});
}

void DiagnosticEngine::error(SourceLocation loc, std::string message) {
    report(DiagLevel::Error, loc, std::move(message));
}

void DiagnosticEngine::warning(SourceLocation loc, std::string message) {
    report(DiagLevel::Warning, loc, std::move(message));
}

void DiagnosticEngine::printAll(std::ostream& out) const {
    for (const auto& d : diags_) {
        switch (d.level) {
            case DiagLevel::Error:   out << "error"; break;
            case DiagLevel::Warning: out << "warning"; break;
            case DiagLevel::Note:    out << "note"; break;
        }
        if (d.loc.line > 0) {
            out << " [" << d.loc.filename << ":" << d.loc.line << ":" << d.loc.col << "]";
        }
        out << ": " << d.message << "\n";
    }
}

} // namespace kern
