#pragma once
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "ls_core/diagnostics.hpp"
#include "ls_core/result.hpp"

namespace logicsentinel::model {

enum class EffectKind { none, set_owner_actor, set_owner_param, set_state, add_amount, subtract_amount, mark_approved, lock_resource, unlock_resource };
enum class CheckKind { none, actor_has_role, actor_is_owner, actor_not_owner, amount_at_most, amount_at_least, field_equals, state_is, not_replayed, requires_mfa };

struct Principal {
    std::string name;
    std::set<std::string> roles;
    bool mfa = false;
};

struct ResourceType {
    std::string name;
    std::string initial_state = "new";
    std::set<std::string> states;
    std::set<std::string> terminal_states;
    bool owner_required = false;
};

struct Guard {
    CheckKind kind = CheckKind::none;
    std::string field;
    std::string value;
    std::int64_t number = 0;
    std::string to_string() const;
};

struct Effect {
    EffectKind kind = EffectKind::none;
    std::string field;
    std::string value;
    std::int64_t number = 0;
    std::string to_string() const;
};

struct Transition {
    std::string name;
    std::string resource_type;
    std::string from_state;
    std::string to_state;
    std::set<std::string> allowed_roles;
    std::vector<Guard> guards;
    std::vector<Effect> effects;
    bool idempotent = false;
    bool audit_required = false;
    bool creates_resource = false;
    std::size_t line = 0;
};

struct Invariant {
    std::string name;
    CheckKind kind = CheckKind::none;
    std::string resource_type;
    std::string field;
    std::string value;
    std::int64_t number = 0;
    std::size_t line = 0;
};

struct WorkflowSpec {
    std::map<std::string, Principal> principals;
    std::map<std::string, ResourceType> resources;
    std::vector<Transition> transitions;
    std::vector<Invariant> invariants;
    std::map<std::string, std::string> metadata;
    core::Diagnostics diagnostics;
};

struct Operation {
    std::string actor;
    std::string action;
    std::string resource_type;
    std::string resource_id;
    std::map<std::string, std::string> params;
    std::size_t line = 0;
};

struct OperationSequence {
    std::vector<Operation> operations;
    core::Diagnostics diagnostics;
};

std::string check_name(CheckKind kind);
std::string effect_name(EffectKind kind);
core::Result<WorkflowSpec> parse_workflow_spec(const std::string& text);
core::Result<OperationSequence> parse_operation_sequence(const std::string& text);
core::Result<Guard> parse_guard(const std::string& token);
core::Result<Effect> parse_effect(const std::string& token);
std::string summarize_spec(const WorkflowSpec& spec);
std::string summarize_sequence(const OperationSequence& sequence);
std::vector<std::string> validate_spec(const WorkflowSpec& spec);

} // namespace logicsentinel::model

