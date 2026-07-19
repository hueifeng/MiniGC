cmake_minimum_required(VERSION 3.20)

project(MiniGC LANGUAGES CXX)

add_library(MiniGC SHARED
    MiniGC.cpp
    MiniGCHandleManager.cpp
    MiniGCHandleStore.cpp
    MiniGCHeap.cpp
)

target_compile_features(MiniGC PRIVATE cxx_std_17)
target_compile_definitions(MiniGC PRIVATE TARGET_UNIX=1)
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
