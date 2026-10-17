# Hardware-facing component: depends on FreeRTOS + driver (UART in Phase 2).
idf_component_register(
    SRCS "reporter.c"
    INCLUDE_DIRS "include"
    REQUIRES processor freertos
)
