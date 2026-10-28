cmake_minimum_required(VERSION 3.22)

project(Respira VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Respira has no plug-in yet: this builds the JUCE-free engine, its offline
# tools and its tests.
option(RESPIRA_BUILD_TOOLS "Build the offline renderers and reed tools" ON)
option(RESPIRA_ENABLE_SANITIZERS "Instrument development builds with ASan and UBSan" OFF)
option(RESPIRA_BUILD_FUZZER "Build RespiraDspFuzz as a Clang libFuzzer target" OFF)
option(RESPIRA_REQUIRE_PYTHON_TESTS "Fail configuration when the Python checks cannot run" OFF)

include(CTest)

if(RESPIRA_ENABLE_SANITIZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(FATAL_ERROR "RESPIRA_ENABLE_SANITIZERS requires Clang or GCC")
    endif()
    add_compile_options(-fsanitize=address,undefined,float-cast-overflow
                        -fno-sanitize-recover=all -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined,float-cast-overflow)
endif()

set(RESPIRA_DSP_SOURCES Source/DSP/RespiraEngine.cpp)
set(RESPIRA_DSP_HEADERS
    Source/DSP/Decimator.h
    Source/DSP/DspMath.h
    Source/DSP/Mechanics.h
    Source/DSP/OutputStage.h
    Source/DSP/ReedBank.h
    Source/DSP/ReedModel.h
    Source/DSP/ReedTuning.h
    Source/DSP/RespiraControls.h
    Source/DSP/RespiraEngine.h
    Source/DSP/RespiraPresets.h)

if(MSVC)
    set(RESPIRA_WARNINGS /W4 /permissive-)
else()
    set(RESPIRA_WARNINGS -Wall -Wextra -Wpedantic)
endif()

add_library(RespiraDSP STATIC ${RESPIRA_DSP_SOURCES} ${RESPIRA_DSP_HEADERS})
target_include_directories(RespiraDSP PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_compile_features(RespiraDSP PUBLIC cxx_std_20)
set_target_properties(RespiraDSP PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(RespiraDSP PRIVATE ${RESPIRA_WARNINGS})

# The Rack Extension toolchain is C++17 with -fno-builtin. Compiling the
# shipping engine that way here keeps Source/DSP portable for the Rack adapter.
if(NOT MSVC)
    add_library(RespiraDSPCxx17 OBJECT ${RESPIRA_DSP_SOURCES})
    target_include_directories(RespiraDSPCxx17 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    set_target_properties(RespiraDSPCxx17 PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)
    target_compile_options(RespiraDSPCxx17 PRIVATE -Wall -Wextra -Wpedantic -fno-builtin -fno-exceptions)
endif()

find_package(Threads REQUIRED)

# Writes and checks Source/DSP/ReedTuning.h by simulating every reed. The
# engine never runs it; it is built for the ReedTuning check.
add_executable(GenerateReedTuning Tools/GenerateReedTuning.cpp)
target_include_directories(GenerateReedTuning PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source"
                                                      "${CMAKE_CURRENT_SOURCE_DIR}/Tools")
target_link_libraries(GenerateReedTuning PRIVATE Threads::Threads)
target_compile_options(GenerateReedTuning PRIVATE ${RESPIRA_WARNINGS})

if(RESPIRA_BUILD_TOOLS)
    # The research renderer with a bellows trace, which Tools/fit_bellows.py drives.
    add_executable(RenderRespira Tools/RenderRespira.cpp)
    target_link_libraries(RenderRespira PRIVATE RespiraDSP)
    target_compile_options(RenderRespira PRIVATE ${RESPIRA_WARNINGS})

    # Single reed cells at a fixed pressure: pitch and air.
    add_executable(ProbeReed Tools/ProbeReed.cpp)
    target_include_directories(ProbeReed PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_options(ProbeReed PRIVATE ${RESPIRA_WARNINGS})

    # Single 8' notes' harmonics against a published accordion measurement.
    add_executable(CompareHarmonics Tools/CompareHarmonics.cpp)
    target_include_directories(CompareHarmonics PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_options(CompareHarmonics PRIVATE ${RESPIRA_WARNINGS})

    # Single 8' notes' pitch and tone against loudness, against a bandoneon's.
    add_executable(CompareBandoneon Tools/CompareBandoneon.cpp)
    target_include_directories(CompareBandoneon PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_options(CompareBandoneon PRIVATE ${RESPIRA_WARNINGS})

    # One README demo per factory preset from Docs/audio/midi (Tools/
    # PresetDemoRenderer.h is the shared portfolio copy; do not edit it here).
    add_executable(RenderRespiraPresetDemos Tools/RenderPresetDemos.cpp)
    target_link_libraries(RenderRespiraPresetDemos PRIVATE RespiraDSP Threads::Threads)
    target_compile_options(RenderRespiraPresetDemos PRIVATE ${RESPIRA_WARNINGS})
endif()

if(BUILD_TESTING)
    if(RESPIRA_REQUIRE_PYTHON_TESTS)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
    else()
        find_package(Python3 COMPONENTS Interpreter)
        if(NOT Python3_Interpreter_FOUND)
            message(WARNING "Python 3 not found: Respira.DemoMidiSources is not registered")
        endif()
    endif()

    add_executable(RespiraEngineTests Tests/EngineTests.cpp)
    target_link_libraries(RespiraEngineTests PRIVATE RespiraDSP)
    target_compile_options(RespiraEngineTests PRIVATE ${RESPIRA_WARNINGS})
    add_test(NAME Respira.Engine COMMAND RespiraEngineTests)
    set_tests_properties(Respira.Engine PROPERTIES TIMEOUT 600)

    # The committed reed-tuning table is what a fresh simulation of every reed gives.
    add_test(NAME Respira.ReedTuning
             COMMAND GenerateReedTuning --check "${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP/ReedTuning.h")
    set_tests_properties(Respira.ReedTuning PROPERTIES TIMEOUT 600)

    # U4 input fuzzing. RespiraDspFuzzReplay is the harness with
    # FuzzHarness.h's main, replaying Tests/Fixtures/fuzz/ and then seeded
    # inputs; with RESPIRA_BUILD_FUZZER (Clang) RespiraDspFuzz is a libFuzzer
    # target over coverage-instrumented DSP.
    file(GLOB RESPIRA_FUZZ_REGRESSIONS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Tests/Fixtures/fuzz/*")
    add_executable(RespiraDspFuzzReplay Tests/DspFuzz.cpp)
    target_link_libraries(RespiraDspFuzzReplay PRIVATE RespiraDSP)
    target_compile_definitions(RespiraDspFuzzReplay PRIVATE PROTOCODUS_FUZZ_MAIN)
    add_test(NAME Respira.DspFuzzReplay COMMAND RespiraDspFuzzReplay -seed=1 -runs=300 ${RESPIRA_FUZZ_REGRESSIONS})
    set_tests_properties(Respira.DspFuzzReplay PROPERTIES TIMEOUT 600)
    if(RESPIRA_BUILD_FUZZER)
        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            message(FATAL_ERROR "RESPIRA_BUILD_FUZZER requires Clang")
        endif()
        add_executable(RespiraDspFuzz Tests/DspFuzz.cpp ${RESPIRA_DSP_SOURCES})
        target_include_directories(RespiraDspFuzz PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
        target_compile_options(RespiraDspFuzz PRIVATE
            -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
        target_link_options(RespiraDspFuzz PRIVATE -fsanitize=fuzzer,address,undefined)
        add_test(NAME Respira.DspFuzz COMMAND RespiraDspFuzz -seed=1 -max_total_time=60 ${RESPIRA_FUZZ_REGRESSIONS})
        set_tests_properties(Respira.DspFuzz PROPERTIES TIMEOUT 600 LABELS fuzz)
    endif()

    # U4 parameter-extremes matrix over the engine and its shared controls.
    # Each case at one rate and block size; `--jobs 8` without --rotation
    # runs every case at all nine.
    add_executable(RespiraParameterMatrixTests Tests/ParameterMatrixTests.cpp)
    target_link_libraries(RespiraParameterMatrixTests PRIVATE RespiraDSP Threads::Threads)
    target_compile_options(RespiraParameterMatrixTests PRIVATE ${RESPIRA_WARNINGS})
    add_test(NAME Respira.ParameterMatrix COMMAND RespiraParameterMatrixTests --rotation --jobs 4)
    set_tests_properties(Respira.ParameterMatrix PROPERTIES TIMEOUT 1800 LABELS fuzz)

    if(Python3_Interpreter_FOUND)
        # The committed demo phrases are what the writer writes (G3a).
        add_test(NAME Respira.DemoMidiSources
                 COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/Tools/write_preset_midi.py" --check)
    endif()

    if(RESPIRA_BUILD_TOOLS)
        # Every preset demo renders finite, audible, under the ceiling, 8-12 s
        # long and bit-identical on a second render.
        add_test(NAME Respira.AudioDemos
                 COMMAND RenderRespiraPresetDemos --jobs 4 --check "${CMAKE_CURRENT_SOURCE_DIR}/Docs/audio/midi")
        set_tests_properties(Respira.AudioDemos PROPERTIES TIMEOUT 600 LABELS audio)

        # U7: the demos' realtime factor on one thread against the budget in
        # Tools/RenderPresetDemos.cpp. Timing means nothing in an unoptimised
        # or instrumented build, so only Release without sanitizers has it.
        if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT RESPIRA_ENABLE_SANITIZERS)
            add_test(NAME Respira.CpuBudget
                     COMMAND RenderRespiraPresetDemos --cpu --json "${CMAKE_CURRENT_BINARY_DIR}/cpu.json"
                             "${CMAKE_CURRENT_SOURCE_DIR}/Docs/audio/midi")
            set_tests_properties(Respira.CpuBudget PROPERTIES TIMEOUT 900 LABELS perf RUN_SERIAL TRUE)
        endif()
    endif()

    # Every test carries exactly one tier label so CI selects tiers with
    # `ctest -L`; tests not labelled above are unit contracts.
    get_property(RESPIRA_TESTS DIRECTORY PROPERTY TESTS)
    foreach(test IN LISTS RESPIRA_TESTS)
        get_test_property(${test} LABELS test_labels)
        if(NOT test_labels)
            set_tests_properties(${test} PROPERTIES LABELS unit)
        endif()
    endforeach()
endif()
