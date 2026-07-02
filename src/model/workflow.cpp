#include "model/workflow.hpp"
#include "ls_core/strings.hpp"
#include <algorithm>
#include <sstream>

namespace logicsentinel::model {
using namespace logicsentinel::core;

std::string check_name(CheckKind kind) {
    switch (kind) {
        case CheckKind::actor_has_role: return "actor_has_role";
        case CheckKind::actor_is_owner: return "actor_is_owner";
        case CheckKind::actor_not_owner: return "actor_not_owner";
        case CheckKind::amount_at_most: return "amount_at_most";
        case CheckKind::amount_at_least: return "amount_at_least";
        case CheckKind::field_equals: return "field_equals";
        case CheckKind::state_is: return "state_is";
        case CheckKind::not_replayed: return "not_replayed";
        case CheckKind::requires_mfa: return "requires_mfa";
        default: return "none";
    }
}
std::string effect_name(EffectKind kind) {
    switch (kind) {
        case EffectKind::set_owner_actor: return "set_owner_actor";
        case EffectKind::set_owner_param: return "set_owner_param";
        case EffectKind::set_state: return "set_state";
        case EffectKind::add_amount: return "add_amount";
        case EffectKind::subtract_amount: return "subtract_amount";
        case EffectKind::mark_approved: return "mark_approved";
        case EffectKind::lock_resource: return "lock_resource";
        case EffectKind::unlock_resource: return "unlock_resource";
        default: return "none";
    }
}
std::string Guard::to_string() const {
    std::ostringstream out;
    out << check_name(kind);
    if (!field.empty()) out << ":" << field;
    if (!value.empty()) out << "=" << value;
    if (number != 0) out << "#" << number;
    return out.str();
}
std::string Effect::to_string() const {
    std::ostringstream out;
    out << effect_name(kind);
    if (!field.empty()) out << ":" << field;
    if (!value.empty()) out << "=" << value;
    if (number != 0) out << "#" << number;
    return out.str();
}
static std::set<std::string> parse_set(const std::string& value) {
    std::set<std::string> out;
    for (auto part : split(value, ',', false)) { part = trim(part); if (!part.empty()) out.insert(part); }
    return out;
}
static std::int64_t safe_i64(const std::string& value, std::int64_t fallback = 0) {
    if (!is_integer(value)) return fallback;
    std::int64_t out = 0;
    bool neg = !value.empty() && value[0] == '-';
    std::size_t start = neg ? 1 : 0;
    for (std::size_t i = start; i < value.size(); ++i) {
        out = out * 10 + static_cast<std::int64_t>(value[i] - '0');
        if (out > 1000000000000LL) return fallback;
    }
    return neg ? -out : out;
}
core::Result<Guard> parse_guard(const std::string& token) {
    auto t = trim(token);
    if (t.empty() || t == "none") return Guard{};
    Guard g;
    auto eq = t.find('=');
    auto body = eq == std::string::npos ? t : t.substr(0, eq);
    auto val = eq == std::string::npos ? std::string{} : strip_quotes(t.substr(eq + 1));
    auto parts = split(body, ':', true);
    auto name = to_lower(parts.empty() ? body : parts[0]);
    g.field = parts.size() > 1 ? parts[1] : "";
    g.value = val;
    if (name == "role" || name == "actor_has_role") g.kind = CheckKind::actor_has_role;
    else if (name == "owner" || name == "actor_is_owner") g.kind = CheckKind::actor_is_owner;
    else if (name == "not_owner" || name == "actor_not_owner") g.kind = CheckKind::actor_not_owner;
    else if (name == "amount_le" || name == "amount_at_most") { g.kind = CheckKind::amount_at_most; g.number = safe_i64(val); }
    else if (name == "amount_ge" || name == "amount_at_least") { g.kind = CheckKind::amount_at_least; g.number = safe_i64(val); }
    else if (name == "field" || name == "field_equals") g.kind = CheckKind::field_equals;
    else if (name == "state" || name == "state_is") g.kind = CheckKind::state_is;
    else if (name == "not_replayed") g.kind = CheckKind::not_replayed;
    else if (name == "mfa" || name == "requires_mfa") g.kind = CheckKind::requires_mfa;
    else return Status::failure("unknown guard: " + token);
    return g;
}
core::Result<Effect> parse_effect(const std::string& token) {
    auto t = trim(token);
    if (t.empty() || t == "none") return Effect{};
    Effect e;
    auto eq = t.find('=');
    auto body = eq == std::string::npos ? t : t.substr(0, eq);
    auto val = eq == std::string::npos ? std::string{} : strip_quotes(t.substr(eq + 1));
    auto parts = split(body, ':', true);
    auto name = to_lower(parts.empty() ? body : parts[0]);
    e.field = parts.size() > 1 ? parts[1] : "";
    e.value = val;
    if (name == "owner_actor" || name == "set_owner_actor") e.kind = EffectKind::set_owner_actor;
    else if (name == "owner_param" || name == "set_owner_param") e.kind = EffectKind::set_owner_param;
    else if (name == "state" || name == "set_state") e.kind = EffectKind::set_state;
    else if (name == "add_amount") { e.kind = EffectKind::add_amount; e.number = safe_i64(val); }
    else if (name == "subtract_amount") { e.kind = EffectKind::subtract_amount; e.number = safe_i64(val); }
    else if (name == "approve" || name == "mark_approved") e.kind = EffectKind::mark_approved;
    else if (name == "lock") e.kind = EffectKind::lock_resource;
    else if (name == "unlock") e.kind = EffectKind::unlock_resource;
    else return Status::failure("unknown effect: " + token);
    return e;
}
static core::Result<Transition> parse_transition(const std::vector<std::string>& tokens, std::size_t line) {
    if (tokens.size() < 2) return Status::failure("transition missing name");
    Transition t;
    t.name = tokens[1];
    t.line = line;
    auto kv = parse_kv_tokens(tokens, 2);
    t.resource_type = kv.count("resource") ? kv["resource"] : kv.count("type") ? kv["type"] : "";
    t.from_state = kv.count("from") ? kv["from"] : "any";
    t.to_state = kv.count("to") ? kv["to"] : t.from_state;
    t.allowed_roles = kv.count("roles") ? parse_set(kv["roles"]) : std::set<std::string>{};
    t.idempotent = kv.count("idempotent") && to_lower(kv["idempotent"]) != "false";
    t.audit_required = kv.count("audit") && to_lower(kv["audit"]) != "false";
    t.creates_resource = kv.count("create") && to_lower(kv["create"]) != "false";
    if (kv.count("guards")) {
        for (const auto& raw : split(kv["guards"], ',', false)) {
            auto g = parse_guard(raw);
            if (!g) return g.status();
            if (g.value().kind != CheckKind::none) t.guards.push_back(g.value());
        }
    }
    if (kv.count("effects")) {
        for (const auto& raw : split(kv["effects"], ',', false)) {
            auto e = parse_effect(raw);
            if (!e) return e.status();
            if (e.value().kind != EffectKind::none) t.effects.push_back(e.value());
        }
    }
    if (t.resource_type.empty()) return Status::failure("transition missing resource type");
    return t;
}
core::Result<WorkflowSpec> parse_workflow_spec(const std::string& text) {
    WorkflowSpec spec;
    std::size_t line_no = 0;
    for (auto line : lines(text)) {
        line_no++;
        auto clean = trim(remove_comment(line));
        if (clean.empty()) continue;
        auto tokens = split_ws_quoted(clean);
        if (tokens.empty()) continue;
        auto directive = to_lower(tokens[0]);
        if (directive == "meta") {
            auto kv = parse_kv_tokens(tokens, 1);
            for (const auto& p : kv) spec.metadata[p.first] = p.second;
        } else if (directive == "principal" || directive == "actor") {
            if (tokens.size() < 2) { spec.diagnostics.warn("spec.principal", "principal missing name", line_no); continue; }
            Principal p; p.name = tokens[1];
            auto kv = parse_kv_tokens(tokens, 2);
            if (kv.count("roles")) p.roles = parse_set(kv["roles"]);
            if (kv.count("mfa")) p.mfa = to_lower(kv["mfa"]) == "true" || kv["mfa"] == "1";
            spec.principals[p.name] = p;
        } else if (directive == "resource") {
            if (tokens.size() < 2) { spec.diagnostics.warn("spec.resource", "resource missing name", line_no); continue; }
            ResourceType r; r.name = tokens[1];
            auto kv = parse_kv_tokens(tokens, 2);
            if (kv.count("initial")) r.initial_state = kv["initial"];
            if (kv.count("states")) r.states = parse_set(kv["states"]);
            if (kv.count("terminal")) r.terminal_states = parse_set(kv["terminal"]);
            if (kv.count("owner_required")) r.owner_required = to_lower(kv["owner_required"]) != "false";
            r.states.insert(r.initial_state);
            spec.resources[r.name] = r;
        } else if (directive == "transition") {
            auto tr = parse_transition(tokens, line_no);
            if (!tr) spec.diagnostics.warn("spec.transition", tr.status().message, line_no);
            else spec.transitions.push_back(tr.value());
        } else if (directive == "invariant") {
            if (tokens.size() < 2) { spec.diagnostics.warn("spec.invariant", "invariant missing name", line_no); continue; }
            Invariant inv; inv.name = tokens[1]; inv.line = line_no;
            auto kv = parse_kv_tokens(tokens, 2);
            inv.resource_type = kv.count("resource") ? kv["resource"] : "";
            if (kv.count("check")) {
                auto g = parse_guard(kv["check"]);
                if (g) { inv.kind = g.value().kind; inv.field = g.value().field; inv.value = g.value().value; inv.number = g.value().number; }
                else spec.diagnostics.warn("spec.invariant", g.status().message, line_no);
            }
            spec.invariants.push_back(inv);
        } else spec.diagnostics.warn("spec.directive", "unknown directive " + tokens[0], line_no);
    }
    for (const auto& warning : validate_spec(spec)) spec.diagnostics.warn("spec.validate", warning, 0);
    return spec;
}
core::Result<OperationSequence> parse_operation_sequence(const std::string& text) {
    OperationSequence seq;
    std::size_t line_no = 0;
    for (auto line : lines(text)) {
        line_no++;
        auto clean = trim(remove_comment(line));
        if (clean.empty()) continue;
        auto tokens = split_ws_quoted(clean);
        if (tokens.empty()) continue;
        auto directive = to_lower(tokens[0]);
        if (directive != "op" && directive != "operation") { seq.diagnostics.warn("sequence.directive", "unknown sequence directive", line_no); continue; }
        auto kv = parse_kv_tokens(tokens, 1);
        Operation op;
        op.line = line_no;
        op.actor = kv.count("actor") ? kv["actor"] : "";
        op.action = kv.count("action") ? kv["action"] : "";
        op.resource_type = kv.count("resource") ? kv["resource"] : kv.count("type") ? kv["type"] : "";
        op.resource_id = kv.count("id") ? kv["id"] : "";
        op.params = kv;
        if (op.actor.empty() || op.action.empty() || op.resource_type.empty() || op.resource_id.empty()) seq.diagnostics.warn("sequence.operation", "operation missing actor/action/resource/id", line_no);
        seq.operations.push_back(std::move(op));
    }
    return seq;
}
std::vector<std::string> validate_spec(const WorkflowSpec& spec) {
    std::vector<std::string> out;
    for (const auto& tr : spec.transitions) {
        if (!spec.resources.count(tr.resource_type)) out.push_back("transition " + tr.name + " references unknown resource " + tr.resource_type);
        if (tr.allowed_roles.empty()) out.push_back("transition " + tr.name + " has no role restriction");
        bool has_replay_guard = false;
        bool has_owner_guard = false;
        for (const auto& g : tr.guards) {
            if (g.kind == CheckKind::not_replayed) has_replay_guard = true;
            if (g.kind == CheckKind::actor_is_owner || g.kind == CheckKind::actor_not_owner) has_owner_guard = true;
        }
        if (!tr.idempotent && !has_replay_guard && (tr.name.find("pay") != std::string::npos || tr.name.find("transfer") != std::string::npos || tr.name.find("approve") != std::string::npos)) out.push_back("sensitive transition " + tr.name + " lacks replay guard");
        if (tr.name.find("approve") != std::string::npos && !has_owner_guard) out.push_back("approval transition " + tr.name + " lacks ownership guard");
    }
    for (const auto& inv : spec.invariants) if (!inv.resource_type.empty() && !spec.resources.count(inv.resource_type)) out.push_back("invariant " + inv.name + " references unknown resource");
    return out;
}
std::string summarize_spec(const WorkflowSpec& spec) {
    std::ostringstream out;
    out << "principals=" << spec.principals.size() << "\nresources=" << spec.resources.size() << "\ntransitions=" << spec.transitions.size() << "\ninvariants=" << spec.invariants.size() << "\n";
    if (!spec.diagnostics.empty()) out << spec.diagnostics.summary();
    return out.str();
}
std::string summarize_sequence(const OperationSequence& sequence) {
    std::ostringstream out;
    out << "operations=" << sequence.operations.size() << "\n";
    if (!sequence.diagnostics.empty()) out << sequence.diagnostics.summary();
    return out.str();
}

} // namespace logicsentinel::model

