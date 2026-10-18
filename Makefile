# Convenience entrypoints at the repo root.
#
#   make docs        build API docs with Doxygen -> docs/html/index.html
#   make docs-clean  remove generated docs
#
# Host unit tests live under test_host/ (see test_host/Makefile):
#   make -C test_host test | asan | valgrind | coverage
#
# Firmware is built with ESP-IDF (not make):
#   . $IDF_PATH/export.sh && idf.py build

.PHONY: docs docs-clean

docs:
	@command -v doxygen >/dev/null 2>&1 || { \
	  echo "error: doxygen not installed."; \
	  echo "  Arch:   sudo pacman -S doxygen"; \
	  echo "  Debian: sudo apt install doxygen"; \
	  exit 1; }
	doxygen docs/Doxyfile
	@echo "docs built -> docs/html/index.html"

docs-clean:
	rm -rf docs/html
