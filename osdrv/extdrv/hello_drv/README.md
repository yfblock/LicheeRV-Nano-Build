# hello_drv - C 内核模块

当前 LicheeRV 使用 Linux 5.10，暂不支持 Rust 内核模块。此为 C 实现。

**Rust 驱动**：升级到内核 6.1+ 后使用 `../rust_hello_drv/`。

## 构建

```bash
make modules
# 或指定: make KERNEL_DIR=... ARCH=riscv
```

## 使用

```bash
insmod hello_drv.ko
dmesg | tail
rmmod hello_drv
```
