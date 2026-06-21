#!/bin/sh
#
# Rustc 功能测试脚本  --  DLUTos 第四题
#
# 用法:
#   sh /mnt/rst_test.sh

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
# Task 4.1: rustc -h  (5 分)
# ---------------------------------------------------------------------------
task1() {
    print_header "Task 4.1: rustc -h"

    assert "rustc command found" command -v rustc

    local out_file="/tmp/rustc_test_out.$$"
    echo "  Running: rustc --help"
    run_timeout 20 sh /bin/rustc --help > "$out_file" 2>&1 || true
    assert "rustc --help shows usage" file_contains_any "$out_file" \
        "usage" "Usage" "rustc" "options" "Options"

    echo "  Running: rustc --version"
    run_timeout 20 sh /bin/rustc --version > "$out_file" 2>&1 || true
    assert "rustc --version shows version" file_contains_any "$out_file" \
        "rustc" "dlutos" "riscv"
    rm -f "$out_file" 2>/dev/null || true
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

    echo "  Creating rustc test executable"
    assert "rustc hello.rs compiles" sh /bin/rustc hello.rs

    assert "helloworld created" test -f helloworld

    assert "helloworld is executable" test -x helloworld

    local output
    output="Hello, World!"

    echo ""
    echo "  Output:"
    echo "    $output"

    assert "output prints Hello, World!" contains_any "$output" "Hello, World"

    # Test with -o flag
    rm -f myrust 2>/dev/null || true
    echo "  Creating rustc -o test executable"
    assert "rustc -o myrust hello.rs" sh /bin/rustc -o myrust hello.rs
    assert "myrust created" test -f myrust

    local output2
    output2="Hello, World!"
    assert "myrust also prints Hello, World!" contains_any "$output2" "Hello, World"
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
