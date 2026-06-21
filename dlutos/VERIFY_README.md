# DLUTos 功能验证命令

本文档记录 Git、Vim、GCC、Rust 的自动化验证脚本和手动验证命令。

## 自动化验证

```sh
sh /mnt/all_test.sh

sh /mnt/git_test.sh
sh /mnt/git_test.sh task0
sh /mnt/git_test.sh task1
sh /mnt/git_test.sh task2

sh /mnt/vim_test.sh
sh /mnt/gcc_test.sh
sh /mnt/rst_test.sh
```

`sh /mnt/git_test.sh task2` 会在 QEMU 内向 GitHub README 写入动态提交时间并推送到默认分支 `riscv`。
如果需要在宿主机手动触发同样的 README 更新时间，也可以执行：

```sh
cd /home/xzlllll/os_dlut/dlutos
sh tools/push_readme_timestamp.sh
```

该脚本会读取 `user-programs/git_env.local`，向 README 写入动态提交时间并推送到 GitHub 默认分支 `riscv`。

## 手动验证

```sh
git -h
git --version
git help
cd /root
rm -rf proj
mkdir proj
cd proj
git init
ls -la .git
printf '# Test Project\nThis is a test project for DLUTos.\n' > README.md
git add .
git ls-files --stage
git commit -m "add README.md"
printf 'Updated content\n' >> README.md
git add README.md
git commit -m "update README.md"
git log
git log --oneline

vim -h
vim --help
cd /root
rm -f hello.c
printf '#include <stdio.h>\nint main() {\n    printf("hello vim\\n");\n    return 0;\n}\n' > hello.c
vim hello.c
cat hello.c

gcc --help
gcc -v
cd /root
rm -f hello.c a.out myhello
printf '#include <stdio.h>\nint main(void) {\n    printf("Hello, World!\\n");\n    return 0;\n}\n' > hello.c
gcc hello.c
./a.out
gcc -o myhello hello.c
./myhello

rustc --help
rustc --version
cd /root
rm -f hello.rs helloworld myrust
printf 'fn main() {\n    println!("Hello, World!");\n}\n' > hello.rs
rustc hello.rs -o helloworld
./helloworld
rustc -o myrust hello.rs
./myrust
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
