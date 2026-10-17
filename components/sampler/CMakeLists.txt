# Hardware-facing component: depends on FreeRTOS. Builds for esp32 target only;
# excluded from the linux host-test build (host tests target `processor`).
idf_component_register(
    SRCS "sampler.c"
    INCLUDE_DIRS "include"
    REQUIRES processor freertos
)
