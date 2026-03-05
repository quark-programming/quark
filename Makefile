#-------------------------------------------------
# Makefile Usage
# > make                  # release build + unit-tests
# > make build            # release build qc
# > make build MODE=debug # debug build qc
# > make test             # qk tests
# > make test-c           # unit-tests
# > make release          # release build qc + unit-tests
# > make debug            # debug build qc + unit-tests
# > make clean            # remove build/files
#-------------------------------------------------

# ========= Tools =========
CC  := gcc
QC  := qc
EXE :=

# ========= Platform detection =========
ifeq ($(OS),Windows_NT)
	PLATFORM := windows
	EXE := .exe
	CC  := $(CC)$(EXE)
	QC  := $(QC)$(EXE)

	MKDIR = if not exist "$(1)" mkdir "$(1)"
	RM_RF = if exist "$(1)" rmdir /S /Q "$(1)"
	RM_F  = if exist "$(1)" del /Q "$(1)"
	COPY  = copy /Y
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		PLATFORM := macos
	else
		PLATFORM := linux
	endif

	MKDIR = mkdir -p "$(1)"
	RM_RF = rm -rf "$(1)"
	RM_F  = rm -f "$(1)"
	COPY  = cp
endif

# ========= Flags =========
CFLAGS  := -Isrc/include -Isrc/lib -Isrc -Wall -Wno-missing-braces -Wno-char-subscripts -DWRAPSETIMPL
LDFLAGS :=
EMCCFLAGS := -s WASM=1 -s MODULARIZE=1 -s EXPORT_NAME="QuarkCompiler" \
	-s EXPORTED_FUNCTIONS='["_main"]' \
	-s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "FS", "callMain"]' -s ALLOW_MEMORY_GROWTH=1 \
	-s NO_EXIT_RUNTIME=1 -s STACK_SIZE=5MB -g -s ASSERTIONS=2 -s STACK_OVERFLOW_CHECK=2 -s SAFE_HEAP=1 \
	-s INVOKE_RUN=0

# ========= Directories =========
BUILD_DIR := build
TEST_BUILD := $(BUILD_DIR)/tests
UT_BUILD   := $(BUILD_DIR)/unit_tests

# ========= Mode normalize =========
MODE ?= release
MODE_L := $(shell echo $(MODE) | tr A-Z a-z)

# ========= Sources =========
MAIN_C := src/main.c
SRCS := \
	$(wildcard src/*/*.c) \
	$(wildcard src/*/*/*.c) \
	$(wildcard src/*/*/*/*.c)

TEST_QK := $(wildcard tests/*.qk)

UT_SRCS := \
	$(wildcard unit-tests/*.c) \
	$(wildcard unit-tests/*/*.c) \
	$(wildcard unit-tests/*/*/*.c) \
	$(wildcard unit-tests/*/*/*/*.c) \

# ========= Output =========
OUT := ./${QC}

.PHONY: all build test test-c release debug clean
.DEFAULT_GOAL := all

# ========= Directories =========
dirs:
	@$(call MKDIR,$(BUILD_DIR))
	@$(call MKDIR,$(TEST_BUILD))
	@$(call MKDIR,$(UT_BUILD))

# ========= Build qc =========
build: dirs
ifeq ($(MODE_L),debug)
	@echo ------ Debug build qc ------
	$(CC) $(CFLAGS) $(MAIN_C) $(SRCS) $(LDFLAGS) -g -ggdb -DEBUG -fsanitize=address -o $(BUILD_DIR)/$(QC)
else
	@echo ------ Release build qc ------
	$(CC) $(CFLAGS) $(MAIN_C) $(SRCS) $(LDFLAGS) -O2 -o $(BUILD_DIR)/$(QC)
endif
	@$(COPY) $(BUILD_DIR)/$(QC) $(OUT)

# ========= Default =========
all: build test-c

# ========= qk tests =========
test:
	@echo ------ qk tests ------
	@$(call MKDIR,$(TEST_BUILD))

ifeq ($(PLATFORM),windows)
	@if not "$(TEST_QK)"=="" ( \
		for %%f in ($(TEST_QK)) do ( \
			$(OUT) %%f -o $(TEST_BUILD)\%%~nf.c -l "$(CURDIR)" && \
			$(CC) $(CFLAGS) -g $(TEST_BUILD)\%%~nf.c -o $(TEST_BUILD)\%%~nf$(EXE) \
		) \
	) else ( \
		echo No .qk tests found \
	)
else
	@set -e; \
	if [ -n "$(TEST_QK)" ]; then \
		for f in $(TEST_QK); do \
			base=$$(basename $$f .qk); \
			$(OUT) "$$f" -o "$(TEST_BUILD)/$$base.c" -l "$(CURDIR)"; \
			$(CC) $(CFLAGS) -g "$(TEST_BUILD)/$$base.c" -o "$(TEST_BUILD)/$$base$(EXE)"; \
		done; \
	else \
		echo "No .qk tests found"; \
	fi
endif

# ========= unit-tests =========
build-test-c: dirs
	$(CC) $(CFLAGS) $(UT_SRCS) $(SRCS) $(LDFLAGS) -g -ggdb -fsanitize=address -o $(UT_BUILD)/unit-tests$(EXE)

test-c: build-test-c
	@echo ------ unit-tests ------
	@$(UT_BUILD)/unit-tests$(EXE)

# ========= Presets =========
release:
	$(MAKE) build MODE=release
	$(MAKE) test-c

debug:
	$(MAKE) build MODE=debug
	$(MAKE) test-c

wasm:
	emcc $(CFLAGS) $(EMCCFLAGS) $(MAIN_C) $(SRCS) -o $(BUILD_DIR)/qc.js

# ========= Clean =========
clean:
	@$(call RM_RF,$(BUILD_DIR))
	@$(call RM_F,$(OUT))

