cmake_minimum_required(VERSION 3.20)
project(credential_providers VERSION ${OPENDAQ_EXAMPLES_VERSION})

find_package(openDAQ)
add_executable(credential_providers credential_providers.cpp)
target_link_libraries(credential_providers PRIVATE daq::opendaq)

add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")
