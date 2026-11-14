#!/usr/bin/env bash
# libjpeg-turbo G3：上游 simd/riscv64/ 有完整 RVV 后端（16 个内核：IDCT/FDCT/色彩/采样/量化）。
# A = WITH_SIMD=ON @ rv64gcv，B = WITH_SIMD=OFF @ 同一 march，唯一变量是 SIMD 后端。
set -euo pipefail
cd "$(dirname "$0")/.."
IMAGE=${IMAGE:-rvv-cross:2}
MARCH=${MARCH:-rv64gcv}
OUT=build/rvv-k

docker run --rm -v "$PWD":/work -w /work "${IMAGE}" bash -lc "
set -e
rm -rf ${OUT}; mkdir -p ${OUT}
for side in on off; do
  opt='-DWITH_SIMD=ON'
  if [ \$side = off ]; then opt='-DWITH_SIMD=OFF'; fi
  cmake -S baseline -B ${OUT}/\$side/bld -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/work/${OUT}/\$side/prefix \
    -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DWITH_TURBOJPEG=OFF \$opt \
    -DCMAKE_TOOLCHAIN_FILE=/work/cmake/toolchain-riscv64-linux.cmake \
    -DCMAKE_C_FLAGS='-march=${MARCH}' > ${OUT}/\$side-cfg.log 2>&1 \
      || { tail -25 ${OUT}/\$side-cfg.log; exit 1; }
  cmake --build ${OUT}/\$side/bld -j > ${OUT}/\$side-build.log 2>&1 \
      || { tail -25 ${OUT}/\$side-build.log; exit 1; }
  cmake --install ${OUT}/\$side/bld > /dev/null

  cmake -S . -B ${OUT}/\$side/harness -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/work/cmake/toolchain-riscv64-linux.cmake \
    -DCMAKE_C_FLAGS='-march=${MARCH}' -DPKG_PREFIX=/work/${OUT}/\$side/prefix > /dev/null 2>&1
  cmake --build ${OUT}/\$side/harness -j > ${OUT}/\$side-harness.log 2>&1 \
      || { tail -15 ${OUT}/\$side-harness.log; exit 1; }
  qemu-riscv64 -L /usr/riscv64-linux-gnu ${OUT}/\$side/harness/jpeg_harness \
    ${OUT}/\$side/out.jpg > ${OUT}/\$side/hash.txt

  riscv64-linux-gnu-gcc -O3 -march=${MARCH} -static -I ${OUT}/\$side/prefix/include \
    test/bench_jpeg.c ${OUT}/\$side/prefix/lib/libjpeg.a -o ${OUT}/\$side/bench 2>/dev/null || true
done

echo '--- A：WITH_SIMD=ON（RVV 后端）---'; cat ${OUT}/on/hash.txt
echo '--- B：WITH_SIMD=OFF（纯 C）---'; cat ${OUT}/off/hash.txt
if cmp -s ${OUT}/on/hash.txt ${OUT}/off/hash.txt; then
  echo '== 功能一致'
else
  echo '== 功能不一致（若只有码流差、像素一致，需按"像素门禁"重新判定）'
fi
"
