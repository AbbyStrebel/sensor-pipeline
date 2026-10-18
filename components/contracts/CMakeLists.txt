# Interface-only component: shared message/type contracts between components
# (e.g. sample_t passed sampler -> reporter via a queue).
#
# It has NO sources and NO dependencies, so any component can depend on it
# without creating a dependency cycle. This is where shared "contract" types
# live so that neither producer nor consumer has to depend on the other.
idf_component_register(
    INCLUDE_DIRS "include"
)
