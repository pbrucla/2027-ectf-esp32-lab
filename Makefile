# Use `pio` from PATH if it's there. Otherwise fall back to where the VS Code
# PlatformIO extension installs it, which isn't on PATH outside VS Code.
PIO ?= $(shell command -v pio 2>/dev/null || echo $(HOME)/.platformio/penv/bin/pio)
ENV := esp32dev

CLANG_TIDY ?= clang-tidy

SOURCES := $(shell find src -type f \( -name '*.cpp' -o -name '*.c' \))

TIDY_CHECKS := -*,bugprone-*,clang-analyzer-*,performance-*,readability-*,modernize-use-nullptr,modernize-use-override,modernize-use-using,cppcoreguidelines-init-variables,cppcoreguidelines-pro-type-member-init

# Host unit tests: the Phase 2 code (no Arduino) built with your computer's
# compiler. Override IMPL_DIR to test another copy of the student files,
# e.g. `make test IMPL_DIR=solutions`. SHA-256 is given, so it always comes
# from src/.
CXX ?= c++
IMPL_DIR ?= src
TEST_SOURCES := test/test.cpp $(IMPL_DIR)/commands.cpp $(IMPL_DIR)/security.cpp src/crypto.cpp
TEST_BIN := build/test

.PHONY: build clean compiledb tidy tidy-fix test

build:
	$(PIO) run -e $(ENV)

clean:
	$(PIO) run -e $(ENV) -t clean
	rm -rf build

test:
	mkdir -p build
	$(CXX) -std=c++17 -Wall -Wextra -Wno-unused-parameter -Isrc -Itest \
		$(TEST_SOURCES) -o $(TEST_BIN)
	./$(TEST_BIN)

compiledb:
	$(PIO) run -e $(ENV) -t compiledb

tidy: compiledb
	$(CLANG_TIDY) -p . \
		--checks='$(TIDY_CHECKS)' \
		--header-filter='.*src/.*' \
		$(SOURCES)

tidy-fix: compiledb
	$(CLANG_TIDY) -p . \
		--checks='$(TIDY_CHECKS)' \
		--header-filter='.*src/.*' \
		--fix \
		$(SOURCES)