#!/bin/sh
#
# GCC 功能测试脚本  --  DLUTos 第三题
#
# 用法:
#   sh /mnt/gcc_test.sh

PASS=0
FAIL=0

assert() {
    local desc="$1"
    shift
    if "$@" > /dev/null 2>&1; then
        echo "  [PASS] $desc"
        PASS=$((PASS + 1))
        return 0
    else
        echo "  [FAIL] $desc"
        FAIL=$((FAIL + 1))
        return 1
    fi
}

print_header() {
    echo ""
    echo "============================================================"
    echo "  $1"
    echo "============================================================"
}

print_result() {
    local total=$((PASS + FAIL))
    echo ""
    echo "============================================================"
    if [ "$FAIL" -eq 0 ]; then
        echo "  [PASS] ALL PASSED  ($PASS/$total)"
    else
        echo "  [FAIL] PASS: $PASS  FAIL: $FAIL  (total $total)"
    fi
    echo "============================================================"
}

# ---------------------------------------------------------------------------
# Task 3.1: gcc --help  (5 分)
# ---------------------------------------------------------------------------
task1() {
    print_header "Task 3.1: gcc --help"

    assert "gcc command found" command -v gcc

    local out
    out=$(gcc --help 2>&1) || true
    assert "gcc --help shows usage" \
        sh -c "echo \"$out\" | grep -qi 'usage\|Usage\|options\|gcc\|compiler'"

    out=$(gcc -v 2>&1) || true
    assert "gcc -v shows version" \
        sh -c "echo \"$out\" | grep -q 'dlutos\|riscv\|gcc'"
}

# ---------------------------------------------------------------------------
# Task 3.2: sh /mnt/gcc hello.c && ./a.out  (10 分)
# ---------------------------------------------------------------------------
task2() {
    print_header "Task 3.2: sh /mnt/gcc hello.c && ./a.out"

    cd /root
    rm -f hello.c a.out 2>/dev/null || true

    # Create hello.c
    cat > hello.c <<'EOF'
#include <stdio.h>
int main(void) {
    printf("Hello, World!\n");
    return 0;
}
EOF

    assert "hello.c created" test -f hello.c

    # Compile
    assert "sh /mnt/gcc hello.c compiles" sh /mnt/gcc hello.c

    assert "a.out created" test -f a.out

    assert "a.out is executable" test -x a.out

    # Run and capture output
    local output
    output=$(./a.out 2>&1) || true

    echo ""
    echo "  a.out output:"
    echo "    $output"

    assert "a.out prints Hello, World!" \
        sh -c "echo \"$output\" | grep -q 'Hello, World'"

    # Test with -o flag
    rm -f myhello 2>/dev/null || true
    assert "sh /mnt/gcc -o myhello hello.c" sh /mnt/gcc -o myhello hello.c
    assert "myhello created" test -f myhello

    local output2
    output2=$(./myhello 2>&1) || true
    assert "myhello also prints Hello, World!" \
        sh -c "echo \"$output2\" | grep -q 'Hello, World'"
}

# ---------------------------------------------------------------------------
# 主入口
# ---------------------------------------------------------------------------
main() {
    echo ""
    echo "  ============================================================"
    echo "         DLUTos GCC Test  --  Problem 3"
    echo "  ============================================================"

    env_check
    task1
    task2

    print_result
}

env_check() {
    print_header "Environment Check"

    echo -n "  gcc:      "
    command -v gcc 2>/dev/null || echo "NOT FOUND"

    echo -n "  uname -m: "
    uname -m 2>/dev/null || echo "unknown"
    echo ""
}

main
