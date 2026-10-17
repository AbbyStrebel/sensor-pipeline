# Application entrypoint component (esp32 target).
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES app_core
)
