#include <iostream>
#include <string>

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

} // anonymous namespace

int main() {
    std::cerr << "kern-lsp: starting...\n";

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
                            "tokenTypes": ["keyword","type","function","variable","number","string","operator","comment"],
                            "tokenModifiers": []
                        }
                    }
                },
                "serverInfo": {"name": "kern-lsp", "version": "0.1.0"}
            })";
            sendResponse(id, caps);
        } else if (method == "shutdown") {
            shutdown_requested = true;
            sendResponse(id, "null");
        } else if (method == "exit") {
            return shutdown_requested ? 0 : 1;
        } else if (method == "textDocument/hover") {
            // TODO: Wire up HoverProvider with document management
            sendResponse(id, "null");
        } else if (method == "textDocument/completion") {
            sendResponse(id, "{\"isIncomplete\":false,\"items\":[]}");
        } else if (method == "textDocument/definition") {
            sendResponse(id, "null");
        } else if (method == "textDocument/references") {
            sendResponse(id, "[]");
        } else if (method == "textDocument/semanticTokens/full") {
            sendResponse(id, "{\"data\":[]}");
        } else if (method == "textDocument/didOpen" ||
                   method == "textDocument/didChange" ||
                   method == "textDocument/didClose") {
            // Notification — no response
        } else if (id >= 0) {
            sendError(id, -32601, "Method not found: " + method);
        }
    }

    return 0;
}
