// libjpeg-turbo A/B 探针：编码 → 解码回环，哈希 JPEG 字节与解码像素。
// 走 jpeg_mem_dest/jpeg_mem_src，不落中间文件，保证同输入同配置下字节确定。
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jpeglib.h"

#define W 128
#define H 96

static uint64_t fnv1a(const void *p, size_t n)
{
    const uint8_t *b = p;
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 1099511628211ULL; }
    return h;
}

int main(int argc, char **argv)
{
    const char *outj = argc > 1 ? argv[1] : NULL;
    unsigned char *rgb = malloc((size_t)W * H * 3);
    if (!rgb) return 2;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            unsigned char *p = rgb + ((size_t)y * W + x) * 3;
            p[0] = (unsigned char)((x * 2 + y) & 0xff);
            p[1] = (unsigned char)((y * 3 + x / 2) & 0xff);
            p[2] = (unsigned char)((x ^ y) & 0xff);
        }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    unsigned char *jpg = NULL;
    unsigned long jlen = 0;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &jpg, &jlen);
    cinfo.image_width = W;
    cinfo.image_height = H;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 85, TRUE);
    cinfo.dct_method = JDCT_ISLOW;      /* 固定算法，排除平台默认差异 */
    jpeg_start_compress(&cinfo, TRUE);
    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPROW row = rgb + (size_t)cinfo.next_scanline * W * 3;
        jpeg_write_scanlines(&cinfo, &row, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    if (!jpg || jlen == 0) return 3;

    struct jpeg_decompress_struct dinfo;
    struct jpeg_error_mgr derr;
    unsigned char *dec = NULL;
    dinfo.err = jpeg_std_error(&derr);
    jpeg_create_decompress(&dinfo);
    jpeg_mem_src(&dinfo, jpg, jlen);
    jpeg_read_header(&dinfo, TRUE);
    dinfo.out_color_space = JCS_RGB;
    dinfo.dct_method = JDCT_ISLOW;
    jpeg_start_decompress(&dinfo);
    size_t dl = (size_t)dinfo.output_width * dinfo.output_height * dinfo.output_components;
    dec = malloc(dl);
    if (!dec) return 4;
    while (dinfo.output_scanline < dinfo.output_height) {
        JSAMPROW row = dec + (size_t)dinfo.output_scanline * dinfo.output_width * 3;
        jpeg_read_scanlines(&dinfo, &row, 1);
    }
    jpeg_finish_decompress(&dinfo);
    jpeg_destroy_decompress(&dinfo);

    printf("size=%dx%d jpeg=%lu fnv_jpeg=%016llx fnv_px=%016llx\n", W, H, jlen,
           (unsigned long long)fnv1a(jpg, jlen), (unsigned long long)fnv1a(dec, dl));

    if (outj) {
        FILE *f = fopen(outj, "wb");
        if (!f) return 5;
        fwrite(jpg, 1, jlen, f);
        fclose(f);
    }
    free(rgb); free(dec); free(jpg);
    return 0;
}
