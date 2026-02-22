#!/bin/bash
# 通过 ADB 在 LicheeRV 板上测试 hello_drv.ko
# 用法: ./run_adb_test.sh
# 前置: 连接 LicheeRV 并启用 ADB，执行 make all

set +e
# insmod 失败时 adb 可能返回非零，继续执行以获取 dmesg
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
KO="${SCRIPT_DIR}/build/hello_drv.ko"
REMOTE_KO="/data/local/tmp/hello_drv.ko"

echo "=== 1. 检查 ADB 设备 ==="
if ! adb devices | grep -q 'device$'; then
    echo "错误: 未检测到 ADB 设备，请连接 LicheeRV 并启用 ADB。"
    exit 1
fi

echo "=== 2. 检查模块文件 ==="
if [ ! -f "$KO" ]; then
    echo "错误: 找不到 hello_drv.ko，请先执行 make all"
    exit 1
fi

echo "=== 3. 推送到设备 ==="
for i in 1 2 3; do
    if adb push "$KO" "$REMOTE_KO" 2>/dev/null; then
        break
    fi
    echo "推送失败，重试 $i/3..."
    sleep 2
done
if ! adb shell "test -f $REMOTE_KO" 2>/dev/null; then
    echo "警告: 推送可能失败，将尝试使用设备上已有文件"
fi

echo "=== 4. 加载模块 (需 root) ==="
echo "--- 提高 printk 级别并卸载旧模块 ---"
adb shell "su -c 'echo 7 > /proc/sys/kernel/printk; rmmod hello_drv 2>/dev/null; true'" 2>/dev/null || true

sleep 4 

echo ""
echo "--- 加载前 dmesg ---"
adb shell "su -c 'dmesg | tail -3'" 2>/dev/null || adb shell "dmesg | tail -3"

echo ""
echo "--- 执行 insmod ---"
adb shell "su -c 'insmod $REMOTE_KO 2>&1; echo EXIT:\$?'" 2>/dev/null | tee /tmp/insmod_out.txt || true
if grep -q 'EXIT:0' /tmp/insmod_out.txt 2>/dev/null; then
    echo "insmod 成功!"
else
    echo "insmod 失败"
fi

echo ""
echo "--- 加载后 dmesg ---"
adb shell "su -c 'dmesg | tail -20'" 2>/dev/null || adb shell "dmesg | tail -20"

echo ""
echo "--- 尝试卸载 ---"
adb shell "su -c 'rmmod hello_drv'" 2>/dev/null || true

echo ""
echo "--- 查看 hello_drv 相关日志 ---"
adb shell "su -c 'dmesg | grep -E \"hello|rust|INFO|DEBUG|Goodbye\"'" 2>/dev/null || adb shell "dmesg | grep -E \"hello|rust|INFO|DEBUG|Goodbye\"" 2>/dev/null || echo "(无匹配)"

echo ""
echo "=== 测试结束 ==="
