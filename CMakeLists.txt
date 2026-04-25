cmake_minimum_required(VERSION 3.25)
project(BonsAI VERSION 0.1.0 LANGUAGES CXX)

# NOTE: CMake is deprecated for production builds. 
# Use the monolithic Bazel build in LiteRT-LM/bonsai/ instead.
# This file is kept for IDE intellisense only.

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- LiteRT-LM Setup ---
set(LITERT_LM_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/LiteRT-LM)
set(LITERT_LM_LIB ${LITERT_LM_ROOT}/bazel-bin/c/libbonsai_engine.so)

# --- Dependencies ---

# 1. Oat++
find_package(oatpp REQUIRED)

# 2. JSON (nlohmann)
find_package(nlohmann_json REQUIRED)

# --- Executable ---

set(SOURCES
    src/main.cpp
    src/inference/ModelManager.cpp
    src/inference/InferenceSession.cpp
)

add_executable(bonsai
    ${SOURCES}
)

target_include_directories(bonsai PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OATPP_BASE_DIR}
    ${LITERT_LM_ROOT}
)

# Link LiteRT-LM and dependencies
target_link_libraries(bonsai PRIVATE
    oatpp::oatpp
    nlohmann_json::nlohmann_json
    ${LITERT_LM_LIB}
)
