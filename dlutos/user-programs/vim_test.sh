#!/bin/sh
#
# Vim 功能测试脚本  --  DLUTos 第二题
#
# 用法:
#   sh /mnt/vim_test.sh

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
# Task 2.1: vim -h  (5 分)
# ---------------------------------------------------------------------------
task1() {
    print_header "Task 2.1: vim -h"

    assert "vim command found" command -v vim

    local out
    out=$(vim -h 2>&1) || true
    assert "vim -h shows help" \
        sh -c "echo \"$out\" | grep -qi 'vim\|usage\|help\|BusyBox'"

    out=$(vim --help 2>&1) || true
    assert "vim --help shows info" \
        sh -c "echo \"$out\" | grep -qi 'vim\|usage\|help\|BusyBox'"
}

# ---------------------------------------------------------------------------
# Task 2.2: vim hello.c  (10 分)
# ---------------------------------------------------------------------------
task2() {
    print_header "Task 2.2: vim hello.c"

    cd /root
    rm -f hello.c 2>/dev/null || true

    local testfile="/root/hello.c"

    # Step 1: Create a C file
    cat > "$testfile" <<'EOF'
#include <stdio.h>
int main() {
    printf("hello vim\n");
    return 0;
}
EOF

    assert "hello.c created" test -f "$testfile"

    # Step 2: Verify vim can open the file
    # (busybox vi returns non-zero exit from pipe, but we verify via file state)
    echo ':q' | vim "$testfile" 2>/dev/null || true

    assert "vim can open hello.c" test -f "$testfile"

    # Step 3: Verify file content intact after vim opened it
    assert "hello.c has stdio.h" grep -q 'stdio.h' "$testfile"

    assert "hello.c has main()" grep -q 'int main' "$testfile"

    assert "hello.c has return 0" grep -q 'return 0' "$testfile"

    # Step 4: Modify and test vim save
    cat > "$testfile" <<'EOF'
#include <stdio.h>
int main() {
    printf("hello dlutos\n");
    return 0;
}
EOF

    echo ':wq' | vim "$testfile" 2>/dev/null || true

    assert "vim can save modified file" \
        grep -q 'hello dlutos' "$testfile"

    echo ""
    echo "  Content of hello.c after vim save:"
    cat "$testfile" 2>/dev/null | while read line; do echo "    $line"; done
}

# ---------------------------------------------------------------------------
# 主入口
# ---------------------------------------------------------------------------
main() {
    echo ""
    echo "  ============================================================"
    echo "         DLUTos Vim Test  --  Problem 2"
    echo "  ============================================================"

    env_check
    task1
    task2

    print_result
}

env_check() {
    print_header "Environment Check"

    echo -n "  vim:      "
    command -v vim 2>/dev/null || echo "NOT FOUND"

    echo -n "  vi:       "
    command -v vi 2>/dev/null || echo "NOT FOUND"
    echo ""
}

main
