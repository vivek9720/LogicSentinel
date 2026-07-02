#include "audit/auditor.hpp"
#include "audit/catalog.hpp"
#include <map>
#include <sstream>

namespace logicsentinel::audit {
using namespace logicsentinel::model;

std::string severity_name(FindingSeverity severity) {
    switch (severity) {
        case FindingSeverity::low: return "low";
        case FindingSeverity::medium: return "medium";
        case FindingSeverity::high: return "high";
        default: return "info";
    }
}
std::vector<Finding> find_authorization_gaps(const WorkflowSpec& spec) {
    std::vector<Finding> out;
    for (const auto& tr : spec.transitions) {
        auto profile = lookup_operation_profile(tr.name);
        if (tr.allowed_roles.empty()) out.push_back({FindingSeverity::high, "auth.unrestricted", "transition has no role restriction", tr.name + " can be attempted by any known actor", tr.line});
        bool owner_guard = false;
        bool mfa_guard = false;
        for (const auto& g : tr.guards) {
            if (g.kind == CheckKind::actor_is_owner || g.kind == CheckKind::actor_not_owner) owner_guard = true;
            if (g.kind == CheckKind::requires_mfa) mfa_guard = true;
        }
        if ((tr.name.find("delete") != std::string::npos || tr.name.find("update") != std::string::npos) && !owner_guard) out.push_back({FindingSeverity::medium, "auth.ownership", "mutation lacks ownership guard", tr.name + " mutates a resource without owner-sensitive checks", tr.line});
        if ((tr.name.find("admin") != std::string::npos || tr.name.find("approve") != std::string::npos || tr.name.find("refund") != std::string::npos) && !mfa_guard) out.push_back({FindingSeverity::medium, "auth.mfa", "sensitive transition lacks MFA guard", tr.name + " is sensitive but does not require MFA", tr.line});
        if (profile && std::string(profile->required_control) == "mfa" && !mfa_guard) out.push_back({FindingSeverity::medium, "auth.catalog_mfa", "cataloged operation lacks MFA guard", operation_profile_summary(*profile), tr.line});
        if (profile && std::string(profile->required_control) == "owner" && !owner_guard) out.push_back({FindingSeverity::medium, "auth.catalog_owner", "cataloged operation lacks ownership guard", operation_profile_summary(*profile), tr.line});
    }
    return out;
}
std::vector<Finding> find_state_machine_gaps(const WorkflowSpec& spec) {
    std::vector<Finding> out;
    std::map<std::string, std::set<std::string>> incoming;
    std::map<std::string, std::set<std::string>> outgoing;
    for (const auto& tr : spec.transitions) {
        incoming[tr.resource_type + ":" + tr.to_state].insert(tr.name);
        outgoing[tr.resource_type + ":" + tr.from_state].insert(tr.name);
        if (tr.from_state == "any") out.push_back({FindingSeverity::medium, "state.any", "transition accepts any source state", tr.name + " can bypass expected workflow ordering", tr.line});
    }
    for (const auto& resource : spec.resources) {
        for (const auto& state : resource.second.states) {
            auto key = resource.first + ":" + state;
            if (state != resource.second.initial_state && !incoming.count(key)) out.push_back({FindingSeverity::low, "state.unreachable", "state has no incoming transition", resource.first + "." + state + " may be unreachable", 0});
            if (!resource.second.terminal_states.count(state) && !outgoing.count(key)) out.push_back({FindingSeverity::low, "state.deadend", "non-terminal state has no outgoing transition", resource.first + "." + state + " may trap resources", 0});
        }
    }
    return out;
}
std::vector<Finding> find_replay_and_approval_gaps(const WorkflowSpec& spec) {
    std::vector<Finding> out;
    for (const auto& tr : spec.transitions) {
        auto profile = lookup_operation_profile(tr.name);
        bool replay_guard = false;
        bool not_owner_guard = false;
        for (const auto& g : tr.guards) {
            if (g.kind == CheckKind::not_replayed) replay_guard = true;
            if (g.kind == CheckKind::actor_not_owner) not_owner_guard = true;
        }
        bool money_like = tr.name.find("pay") != std::string::npos || tr.name.find("transfer") != std::string::npos || tr.name.find("refund") != std::string::npos;
        if (money_like && !tr.idempotent && !replay_guard) out.push_back({FindingSeverity::high, "logic.replay", "money-like transition lacks replay guard", tr.name + " may allow repeated operation effects", tr.line});
        if (tr.name.find("approve") != std::string::npos && !not_owner_guard) out.push_back({FindingSeverity::medium, "logic.self_approval", "approval lacks self-approval prevention", tr.name + " does not require actor_not_owner", tr.line});
        if (profile && std::string(profile->required_control) == "not_replayed" && !tr.idempotent && !replay_guard) out.push_back({FindingSeverity::high, "logic.catalog_replay", "cataloged operation lacks replay guard", operation_profile_summary(*profile), tr.line});
        if (profile && std::string(profile->required_control) == "not_owner" && !not_owner_guard) out.push_back({FindingSeverity::medium, "logic.catalog_separation", "cataloged operation lacks separation guard", operation_profile_summary(*profile), tr.line});
    }
    return out;
}
AuditReport audit_spec(const WorkflowSpec& spec) {
    AuditReport report;
    auto append = [&](std::vector<Finding> v) { report.findings.insert(report.findings.end(), v.begin(), v.end()); };
    append(find_authorization_gaps(spec));
    append(find_state_machine_gaps(spec));
    append(find_replay_and_approval_gaps(spec));
    return report;
}
AuditReport audit_execution(const WorkflowSpec&, const engine::ExecutionReport& exec) {
    AuditReport report;
    for (const auto& v : exec.invariant_violations) report.findings.push_back({FindingSeverity::high, "exec.invariant", "invariant violation observed", v, 0});
    for (const auto& d : exec.decisions) if (!d.allowed && d.reason == "transition not found") report.findings.push_back({FindingSeverity::low, "exec.unknown_action", "operation referenced unknown transition", d.action + " by " + d.actor, d.operation_index});
    return report;
}
std::string summarize_audit(const AuditReport& report) {
    std::ostringstream out;
    std::map<std::string, std::size_t> by_sev;
    for (const auto& f : report.findings) by_sev[severity_name(f.severity)]++;
    out << "findings=" << report.findings.size() << "\n";
    for (const auto& kv : by_sev) out << "severity." << kv.first << "=" << kv.second << "\n";
    for (const auto& f : report.findings) out << severity_name(f.severity) << ":" << f.code << ":" << f.line << ": " << f.title << " - " << f.detail << "\n";
    return out.str();
}

} // namespace logicsentinel::audit
