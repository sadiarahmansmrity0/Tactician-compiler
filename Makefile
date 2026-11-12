CC = gcc
FLEX = flex
BISON = bison
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -Isrc/lexer -Isrc/common
BUILD_DIR = build
LEXER_C = $(BUILD_DIR)/lexer.c
LEXER_EXE = $(BUILD_DIR)/tactician-lexer.exe
LEXER_SOURCES = src/lexer/token.c src/lexer/lexer_driver.c
PARSER_C = $(BUILD_DIR)/parser.tab.c
PARSER_H = $(BUILD_DIR)/parser.tab.h
PARSER_EXE = $(BUILD_DIR)/tactician-parser.exe
PARSER_CORE_SOURCES = src/lexer/token.c src/ast/ast.c src/parser/lexer_adapter.c
PARSER_SOURCES = $(PARSER_CORE_SOURCES) src/parser/parser_driver.c
SYMBOL_TABLE_EXE = $(BUILD_DIR)/tactician-symbol-table.exe
SYMBOL_TABLE_TEST_EXE = $(BUILD_DIR)/symbol-table-tests.exe
SYMBOL_TABLE_SOURCE = src/symbol_table/symbol_table.c

.PHONY: all lexer parser symbol-table test clean

all: parser

lexer: $(LEXER_EXE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LEXER_C): src/lexer/lexer.l | $(BUILD_DIR)
	$(FLEX) -o $@ $<

$(LEXER_EXE): $(LEXER_C) $(LEXER_SOURCES) src/lexer/lexer.h src/lexer/token.h src/common/source_location.h
	$(CC) $(CFLAGS) -o $@ $(LEXER_C) $(LEXER_SOURCES)

$(PARSER_C): src/parser/parser.y src/parser/parser.h src/parser/lexer_adapter.h src/ast/ast.h | $(BUILD_DIR)
	$(BISON) -d -o $@ src/parser/parser.y

$(PARSER_H): $(PARSER_C)
	@test -f $@ || $(BISON) -d -o $(PARSER_C) src/parser/parser.y

$(PARSER_EXE): $(PARSER_C) $(PARSER_H) $(LEXER_C) $(PARSER_SOURCES) src/parser/parser.h src/ast/ast.h src/lexer/lexer.h src/lexer/token.h src/common/source_location.h
	$(CC) $(CFLAGS) -Ibuild -Isrc/parser -Isrc/ast -o $@ $(PARSER_C) $(LEXER_C) $(PARSER_SOURCES)

parser: $(PARSER_EXE)

$(SYMBOL_TABLE_EXE): $(PARSER_C) $(PARSER_H) $(LEXER_C) $(PARSER_CORE_SOURCES) $(SYMBOL_TABLE_SOURCE) src/symbol_table/symbol_table_driver.c src/symbol_table/symbol_table.h src/parser/parser.h src/ast/ast.h src/lexer/lexer.h src/lexer/token.h src/common/source_location.h
	$(CC) $(CFLAGS) -Ibuild -Isrc/parser -Isrc/ast -Isrc/symbol_table -o $@ $(PARSER_C) $(LEXER_C) $(PARSER_CORE_SOURCES) $(SYMBOL_TABLE_SOURCE) src/symbol_table/symbol_table_driver.c

$(SYMBOL_TABLE_TEST_EXE): $(PARSER_C) $(PARSER_H) $(LEXER_C) $(PARSER_CORE_SOURCES) $(SYMBOL_TABLE_SOURCE) tests/symbol_table_unit.c src/symbol_table/symbol_table.h src/parser/parser.h src/ast/ast.h src/lexer/lexer.h src/lexer/token.h src/common/source_location.h
	$(CC) $(CFLAGS) -Ibuild -Isrc/parser -Isrc/ast -Isrc/symbol_table -o $@ $(PARSER_C) $(LEXER_C) $(PARSER_CORE_SOURCES) $(SYMBOL_TABLE_SOURCE) tests/symbol_table_unit.c

symbol-table: $(SYMBOL_TABLE_EXE)

test: $(LEXER_EXE) $(PARSER_EXE) $(SYMBOL_TABLE_EXE) $(SYMBOL_TABLE_TEST_EXE)
	sh tests/run_lexer_tests.sh
	sh tests/run_parser_tests.sh
	sh tests/run_symbol_table_tests.sh

clean:
	rm -rf $(BUILD_DIR)
