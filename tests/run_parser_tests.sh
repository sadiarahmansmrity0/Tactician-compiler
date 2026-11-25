#!/bin/sh
set -eu

program=build/tactician-parser.exe
passed=0
failed=0

for input in tests/parser/valid/*.tac; do
    name=$(basename "$input" .tac)
    expected="tests/parser/expected/$name.expected"
    actual="build/parser-$name.actual"
    normalized="build/parser-$name.normalized"
    normalized_expected="build/parser-$name.expected.normalized"

    set +e
    "$program" "$input" >"$actual" 2>&1
    status=$?
    set -e
    tr -d '\r' < "$actual" > "$normalized"
    tr -d '\r' < "$expected" > "$normalized_expected"
    if [ "$status" -eq 0 ] && diff -u "$normalized_expected" "$normalized"; then
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
    normalized="build/parser-$name.normalized"
    normalized_expected="build/parser-$name.expected.normalized"

    set +e
    "$program" "$input" >"$actual" 2>&1
    status=$?
    set -e
    tr -d '\r' < "$actual" > "$normalized"
    tr -d '\r' < "$expected" > "$normalized_expected"
    if [ "$status" -eq 1 ] && diff -u "$normalized_expected" "$normalized"; then
        echo "PASS parser invalid: $name"
        passed=$((passed + 1))
    else
        echo "FAIL parser invalid: $name (exit $status)"
        failed=$((failed + 1))
    fi
done

echo "$passed passed, $failed failed"
[ "$failed" -eq 0 ]
