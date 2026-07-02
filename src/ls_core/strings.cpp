#include "ls_core/strings.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace logicsentinel::core {

std::string trim(std::string value) {
    auto b = std::find_if_not(value.begin(), value.end(), [](unsigned char c){ return std::isspace(c); });
    auto e = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c){ return std::isspace(c); }).base();
    if (b >= e) return {};
    return std::string(b, e);
}
std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return value;
}
std::string to_upper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
    return value;
}
bool starts_with(const std::string& value, const std::string& prefix) { return value.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), value.begin()); }
bool ends_with(const std::string& value, const std::string& suffix) { return value.size() >= suffix.size() && std::equal(suffix.rbegin(), suffix.rend(), value.rbegin()); }
std::vector<std::string> split(const std::string& value, char delim, bool keep_empty) {
    std::vector<std::string> out;
    std::string cur;
    std::istringstream in(value);
    while (std::getline(in, cur, delim)) if (keep_empty || !cur.empty()) out.push_back(cur);
    if (keep_empty && !value.empty() && value.back() == delim) out.push_back({});
    return out;
}
std::vector<std::string> split_ws_quoted(const std::string& value) {
    std::vector<std::string> out;
    std::string cur;
    bool quote = false;
    char quote_char = 0;
    for (std::size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if ((c == '"' || c == '\'') && (i == 0 || value[i - 1] != '\\')) {
            if (!quote) { quote = true; quote_char = c; }
            else if (quote_char == c) quote = false;
            cur.push_back(c);
        } else if (std::isspace(static_cast<unsigned char>(c)) && !quote) {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}
std::string strip_quotes(std::string value) {
    value = trim(std::move(value));
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) return value.substr(1, value.size() - 2);
    return value;
}
std::string remove_comment(const std::string& value) {
    bool quote = false;
    char quote_char = 0;
    for (std::size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if ((c == '"' || c == '\'') && (i == 0 || value[i - 1] != '\\')) {
            if (!quote) { quote = true; quote_char = c; }
            else if (quote_char == c) quote = false;
        }
        if (!quote && c == '#') return value.substr(0, i);
        if (!quote && c == '/' && i + 1 < value.size() && value[i + 1] == '/') return value.substr(0, i);
    }
    return value;
}
bool is_identifier(const std::string& value) {
    if (value.empty()) return false;
    if (!std::isalpha(static_cast<unsigned char>(value[0])) && value[0] != '_') return false;
    for (unsigned char c : value) if (!std::isalnum(c) && c != '_' && c != '-' && c != '.') return false;
    return true;
}
bool is_integer(const std::string& value) {
    if (value.empty()) return false;
    std::size_t i = value[0] == '-' ? 1 : 0;
    if (i == value.size()) return false;
    for (; i < value.size(); ++i) if (!std::isdigit(static_cast<unsigned char>(value[i]))) return false;
    return true;
}
std::map<std::string, std::string> parse_kv_tokens(const std::vector<std::string>& tokens, std::size_t start) {
    std::map<std::string, std::string> out;
    for (std::size_t i = start; i < tokens.size(); ++i) {
        auto token = tokens[i];
        auto eq = token.find('=');
        if (eq == std::string::npos) out[to_lower(token)] = "true";
        else out[to_lower(trim(token.substr(0, eq)))] = strip_quotes(trim(token.substr(eq + 1)));
    }
    return out;
}
std::string join(const std::vector<std::string>& values, const std::string& sep) {
    std::ostringstream out;
    for (std::size_t i = 0; i < values.size(); ++i) { if (i) out << sep; out << values[i]; }
    return out.str();
}
std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) { if (!line.empty() && line.back() == '\r') line.pop_back(); out.push_back(line); }
    return out;
}

} // namespace logicsentinel::core

