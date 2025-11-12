#!/usr/bin/env bash
set -e

TEST_BIN="$1"

echo "Discovering allocation sites..."
MAX=$("$TEST_BIN" --discover | tail -n1)
echo "Discovered $MAX sites."

FAILURES=0

for i in $(seq 1 "$MAX"); do
    echo "=== Testing with fail site $i ==="
    echo TEST_FAIL_SITE=$i "$TEST_BIN"
    TEST_FAIL_SITE=$i "$TEST_BIN"
    RET=$?
    if [ $RET -ne 0 ]; then
        echo "❌ Test binary returned $RET with TEST_FAIL_SITE=$i"
        FAILURES=$((FAILURES + 1))
    fi
done

if [ "$FAILURES" -ne 0 ]; then
    echo "Total failures: $FAILURES"
    exit 1
fi

echo "✅ All fail-site tests passed."
