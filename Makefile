CC = gcc
FLEX = flex
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -Isrc/lexer -Isrc/common
BUILD_DIR = build
LEXER_C = $(BUILD_DIR)/lexer.c
LEXER_EXE = $(BUILD_DIR)/tactician-lexer.exe
LEXER_SOURCES = src/lexer/token.c src/lexer/lexer_driver.c

.PHONY: all lexer test clean

all: lexer

lexer: $(LEXER_EXE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LEXER_C): src/lexer/lexer.l | $(BUILD_DIR)
	$(FLEX) -o $@ $<

$(LEXER_EXE): $(LEXER_C) $(LEXER_SOURCES) src/lexer/lexer.h src/lexer/token.h src/common/source_location.h
	$(CC) $(CFLAGS) -o $@ $(LEXER_C) $(LEXER_SOURCES)

test: $(LEXER_EXE)
	sh tests/run_lexer_tests.sh

clean:
	rm -rf $(BUILD_DIR)
