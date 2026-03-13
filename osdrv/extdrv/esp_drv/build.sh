#!/usr/bin/bash
# 使用 SoC 工具链编译 camera.ko（必须包含 sg200x-vi Rust，模块链接使用 ld.lld）
source ../../../build/envsetup_soc.sh >/dev/null 2>&1
olddefconfig >/dev/null 2>&1
make