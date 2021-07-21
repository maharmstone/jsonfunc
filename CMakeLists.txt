cmake_minimum_required(VERSION 3.10)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

project(jsonfunc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# ----------------------------------------

set(SRC_FILES src/jsonfunc.cpp
    src/git.cpp
    src/format-term.cpp
    src/xml-reader.cpp)

add_library(jsonfunc SHARED ${SRC_FILES})

find_package(libgit2 REQUIRED)

target_link_libraries(jsonfunc libgit2)

target_compile_options(jsonfunc PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Wall>
     $<$<CXX_COMPILER_ID:MSVC>:
          /W4>)

if(WIN32)
    set_target_properties(jsonfunc PROPERTIES PREFIX "")
endif()

if(NOT MSVC)
    target_link_options(jsonfunc PUBLIC -static-libgcc)
	target_compile_options(jsonfunc PUBLIC -Wall -Werror=cast-function-type -Wno-expansion-to-defined -Wunused-parameter -Wtype-limits -Wextra -Wconversion -fstrict-aliasing)
endif()
