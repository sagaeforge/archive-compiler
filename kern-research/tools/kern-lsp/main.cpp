#include "kern/ide/IDEContext.h"
#include "kern/ide/CompletionProvider.h"
#include "kern/ide/HoverProvider.h"
#include "kern/ide/DefinitionProvider.h"
#include "kern/ide/ReferencesProvider.h"
#include "kern/ide/SemanticTokens.h"
#include "kern/ide/DiagnosticProvider.h"
#include "kern/support/CompilationContext.h"

#include <iostream>
#include <string>
#include <sstream>

// Minimal JSON-RPC/LSP server over stdio.
// Reads Content-Length-framed messages, dispatches to IDE providers.

namespace {

std::string readMessage() {
    std::string line;
    int content_length = 0;

    while (std::getline(std::cin, line)) {
        if (line.empty() || line == "\r") break;
        if (line.find("Content-Length:") == 0) {
            content_length = std::stoi(line.substr(15));
        }
    }

    if (content_length <= 0) return "";

    std::string body(content_length, '\0');
    std::cin.read(body.data(), content_length);
    return body;
}

void sendMessage(const std::string& msg) {
    std::cout << "Content-Length: " << msg.size() << "\r\n\r\n" << msg;
    std::cout.flush();
}

void sendResponse(int id, const std::string& result) {
    std::string resp = "{\"jsonrpc\":\"2.0\",\"id\":" + std::to_string(id)
                     + ",\"result\":" + result + "}";
    sendMessage(resp);
}

void sendNotification(const std::string& method, const std::string& params) {
    std::string msg = "{\"jsonrpc\":\"2.0\",\"method\":\"" + method
                    + "\",\"params\":" + params + "}";
    sendMessage(msg);
}

void sendError(int id, int code, const std::string& message) {
    std::string resp = "{\"jsonrpc\":\"2.0\",\"id\":" + std::to_string(id)
                     + ",\"error\":{\"code\":" + std::to_string(code)
                     + ",\"message\":\"" + message + "\"}}";
    sendMessage(resp);
}

// Minimal JSON string extraction (no full parser — just key lookup)
std::string extractString(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    pos = json.find('\"', pos + key.size() + 2);
    if (pos == std::string::npos) return "";
    auto end = json.find('\"', pos + 1);
    if (end == std::string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

int extractInt(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return -1;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return -1;
    while (pos < json.size() && !std::isdigit(json[pos]) && json[pos] != '-') ++pos;
    return std::stoi(json.substr(pos));
}

// Extract nested URI from textDocument
std::string extractUri(const std::string& json) {
    return extractString(json, "uri");
}

// Extract position (line/character) from JSON
void extractPosition(const std::string& json, uint32_t& line, uint32_t& character) {
    // Find "position" object, then "line" and "character" within it
    auto pos_start = json.find("\"position\"");
    if (pos_start == std::string::npos) {
        line = character = 0;
        return;
    }
    auto sub = json.substr(pos_start);
    line = static_cast<uint32_t>(extractInt(sub, "line"));
    character = static_cast<uint32_t>(extractInt(sub, "character"));
}

// Escape a string for JSON output
std::string jsonEscape(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

// Extract text content from didOpen/didChange
std::string extractText(const std::string& json) {
    // For didOpen: look for "text" inside "textDocument"
    auto text = extractString(json, "text");
    return text;
}

// Publish diagnostics notification
void publishDiagnostics(kern::IDEContext& ide, kern::DiagnosticProvider& diag_provider,
                        const std::string& uri) {
    auto diags = diag_provider.diagnose(ide, uri);
    std::ostringstream ss;
    ss << "{\"uri\":\"" << jsonEscape(uri) << "\",\"diagnostics\":[";
    for (size_t i = 0; i < diags.size(); ++i) {
        if (i > 0) ss << ",";
        int severity = 1; // Error
        if (diags[i].severity == kern::DiagLevel::Warning) severity = 2;
        if (diags[i].severity == kern::DiagLevel::Note) severity = 3;
        uint32_t line = diags[i].loc.line > 0 ? diags[i].loc.line - 1 : 0;
        uint32_t col = diags[i].loc.col > 0 ? diags[i].loc.col - 1 : 0;
        ss << "{\"range\":{\"start\":{\"line\":" << line << ",\"character\":" << col
           << "},\"end\":{\"line\":" << line << ",\"character\":" << (col + 1)
           << "}},\"severity\":" << severity
           << ",\"source\":\"kern\",\"message\":\"" << jsonEscape(diags[i].message) << "\"}";
    }
    ss << "]}";
    sendNotification("textDocument/publishDiagnostics", ss.str());
}

} // anonymous namespace

int main() {
    std::cerr << "kern-lsp: starting...\n";

    kern::CompilationContext ctx;
    kern::IDEContext ide(ctx);
    kern::HoverProvider hover_provider;
    kern::CompletionProvider completion_provider;
    kern::DefinitionProvider definition_provider;
    kern::ReferencesProvider references_provider;
    kern::SemanticTokensProvider semantic_tokens_provider;
    kern::DiagnosticProvider diag_provider;

    bool shutdown_requested = false;

    while (true) {
        auto msg = readMessage();
        if (msg.empty()) {
            if (std::cin.eof()) break;
            continue;
        }

        auto method = extractString(msg, "method");
        int id = extractInt(msg, "id");

        if (method == "initialize") {
            std::string caps = R"({
                "capabilities": {
                    "textDocumentSync": 1,
                    "completionProvider": {"triggerCharacters": ["."]},
                    "hoverProvider": true,
                    "definitionProvider": true,
                    "referencesProvider": true,
                    "semanticTokensProvider": {
                        "full": true,
                        "legend": {
                            "tokenTypes": ["keyword","type","function","variable","number","string","operator","parameter","comment","enumMember"],
                            "tokenModifiers": []
                        }
                    }
                },
                "serverInfo": {"name": "kern-lsp", "version": "0.2.0"}
            })";
            sendResponse(id, caps);

        } else if (method == "initialized") {
            // Client ready — no response needed

        } else if (method == "shutdown") {
            shutdown_requested = true;
            sendResponse(id, "null");

        } else if (method == "exit") {
            return shutdown_requested ? 0 : 1;

        } else if (method == "textDocument/didOpen") {
            auto uri = extractUri(msg);
            auto text = extractText(msg);
            ide.openFile(uri, text);
            publishDiagnostics(ide, diag_provider, uri);

        } else if (method == "textDocument/didChange") {
            auto uri = extractUri(msg);
            auto text = extractText(msg);
            if (!text.empty()) {
                ide.updateFile(uri, text);
                publishDiagnostics(ide, diag_provider, uri);
            }

        } else if (method == "textDocument/didClose") {
            auto uri = extractUri(msg);
            ide.closeFile(uri);

        } else if (method == "textDocument/hover") {
            auto uri = extractUri(msg);
            uint32_t line, character;
            extractPosition(msg, line, character);
            // LSP uses 0-based, Kern uses 1-based
            auto result = hover_provider.hover(ide, uri, line + 1, character + 1);
            if (result) {
                std::ostringstream ss;
                ss << "{\"contents\":{\"kind\":\"markdown\",\"value\":\"";
                ss << "**" << jsonEscape(result->type_info) << "**";
                if (!result->purity.empty()) {
                    ss << "\\n\\n" << jsonEscape(result->purity);
                }
                ss << "\"}}";
                sendResponse(id, ss.str());
            } else {
                sendResponse(id, "null");
            }

        } else if (method == "textDocument/completion") {
            auto uri = extractUri(msg);
            uint32_t line, character;
            extractPosition(msg, line, character);
            auto items = completion_provider.complete(ide, uri, line + 1, character + 1);
            std::ostringstream ss;
            ss << "{\"isIncomplete\":false,\"items\":[";
            for (size_t i = 0; i < items.size(); ++i) {
                if (i > 0) ss << ",";
                // LSP CompletionItemKind: 1=Text, 3=Function, 6=Variable, 14=Keyword, 22=Struct, 13=Enum
                int kind = 1;
                switch (items[i].kind) {
                    case kern::CompletionItem::Function: kind = 3; break;
                    case kern::CompletionItem::Variable: kind = 6; break;
                    case kern::CompletionItem::Type: kind = 22; break;
                    case kern::CompletionItem::Keyword: kind = 14; break;
                    case kern::CompletionItem::Field: kind = 5; break;
                    case kern::CompletionItem::EnumVariant: kind = 13; break;
                }
                ss << "{\"label\":\"" << jsonEscape(std::string(items[i].label)) << "\""
                   << ",\"kind\":" << kind;
                if (!items[i].detail.empty()) {
                    ss << ",\"detail\":\"" << jsonEscape(std::string(items[i].detail)) << "\"";
                }
                ss << "}";
            }
            ss << "]}";
            sendResponse(id, ss.str());

        } else if (method == "textDocument/definition") {
            auto uri = extractUri(msg);
            uint32_t line, character;
            extractPosition(msg, line, character);
            auto result = definition_provider.findDefinition(ide, uri, line + 1, character + 1);
            if (result) {
                std::ostringstream ss;
                uint32_t def_line = result->location.line > 0 ? result->location.line - 1 : 0;
                uint32_t def_col = result->location.col > 0 ? result->location.col - 1 : 0;
                ss << "{\"uri\":\"" << jsonEscape(uri) << "\""
                   << ",\"range\":{\"start\":{\"line\":" << def_line << ",\"character\":" << def_col
                   << "},\"end\":{\"line\":" << def_line << ",\"character\":" << def_col << "}}}";
                sendResponse(id, ss.str());
            } else {
                sendResponse(id, "null");
            }

        } else if (method == "textDocument/references") {
            auto uri = extractUri(msg);
            uint32_t line, character;
            extractPosition(msg, line, character);
            auto refs = references_provider.findReferences(ide, uri, line + 1, character + 1);
            std::ostringstream ss;
            ss << "[";
            for (size_t i = 0; i < refs.size(); ++i) {
                if (i > 0) ss << ",";
                uint32_t ref_line = refs[i].location.line > 0 ? refs[i].location.line - 1 : 0;
                uint32_t ref_col = refs[i].location.col > 0 ? refs[i].location.col - 1 : 0;
                ss << "{\"uri\":\"" << jsonEscape(uri) << "\""
                   << ",\"range\":{\"start\":{\"line\":" << ref_line << ",\"character\":" << ref_col
                   << "},\"end\":{\"line\":" << ref_line << ",\"character\":" << ref_col << "}}}";
            }
            ss << "]";
            sendResponse(id, ss.str());

        } else if (method == "textDocument/semanticTokens/full") {
            auto uri = extractUri(msg);
            auto tokens = semantic_tokens_provider.tokenize(ide, uri);
            // Encode as LSP delta format
            std::ostringstream ss;
            ss << "{\"data\":[";
            uint32_t prev_line = 0, prev_col = 0;
            for (size_t i = 0; i < tokens.size(); ++i) {
                if (i > 0) ss << ",";
                uint32_t line = tokens[i].line > 0 ? tokens[i].line - 1 : 0;
                uint32_t col = tokens[i].column > 0 ? tokens[i].column - 1 : 0;
                uint32_t delta_line = line - prev_line;
                uint32_t delta_col = (delta_line == 0) ? col - prev_col : col;
                ss << delta_line << "," << delta_col << ","
                   << tokens[i].length << "," << static_cast<uint32_t>(tokens[i].type)
                   << ",0";
                prev_line = line;
                prev_col = col;
            }
            ss << "]}";
            sendResponse(id, ss.str());

        } else if (id >= 0) {
            sendError(id, -32601, "Method not found: " + method);
        }
    }

    return 0;
}
