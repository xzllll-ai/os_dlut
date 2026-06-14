#!/bin/sh
#
# Rustc 功能测试脚本  --  DLUTos 第四题
#
# 用法:
#   sh /mnt/rustc_test.sh

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
# Task 4.1: rustc -h  (5 分)
# ---------------------------------------------------------------------------
task1() {
    print_header "Task 4.1: rustc -h"

    assert "rustc command found" command -v rustc

    local out
    out=$(sh /mnt/rustc --help 2>&1) || true
    assert "rustc --help shows usage" \
        sh -c "echo \"$out\" | grep -qi 'usage\|Usage\|rustc\|options'"

    out=$(sh /mnt/rustc -v 2>&1) || true
    assert "rustc -v shows version" \
        sh -c "echo \"$out\" | grep -q 'rustc\|dlutos\|riscv'"
}

# ---------------------------------------------------------------------------
# Task 4.2: rustc hello.rs && ./helloworld  (10 分)
# ---------------------------------------------------------------------------
task2() {
    print_header "Task 4.2: rustc hello.rs && ./a.out"

    cd /root
    rm -f hello.rs helloworld a.out 2>/dev/null || true

    cat > hello.rs <<'EOF'
fn main() {
    println!("Hello, World!");
}
EOF

    assert "hello.rs created" test -f hello.rs

    assert "rustc hello.rs compiles" sh /mnt/rustc hello.rs

    assert "helloworld created" test -f helloworld

    assert "helloworld is executable" test -x helloworld

    local output
    output=$(./helloworld 2>&1) || true

    echo ""
    echo "  Output:"
    echo "    $output"

    assert "output prints Hello, World!" \
        sh -c "echo \"$output\" | grep -q 'Hello, World'"

    # Test with -o flag
    rm -f myrust 2>/dev/null || true
    assert "rustc -o myrust hello.rs" sh /mnt/rustc -o myrust hello.rs
    assert "myrust created" test -f myrust

    local output2
    output2=$(./myrust 2>&1) || true
    assert "myrust also prints Hello, World!" \
        sh -c "echo \"$output2\" | grep -q 'Hello, World'"
}

# ---------------------------------------------------------------------------
# 主入口
# ---------------------------------------------------------------------------
main() {
    echo ""
    echo "  ============================================================"
    echo "         DLUTos Rustc Test  --  Problem 4"
    echo "  ============================================================"

    env_check
    task1
    task2

    print_result
}

env_check() {
    print_header "Environment Check"

    echo -n "  rustc:    "
    command -v rustc 2>/dev/null || echo "NOT FOUND"

    echo -n "  uname -m: "
    uname -m 2>/dev/null || echo "unknown"
    echo ""
}

main
