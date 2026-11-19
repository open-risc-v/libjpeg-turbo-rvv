# 上游接入：libjpeg-turbo（CMake 原生）→ <build>/pkg/prefix，探针链接 libjpeg 静态库。
# 保留 WITH_SIMD=ON：x86 SIMD 是上游参照实现，关掉它 A/B 就失去意义。

set(PKG_ROOT "${CMAKE_BINARY_DIR}/pkg")
if(NOT DEFINED PKG_PREFIX)
  set(PKG_PREFIX "${PKG_ROOT}/prefix")
  add_custom_target(pkg ALL
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pkg-build.sh" "${SRC_DIR}" "${PKG_ROOT}" "${RVV_MARCH}" "$<IF:$<BOOL:${RVV_KERNELS}>,1,0>"
  COMMENT "构建上游 libjpeg-turbo（${SRC_DIR}）"
  VERBATIM)
  set(HAVE_PKG_TARGET TRUE)
endif()

add_executable(jpeg_harness test/pkg_jpeg.c)
if(HAVE_PKG_TARGET)
  add_dependencies(jpeg_harness pkg)
endif()
target_include_directories(jpeg_harness PRIVATE "${PKG_PREFIX}/include")
target_link_directories(jpeg_harness PRIVATE "${PKG_PREFIX}/lib")
target_link_libraries(jpeg_harness PRIVATE jpeg m)
target_compile_options(jpeg_harness PRIVATE ${RVV_FLAGS})

add_test(NAME jpeg_harness COMMAND jpeg_harness)
