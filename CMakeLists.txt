cmake_minimum_required(VERSION 3.12)
project(LogicSentinel VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(logicsentinel
  src/ls_core/diagnostics.cpp
  src/ls_core/strings.cpp
  src/model/workflow.cpp
  src/engine/runner.cpp
  src/audit/catalog.cpp
  src/audit/auditor.cpp
)
target_include_directories(logicsentinel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(logicsentinel PRIVATE /W4)
else()
  target_compile_options(logicsentinel PRIVATE -Wall -Wextra -Wpedantic)
endif()

foreach(tool logiclint flowrun sequencecheck)
  add_executable(${tool} tools/${tool}.cpp)
  target_link_libraries(${tool} PRIVATE logicsentinel)
endforeach()

enable_testing()
add_executable(logicsentinel_tests tests/test_main.cpp)
target_link_libraries(logicsentinel_tests PRIVATE logicsentinel)
add_test(NAME logicsentinel_tests COMMAND logicsentinel_tests)
