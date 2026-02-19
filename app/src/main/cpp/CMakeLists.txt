cmake_minimum_required(VERSION 3.22.1)

project("PubertSDKGen")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(kNO_KEYSTONE)
file(GLOB_RECURSE KITTY_MEMORY_SOURCES extern/KittyMemory/KittyMemory/*.cpp)

add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp
        include/SDK/codegen.cpp
	include/SDK/fmt/src/fmt.cc
	include/SDK/fmt/src/format.cc
	include/SDK/fmt/src/os.cc
        ${KITTY_MEMORY_SOURCES}
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
    include
    extern/KittyMemory
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log
        ${KEYSTONE_LIB}
)