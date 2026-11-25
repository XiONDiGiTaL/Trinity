cmake_minimum_required(VERSION 3.20)
project(trinity VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The compatibility adapter needs the host's public nxrt headers. A native host
# implementation does not; it needs only include/nxp/nxp.h.
set(NXP_NXMOD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../EmblemWeave/engine/fefw-recomp"
    CACHE PATH "Checkout containing runtime/include/nxrt/mod_api.h and recomp/include/nxrecomp/ctx.h")

option(TRINITY_BUILD_TESTS "Build the standalone host harness" ON)
option(TRINITY_BUILD_DEMO "Build the minimal hello-panel plugin" ON)

set(TRINITY_HOST_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${NXP_NXMOD_ROOT}/runtime/include"
    "${NXP_NXMOD_ROOT}/recomp/include")

# The reference plugin is both an NXP plugin and, through the compatibility
# adapter, a legacy nxmod_init plugin. Keeping the adapter in the target makes
# the first host integration possible without changing a host ABI.
add_library(trinity_core SHARED
    plugins/trinity_core/trinity_plugin.cpp
    hosts/compat/nxmod_adapter.cpp)
target_include_directories(trinity_core PRIVATE ${TRINITY_HOST_INCLUDES})
target_compile_definitions(trinity_core PRIVATE _CRT_SECURE_NO_WARNINGS)
set_target_properties(trinity_core PROPERTIES
    PREFIX ""
    OUTPUT_NAME "trinity")

if(TRINITY_BUILD_DEMO)
    add_library(nxp_demo SHARED
        examples/hello_panel/hello_panel.cpp
        hosts/compat/nxmod_adapter.cpp)
    target_include_directories(nxp_demo PRIVATE ${TRINITY_HOST_INCLUDES})
    target_compile_definitions(nxp_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
    set_target_properties(nxp_demo PROPERTIES
        PREFIX ""
        OUTPUT_NAME "nxp_demo")
endif()

if(TRINITY_BUILD_TESTS)
    enable_testing()
    add_executable(nxp_host_test tests/nxp_host_test.cpp)
    target_include_directories(nxp_host_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_compile_definitions(nxp_host_test PRIVATE _CRT_SECURE_NO_WARNINGS)

    add_test(NAME trinity_core_host_test
        COMMAND nxp_host_test "$<TARGET_FILE:trinity_core>" trinity)
    if(TRINITY_BUILD_DEMO)
        add_test(NAME hello_panel_host_test
            COMMAND nxp_host_test "$<TARGET_FILE:nxp_demo>" demo)
    endif()
endif()
