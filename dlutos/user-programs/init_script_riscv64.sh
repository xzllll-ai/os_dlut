#!/mnt/busybox sh

set -x

BUSYBOX=/mnt/busybox

freeze() {
    echo "an error occurred while executing '''$@''', freezing..." >&2

    while true; do
        true
    done
}

do_or_freeze() {
    if $@; then
        return
    fi

    freeze $@
}

do_or_freeze $BUSYBOX mkdir -p /tmp

do_or_freeze $BUSYBOX mkdir -p /dev

do_or_freeze $BUSYBOX mknod -m 666 /dev/console c 5 1
do_or_freeze $BUSYBOX mknod -m 666 /dev/null c 1 3
do_or_freeze $BUSYBOX mknod -m 666 /dev/zero c 1 5
do_or_freeze $BUSYBOX mknod -m 666 /dev/random c 1 8
do_or_freeze $BUSYBOX mknod -m 666 /dev/urandom c 1 9
do_or_freeze $BUSYBOX mknod -m 666 /dev/vda b 8 0
do_or_freeze $BUSYBOX mknod -m 666 /dev/vdb b 8 16
do_or_freeze $BUSYBOX mknod -m 666 /dev/vdb1 b 8 17
do_or_freeze $BUSYBOX mknod -m 666 /dev/ttyS0 c 4 64
do_or_freeze $BUSYBOX mknod -m 666 /dev/ttyS1 c 4 65

echo -n -e "deploying busybox... " >&2

do_or_freeze $BUSYBOX mkdir -p /bin

install_busybox_applet() {
    do_or_freeze $BUSYBOX ln -sf /mnt/busybox "/bin/$1"
}

for applet in \
    sh ash cat chmod cp date dd dirname basename grep ln mkdir mknod mount \
    mv rm sleep uname wc wget base64 vi test true false kill
do
    install_busybox_applet "$applet"
done
do_or_freeze $BUSYBOX ln -sf /mnt/busybox /bin/[

export PATH="/bin"

echo ok >&2

do_or_freeze mkdir -p /etc /root /proc
do_or_freeze mount -t procfs proc proc

# Check if the device /dev/vda is available and can be read
if dd if=/dev/vdb of=/dev/null bs=512 count=1; then
    echo -n -e "Mounting the ext4 image... " >&2
    do_or_freeze mkdir -p /mnt1
    do_or_freeze mount -t ext4 /dev/vdb /mnt1
    echo ok >&2
fi

cat > /etc/passwd <<EOF
root:x:0:0:root:/root:/mnt/busybox sh
EOF

cat > /etc/group <<EOF
root:x:0:root
EOF

cat > /etc/profile <<EOF
export PATH=/bin
export TERM=xterm
EOF

ln -s /mnt1/lib /lib
ln -s /mnt1/usr /usr
mkdir -p /etc/ssl
ln -sf /mnt1/etc/ssl/cert.pem /etc/ssl/cert.pem
ln -sf /mnt1/etc/ssl/certs /etc/ssl/certs

# Make bundled user programs available
chmod +x /mnt/git /mnt/vim /mnt/gcc /mnt/rustc /mnt/lswrap /mnt/curl 2>/dev/null || true
chmod +x /mnt/all_test.sh /mnt/git_test.sh /mnt/vim_test.sh /mnt/gcc_test.sh /mnt/rst_test.sh 2>/dev/null || true

# Disable BusyBox ls color escape sequences; the DLUTos serial console may show
# raw ANSI color fragments such as "[1;34mtmp".
rm -f /bin/ls
cp /mnt/lswrap /bin/ls
chmod +x /bin/ls

# Create git wrapper (avoid Alpine git SIGBUS paths on DLUTos)
cp /mnt/git /bin/git
chmod +x /bin/git
cp /mnt/curl /bin/curl
chmod +x /bin/curl

cp /mnt/vim /bin/vim
chmod +x /bin/vim
cp /mnt/gcc /bin/gcc
chmod +x /bin/gcc
cp /mnt/rustc /bin/rustc
chmod +x /bin/rustc
export PATH="/bin:/usr/bin:$PATH"
export SSL_CERT_FILE=/mnt1/etc/ssl/certs/ca-certificates.crt
export GIT_SSL_CAINFO=/mnt1/etc/ssl/certs/ca-certificates.crt

cat > /etc/resolv.conf <<EOF
nameserver 10.0.2.3
nameserver 8.8.8.8
EOF

if [ -f /mnt/githosts ]; then
    cp /mnt/githosts /etc/hosts
else
    cat > /etc/hosts <<EOF
127.0.0.1 localhost
198.18.0.61 api.github.com
198.18.0.61 github.com
EOF
fi

# ---- Git 环境配置 (第一题) ----
mkdir -p /root/.ssh
cat > /root/.ssh/config <<'SSHEOF'
Host github.com
    StrictHostKeyChecking no
    UserKnownHostsFile /dev/null
Host gitlink.org.cn
    StrictHostKeyChecking no
    UserKnownHostsFile /dev/null
Host gitee.com
    StrictHostKeyChecking no
    UserKnownHostsFile /dev/null
SSHEOF

cat > /root/.gitconfig <<'GITEOF'
[user]
	name = dlutos-tester
	email = dlutos@test.dlut.edu.cn
[http]
	sslVerify = false
	lowSpeedLimit = 1000
	lowSpeedTime = 60
[init]
	defaultBranch = master
GITEOF

# 创建方便的别名
cat > /root/.profile <<'PROFEOF'
export HOME=/root
export PATH="/bin:/usr/bin:$PATH"
export GIT_REMOTE_URL="https://github.com/xzllll-ai/xv6-riscv-dlutos.git"

alias ll="ls -l "
alias la="ls -la "
alias all-test="sh /mnt/all_test.sh"
alias git-test="sh /mnt/git_test.sh"
alias git-test0="sh /mnt/git_test.sh task0"
alias git-test1="sh /mnt/git_test.sh task1"
alias git-test2="sh /mnt/git_test.sh task2"
	alias vim-test="sh /mnt/vim_test.sh"
	alias gcc-test="sh /mnt/gcc_test.sh"
	alias rustc-test="sh /mnt/rst_test.sh"

	echo ""
	echo "  DLUTos 功能测试就绪!"
	echo "  All:  all-test   # 全部测试"
	echo "  Git:  git-test    # 全部测试"
	echo "  Vim:  vim-test    # 全部测试"
	echo "  GCC:  gcc-test    # 全部测试"
	echo "  Rustc: rustc-test  # 全部测试"
	echo ""
PROFEOF

set +x
export HOME=/root
export TERM=xterm
export GIT_REMOTE_URL="https://github.com/xzllll-ai/xv6-riscv-dlutos.git"
if [ -f /mnt/git_env ]; then
	. /mnt/git_env
fi
export PS1='\w # '

exec /mnt/tshell < /dev/ttyS0 > /dev/ttyS0 2> /dev/ttyS0
