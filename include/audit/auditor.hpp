#pragma once
#include <string>
#include <vector>
#include "engine/runner.hpp"
#include "model/workflow.hpp"

namespace logicsentinel::audit {

enum class FindingSeverity { info, low, medium, high };

struct Finding {
    FindingSeverity severity = FindingSeverity::info;
    std::string code;
    std::string title;
    std::string detail;
    std::size_t line = 0;
};

struct AuditReport {
    std::vector<Finding> findings;
};

std::string severity_name(FindingSeverity severity);
AuditReport audit_spec(const model::WorkflowSpec& spec);
AuditReport audit_execution(const model::WorkflowSpec& spec, const engine::ExecutionReport& report);
std::string summarize_audit(const AuditReport& report);
std::vector<Finding> find_authorization_gaps(const model::WorkflowSpec& spec);
std::vector<Finding> find_state_machine_gaps(const model::WorkflowSpec& spec);
std::vector<Finding> find_replay_and_approval_gaps(const model::WorkflowSpec& spec);

} // namespace logicsentinel::audit

