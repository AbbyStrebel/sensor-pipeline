# Application entrypoint component (esp32 target).
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES app_core
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
