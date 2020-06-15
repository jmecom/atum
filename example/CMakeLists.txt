# add_compile_options(-fsanitize=address -g)
# add_link_options(-fsanitize=address)

add_library(payload SHARED payload.c)

add_executable(target target.c)

add_executable(injector injector.c)
target_link_libraries(injector libatum)
