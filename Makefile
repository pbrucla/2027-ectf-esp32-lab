PIO := pio
ENV := esp32dev

CLANG_TIDY ?= clang-tidy

SOURCES := $(shell find src -type f \( -name '*.cpp' -o -name '*.c' \))

TIDY_CHECKS := -*,bugprone-*,clang-analyzer-*,performance-*,readability-*,modernize-use-nullptr,modernize-use-override,modernize-use-using,cppcoreguidelines-init-variables,cppcoreguidelines-pro-type-member-init

.PHONY: build clean compiledb tidy tidy-fix

build:
	$(PIO) run -e $(ENV)

clean:
	$(PIO) run -e $(ENV) -t clean

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