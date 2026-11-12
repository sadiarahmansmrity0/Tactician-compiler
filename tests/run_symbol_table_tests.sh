#!/bin/sh
set -eu

unit=build/symbol-table-tests.exe
program=build/tactician-symbol-table.exe
passed=0
failed=0

if "$unit" tests/symbol_table/nested_scopes.tac; then
    passed=$((passed + 1))
else
    failed=$((failed + 1))
fi

for name in nested_scopes duplicates; do
    input="tests/symbol_table/$name.tac"
    expected="tests/symbol_table/expected/$name.expected"
    actual="build/symbol-table-$name.actual"
    expected_status=0
    [ "$name" != duplicates ] || expected_status=1

    set +e
    "$program" "$input" >"$actual" 2>&1
    status=$?
    set -e
    tr -d '\r' < "$actual" > "build/symbol-table-$name.normalized"
    if [ "$status" -eq "$expected_status" ] &&
       diff -u "$expected" "build/symbol-table-$name.normalized"; then
        echo "PASS symbol-table AST: $name"
        passed=$((passed + 1))
    else
        echo "FAIL symbol-table AST: $name (exit $status)"
        failed=$((failed + 1))
    fi
done

echo "$passed passed, $failed failed"
[ "$failed" -eq 0 ]
