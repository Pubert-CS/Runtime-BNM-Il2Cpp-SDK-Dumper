cmake_minimum_required(VERSION 3.22.1)

project("PubertSDKGen")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(kNO_KEYSTONE)
file(GLOB_RECURSE KITTY_MEMORY_SOURCES extern/KittyMemory/KittyMemory/*.cpp)

set(DOBBY_DIR extern/Dobby)
macro(SET_OPTION option value)
    set(${option} ${value} CACHE INTERNAL "" FORCE)
endmacro()
SET_OPTION(DOBBY_DEBUG OFF)
SET_OPTION(DOBBY_GENERATE_SHARED OFF)
add_subdirectory(${DOBBY_DIR} dobby)
get_property(DOBBY_INCLUDE_DIRECTORIES
        TARGET dobby
        PROPERTY INCLUDE_DIRECTORIES)
include_directories(
        .
        ${DOBBY_INCLUDE_DIRECTORIES}
        $<TARGET_PROPERTY:dobby,INCLUDE_DIRECTORIES>
)

add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp
        include/SDK/codegen.cpp
        extern/Dobby/include
        ${KITTY_MEMORY_SOURCES}
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
    include
    extern/KittyMemory
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log
        dobby
        ${KEYSTONE_LIB}
)