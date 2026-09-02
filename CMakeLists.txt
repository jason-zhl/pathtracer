cmake_minimum_required(VERSION 3.20)

project(PathTracer LANGUAGES CXX CUDA)

find_package(CUDAToolkit REQUIRED)

include(FetchContent)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(yaml-cpp)

set(CMAKE_CUDA_ARCHITECTURES native)

add_executable(PathTracer
    src/main.cpp
    src/configuration.cpp
    src/environment/ibl.cpp
    src/cuda/ray_colour.cu
)

target_include_directories(PathTracer PRIVATE src)
target_link_libraries(PathTracer PRIVATE CUDA::cudart yaml-cpp)

set_target_properties(PathTracer PROPERTIES
    CUDA_SEPARABLE_COMPILATION OFF
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(PathTracer PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
        $<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>
        $<$<COMPILE_LANGUAGE:CXX>:/nologo>
        $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Release>>:/O2>
        $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:RelWithDebInfo>>:/O2>
    )
else()
    target_compile_options(PathTracer PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-O2>
    )
    target_compile_features(PathTracer PRIVATE cxx_std_23)
endif()
