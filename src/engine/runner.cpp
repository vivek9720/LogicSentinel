#include "engine/runner.hpp"
#include "ls_core/strings.hpp"
#include <sstream>

namespace logicsentinel::engine {
using namespace logicsentinel::model;

static std::int64_t bounded_i64(const std::string& value, std::int64_t fallback = 0) {
    if (!core::is_integer(value)) return fallback;
    bool negative = !value.empty() && value.front() == '-';
    std::size_t start = negative ? 1 : 0;
    std::int64_t parsed = 0;
    for (std::size_t i = start; i < value.size(); ++i) {
        parsed = parsed * 10 + static_cast<std::int64_t>(value[i] - '0');
        if (parsed > 1000000000000LL) return fallback;
    }
    return negative ? -parsed : parsed;
}

Runner::Runner(WorkflowSpec spec) : spec_(std::move(spec)) {}

const Transition* Runner::find_transition(const Operation& operation) const {
    for (const auto& t : spec_.transitions) if (t.name == operation.action && t.resource_type == operation.resource_type) return &t;
    return nullptr;
}
bool Runner::principal_has_role(const std::string& actor, const std::set<std::string>& roles) const {
    if (roles.empty()) return true;
    auto it = spec_.principals.find(actor);
    if (it == spec_.principals.end()) return false;
    for (const auto& role : roles) if (it->second.roles.count(role)) return true;
    return false;
}
std::string Runner::replay_key(const Operation& operation) const {
    auto it = operation.params.find("request");
    auto request = it == operation.params.end() ? operation.actor + ":" + operation.action + ":" + operation.resource_id : it->second;
    return operation.resource_type + ":" + request;
}
bool Runner::guard_passes(const Guard& guard, const Operation& operation, const Entity* entity, std::string& reason) const {
    switch (guard.kind) {
        case CheckKind::actor_has_role: {
            std::set<std::string> roles;
            roles.insert(guard.value);
            bool ok = principal_has_role(operation.actor, roles);
            reason = ok ? "role ok" : "role missing";
            return ok;
        }
        case CheckKind::actor_is_owner: {
            bool ok = entity && entity->owner == operation.actor;
            reason = ok ? "owner ok" : "actor is not owner";
            return ok;
        }
        case CheckKind::actor_not_owner: {
            bool ok = !entity || entity->owner != operation.actor;
            reason = ok ? "not owner ok" : "actor owns resource";
            return ok;
        }
        case CheckKind::amount_at_most: {
            auto it = operation.params.find("amount");
            auto amount = it == operation.params.end() ? (entity ? entity->amount : 0) : bounded_i64(it->second);
            bool ok = amount <= guard.number;
            reason = ok ? "amount upper bound ok" : "amount exceeds bound";
            return ok;
        }
        case CheckKind::amount_at_least: {
            auto it = operation.params.find("amount");
            auto amount = it == operation.params.end() ? (entity ? entity->amount : 0) : bounded_i64(it->second);
            bool ok = amount >= guard.number;
            reason = ok ? "amount lower bound ok" : "amount below bound";
            return ok;
        }
        case CheckKind::field_equals: {
            if (!entity) { reason = "resource missing"; return false; }
            auto it = entity->fields.find(guard.field);
            bool ok = it != entity->fields.end() && it->second == guard.value;
            reason = ok ? "field ok" : "field mismatch";
            return ok;
        }
        case CheckKind::state_is: {
            bool ok = entity && entity->state == guard.value;
            reason = ok ? "state ok" : "state mismatch";
            return ok;
        }
        case CheckKind::not_replayed: {
            bool ok = !replay_seen_.count(replay_key(operation));
            reason = ok ? "not replayed" : "replay detected";
            return ok;
        }
        case CheckKind::requires_mfa: {
            auto it = spec_.principals.find(operation.actor);
            bool ok = it != spec_.principals.end() && it->second.mfa;
            reason = ok ? "mfa ok" : "mfa missing";
            return ok;
        }
        default: reason = "no guard"; return true;
    }
}
void Runner::apply_effect(const Effect& effect, const Operation& operation, Entity& entity) {
    switch (effect.kind) {
        case EffectKind::set_owner_actor: entity.owner = operation.actor; break;
        case EffectKind::set_owner_param: {
            auto it = operation.params.find(effect.value.empty() ? "owner" : effect.value);
            if (it != operation.params.end()) entity.owner = it->second;
            break;
        }
        case EffectKind::set_state: entity.state = effect.value; break;
        case EffectKind::add_amount: entity.amount += effect.number; break;
        case EffectKind::subtract_amount: entity.amount -= effect.number; break;
        case EffectKind::mark_approved: entity.approved = true; entity.fields["approved_by"] = operation.actor; break;
        case EffectKind::lock_resource: entity.locked = true; break;
        case EffectKind::unlock_resource: entity.locked = false; break;
        default: break;
    }
}
void Runner::check_invariants(const Operation& operation, const Entity& entity) {
    for (const auto& inv : spec_.invariants) {
        if (!inv.resource_type.empty() && inv.resource_type != entity.type) continue;
        bool violation = false;
        if (inv.kind == CheckKind::amount_at_least && entity.amount < inv.number) violation = true;
        else if (inv.kind == CheckKind::amount_at_most && entity.amount > inv.number) violation = true;
        else if (inv.kind == CheckKind::field_equals) {
            auto it = entity.fields.find(inv.field);
            violation = it == entity.fields.end() || it->second != inv.value;
        }
        if (violation) invariant_violations_.push_back(inv.name + " after " + operation.action + " on " + entity.id);
    }
}
Decision Runner::apply(const Operation& operation, std::size_t index) {
    Decision d;
    d.actor = operation.actor;
    d.action = operation.action;
    d.operation_index = index;
    d.resource_key = operation.resource_type + ":" + operation.resource_id;
    const auto* transition = find_transition(operation);
    if (!transition) { d.reason = "transition not found"; return d; }
    if (!principal_has_role(operation.actor, transition->allowed_roles)) { d.reason = "actor role not allowed"; return d; }
    Entity* entity = nullptr;
    auto existing = entities_.find(d.resource_key);
    if (existing != entities_.end()) entity = &existing->second;
    if (!entity && !transition->creates_resource) { d.reason = "resource not found"; return d; }
    if (entity && transition->from_state != "any" && transition->from_state != entity->state) { d.reason = "transition from-state mismatch"; return d; }
    for (const auto& guard : transition->guards) {
        std::string reason;
        if (!guard_passes(guard, operation, entity, reason)) { d.guard_results.push_back(guard.to_string() + ":" + reason); d.reason = reason; return d; }
        d.guard_results.push_back(guard.to_string() + ":" + reason);
    }
    if (!entity) {
        Entity created;
        created.type = operation.resource_type;
        created.id = operation.resource_id;
        auto rt = spec_.resources.find(operation.resource_type);
        created.state = rt == spec_.resources.end() ? "new" : rt->second.initial_state;
        auto inserted = entities_.emplace(d.resource_key, std::move(created));
        entity = &inserted.first->second;
    }
    entity->state = transition->to_state;
    for (const auto& kv : operation.params) if (kv.first != "actor" && kv.first != "action" && kv.first != "resource" && kv.first != "id") entity->fields[kv.first] = kv.second;
    for (const auto& effect : transition->effects) apply_effect(effect, operation, *entity);
    replay_seen_.insert(replay_key(operation));
    check_invariants(operation, *entity);
    d.allowed = true;
    d.reason = "allowed";
    return d;
}
ExecutionReport Runner::run(const OperationSequence& sequence) {
    ExecutionReport report;
    for (std::size_t i = 0; i < sequence.operations.size(); ++i) report.decisions.push_back(apply(sequence.operations[i], i));
    report.entities = entities_;
    report.invariant_violations = invariant_violations_;
    report.replay_keys.assign(replay_seen_.begin(), replay_seen_.end());
    return report;
}
std::string summarize_report(const ExecutionReport& report) {
    std::ostringstream out;
    std::size_t allowed = 0, denied = 0;
    for (const auto& d : report.decisions) d.allowed ? ++allowed : ++denied;
    out << "decisions=" << report.decisions.size() << "\nallowed=" << allowed << "\ndenied=" << denied << "\nentities=" << report.entities.size() << "\ninvariant_violations=" << report.invariant_violations.size() << "\n";
    for (const auto& v : report.invariant_violations) out << "violation=" << v << "\n";
    return out.str();
}

} // namespace logicsentinel::engine
