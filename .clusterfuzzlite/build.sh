#!/bin/bash -eu
PROJECT="${SRC:-$(pwd)}"
cd "$PROJECT"
: "${OUT:?OUT must be set}"
mkdir -p "$OUT"
COMMON_FLAGS="${CXXFLAGS:-} -std=c++17 -I$PROJECT/include"
SOURCES=$(find "$PROJECT/src" -name '*.cpp' | sort)
for target in spec_fuzzer sequence_fuzzer policy_fuzzer audit_fuzzer; do
  $CXX $COMMON_FLAGS "$PROJECT/fuzz/${target}.cc" $SOURCES ${LIB_FUZZING_ENGINE:-} -o "$OUT/$target"
done

