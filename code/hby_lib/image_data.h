#pragma once

#include <stdint.h>
#include "vec.h"
#include "basic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Image_Data {
  int           width;
  int           height;
  uint8_t      *pixels;
  void         *data_storage;
} Image_Data;

Image_Data make_image_data(int width, int height);
Image_Data image_data_make_a(int width, int height, Allocator a);
void image_data_fill(Image_Data *image, float r, float g, float b, float a);
void image_data_fill_u(Image_Data *image, uint32_t color);
Vec4 image_data_get_pixel(Image_Data *image, int x, int y);
void destroy_image_data(Image_Data *image);
uint32_t image_data_get_pixel_u(const Image_Data *image, int x, int y);
void image_data_set_pixel(Image_Data *image, int x, int y, float r, float g, float b, float a);
void image_data_set_pixel_u(Image_Data *image, int x, int y, uint32_t color);
void image_data_copy(Image_Data *dst, Image_Data src);
Vec4 image_data_sample(Image_Data *image, Vec2 uv);
void image_data_save(Image_Data *image, const char *path);

#ifdef __cplusplus
} // extern "C"
#endif
