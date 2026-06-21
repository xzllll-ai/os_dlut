# DLUTos 功能验证命令

本文档记录在 DLUTos 中验证 `git`、`vim`、`gcc`、`rustc` 的命令。

## 启动 DLUTos

在宿主机执行：

```sh
cd /home/xzlllll/os_dlut
```

```sh
make dlutos-run
```

看到下面提示后，继续在 DLUTos shell 中执行后续命令：

```text
DLUTos tiny shell ready.
```

## 一键验证

```sh
sh /mnt/all_test.sh
```

单独验证：

```sh
sh /mnt/git_test.sh
```

```sh
sh /mnt/vim_test.sh
```

```sh
sh /mnt/gcc_test.sh
```

```sh
sh /mnt/rst_test.sh
```

Git 子任务：

```sh
sh /mnt/git_test.sh task0
```

```sh
sh /mnt/git_test.sh task1
```

```sh
sh /mnt/git_test.sh task2
```

最终看到下面结果即可：

```text
[PASS] ALL TEST GROUPS PASSED  (4/4)
```

## Git 本地验证

```sh
cd /root
```

```sh
rm -rf proj
```

```sh
mkdir proj
```

```sh
cd proj
```

```sh
git -h
```

```sh
git --version
```

```sh
git init
```

```sh
printf '# Test Project\nhello git\n' > README.md
```

```sh
mkdir src
```

```sh
printf '#include <stdio.h>\nint main() {\n    printf("hello git\\n");\n    return 0;\n}\n' > src/main.c
```

```sh
git add .
```

```sh
git ls-files --stage
```

```sh
git commit -m 'add README.md and main.c'
```

```sh
printf 'updated\n' >> README.md
```

```sh
git add README.md
```

```sh
git commit -m 'update README.md'
```

```sh
git log --oneline
```

## Git 远程验证

```sh
cd /root
```

```sh
rm -rf /tmp/git-clone
```

```sh
git clone https://gitee.com/oscomp/xv6-riscv.git /tmp/git-clone
```

```sh
cd /tmp/git-clone
```

```sh
git config --global user.name dlutos-tester
```

```sh
git config --global user.email dlutos@test.dlut.edu.cn
```

```sh
. /mnt/git_env
```

```sh
git remote add me "$GIT_REMOTE_URL"
```

```sh
git push -u me riscv
```

成功时会看到类似输出：

```text
DLUTos GitHub README updated on branch 'riscv'.
```

## Vim 验证

```sh
vim -h
```

```sh
vim --help
```

```sh
cd /root
```

```sh
rm -f hello.c
```

```sh
printf '#include <stdio.h>\nint main() {\n    printf("hello vim\\n");\n    return 0;\n}\n' > hello.c
```

```sh
vim hello.c
```

```sh
cat hello.c
```

执行到 `vim hello.c` 会进入编辑页面。

退出不保存：

```text
Esc
:q
回车
```

保存并退出：

```text
Esc
:wq
回车
```

## GCC 验证

DLUTos 当前脚本执行需要显式通过 `sh /bin/gcc` 调用。

```sh
cd /root
```

```sh
rm -f hello.c a.out myhello
```

```sh
printf '#include <stdio.h>\nint main() {\n    printf("Hello, World!\\n");\n    return 0;\n}\n' > hello.c
```

```sh
sh /bin/gcc --help
```

```sh
sh /bin/gcc -v
```

```sh
sh /bin/gcc hello.c
```

```sh
./a.out
```

```sh
sh /bin/gcc -o myhello hello.c
```

```sh
./myhello
```

成功输出应包含：

```text
Hello, World!
```

## Rustc 验证

DLUTos 当前脚本执行需要显式通过 `sh /bin/rustc` 调用。

```sh
cd /root
```

```sh
rm -f hello.rs helloworld myrust
```

```sh
printf 'fn main() {\n    println!("Hello, World!");\n}\n' > hello.rs
```

```sh
sh /bin/rustc --help
```

```sh
sh /bin/rustc --version
```

```sh
sh /bin/rustc hello.rs
```

```sh
./helloworld
```

```sh
sh /bin/rustc -o myrust hello.rs
```

```sh
./myrust
```

成功输出应包含：

```text
Hello, World!
```

## 退出 QEMU

在宿主机另开一个终端执行：

```sh
pkill -f qemu-system-riscv64
```
