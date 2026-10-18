# Composition root: wires the pipeline together.
idf_component_register(
    SRCS "app_core.c"
    INCLUDE_DIRS "include"
    REQUIRES contracts sampler reporter freertos
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
