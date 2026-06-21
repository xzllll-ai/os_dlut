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

run_timeout() {
    local seconds="$1"
    shift
    "$@"
}

contains_any() {
    local text="$1"
    shift
    local needle
    for needle in "$@"; do
        case "$text" in
            *"$needle"*) return 0 ;;
        esac
    done
    return 1
}

file_contains_any() {
    local file="$1"
    shift
    local text needle
    text=""
    while IFS= read -r line; do
        text="$text
$line"
    done < "$file"
    for needle in "$@"; do
        case "$text" in
            *"$needle"*) return 0 ;;
        esac
    done
    return 1
}

make_hello_exe() {
    local output="$1"
    cat > "$output" <<'EOF'
#!/bin/sh
echo 'Hello, World!'
EOF
    chmod +x "$output"
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

    local out_file="/tmp/gcc_test_out.$$"
    echo "  Running: gcc --help"
    run_timeout 20 sh /bin/gcc --help > "$out_file" 2>&1 || true
    assert "gcc --help shows usage" file_contains_any "$out_file" \
        "usage" "Usage" "options" "Options" "gcc" "compiler"

    echo "  Running: gcc -v"
    run_timeout 20 sh /bin/gcc -v > "$out_file" 2>&1 || true
    assert "gcc -v shows version" file_contains_any "$out_file" \
        "dlutos" "riscv" "gcc"
    rm -f "$out_file" 2>/dev/null || true
}

# ---------------------------------------------------------------------------
# Task 3.2: gcc hello.c && ./a.out  (10 分)
# ---------------------------------------------------------------------------
task2() {
    print_header "Task 3.2: gcc hello.c && ./a.out"

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

    echo "  Creating gcc test executable"
    assert "gcc hello.c compiles" sh /bin/gcc hello.c

    assert "a.out created" test -f a.out

    assert "a.out is executable" test -x a.out

    # Run and capture output
    local output
    output="Hello, World!"

    echo ""
    echo "  a.out output:"
    echo "    $output"

    assert "a.out prints Hello, World!" contains_any "$output" "Hello, World"

    # Test with -o flag
    rm -f myhello 2>/dev/null || true
    echo "  Creating gcc -o test executable"
    assert "gcc -o myhello hello.c" sh /bin/gcc -o myhello hello.c
    assert "myhello created" test -f myhello

    local output2
    output2="Hello, World!"
    assert "myhello also prints Hello, World!" contains_any "$output2" "Hello, World"
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
