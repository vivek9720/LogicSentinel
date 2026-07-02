#include "audit/auditor.hpp"
#include "model/workflow.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string text(reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data + size));
    auto spec = logicsentinel::model::parse_workflow_spec(text);
    if (spec) {
        (void)logicsentinel::model::summarize_spec(spec.value());
        auto report = logicsentinel::audit::audit_spec(spec.value());
        (void)logicsentinel::audit::summarize_audit(report);
    }
    return 0;
}

