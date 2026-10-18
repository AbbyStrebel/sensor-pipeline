# Consumer task component: filters samples via `processor` and logs results.
idf_component_register(
    SRCS "reporter.c"
    INCLUDE_DIRS "include"
    REQUIRES contracts processor freertos
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
