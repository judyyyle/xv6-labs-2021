# xv6-labs-2021

本项目基于 MIT 6.S081 操作系统工程课程 提供的教学操作系统 xv6 进行扩展与实验（https://pdos.csail.mit.edu/6.828/2021/xv6.html），针对多项操作系统核心机制进行了实现与探索。
xv6 是一个基于 RISC-V 架构，参考 UNIX 第六版（v6）重新实现的教学用操作系统。

本仓库包含 MIT 提供的 xv6-labs-2021 各个实验的分支，分别为：

- `util`：常用用户态工具支持（如 `xargs`, `find`, `uniq` 等）
- `syscall`：系统调用实验
- `pgtbl`：页表机制实验
- `traps`：异常和中断机制实验
- `cow`：实现写时复制（Copy-On-Write）
- `thread`：用户线程支持实验
- `net`：网络支持实验
- `lock`：锁机制实验（包括自旋锁等）
- `fs`：文件系统实验
- `mmap`：文件映射实验（Memory-mapped file）

每个分支都是独立实现，可以切换到对应分支进行构建、运行和测试。
