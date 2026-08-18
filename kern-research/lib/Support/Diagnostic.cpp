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

void DiagnosticEngine::note(SourceLocation loc, std::string message) {
    report(DiagLevel::Note, loc, std::move(message));
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

        // Print source line and caret if source is available
        if (!source_.empty() && d.loc.line > 0) {
            uint32_t current_line = 1;
            size_t line_start = 0;
            for (size_t i = 0; i < source_.size(); ++i) {
                if (current_line == d.loc.line) {
                    line_start = i;
                    break;
                }
                if (source_[i] == '\n') {
                    current_line++;
                }
            }
            if (current_line == d.loc.line) {
                size_t line_end = source_.find('\n', line_start);
                if (line_end == std::string_view::npos) line_end = source_.size();
                out << "  " << source_.substr(line_start, line_end - line_start) << "\n";
                if (d.loc.col > 0) {
                    out << "  " << std::string(d.loc.col - 1, ' ') << "^\n";
                }
            }
        }
    }
}

} // namespace kern
