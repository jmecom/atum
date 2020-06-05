FILE(GLOB CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/core/*.c)
FILE(GLOB ARCH_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/arch/*.c)

add_library(libatum SHARED ${CMAKE_CURRENT_SOURCE_DIR}/src/core/inject.c)

target_include_directories(libatum PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
