#pragma once

#include "image_data.h"

#ifdef __cplusplus
extern "C" {
#endif

char *load_file(const char *path, iptr *size);

typedef struct Gfx_Texture Texture;
typedef struct Gfx_Shader Shader;
typedef struct Sprite Sprite;
typedef struct Font Font;
typedef struct Audio_Source Audio_Source;
typedef struct Audio_Decoder Audio_Decoder;
typedef struct Slot_Data Slot_Data;
typedef struct Mesh Mesh;

bool loader_file_exists(const char *path);

Sprite *load_sprite(const char *path);
Sprite *load_sprite_from_memory(const void *data, int size);
Texture *load_texture(const char *path);
Texture *load_texture_from_memory(const void *data, int size);
Font *load_image_font(const char *path, const char *characters, int spacing);
Font *load_image_font_with_manifest(const char *path, int spacing);
Font *load_ttf_font(const char *path, int size);
Font *load_ttf_font_from_memory(const void *data, int data_size, int font_size);
Image_Data load_image_data(const char *path);

Shader *load_shader_ps_only(const char *ps_path);
Shader *load_shader_full(const char *vs_path, const char *ps_path);

void unload_texture(Texture *texture);
void unload_sprite(Sprite *sprite);

#ifdef __cplusplus
} // extern "C"
#endif
