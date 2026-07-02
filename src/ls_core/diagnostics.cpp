#include "ls_core/diagnostics.hpp"
#include <sstream>

namespace logicsentinel::core {

void Diagnostics::add(Severity severity, std::string code, std::string message, std::size_t line) {
    entries_.push_back({severity, std::move(code), std::move(message), line});
}
void Diagnostics::info(std::string code, std::string message, std::size_t line) { add(Severity::info, std::move(code), std::move(message), line); }
void Diagnostics::warn(std::string code, std::string message, std::size_t line) { add(Severity::warning, std::move(code), std::move(message), line); }
void Diagnostics::error(std::string code, std::string message, std::size_t line) { add(Severity::error, std::move(code), std::move(message), line); }
bool Diagnostics::has_errors() const {
    for (const auto& d : entries_) if (d.severity == Severity::error) return true;
    return false;
}
std::string Diagnostics::summary() const {
    std::ostringstream out;
    for (const auto& d : entries_) {
        out << (d.severity == Severity::error ? "error" : d.severity == Severity::warning ? "warning" : "info")
            << ":" << d.code << ":" << d.line << ": " << d.message << "\n";
    }
    return out.str();
}

} // namespace logicsentinel::core

