#!/usr/bin/env bash
# 真实包用例：libjpeg-turbo 编码→解码回环，比较 JPEG 字节与解码像素哈希。
set -euo pipefail
: "${STAGE:=run}" "${SIDE:=A}" "${BIN:=build/native}" "${DATA:=build/ab-data}" "${OUT:=build/ab-data/out}"
mkdir -p "$DATA" "$OUT"
case "$STAGE" in
  gen) ;;
  run)
    [ -x "$BIN/jpeg_harness" ] || { echo "缺少 $BIN/jpeg_harness" >&2; exit 1; }
    "$BIN/jpeg_harness" "$OUT/out.jpg" > "$OUT/hash.txt"
    size=$(wc -c < "$OUT/out.jpg" | tr -d ' ')
    # 128x96 q85 的 JPEG 通常 2–8KB；太小说明编码器没真正干活
    [ "$size" -gt 1024 ] || { echo "JPEG 过小：$size" >&2; exit 1; }
    ;;
  *) echo "未知 STAGE=$STAGE" >&2; exit 2 ;;
esac
