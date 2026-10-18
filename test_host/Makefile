# Host unit-test build for the pure `processor` logic.
#
# Plain native gcc build (NOT the ESP-IDF linux target). Because `processor`
# has no IDF dependencies, this compiles in well under a second, produces
# working gcov coverage, and runs cleanly under valgrind.
#
# Targets:
#   make test       build and run the unit tests
#   make coverage   build with gcov, run, and print a coverage summary
#   make valgrind   run the tests under valgrind (fails on leaks/errors)
#   make clean      remove build artifacts
#
# Unity is sourced from ESP-IDF so we don't vendor a copy. Override UNITY_DIR
# if your Unity lives elsewhere.

CC      ?= gcc
IDF_PATH ?= $(HOME)/esp/esp-idf
UNITY_DIR ?= $(IDF_PATH)/components/unity/unity/src
PROC_DIR  := ../components/processor

CFLAGS  := -std=c11 -Wall -Wextra -O0 -g \
           -I$(UNITY_DIR) -I$(PROC_DIR)/include
LDFLAGS := -lm

SRCS := test_processor.c $(UNITY_DIR)/unity.c $(PROC_DIR)/processor.c
BIN  := test_host

.PHONY: test coverage valgrind clean

test: $(BIN)
	./$(BIN)

$(BIN): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(BIN) $(LDFLAGS)

coverage: CFLAGS += --coverage
coverage: LDFLAGS += --coverage
coverage: clean $(BIN)
	./$(BIN)
	@echo "---- coverage ----"
	@gcovr --root .. --filter '$(PROC_DIR)/' --print-summary --txt 2>/dev/null \
	  || gcov -b $(BIN)-processor 2>/dev/null \
	  || gcov processor.c 2>/dev/null

valgrind: $(BIN)
	valgrind --leak-check=full --error-exitcode=1 ./$(BIN)

clean:
	rm -f $(BIN) *.gcno *.gcda *.gcov coverage.xml
