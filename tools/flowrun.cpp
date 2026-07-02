#include "audit/auditor.hpp"
#include "engine/runner.hpp"
#include "model/workflow.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

static std::string read_all(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("unable to open file: " + path);
    std::ostringstream out; out << in.rdbuf(); return out.str();
}
int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "usage: flowrun <workflow.logic> <sequence.ops>\n"; return 2; }
    try {
        auto spec = logicsentinel::model::parse_workflow_spec(read_all(argv[1]));
        auto seq = logicsentinel::model::parse_operation_sequence(read_all(argv[2]));
        if (!spec) { std::cerr << spec.status().message << "\n"; return 1; }
        if (!seq) { std::cerr << seq.status().message << "\n"; return 1; }
        logicsentinel::engine::Runner runner(spec.value());
        auto report = runner.run(seq.value());
        std::cout << logicsentinel::engine::summarize_report(report);
        std::cout << logicsentinel::audit::summarize_audit(logicsentinel::audit::audit_execution(spec.value(), report));
    } catch (const std::exception& e) { std::cerr << e.what() << "\n"; return 1; }
    return 0;
}
