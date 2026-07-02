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
    if (argc != 2) { std::cerr << "usage: sequencecheck <sequence.ops>\n"; return 2; }
    try {
        auto seq = logicsentinel::model::parse_operation_sequence(read_all(argv[1]));
        if (!seq) { std::cerr << seq.status().message << "\n"; return 1; }
        std::cout << logicsentinel::model::summarize_sequence(seq.value());
    } catch (const std::exception& e) { std::cerr << e.what() << "\n"; return 1; }
    return 0;
}
