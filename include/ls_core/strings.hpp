#pragma once
#include <map>
#include <string>
#include <vector>

namespace logicsentinel::core {

std::string trim(std::string value);
std::string to_lower(std::string value);
std::string to_upper(std::string value);
bool starts_with(const std::string& value, const std::string& prefix);
bool ends_with(const std::string& value, const std::string& suffix);
std::vector<std::string> split(const std::string& value, char delim, bool keep_empty = false);
std::vector<std::string> split_ws_quoted(const std::string& value);
std::string strip_quotes(std::string value);
std::string remove_comment(const std::string& value);
bool is_identifier(const std::string& value);
bool is_integer(const std::string& value);
std::map<std::string, std::string> parse_kv_tokens(const std::vector<std::string>& tokens, std::size_t start = 0);
std::string join(const std::vector<std::string>& values, const std::string& sep);
std::vector<std::string> lines(const std::string& text);

} // namespace logicsentinel::core

