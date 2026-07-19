cmake_minimum_required(VERSION 3.20)

project(MiniGC LANGUAGES CXX)

add_library(MiniGC SHARED
    MiniGC.cpp
    MiniGCHandleManager.cpp
    MiniGCHandleStore.cpp
    MiniGCHeap.cpp
)

target_compile_features(MiniGC PRIVATE cxx_std_17)

# TARGET_UNIX gates the Win32-typedef shim block in gcenv.base.h; on Windows
# the SDK already provides these types and redefining them fails the build.
if(NOT WIN32)
    target_compile_definitions(MiniGC PRIVATE TARGET_UNIX=1)
endif()

# HOST_AMD64 / HOST_X86 select the right inline-asm / intrinsics branch
# inside gcenv.base.h. CMake's CMAKE_SIZEOF_VOID_P tells us 32-bit (4) vs
# 64-bit (8); on Windows we always build x64 on the GitHub-hosted runner,
# so HOST_AMD64 covers it. Add an arm64 branch if/when we target that.
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_compile_definitions(MiniGC PRIVATE HOST_AMD64=1)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_compile_definitions(MiniGC PRIVATE HOST_X86=1)
    else()
        message(FATAL_ERROR "Unsupported Windows architecture (sizeof(void*) = ${CMAKE_SIZEOF_VOID_P})")
    endif()
endif()

target_include_directories(MiniGC PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include/coreclr/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/include/coreclr/gc/env
    ${CMAKE_CURRENT_SOURCE_DIR}/include/coreclr/gc
    ${CMAKE_CURRENT_SOURCE_DIR}/include/coreclr/minipal
    ${CMAKE_CURRENT_SOURCE_DIR}/include/native
)

set_target_properties(MiniGC PROPERTIES
    OUTPUT_NAME MiniGC
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)
