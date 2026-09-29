cmake_minimum_required(VERSION 3.15)
project(VirtualHidChecks LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
add_executable(VirtualHidBackendTests VirtualHidBackendTests.cpp)
add_test(NAME VirtualHidBackendTests COMMAND VirtualHidBackendTests)

# No OpenCV, detector, capture, GPU or driver build dependency.
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
add_subdirectory(../../deps/yaml-cpp yaml-cpp EXCLUDE_FROM_ALL)
add_executable(VirtualHidCheck VirtualHidCheck.cpp
    ../../src/controller/VirtualHidBackend.cpp
    ../../src/controller/MouseController.cpp)
target_include_directories(VirtualHidCheck PRIVATE ../../deps/IbInputSimulator)
target_link_libraries(VirtualHidCheck PRIVATE yaml-cpp user32)
add_test(NAME VirtualHidControllerInvalidConfig COMMAND VirtualHidCheck --controller-noop)
set_tests_properties(VirtualHidControllerInvalidConfig PROPERTIES
    ENVIRONMENT "AIMER_INPUT_BACKEND=invalid_backend"
    PASS_REGULAR_EXPRESSION "movement and clicks disabled; no automatic fallback")
