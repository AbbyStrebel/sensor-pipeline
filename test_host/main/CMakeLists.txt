# Host test runner component.
idf_component_register(
    SRCS "test_processor.c"
    INCLUDE_DIRS "."
    REQUIRES processor unity
)
