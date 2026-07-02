#include "audit/auditor.hpp"
#include "engine/runner.hpp"
#include "model/workflow.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string text(reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data + size));
    auto split = text.find("\n---\n");
    auto spec_text = split == std::string::npos ? text : text.substr(0, split);
    auto seq_text = split == std::string::npos ? std::string{} : text.substr(split + 5);
    auto spec = logicsentinel::model::parse_workflow_spec(spec_text);
    auto seq = logicsentinel::model::parse_operation_sequence(seq_text);
    if (spec) {
        auto spec_audit = logicsentinel::audit::audit_spec(spec.value());
        (void)logicsentinel::audit::summarize_audit(spec_audit);
        if (seq) {
            logicsentinel::engine::Runner runner(spec.value());
            auto exec = runner.run(seq.value());
            auto exec_audit = logicsentinel::audit::audit_execution(spec.value(), exec);
            (void)logicsentinel::audit::summarize_audit(exec_audit);
        }
    }
    return 0;
}

