#include "image_data.h"
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

Image_Data make_image_data(int width, int height) {
  Image_Data ret = {0};
  int pixel_size = 4;
  int data_size_in_bytes = width * height * pixel_size;

  if (width <= 0 || height <= 0) {
    return ret;
  }

  ret.width = width;
  ret.height = height;
  ret.pixels = (uint8_t *)calloc(1, width * height * pixel_size);
  memset(ret.pixels, 0, data_size_in_bytes);

  return ret;
}

void image_data_fill_u(Image_Data *image, uint32_t color) {
  int y = 0, x = 0;
  for (y = 0; y < image->height; y++) {
    for (x = 0; x < image->width; x++) {
      image_data_set_pixel_u(image, x, y, color);
    }
  }
}

void image_data_fill(Image_Data *image, float r, float g, float b, float a) {
  int x = 0, y = 0;
  for (y = 0; y < image->height; y++) {
    for (x = 0; x < image->width; x++) {
      image_data_set_pixel(image, x, y, r, g, b, a);
    }
  }
}

uint32_t image_data_get_pixel_u(const Image_Data *image, int x, int y) {
  uint8_t *p = 0;
  uint32_t ret = 0;

  x = x < 0 ? 0 : x;
  x = x >= image->width  ? image->width-1  : x;
  y = y < 0 ? 0 : y;
  y = y >= image->height ? image->height-1 : y;

  p = &image->pixels[(y * image->width + x) * 4];
  memcpy(&ret, p, sizeof(ret));
  return ret;
}

Vec4 image_data_get_pixel(Image_Data *image, int x, int y) {
  return uint32_to_vec4( image_data_get_pixel_u(image, x, y) );
}

void destroy_image_data(Image_Data *image) {
  if (image->data_storage)
    free(image->data_storage);
  else if (image->pixels)
    free(image->pixels);
  ZERO_MEMORY(*image);
}

void image_data_set_pixel(Image_Data *image, int x, int y, float r, float g, float b, float a) {
  uint8_t *p = NULL;
  if (!(x >= 0 && x < image->width) || !(y >= 0 && y < image->height))
    return;

  p = &image->pixels[(y * image->width + x) * 4];

  p[0] = (uint8_t)(clamp01(r) * 255.f);
  p[1] = (uint8_t)(clamp01(g) * 255.f);
  p[2] = (uint8_t)(clamp01(b) * 255.f);
  p[3] = (uint8_t)(clamp01(a) * 255.f);
}

Vec4 image_data_sample(Image_Data *image, Vec2 uv) {
  int x = uv.x * (float)image->width;
  int y = uv.y * (float)image->height;
  if (x < 0 || x >= image->width || y < 0 || y >= image->height)
    return vec4(0,0,0,1);
  if (x < 0) x = 0;
  if (x >= image->width) x = image->width-1;
  if (y < 0) y = 0;
  if (y >= image->height) y = image->height-1;
  return image_data_get_pixel(image, x, y);
}

void image_data_set_pixel_u(Image_Data *image, int x, int y, uint32_t color) {
  Vec4 c = uint32_to_vec4(color);
  image_data_set_pixel(image, x, y, c.r, c.g, c.b, c.a);
}

void image_data_copy(Image_Data *dst, Image_Data src) {
  if (dst->width != src.width || dst->height != src.height) {
    fprintf(stderr, "[IMAGE_DATA] Couldn't copy image data of different sizes.");
    return;
  }
  memcpy(dst->pixels, src.pixels, src.width*src.height * sizeof(uint32_t));
}

void image_data_save(Image_Data *image, const char *path) {
  if (!stbi_write_png(path, image->width, image->height, 4, image->pixels, image->width * 4)) {
    fprintf(stderr, "[IMAGE_DATA] Failed to save to '%s'\n", path);
  } else {
    printf("[IMAGE_DATA] Saved to '%s'\n", path);
  }
}


#ifdef __cplusplus
} // extern "C"
#endif
