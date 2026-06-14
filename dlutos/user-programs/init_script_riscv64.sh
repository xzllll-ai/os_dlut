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
do_or_freeze $BUSYBOX --install -s /bin

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
EOF

ln -s /mnt1/lib /lib
ln -s /mnt1/usr /usr
mkdir -p /etc/ssl
ln -sf /mnt1/etc/ssl/cert.pem /etc/ssl/cert.pem
ln -sf /mnt1/etc/ssl/certs /etc/ssl/certs

# Make vim wrapper available
chmod +x /mnt/vim /mnt/gcc 2>/dev/null || true

# Create gcc wrapper (symlink shebang has issues on DLUTos)
cat > /bin/gcc <<'GCCEOF'
#!/bin/sh
exec sh /mnt/gcc "$@"
GCCEOF
chmod +x /bin/gcc
ln -sf /mnt/vim /bin/vim 2>/dev/null || true
ln -sf /mnt/rustc /bin/rustc 2>/dev/null || true
export PATH="/bin:/usr/bin:$PATH"
export SSL_CERT_FILE=/mnt1/etc/ssl/certs/ca-certificates.crt
export GIT_SSL_CAINFO=/mnt1/etc/ssl/certs/ca-certificates.crt

cat > /etc/resolv.conf <<EOF
nameserver 10.0.2.3
nameserver 8.8.8.8
EOF

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

alias ll="ls -l "
alias la="ls -la "
alias git-test="sh /mnt/git_test.sh"
alias git-test0="sh /mnt/git_test.sh task0"
alias git-test1="sh /mnt/git_test.sh task1"
alias git-test2="sh /mnt/git_test.sh task2"
	alias vim-test="sh /mnt/vim_test.sh"
	alias gcc-test="sh /mnt/gcc_test.sh"
	alias rustc-test="sh /mnt/rst_test.sh"

	echo ""
	echo "  DLUTos 功能测试就绪!"
	echo "  Git:  git-test    # 全部测试"
	echo "  Vim:  vim-test    # 全部测试"
	echo "  GCC:  gcc-test    # 全部测试"
	echo "  Rustc: rustc-test  # 全部测试"
	echo ""
PROFEOF

set +x
export HOME=/root
export TERM=dumb
export PS1='\w # '

exec /mnt/tshell < /dev/ttyS0 > /dev/ttyS0 2> /dev/ttyS0
