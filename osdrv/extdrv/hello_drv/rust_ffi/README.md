# rust_ffi - Rust 内核打印 (pr_info/pr_debug/pr_err)

## 功能

将 Rust 的 `pr_info!`、`pr_debug!`、`pr_err!`、`pr_warn!` 宏映射到内核的 `pr_info`、`pr_debug`、`pr_err`、`pr_warn`。

## 用法

```rust
pr_info!("hello from Rust\n");
pr_debug!("debug: x={}\n", 42);
pr_err!("error occurred\n");
pr_warn!("warning\n");
```

## 实现

- **Rust** (`kernel_print.rs`): 格式化到缓冲区，调用 `extern "C"` 函数
- **C** (`rust_kernel.c`): 实现 `rust_kernel_pr_info` 等，内部调用内核 `pr_info` 等

## 依赖

需与 C 内核模块链接，C 侧提供 `rust_kernel_pr_*` 实现。
