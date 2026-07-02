# LogicSentinel

LogicSentinel is an offline C++17 toolkit for modeling and testing business-logic security workflows. It helps application security engineers describe roles, resources, state transitions, invariants, and operation sequences so local fuzzing and audits can exercise authorization and state-machine behavior without calling external services.

## Use Cases

- Review workflow specifications for missing role checks, self-approval paths, replay-sensitive actions, and unsafe state transitions.
- Execute local operation sequences against a deterministic stateful model.
- Produce audit summaries for authorization, approval, replay, and invariant findings.
- Reuse the C++ library in internal tooling for workflow and policy validation.
- Run developer robustness fuzzing over specs, sequences, combined policy inputs, and audit logic.

## Format

Workflow specs are line-oriented:

```text
principal alice roles=user mfa=true
resource invoice initial=draft states=draft,submitted,approved,paid terminal=paid
transition create resource=invoice from=any to=draft roles=user create=true effects=owner_actor,state=draft
transition approve resource=invoice from=submitted to=approved roles=manager guards=not_owner,mfa effects=approve,state=approved
invariant nonnegative resource=invoice check=amount_ge=0
```

Operation sequences are also line-oriented:

```text
op actor=alice action=create resource=invoice id=inv1 amount=10 request=r1
```

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## Tools

```bash
logiclint workflow.logic
sequencecheck sequence.ops
flowrun workflow.logic sequence.ops
```

## Developer Robustness Fuzzing

```bash
mkdir -p out
CXX=clang++ OUT=$PWD/out LIB_FUZZING_ENGINE=-fsanitize=fuzzer \
  CXXFLAGS="-fsanitize=address,undefined -g -O1" \
  bash .clusterfuzzlite/build.sh

out/spec_fuzzer fuzz/corpus/spec_fuzzer -dict=fuzz/dictionary.txt -runs=1000
out/sequence_fuzzer fuzz/corpus/sequence_fuzzer -dict=fuzz/dictionary.txt -runs=1000
out/policy_fuzzer fuzz/corpus/policy_fuzzer -dict=fuzz/dictionary.txt -runs=1000
out/audit_fuzzer fuzz/corpus/audit_fuzzer -dict=fuzz/dictionary.txt -runs=1000
```

The fuzz targets call real parser, runner, and audit code. They are intended to improve parser and workflow-engine robustness for defensive engineering use.

