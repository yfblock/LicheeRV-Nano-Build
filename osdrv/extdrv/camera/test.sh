#!/usr/bin/bash
# test-vi 工作流：编译 → 推送到设备（SSH/SCP 或 ADB）→ 卸载/挂载模块 → 运行 sensor_test → 查看 dmesg
# 使用前请先在 osdrv/extdrv/test-vi 中完成编辑（步骤 1），再执行本脚本。
#
# 传输方式（二选一）：
#   TRANSPORT=ssh  默认，依赖 ssh/scp 可连接设备（如 ssh lichee）
#   TRANSPORT=adb  使用 adb push / adb shell / adb pull，依赖 adb 已连接设备（adb connect 或 USB）
#
# 示例：
#   TRANSPORT=adb ./test.sh
#   SSH_TARGET=root@192.168.1.100 ./test.sh

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}"
KO_NAME="camera.ko"
DEVICE_KO_PATH="/root/camera.ko"
SENSOR_TEST="/root/sensor_test"
SAMPLE_YUV="/root/sample_0.yuv"

# 传输方式: ssh | adb
TRANSPORT="${TRANSPORT:-ssh}"
# SSH 目标主机（仅 TRANSPORT=ssh 时生效），如 lichee 或 root@192.168.1.100
SSH_TARGET="${SSH_TARGET:-lichee}"
# ADB 设备（仅 TRANSPORT=adb 时生效），多设备时可用 adb -s <serial>
ADB_DEVICE="${ADB_DEVICE:-}"

# -----------------------------------------------------------------------------
# 封装：在设备上执行命令
# -----------------------------------------------------------------------------
run_remote() {
	local cmd="$1"
	if [[ "${TRANSPORT}" == "adb" ]]; then
		if [[ -n "${ADB_DEVICE}" ]]; then
			adb -s "${ADB_DEVICE}" shell "${cmd}"
		else
			adb shell "${cmd}"
		fi
	else
		ssh "${SSH_TARGET}" "${cmd}"
	fi
}

# -----------------------------------------------------------------------------
# 封装：推送本地文件到设备
# -----------------------------------------------------------------------------
push_file() {
	local local_path="$1"
	local device_path="$2"
	if [[ "${TRANSPORT}" == "adb" ]]; then
		if [[ -n "${ADB_DEVICE}" ]]; then
			adb -s "${ADB_DEVICE}" push "${local_path}" "${device_path}"
		else
			adb push "${local_path}" "${device_path}"
		fi
	else
		scp "${local_path}" "${SSH_TARGET}:${device_path}"
	fi
}

# -----------------------------------------------------------------------------
# 封装：从设备拉取文件到本地（或把设备上命令输出写入本地文件）
# -----------------------------------------------------------------------------
get_remote_file() {
	local device_path="$1"
	local local_path="$2"
	if [[ "${TRANSPORT}" == "adb" ]]; then
		if [[ -n "${ADB_DEVICE}" ]]; then
			adb -s "${ADB_DEVICE}" pull "${device_path}" "${local_path}"
		else
			adb pull "${device_path}" "${local_path}"
		fi
	else
		scp "${SSH_TARGET}:${device_path}" "${local_path}"
	fi
}

# 将设备上命令输出保存到本地文件（用于 dmesg 等）
run_remote_to_file() {
	local cmd="$1"
	local local_path="$2"
	if [[ "${TRANSPORT}" == "adb" ]]; then
		if [[ -n "${ADB_DEVICE}" ]]; then
			adb -s "${ADB_DEVICE}" shell "${cmd}" > "${local_path}"
		else
			adb shell "${cmd}" > "${local_path}"
		fi
	else
		ssh "${SSH_TARGET}" "${cmd}" > "${local_path}"
	fi
}

# -----------------------------------------------------------------------------
# 主流程
# -----------------------------------------------------------------------------
echo "========== 传输方式: ${TRANSPORT} =========="

echo "========== 步骤 2: 编译 =========="
cd "${SCRIPT_DIR}"
./build.sh >/dev/null

if [[ ! -f "${BUILD_DIR}/${KO_NAME}" ]]; then
	echo "错误: 未生成 ${BUILD_DIR}/${KO_NAME}"
	exit 1
fi

echo "========== 步骤 3: 推送 ${KO_NAME} 到设备（${TRANSPORT}）=========="
push_file "${BUILD_DIR}/${KO_NAME}" "${DEVICE_KO_PATH}"

echo "========== 步骤 4: 卸载旧模块（若已挂载）并挂载新模块 =========="
run_remote "rmmod camera 2>/dev/null || true"
run_remote "dmesg -c >/dev/null || true"
run_remote "insmod ${DEVICE_KO_PATH}"

echo "========== 步骤 5: 清空 dmesg、删除已有 sample_0.yuv 并运行 sensor_test =========="
run_remote "rm -f ${SAMPLE_YUV}"
run_remote "cd /root && ${SENSOR_TEST} >/dev/null 2>&1"
if run_remote "test -f ${SAMPLE_YUV}" 2>/dev/null; then
	echo "成功: 已生成 ${SAMPLE_YUV}"
else
	echo "警告: 未检测到 ${SAMPLE_YUV}，请检查 sensor_test 输出"
fi

echo "========== 步骤 6: 抓取完整 dmesg 并提取 test_vi 所用 ioctl/SDK id =========="
mkdir -p "${BUILD_DIR}"
run_remote_to_file "dmesg" "${BUILD_DIR}/dmesg_capture.txt"
echo "已保存到 ${BUILD_DIR}/dmesg_capture.txt"

echo "========== 步骤 7: 查看 dmesg 尾部 =========="
# tail -n 100 "${BUILD_DIR}/dmesg_capture.txt"
cat "${BUILD_DIR}/dmesg_capture.txt"

echo "========== 步骤 8: 拉取 sample_0.yuv 并生成 preview.png =========="
rm -f "${BUILD_DIR}/sample_0.yuv" "${BUILD_DIR}/preview.png"
get_remote_file "${SAMPLE_YUV}" "${BUILD_DIR}/sample_0.yuv" 2>/dev/null || true
if [[ -f "${BUILD_DIR}/sample_0.yuv" ]]; then
	ffmpeg -y -f rawvideo -pixel_format nv21 -video_size 2560x1440 -i "${BUILD_DIR}/sample_0.yuv" \
		-vf "scale=800:-1" -frames:v 1 "${BUILD_DIR}/preview.png" 2>/dev/null || true
	if [[ -f "${BUILD_DIR}/preview.png" ]]; then
		imgcat "${BUILD_DIR}/preview.png" 2>/dev/null || echo "已生成 ${BUILD_DIR}/preview.png（无 imgcat 则请手动打开）"
	else
		echo "ffmpeg 未安装或生成失败，请检查 ${BUILD_DIR}/sample_0.yuv"
	fi
else
	echo "未拉取到 ${SAMPLE_YUV}，跳过预览"
fi
