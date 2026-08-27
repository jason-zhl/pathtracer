cmake_minimum_required(VERSION 3.20)

project(PathTracer LANGUAGES CXX)

add_executable(PathTracer
    src/main.cpp
    src/environment/ibl.cpp
)

target_include_directories(PathTracer PRIVATE src)

if(MSVC)
    target_compile_options(PathTracer PRIVATE
        /W4
        /EHsc
        /std:c++latest
        /nologo
    )
else()
    target_compile_options(PathTracer PRIVATE
        -Wall
        -Wextra
        -O2
    )
    target_compile_features(PathTracer PRIVATE cxx_std_23)
endif()

if(MSVC)
    target_compile_options(PathTracer PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/O2>
    )
endif()
