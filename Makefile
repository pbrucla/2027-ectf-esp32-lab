# Use `pio` from PATH if it's there. Otherwise fall back to where the VS Code
# PlatformIO extension installs it, which isn't on PATH outside VS Code.
PIO ?= $(shell command -v pio 2>/dev/null || echo $(HOME)/.platformio/penv/bin/pio)
ENV := esp32dev

# Use `uv` from PATH if it's there. Otherwise fall back to where uv's installer
# puts it: %USERPROFILE%\.local\bin on Windows (the MSYS2 terminal doesn't put
# the Windows PATH on its own PATH), or ~/.local/bin on macOS and Linux.
UV ?= $(shell command -v uv 2>/dev/null \
	|| { [ -n "$$USERPROFILE" ] && command -v cygpath >/dev/null 2>&1 \
	     && echo "$$(cygpath -u "$$USERPROFILE")/.local/bin/uv"; } \
	|| echo "$(HOME)/.local/bin/uv")

CLANG_FORMAT ?= clang-format
CLANG_TIDY ?= clang-tidy
RUFF ?= uvx ruff

SOURCES := $(shell find src -type f \( -name '*.cpp' -o -name '*.c' -o -name '*.hpp' -o -name '*.h' \))
PYTHON_SOURCES := $(shell find test -type f -name '*.py')
TIDY_CHECKS := -*,bugprone-*,clang-analyzer-*,performance-*,readability-*,modernize-use-nullptr,modernize-use-override,modernize-use-using,cppcoreguidelines-init-variables,cppcoreguidelines-pro-type-member-init

# Host unit tests: the Phase 2 and 3 code (no Arduino) built with your
# computer's compiler. Override IMPL_DIR to test another copy of the student
# file, e.g. `make test IMPL_DIR=solutions`. encode()/decode(), the crypto
# (SHA-256, SHA-512, Ed25519) and the hex helpers are given, so they always
# come from src/.
CXX ?= c++
IMPL_DIR ?= src
TEST_SOURCES := test/test.cpp $(IMPL_DIR)/security.cpp src/commands.cpp \
	src/crypto.cpp src/ed25519.cpp src/hex.cpp
TEST_BIN := build/test

.PHONY: build clean compiledb format keys tidy tidy-fix test

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

# Phase 3: make a signing keypair. Writes the private key to keys/ and the
# public key to src/public_key.hpp for the board. uv installs the Python
# packages keygen.py needs on its own.
keys:
	"$(UV)" run test/keygen.py --passcode ILOVEECTFSOMUCH

compiledb:
	$(PIO) run -e $(ENV) -t compiledb

format:
	$(CLANG_FORMAT) -i $(SOURCES)
	$(RUFF) format $(PYTHON_SOURCES)

lint:
	$(RUFF) check $(PYTHON_SOURCES)

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