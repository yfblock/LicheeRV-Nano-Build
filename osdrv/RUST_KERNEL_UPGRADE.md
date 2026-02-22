# LicheeRV 内核升级与 Rust 驱动指南

## 现状

| 项目 | 当前 | Rust 要求 |
|------|------|-----------|
| 内核版本 | linux_5.10 | 6.1+ |
| Rust 内核模块 | 不支持 | CONFIG_RUST=y |

## 为何需要升级

- **Rust 内核支持** 于 2022 年 10 月合并到 Linux 6.1
- Linux 5.10 无 Rust 基础设施，无法编译 Rust 内核模块
- 若计划用 Rust 编写驱动，必须使用 6.1 及以上内核

## 升级步骤概要

### 1. 选择内核来源

- **主线内核**：2024 年 5 月起主线已包含 LicheeRV Nano (SG2002) 的 device tree
- **厂商内核**：确认 Sophgo/CVitek 是否提供 6.1+ 分支

### 2. 配置 Rust

```kconfig
CONFIG_RUST=y
CONFIG_RUST_DEBUG_ASSERTIONS=n   # 生产环境可关闭
```

依赖：`rustc`、`bindgen`、`libclang-dev`、`llvm`

### 3. 修改 LicheeRV 构建

在 `build/` 中调整 `KERNEL_SRC` 指向新内核，并确保 defconfig 包含 `CONFIG_RUST=y`。

### 4. 构建 Rust 驱动

```bash
cd osdrv/extdrv/rust_hello_drv
make KDIR=$KERNEL_PATH/build/sg2002_licheervnano_sd LLVM=1
```

## 目录结构

```
osdrv/extdrv/
├── hello_drv/          # C 驱动（当前 5.10 可用）
├── rust_hello_drv/     # Rust 驱动（需 6.1+）
└── RUST_KERNEL_UPGRADE.md  # 本文件
```

## 参考

- [Rust for Linux](https://rust-for-linux.com/)
- [Kernel Rust 文档](https://docs.kernel.org/rust/)
- [Backport 说明](https://rust-for-linux.com/backporting-and-stable-lts-releases)
