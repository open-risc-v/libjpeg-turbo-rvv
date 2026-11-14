/* libjpeg-turbo 吞吐基准：压缩 + 解压一张 1024x768 RGB 图，取最优轮次。
   A（RVV 后端）/ B（纯 C）同 -march，真机上交替跑。 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <jpeglib.h>

#define W 1024
#define H 768

static double now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

int main(int argc, char **argv)
{
	int reps = argc > 1 ? atoi(argv[1]) : 5;
	unsigned char *rgb = malloc(W * H * 3);
	unsigned char *jpg = malloc(W * H * 3);
	if (!rgb || !jpg) return 2;
	for (int i = 0; i < W * H * 3; i++) rgb[i] = (unsigned char)((i * 2654435761u) >> 9);

	double best_c = 1e30, best_d = 1e30;
	for (int r = 0; r < reps; r++) {
		struct jpeg_compress_struct ci;
		struct jpeg_error_mgr je;
		ci.err = jpeg_std_error(&je);
		jpeg_create_compress(&ci);
		unsigned char *p = jpg;
		unsigned long sz = (unsigned long)(W * H * 3);
		jpeg_mem_dest(&ci, &p, &sz);
		ci.image_width = W; ci.image_height = H; ci.input_components = 3;
		ci.in_color_space = JCS_RGB;
		jpeg_set_defaults(&ci);
		jpeg_set_quality(&ci, 90, TRUE);
		double t0 = now_ns();
		jpeg_start_compress(&ci, TRUE);
		while (ci.next_scanline < ci.image_height) {
			JSAMPLE *row = (JSAMPLE *)(rgb + (size_t)ci.next_scanline * W * 3);
			jpeg_write_scanlines(&ci, &row, 1);   /* JSAMPARRAY 是"行指针的数组"，不是行首地址 */
		}
		jpeg_finish_compress(&ci);
		double dt = now_ns() - t0;
		if (dt < best_c) best_c = dt;
		if (r == 0) printf("jpeg_bytes=%lu\n", (unsigned long)sz);
		unsigned long out_sz = sz;
		jpeg_destroy_compress(&ci);

		struct jpeg_decompress_struct di;
		struct jpeg_error_mgr de;
		di.err = jpeg_std_error(&de);
		jpeg_create_decompress(&di);
		jpeg_mem_src(&di, jpg, out_sz);
		jpeg_read_header(&di, TRUE);
		di.out_color_space = JCS_RGB;
		t0 = now_ns();
		jpeg_start_decompress(&di);
		while (di.output_scanline < di.output_height) {
			JSAMPLE *row = (JSAMPLE *)(rgb + (size_t)di.output_scanline * W * 3);
			jpeg_read_scanlines(&di, &row, 1);
		}
		jpeg_finish_decompress(&di);
		double dt2 = now_ns() - t0;
		if (dt2 < best_d) best_d = dt2;
		jpeg_destroy_decompress(&di);
	}
	printf("jpeg %dx%d compress_ms=%.2f decompress_ms=%.2f\n",
	       W, H, best_c / 1e6, best_d / 1e6);
	free(rgb); free(jpg);
	return 0;
}
