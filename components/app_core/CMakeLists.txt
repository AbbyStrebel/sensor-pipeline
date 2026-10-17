# Composition root: wires the pipeline together.
idf_component_register(
    SRCS "app_core.c"
    INCLUDE_DIRS "include"
    REQUIRES sampler reporter freertos
)
