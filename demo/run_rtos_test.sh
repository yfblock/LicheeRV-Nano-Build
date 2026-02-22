#!/bin/bash
# 通过 ADB 测试设备上 RTOS（soph_rtos_cmdqu）是否在运行
# 用法: ./run_rtos_test.sh [路径/to/rtos_cmdqu_test]
# 不传参数时只做脚本检查；传可执行文件路径则推送到设备并运行该测试程序

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REMOTE_DIR="/data/local/tmp"
REMOTE_TEST="$REMOTE_DIR/rtos_cmdqu_test"

echo "=== 1. 检查 ADB 设备 ==="
if ! adb devices | grep -q 'device$'; then
    echo "错误: 未检测到 ADB 设备。"
    exit 1
fi
echo "设备已连接。"

echo ""
echo "=== 2. 检查 RTOS 相关模块 ==="
adb shell "lsmod 2>/dev/null | grep -E 'rtos|fast_image' || true"
if ! adb shell "lsmod 2>/dev/null" | grep -q soph_rtos_cmdqu; then
    echo "警告: soph_rtos_cmdqu 未加载，RTOS 通信不可用。"
fi

echo ""
echo "=== 3. 检查设备节点 ==="
adb shell "ls -l /dev/cvi-rtos-cmdqu 2>/dev/null" || echo "设备节点不存在。"

echo ""
echo "=== 4. 节点可访问性（root）==="
adb shell "su -c 'test -r /dev/cvi-rtos-cmdqu && echo 可读; test -w /dev/cvi-rtos-cmdqu && echo 可写'" 2>/dev/null || true

BIN="$1"
if [ -n "$BIN" ] && [ -f "$BIN" ]; then
    echo ""
    echo "=== 5. 推送并运行用户态测试程序 ==="
    adb push "$BIN" "$REMOTE_TEST"
    adb shell "su -c 'chmod 755 $REMOTE_TEST && $REMOTE_TEST'" 2>/dev/null || adb shell "chmod 755 $REMOTE_TEST && $REMOTE_TEST"
else
    echo ""
    echo "=== 5. 用户态测试（可选）==="
    if [ -f "$SCRIPT_DIR/rtos_cmdqu_test.c" ]; then
        echo "已存在 demo/rtos_cmdqu_test.c。"
        echo "交叉编译示例: riscv64-linux-gnu-gcc -static -o rtos_cmdqu_test rtos_cmdqu_test.c"
        echo "然后执行: $0 $SCRIPT_DIR/rtos_cmdqu_test"
    else
        echo "未找到 rtos_cmdqu_test 可执行文件，跳过用户态 ioctl 测试。"
    fi
fi

echo ""
echo "=== 测试结束 ==="
