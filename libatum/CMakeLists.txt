FILE(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

add_library(libatum SHARED ${SOURCES})

target_include_directories(libatum PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
