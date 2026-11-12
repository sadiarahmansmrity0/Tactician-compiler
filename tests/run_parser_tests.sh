#!/bin/sh
set -eu

program=build/tactician-parser.exe
passed=0
failed=0

for input in tests/parser/valid/*.tac; do
    name=$(basename "$input" .tac)
    expected="tests/parser/expected/$name.expected"
    actual="build/parser-$name.actual"
    set +e
    "$program" "$input" >"$actual" 2>&1
    status=$?
    set -e
    tr -d '\r' < "$actual" > "build/parser-$name.normalized"
    if [ "$status" -eq 0 ] && diff -u "$expected" "build/parser-$name.normalized"; then
        echo "PASS parser valid: $name"
        passed=$((passed + 1))
    else
        echo "FAIL parser valid: $name (exit $status)"
        failed=$((failed + 1))
    fi
done

for input in tests/parser/invalid/*.tac; do
    name=$(basename "$input" .tac)
    expected="tests/parser/expected/$name.expected"
    actual="build/parser-$name.actual"
    set +e
    "$program" "$input" >"$actual" 2>&1
    status=$?
    set -e
    tr -d '\r' < "$actual" > "build/parser-$name.normalized"
    if [ "$status" -eq 1 ] && diff -u "$expected" "build/parser-$name.normalized"; then
        echo "PASS parser invalid: $name"
        passed=$((passed + 1))
    else
        echo "FAIL parser invalid: $name (exit $status)"
        failed=$((failed + 1))
    fi
done

echo "$passed passed, $failed failed"
[ "$failed" -eq 0 ]
