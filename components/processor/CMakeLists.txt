# Pure-logic component: no hardware/IDF requirements, so it builds unchanged
# for both the esp32 target and the linux host target.
idf_component_register(
    SRCS "processor.c"
    INCLUDE_DIRS "include"
)
