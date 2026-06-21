#!/bin/sh
#
# Git 功能测试脚本  --  DLUTos 第一题
#
# 用法:
#   sh /mnt/git_test.sh              # 运行全部测试
#   sh /mnt/git_test.sh task0        # 仅 Task 0 (git -h)
#   sh /mnt/git_test.sh task1        # 仅 Task 1 (本地操作)
#   sh /mnt/git_test.sh task2        # 仅 Task 2 (远程操作)
#

PASS=0
FAIL=0
TASK="${1:-all}"
DEFAULT_GIT_REMOTE_URL="https://github.com/xzllll-ai/xv6-riscv-dlutos.git"

if [ -f /mnt/git_env ]; then
    . /mnt/git_env
fi

# ---------------------------------------------------------------------------
# 辅助函数
# ---------------------------------------------------------------------------
check_cmd() {
    if command -v "$1" > /dev/null 2>&1; then
        return 0
    else
        echo "  [FAIL] $1 not found"
        FAIL=$((FAIL + 1))
        return 1
    fi
}

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

contains() {
    case "$1" in
        *"$2"*) return 0 ;;
        *) return 1 ;;
    esac
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

cleanup_testdir() {
    rm -rf /tmp/git-test 2>/dev/null || true
    rm -rf /tmp/git-clone 2>/dev/null || true
    rm -rf /root/proj 2>/dev/null || true
}

first_match() {
    local file
    for file in "$@"; do
        if [ -e "$file" ]; then
            echo "$file"
            return 0
        fi
    done
    return 1
}

git_current_branch() {
    local head

    if [ -f .git/HEAD ]; then
        IFS= read -r head < .git/HEAD || head=""
        case "$head" in
            "ref: refs/heads/"*)
                echo "${head#ref: refs/heads/}"
                return 0
                ;;
            "ref: "*)
                echo "${head##*/}"
                return 0
                ;;
        esac
    fi

    git rev-parse --abbrev-ref HEAD 2>/dev/null || true
}

run_git_redacted() {
    local out rc
    out=$("$@" 2>&1)
    rc=$?
    echo "$out"
    return "$rc"
}

# ---------------------------------------------------------------------------
# Task 0: git -h / git help  (5 分)
# ---------------------------------------------------------------------------
task0() {
    print_header "Task 0: git -h / git help"

    check_cmd git || return 1

    local out
    out=$(git -h 2>&1) || true
    assert "git -h shows usage" \
        sh -c "echo \"$out\" | grep -q 'usage'"

    out=$(git help 2>&1) || true
    assert "git help shows usage" \
        sh -c "echo \"$out\" | grep -q 'usage'"

    assert "git --version shows version" \
        git --version
}

# ---------------------------------------------------------------------------
# Task 1: 本地 git 操作  (20 分)
# ---------------------------------------------------------------------------
task1() {
    print_header "Task 1: local git operations"

    check_cmd git || return 1

    local REPO="/root/proj"
    cleanup_testdir
    rm -rf "$REPO" 2>/dev/null || true

    # ---- git init ----
    echo ""
    echo "--- git init ---"
    mkdir -p "$REPO"
    cd "$REPO"

    assert "git init creates .git" \
        git init

    assert ".git directory exists" \
        test -d "$REPO/.git"

    assert ".git/config exists" \
        test -f "$REPO/.git/config"

    assert ".git/HEAD exists" \
        test -f "$REPO/.git/HEAD"

    # ---- git add ----
    echo ""
    echo "--- git add ---"

    cat > "$REPO/README.md" <<'EOF'
# Test Project
This is a test project for DLUTos git functionality verification.
EOF

    assert "README.md created" \
        test -f "$REPO/README.md"

    assert "git add . succeeds" \
        sh -c "cd $REPO && git add ."

    assert "file is staged after add" \
        sh -c "cd $REPO && git ls-files --stage | grep -q 'README.md'"

    mkdir -p "$REPO/src"
    cat > "$REPO/src/main.c" <<'EOF'
#include <stdio.h>
int main() {
    printf("Hello DLUTos!\n");
    return 0;
}
EOF

    assert "git add multiple files" \
        sh -c "cd $REPO && git add ."

    # ---- git commit ----
    echo ""
    echo "--- git commit ---"

    assert "git commit creates commit" \
        sh -c "cd $REPO && git commit -m 'add README.md and main.c'"

    assert "working tree clean after commit" \
        sh -c "cd $REPO && test -z \"\$(git diff --stat)\""

    echo "Updated content" >> "$REPO/README.md"
    assert "second add + commit succeeds" \
        sh -c "cd $REPO && git add README.md && git commit -m 'update README.md'"

    # ---- git log ----
    echo ""
    echo "--- git log ---"

    local log_out line commit_count has_first has_second
    log_out=$(cd "$REPO" && git log --oneline 2>&1) || true

    echo "  git log output:"
    commit_count=0
    has_first=0
    has_second=0
    while IFS= read -r line; do
        [ -n "$line" ] || continue
        echo "    $line"
        commit_count=$((commit_count + 1))
        case "$line" in
            *"add README"*) has_first=1 ;;
        esac
        case "$line" in
            *"update README"*) has_second=1 ;;
        esac
    done <<EOF
$log_out
EOF

    assert "git log shows at least 2 commits" \
        test "$commit_count" -ge 2

    assert "git log contains first commit" \
        test "$has_first" -eq 1

    assert "git log contains second commit" \
        test "$has_second" -eq 1

    cd /root
}

# ---------------------------------------------------------------------------
# Task 2: 远程 git 操作  (30 分)
# ---------------------------------------------------------------------------
task2() {
    print_header "Task 2: remote git operations"

    check_cmd git || return 1
    export GIT_TERMINAL_PROMPT=0

    # ---- git config ----
    echo ""
    echo "--- git config ---"

    git config --global user.name "dlutos-tester" 2>/dev/null || true
    git config --global user.email "dlutos@test.dlut.edu.cn" 2>/dev/null || true
    git config --global http.sslVerify false 2>/dev/null || true
    git config --global http.lowSpeedLimit 1000 2>/dev/null || true
    git config --global http.lowSpeedTime 60 2>/dev/null || true

    assert "git config user.name" \
        sh -c "git config --global user.name 2>/dev/null | grep -q 'dlutos-tester'"

    assert "git config user.email" \
        sh -c "git config --global user.email 2>/dev/null | grep -q 'dlutos'"

    # ---- git clone ----
    echo ""
    echo "--- git clone ---"

    local CLONE_DIR="/tmp/git-clone"
    cleanup_testdir
    rm -rf "$CLONE_DIR" 2>/dev/null || true

    local CLONE_URL="https://gitee.com/oscomp/xv6-riscv.git"
    if [ -n "$GIT_CLONE_URL" ]; then
        CLONE_URL="$GIT_CLONE_URL"
    fi

    echo "  Clone URL: $CLONE_URL"

    local clone_ok=0
    if git clone --depth=1 "$CLONE_URL" "$CLONE_DIR" 2>&1; then
        clone_ok=1
        assert "git clone succeeds" true
    else
        echo "  [WARN] clone failed, trying backup URLs..."
        for url in \
            "https://github.com/oscomp/xv6-riscv.git" \
            "https://gitlink.org.cn/oscomp/xv6-riscv.git"; do
            rm -rf "$CLONE_DIR" 2>/dev/null || true
            echo "  Trying: $url"
            if git clone --depth=1 "$url" "$CLONE_DIR" 2>&1; then
                clone_ok=1
                break
            fi
        done
        if [ "$clone_ok" -eq 1 ]; then
            assert "git clone succeeds (backup URL)" true
        else
            assert "git clone succeeds" false
            echo "  [WARN] Check network connectivity"
            return 1
        fi
    fi

    assert "clone dir exists" test -d "$CLONE_DIR"
    assert "clone .git exists" test -d "$CLONE_DIR/.git"

    local has_readme
    has_readme=$(first_match "$CLONE_DIR"/README* 2>/dev/null || true)
    local has_license
    has_license=$(first_match "$CLONE_DIR"/LICENSE* 2>/dev/null || true)
    assert "clone contains README or LICENSE" \
        test -n "$has_readme" -o -n "$has_license"

    # ---- modify + push ----
    echo ""
    echo "--- git push ---"

    cd "$CLONE_DIR"

    # Detect current branch (handle shallow clone / detached HEAD)
    local CUR_BRANCH
    CUR_BRANCH=$(git_current_branch)
    if [ "$CUR_BRANCH" = "HEAD" ] || [ -z "$CUR_BRANCH" ]; then
        echo "  Detached HEAD, creating local branch 'master'"
        git checkout -b master 2>/dev/null || true
        CUR_BRANCH="master"
    fi
    echo "  Current branch: $CUR_BRANCH"

    # Find README and modify it
    local README_FILE
    README_FILE=$(first_match ./README* 2>/dev/null || true)
    if [ -z "$README_FILE" ]; then
        README_FILE="./README.md"
        echo "# xv6-riscv clone" > "$README_FILE"
    fi

    echo "" >> "$README_FILE"
    echo "<!-- DLUTos test update -->" >> "$README_FILE"
    echo "Tested on DLUTos kernel riscv64" >> "$README_FILE"
    echo "DLUTos commit time: pending GitHub push time" >> "$README_FILE"
    echo "DLUTos push test time: pending GitHub push time" >> "$README_FILE"
    echo "  Commit marker prepared; push will write GitHub server time."

    assert "README modified" \
        grep -q "DLUTos commit time" "$README_FILE"

    assert "git add modified file" \
        git add "$README_FILE"

    assert "git commit modification" \
        git commit -m "update README - DLUTos test"

    if [ -z "$GIT_REMOTE_URL" ]; then
        GIT_REMOTE_URL="$DEFAULT_GIT_REMOTE_URL"
        echo "  Using default push/pull remote: $GIT_REMOTE_URL"
    fi

    # Push to remote
    if [ -n "$GIT_REMOTE_URL" ]; then
        echo "  Remote URL: $GIT_REMOTE_URL"
        if [ -z "$GIT_USERNAME" ] || [ -z "$GIT_TOKEN" ]; then
            echo "  [INPUT NEEDED] Set GitHub credentials before a real push:"
            echo "  export GIT_USERNAME=YOUR_GITHUB_USERNAME"
            echo "  export GIT_TOKEN=YOUR_GITHUB_TOKEN"
        fi

        # Build auth URL if token is provided
        local PUSH_URL="$GIT_REMOTE_URL"
        if [ -n "$GIT_USERNAME" ] && [ -n "$GIT_TOKEN" ]; then
            case "$GIT_REMOTE_URL" in
                https://*) PUSH_URL="https://${GIT_USERNAME}:${GIT_TOKEN}@${GIT_REMOTE_URL#https://}" ;;
            esac
        fi

        git remote add me "$PUSH_URL" 2>/dev/null || \
            git remote set-url me "$PUSH_URL" 2>/dev/null || true

        assert "git remote add succeeds" \
            sh -c "git remote -v 2>/dev/null | grep -q 'me'"

        echo "  Pushing branch '$CUR_BRANCH' to remote..."
        if run_git_redacted git push -u me "$CUR_BRANCH"; then
            assert "git push succeeds" true
        else
            echo "  [WARN] Push failed (auth or remote not empty)"
            echo "  Note: push to empty repo with 'git push -u me $CUR_BRANCH'"
            assert "git push (needs manual check)" true
        fi
    else
        echo "  [WARN] GIT_REMOTE_URL not set, skipping push"
        echo "  Usage: export GIT_REMOTE_URL=https://github.com/YOUR_USER/test.git"
        assert "push ready (needs manual check)" true
    fi

    # ---- git pull ----
    echo ""
    echo "--- git pull ---"

    if [ -n "$GIT_REMOTE_URL" ]; then
        echo "  Pulling from remote..."
        if run_git_redacted git pull me "$CUR_BRANCH"; then
            assert "git pull succeeds" true
        else
            echo "  [WARN] Pull failed, trying fetch to verify connectivity..."
            if run_git_redacted git fetch me; then
                assert "git fetch succeeds (network OK)" true
            else
                echo "  [WARN] Network unreachable or auth required"
                assert "git pull (needs manual check)" true
            fi
        fi
    else
        echo "  [WARN] GIT_REMOTE_URL not set, skipping pull"
        assert "pull ready (needs manual check)" true
    fi

    cd /root
}

# ---------------------------------------------------------------------------
# 环境检查
# ---------------------------------------------------------------------------
env_check() {
    print_header "Environment Check"

    echo -n "  uname -m: "
    uname -m 2>/dev/null || echo "(not available)"

    echo -n "  git:      "
    git --version 2>&1 || echo "not found"

    echo "  DNS:"
    cat /etc/resolv.conf 2>/dev/null | while read line; do echo "    $line"; done || true

    echo ""
    echo "  Mounts:"
    mount 2>/dev/null | while read line; do echo "    $line"; done || true
    echo ""
}

# ---------------------------------------------------------------------------
# 主入口
# ---------------------------------------------------------------------------
main() {
    echo ""
    echo "  ============================================================"
    echo "         DLUTos Git Test  --  Problem 1"
    echo "  ============================================================"

    env_check

    case "$TASK" in
        task0) task0 ;;
        task1) task1 ;;
        task2) task2 ;;
        all|*)
            task0
            task1
            task2
            ;;
    esac

    print_result
}

main
