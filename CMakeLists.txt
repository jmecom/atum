cmake_minimum_required(VERSION 3.17)

project(ATUM)

add_compile_options(-Wall -Werror)

add_subdirectory(libatum)
add_subdirectory(example)
add_subdirectory(test)
