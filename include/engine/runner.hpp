#pragma once
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "model/workflow.hpp"

namespace logicsentinel::engine {

struct Entity {
    std::string type;
    std::string id;
    std::string state;
    std::string owner;
    std::int64_t amount = 0;
    bool approved = false;
    bool locked = false;
    std::map<std::string, std::string> fields;
};

struct Decision {
    bool allowed = false;
    std::string action;
    std::string actor;
    std::string resource_key;
    std::string reason;
    std::vector<std::string> guard_results;
    std::size_t operation_index = 0;
};

struct ExecutionReport {
    std::vector<Decision> decisions;
    std::map<std::string, Entity> entities;
    std::vector<std::string> invariant_violations;
    std::vector<std::string> replay_keys;
};

class Runner {
public:
    explicit Runner(model::WorkflowSpec spec);
    Decision apply(const model::Operation& operation, std::size_t index);
    ExecutionReport run(const model::OperationSequence& sequence);
    const std::map<std::string, Entity>& entities() const { return entities_; }
    const std::vector<std::string>& invariant_violations() const { return invariant_violations_; }
private:
    model::WorkflowSpec spec_;
    std::map<std::string, Entity> entities_;
    std::set<std::string> replay_seen_;
    std::vector<std::string> invariant_violations_;
    const model::Transition* find_transition(const model::Operation& operation) const;
    bool principal_has_role(const std::string& actor, const std::set<std::string>& roles) const;
    bool guard_passes(const model::Guard& guard, const model::Operation& operation, const Entity* entity, std::string& reason) const;
    void apply_effect(const model::Effect& effect, const model::Operation& operation, Entity& entity);
    void check_invariants(const model::Operation& operation, const Entity& entity);
    std::string replay_key(const model::Operation& operation) const;
};

std::string summarize_report(const ExecutionReport& report);

} // namespace logicsentinel::engine

