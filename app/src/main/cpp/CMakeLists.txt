cmake_minimum_required(VERSION 3.22.1)

project("ultron")

# Disable unnecessary llama.cpp components
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "llama: build tests" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "llama: build examples" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "llama: build server example" FORCE)

set(GGML_BUILD_TESTS OFF CACHE BOOL "ggml: build tests" FORCE)
set(GGML_BUILD_EXAMPLES OFF CACHE BOOL "ggml: build examples" FORCE)

# Disable GPU backends for a stable CPU-only Android build
set(GGML_CUDA OFF CACHE BOOL "ggml: use CUDA" FORCE)
set(GGML_METAL OFF CACHE BOOL "ggml: use Metal" FORCE)
set(GGML_VULKAN OFF CACHE BOOL "ggml: use Vulkan" FORCE)
set(GGML_OPENCL OFF CACHE BOOL "ggml: use OpenCL" FORCE)

# Add llama.cpp subdirectory
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../../native/llama.cpp" "${CMAKE_CURRENT_BINARY_DIR}/llama_build")

add_library(
        ultron
        SHARED
        native-lib.cpp)

# Include paths for llama.cpp
target_include_directories(ultron PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../native/llama.cpp/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../native/llama.cpp/ggml/include"
)

find_library(
        log-lib
        log)

target_link_libraries(
        ultron
        llama
        ${log-lib})
