# Pure-logic component: no hardware/IDF requirements, so it builds unchanged
# for both the esp32 target and the linux host target.
idf_component_register(
    SRCS "processor.c"
    INCLUDE_DIRS "include"
)

# Treat warnings as errors for OUR code only (not IDF's own components).
# Keeps the firmware honest without fighting upstream warnings.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
