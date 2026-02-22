#!/bin/sh
# 在 LicheeRV 板上以 root 运行: ./run_test.sh
# 用于测试 hello_drv.ko 加载并收集 dmesg 调试信息

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
KO="${SCRIPT_DIR}/hello_drv.ko"

echo "=== hello_drv 加载测试 ==="
echo "模块: $KO"
echo "内核: $(uname -r)"
echo "架构: $(uname -m)"
echo ""

if [ ! -f "$KO" ]; then
    echo "错误: 找不到 hello_drv.ko，请先 make all"
    exit 1
fi

echo "--- 加载前 dmesg ---"
dmesg | tail -3
echo ""

echo "--- 执行 insmod ---"
if insmod "$KO" 2>&1; then
    echo "insmod 成功!"
    echo ""
    echo "--- 加载后 dmesg ---"
    dmesg | tail -15
    echo ""
    echo "--- 卸载 ---"
    rmmod hello_drv 2>&1 || true
    echo "--- 卸载后 dmesg ---"
    dmesg | tail -5
else
    echo "insmod 失败!"
    echo ""
    echo "--- 失败后 dmesg（关键调试信息）---"
    dmesg | tail -20
fi
