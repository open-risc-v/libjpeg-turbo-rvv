#!/usr/bin/env bash
# libjpeg-turbo：CMake 上游，静态库 + x86 SIMD（nasm 已就位）。
# 关掉 turbojpeg 与共享库：A/B 只比较 core codec 路径，产物更少、构建更快。
set -euo pipefail
SRC=$1
OUT=$2
MARCH=${3:-rv64gcv}
KERN=${4:-0}
EXTRA=""
if [ "$KERN" = "1" ]; then EXTRA="-march=${MARCH} -DRVV_KERNELS=1"; fi
rm -rf "${OUT}"
cmake -S "${SRC}" -B "${OUT}/bld" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${OUT}/prefix" \
  -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DWITH_TURBOJPEG=OFF -DWITH_SIMD=ON -DCMAKE_C_FLAGS="$EXTRA" >/dev/null
cmake --build "${OUT}/bld" -j >/dev/null
cmake --install "${OUT}/bld" >/dev/null
echo "== 上游 libjpeg-turbo 构建完成：${OUT}/prefix"
