#!/bin/sh
#
# Run all DLUTos feature tests.
#
# Usage:
#   sh /mnt/all_test.sh

PASS=0
FAIL=0

run_test() {
    name="$1"
    script="$2"

    echo ""
    echo "============================================================"
    echo "  $name"
    echo "============================================================"

    if sh "$script"; then
        echo "  [PASS] $name"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $name"
        FAIL=$((FAIL + 1))
    fi
}

echo ""
echo "  ============================================================"
echo "         DLUTos All Tests"
echo "  ============================================================"

run_test "Git Test" /mnt/git_test.sh
run_test "Vim Test" /mnt/vim_test.sh
run_test "GCC Test" /mnt/gcc_test.sh
run_test "Rustc Test" /mnt/rst_test.sh

echo ""
echo "============================================================"
if [ "$FAIL" -eq 0 ]; then
    echo "  [PASS] ALL TEST GROUPS PASSED  ($PASS/4)"
    echo "============================================================"
    exit 0
fi

echo "  [FAIL] PASS: $PASS  FAIL: $FAIL  (total 4)"
echo "============================================================"
exit 1
