#!/bin/sh
set -eu

program=build/tactician-lexer.exe
passed=0
failed=0

for input in tests/cases/*.tac; do
    name=$(basename "$input" .tac)
    expected="tests/expected/$name.expected"
    actual="build/$name.actual"
    normalized="build/$name.normalized"
    expected_status=0
    case "$name" in
        invalid_characters|operator_prefixes|unterminated_comment|unsupported_exponent)
            expected_status=1
            ;;
    esac

    set +e
    "$program" "$input" >"$actual"
    status=$?
    set -e

    tr -d '\r' < "$actual" > "$normalized"

    if [ "$status" -ne "$expected_status" ]; then
        echo "FAIL $name: expected exit $expected_status, got $status"
        failed=$((failed + 1))
        continue
    fi

    if diff -u "$expected" "$normalized"; then
        echo "PASS $name"
        passed=$((passed + 1))
    else
        echo "FAIL $name: output differs"
        failed=$((failed + 1))
    fi
done

printf '/* CRLF comment\r\n   continues */\r\nprint item;\r\n' > build/crlf_comment.tac
set +e
"$program" build/crlf_comment.tac > build/crlf_comment.actual
status=$?
set -e
tr -d '\r' < build/crlf_comment.actual > build/crlf_comment.normalized
if [ "$status" -eq 0 ] && diff -u tests/expected/crlf_comment.expected build/crlf_comment.normalized; then
    echo "PASS crlf_comment"
    passed=$((passed + 1))
else
    echo "FAIL crlf_comment"
    failed=$((failed + 1))
fi

echo "$passed passed, $failed failed"
[ "$failed" -eq 0 ]
