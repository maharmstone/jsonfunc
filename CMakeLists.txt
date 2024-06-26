cmake_minimum_required(VERSION 3.10)

project(jsonfunc
	LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# ----------------------------------------

set(SRC_FILES src/jsonfunc.cpp
    src/git.cpp
    src/format-term.cpp
    src/xml-reader.cpp
    src/xml.cpp)

add_library(jsonfunc SHARED ${SRC_FILES})

find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBGIT2 REQUIRED IMPORTED_TARGET libgit2)

target_link_libraries(jsonfunc nlohmann_json::nlohmann_json)
target_link_libraries(jsonfunc PkgConfig::LIBGIT2)

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
else()
	install(FILES $<TARGET_PDB_FILE:jsonfunc> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif()

install(TARGETS jsonfunc
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE EXCLUDE_FROM_ALL
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
