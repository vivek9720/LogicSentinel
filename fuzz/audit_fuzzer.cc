#include "audit/auditor.hpp"
#include "model/workflow.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string text(reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data + size));
    auto spec = logicsentinel::model::parse_workflow_spec(text);
    if (spec) {
        auto a = logicsentinel::audit::find_authorization_gaps(spec.value());
        auto s = logicsentinel::audit::find_state_machine_gaps(spec.value());
        auto r = logicsentinel::audit::find_replay_and_approval_gaps(spec.value());
        (void)a.size();
        (void)s.size();
        (void)r.size();
    }
    return 0;
}

