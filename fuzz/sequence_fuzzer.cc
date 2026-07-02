#include "engine/runner.hpp"
#include "model/workflow.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    static const char* spec_text =
        "principal alice roles=user mfa=true\n"
        "principal bob roles=user\n"
        "principal manager roles=manager mfa=true\n"
        "resource invoice initial=draft states=draft,submitted,approved,paid terminal=paid owner_required=true\n"
        "transition create resource=invoice from=any to=draft roles=user create=true effects=owner_actor,state=draft\n"
        "transition submit resource=invoice from=draft to=submitted roles=user guards=owner effects=state=submitted\n"
        "transition approve resource=invoice from=submitted to=approved roles=manager guards=not_owner,mfa effects=approve,state=approved\n"
        "transition pay resource=invoice from=approved to=paid roles=manager guards=not_replayed,mfa effects=state=paid\n"
        "invariant nonnegative resource=invoice check=amount_ge=0\n";
    auto spec = logicsentinel::model::parse_workflow_spec(spec_text);
    std::string text(reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data + size));
    auto seq = logicsentinel::model::parse_operation_sequence(text);
    if (spec && seq) {
        logicsentinel::engine::Runner runner(spec.value());
        auto report = runner.run(seq.value());
        (void)logicsentinel::engine::summarize_report(report);
    }
    return 0;
}

