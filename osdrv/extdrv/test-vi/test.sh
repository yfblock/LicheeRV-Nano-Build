#!/usr/bin/bash
# test-vi 工作流：编译 → 通过 SSH/SCP 推送到设备 → 卸载/挂载模块 → 运行 sensor_test → 查看 dmesg
# test-vi 仅保留与 sensor_test 兼容的必要逻辑（无 ioctl/sdk 白名单，避免漏掉初始化路径）。
# 使用前请先在 osdrv/extdrv/test-vi 中完成编辑（步骤 1），再执行本脚本。
# 依赖：ssh 可连接 lichee（如 ssh lichee），scp 可向该主机传文件。

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
KO_NAME="test_vi.ko"
DEVICE_KO_PATH="/root/test_vi.ko"
SENSOR_TEST="/root/sensor_test"
SAMPLE_YUV="/root/sample_0.yuv"

# SSH 目标主机，如 lichee 或 root@lichee（需在 ~/.ssh/config 或 /etc/hosts 中可解析）
SSH_TARGET="${SSH_TARGET:-lichee}"

echo "========== 步骤 2: 编译 =========="
cd "${SCRIPT_DIR}"
source ../../../build/envsetup_soc.sh
olddefconfig

# ./build.sh
if [[ ! -f "${BUILD_DIR}/${KO_NAME}" ]]; then
	echo "错误: 未生成 ${BUILD_DIR}/${KO_NAME}"
	exit 1
fi

echo ""
echo "========== 步骤 3: 通过 scp 推送 test_vi.ko 到 ${SSH_TARGET} =========="
scp "${BUILD_DIR}/${KO_NAME}" "${SSH_TARGET}:${DEVICE_KO_PATH}"

echo ""
echo "========== 步骤 4: 卸载旧模块（若已挂载）并挂载新模块 =========="
ssh "${SSH_TARGET}" "rmmod test_vi 2>/dev/null || true"
ssh "${SSH_TARGET}" "insmod ${DEVICE_KO_PATH}"

echo ""
echo "========== 步骤 5: 清空 dmesg、删除已有 sample_0.yuv 并运行 sensor_test =========="
ssh "${SSH_TARGET}" "dmesg -c 2>/dev/null || true"
ssh "${SSH_TARGET}" "rm -f ${SAMPLE_YUV}"
ssh "${SSH_TARGET}" "cd /root && ${SENSOR_TEST}"
if ssh "${SSH_TARGET}" "test -f ${SAMPLE_YUV}" 2>/dev/null; then
	echo "成功: 已生成 ${SAMPLE_YUV}"
else
	echo "警告: 未检测到 ${SAMPLE_YUV}，请检查 sensor_test 输出"
fi

echo ""
echo "========== 步骤 6: 抓取完整 dmesg 并提取 test_vi 所用 ioctl/SDK id =========="
mkdir -p "${BUILD_DIR}"
ssh "${SSH_TARGET}" "dmesg" > "${BUILD_DIR}/dmesg_capture.txt"
echo "已保存到 ${BUILD_DIR}/dmesg_capture.txt"
# 提取本轮出现的 vi_ioctl id= 与 vi_sdk_ctrl id=
grep -E "vi_ioctl cmd=.* id=[0-9]+|vi_sdk_ctrl id=[0-9]+" "${BUILD_DIR}/dmesg_capture.txt" | sed -nE 's/.* id=([0-9]+).*/\1/p' | sort -n -u > "${BUILD_DIR}/used_ids.txt" 2>/dev/null || true
if [[ -s "${BUILD_DIR}/used_ids.txt" ]]; then
	echo "本轮 sensor_test 用到的 id（可作白名单）: $(tr '\n' ' ' < "${BUILD_DIR}/used_ids.txt")"
fi

echo ""
echo "========== 步骤 7: 查看 dmesg 尾部 =========="
tail -n 80 "${BUILD_DIR}/dmesg_capture.txt"

echo "========== 步骤 8: 查看 preview.png =========="
rm -f build/sample_0.yuv build/preview.png
adb pull /root/sample_0.yuv build/
ffmpeg -f rawvideo -pixel_format nv21 -video_size 2560x1440 -i build/sample_0.yuv \
  -vf "scale=800:-1" -frames:v 1 build/preview.png
imgcat build/preview.png
