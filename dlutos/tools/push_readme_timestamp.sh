#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ENV_FILE="$ROOT_DIR/user-programs/git_env.local"

if [ -f "$ENV_FILE" ]; then
    # shellcheck disable=SC1090
    . "$ENV_FILE"
fi

GIT_REMOTE_URL=${GIT_REMOTE_URL:-https://github.com/xzllll-ai/xv6-riscv-dlutos.git}
GIT_USERNAME=${GIT_USERNAME:-}
GIT_TOKEN=${GIT_TOKEN:-}
BRANCH=${1:-riscv}

if [ -z "$GIT_USERNAME" ] || [ -z "$GIT_TOKEN" ]; then
    echo "GIT_USERNAME or GIT_TOKEN is not set in $ENV_FILE" >&2
    exit 1
fi

case "$GIT_REMOTE_URL" in
    https://*) PUSH_URL="https://${GIT_USERNAME}:${GIT_TOKEN}@${GIT_REMOTE_URL#https://}" ;;
    *) PUSH_URL="$GIT_REMOTE_URL" ;;
esac

WORKDIR=$(mktemp -d)
cleanup() {
    rm -rf "$WORKDIR"
}
trap cleanup EXIT

COMMIT_TIME=$(date '+%Y-%m-%d %H:%M:%S %Z')

git clone -q --depth=1 --branch "$BRANCH" "$PUSH_URL" "$WORKDIR/repo"
cd "$WORKDIR/repo"
git config user.name "dlutos-tester"
git config user.email "dlutos@test.dlut.edu.cn"

if [ ! -f README.md ]; then
    cat > README.md <<'EOF'
# DLUTos Git Push Verification

This README is updated by the DLUTos verification workflow.
EOF
fi

cat >> README.md <<EOF

<!-- DLUTos host push verification -->
DLUTos commit time: $COMMIT_TIME
DLUTos push test time: $COMMIT_TIME

Repository: $GIT_REMOTE_URL
Branch: $BRANCH
EOF

git add README.md
git commit -q -m "update README - DLUTos test $COMMIT_TIME"
git push -q origin "HEAD:$BRANCH"

echo "Pushed README.md to $GIT_REMOTE_URL branch $BRANCH"
echo "Commit time: $COMMIT_TIME"
