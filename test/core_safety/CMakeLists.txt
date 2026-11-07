cmake_minimum_required(VERSION 3.16)
project(qjs_core_safety LANGUAGES C CXX)
enable_testing()
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cxx/quickjs.cmake")
set_target_properties(quickjs PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(core_safety_test core_safety_test.cpp)
target_compile_features(core_safety_test PRIVATE cxx_std_17)
target_include_directories(core_safety_test PRIVATE "${QUICK_JS_LIB_DIR}")
target_link_libraries(core_safety_test PRIVATE quickjs)

# The actual Dart FFI entry points use this same compiled, patched core.
add_library(flutter_qjs_plugin SHARED "${CXX_LIB_DIR}/ffi.cpp")
target_compile_features(flutter_qjs_plugin PRIVATE cxx_std_17)
find_package(Threads REQUIRED)
target_link_libraries(flutter_qjs_plugin PRIVATE quickjs Threads::Threads)
if(NOT WIN32)
  target_link_libraries(core_safety_test PRIVATE m)
  target_link_libraries(flutter_qjs_plugin PRIVATE m)
endif()

option(QJS_ENABLE_SANITIZERS "Instrument the real native core and callers" OFF)
if(QJS_ENABLE_SANITIZERS)
  foreach(target quickjs core_safety_test flutter_qjs_plugin)
    if(MSVC)
      target_compile_options(${target} PRIVATE /fsanitize=address /Zi)
      target_link_options(${target} PRIVATE /INCREMENTAL:NO)
    else()
      target_compile_options(${target} PRIVATE -fsanitize=address,undefined
        -fno-omit-frame-pointer -fno-sanitize-recover=all)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
  endforeach()
endif()

math(EXPR QJS_POINTER_BITS "${CMAKE_SIZEOF_VOID_P} * 8")
add_test(NAME core_safety COMMAND core_safety_test
  "${CMAKE_CURRENT_SOURCE_DIR}/core_regressions.js" ${QJS_POINTER_BITS})
set_tests_properties(core_safety PROPERTIES TIMEOUT 120)
