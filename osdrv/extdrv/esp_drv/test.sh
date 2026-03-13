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
KO_NAME="esp_drv.ko"
DEVICE_KO_PATH="/root/esp_drv.ko"

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
run_remote "rmmod esp_drv 2>/dev/null || true"
run_remote "dmesg -c >/dev/null || true"
run_remote "insmod ${DEVICE_KO_PATH}"


echo "========== 步骤 5: 抓取完整 dmesg 并提取 test_vi 所用 ioctl/SDK id =========="
run_remote "dmesg"