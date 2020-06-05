FILE(GLOB CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c)

add_library(libatum SHARED ${CORE_SOURCES})

target_include_directories(libatum PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
