#include "audit/auditor.hpp"
#include "engine/runner.hpp"
#include "model/workflow.hpp"
#include <cstdlib>
#include <iostream>

static void require(bool value, const char* msg) {
    if (!value) { std::cerr << "test failed: " << msg << "\n"; std::exit(1); }
}
static const char* kSpec =
    "principal alice roles=user mfa=true\n"
    "principal manager roles=manager mfa=true\n"
    "resource invoice initial=draft states=draft,submitted,approved,paid terminal=paid owner_required=true\n"
    "transition create resource=invoice from=any to=draft roles=user create=true effects=owner_actor,state=draft\n"
    "transition submit resource=invoice from=draft to=submitted roles=user guards=owner effects=state=submitted\n"
    "transition approve resource=invoice from=submitted to=approved roles=manager guards=not_owner,mfa effects=approve,state=approved\n"
    "transition pay resource=invoice from=approved to=paid roles=manager guards=not_replayed,mfa effects=state=paid\n"
    "invariant nonnegative resource=invoice check=amount_ge=0\n";
static const char* kSeq =
    "op actor=alice action=create resource=invoice id=inv1 amount=10 request=r1\n"
    "op actor=alice action=submit resource=invoice id=inv1 request=r2\n"
    "op actor=manager action=approve resource=invoice id=inv1 request=r3\n"
    "op actor=manager action=pay resource=invoice id=inv1 request=r4\n";

int main() {
    auto spec = logicsentinel::model::parse_workflow_spec(kSpec);
    require(spec.ok(), "spec parses");
    require(spec.value().transitions.size() == 4, "transition count");
    auto seq = logicsentinel::model::parse_operation_sequence(kSeq);
    require(seq.ok(), "sequence parses");
    logicsentinel::engine::Runner runner(spec.value());
    auto report = runner.run(seq.value());
    require(report.decisions.size() == 4, "decision count");
    require(report.decisions.back().allowed, "final operation allowed");
    auto audit = logicsentinel::audit::audit_spec(spec.value());
    require(audit.findings.empty(), "clean spec audit");
    auto risky = logicsentinel::model::parse_workflow_spec(
        "principal a roles=user\n"
        "resource order initial=new states=new,done\n"
        "transition approve resource=order from=any to=done roles=user effects=state=done\n");
    require(risky.ok(), "risky spec parses");
    auto risky_audit = logicsentinel::audit::audit_spec(risky.value());
    require(!risky_audit.findings.empty(), "risky spec findings");
    std::cout << "all tests passed\n";
    return 0;
}

