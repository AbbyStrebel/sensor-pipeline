# Host-based (linux target) test project.
#
# Builds ONLY the pure-logic `processor` component plus Unity, compiled for the
# host (x86) so tests run natively and under valgrind / AddressSanitizer.
#
# Build & run:
#   idf.py --preview set-target linux
#   idf.py build
#   ./build/test_host.elf
cmake_minimum_required(VERSION 3.16)

# Pull in the project's real components so we test the SAME source the firmware
# uses — no duplicated logic.
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../components/processor")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(test_host)
