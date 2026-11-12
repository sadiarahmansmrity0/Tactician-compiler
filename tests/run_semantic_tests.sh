#!/bin/sh
set -eu

program=build/tactician-semantic.exe
passed=0
failed=0

for input in tests/semantic/valid/*.tac; do
    name=$(basename "$input" .tac)
    expected="tests/semantic/expected/$name.expected"
    actual="build/semantic-$name.actual"
    set +e
    "$program" "$input" >"$actual" 2>&1
    status=$?
    set -e
    tr -d '\r' < "$actual" > "build/semantic-$name.normalized"
    if [ "$status" -eq 0 ] && diff -u "$expected" "build/semantic-$name.normalized"; then
        echo "PASS semantic valid: $name"
        passed=$((passed + 1))
    else
        echo "FAIL semantic valid: $name (exit $status)"
        failed=$((failed + 1))
    fi
done

for input in tests/semantic/invalid/*.tac; do
    name=$(basename "$input" .tac)
    expected="tests/semantic/expected/$name.expected"
    actual="build/semantic-$name.actual"
    set +e
    "$program" "$input" >"$actual" 2>&1
    status=$?
    set -e
    tr -d '\r' < "$actual" > "build/semantic-$name.normalized"
    if [ "$status" -eq 1 ] && diff -u "$expected" "build/semantic-$name.normalized"; then
        echo "PASS semantic invalid: $name"
        passed=$((passed + 1))
    else
        echo "FAIL semantic invalid: $name (exit $status)"
        failed=$((failed + 1))
    fi
done

echo "$passed passed, $failed failed"
[ "$failed" -eq 0 ]
