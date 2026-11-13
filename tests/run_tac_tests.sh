#!/bin/sh
set -eu

program=build/tactician-tac.exe
passed=0
failed=0

for input in tests/tac/valid/*.tac; do
    name=$(basename "$input" .tac)
    expected="tests/tac/expected/$name.expected"
    actual="build/tac-$name.actual"
    "$program" "$input" >"$actual" 2>&1
    tr -d '\r' < "$actual" > "build/tac-$name.normalized"
    if diff -u "$expected" "build/tac-$name.normalized"; then
        echo "PASS TAC valid: $name"
        passed=$((passed + 1))
    else
        echo "FAIL TAC valid: $name"
        failed=$((failed + 1))
    fi
done

for input in tests/tac/invalid/*.tac; do
    name=$(basename "$input" .tac)
    expected="tests/tac/expected/$name.expected"
    actual="build/tac-$name.actual"
    set +e
    "$program" "$input" >"$actual" 2>&1
    status=$?
    set -e
    tr -d '\r' < "$actual" > "build/tac-$name.normalized"
    if [ "$status" -eq 1 ] && diff -u "$expected" "build/tac-$name.normalized"; then
        echo "PASS TAC invalid (no TAC): $name"
        passed=$((passed + 1))
    else
        echo "FAIL TAC invalid: $name (exit $status)"
        failed=$((failed + 1))
    fi
done

echo "$passed passed, $failed failed"
[ "$failed" -eq 0 ]
