#include "json/json.h"
#include "sprite.h"
#include "image_data.h"
#include "basic.h"
#include "loader.h"
#include "gfx.h"
#include "util.h"
#include "data.h"
#include "app.h"
#include <assert.h>

#define CACHE_TEXD 0

#define STBI_ONLY_PNG
#include "stb/stb_image.h"

#include "json/json.c"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define LOADER_NEW(T) ((T *)calloc(1, sizeof(T)))


#ifdef __cplusplus
extern "C" {
#endif

typedef enum Asset_Type {
  ASSET_TYPE_TEXTURE,
  ASSET_TYPE_SPRITE,
  ASSET_TYPE_MESH,
} Asset_Type;

typedef struct Loaded_Asset {
  union {
    Sprite  *sprite;
    Texture *texture;
    Mesh    *mesh;
  };
  Asset_Type type;
} Loaded_Asset;

bool loader_file_exists(const char *path) {
  FILE* f = 0;
#ifdef _MSC_VER
  fopen_s(&f, path, "rb");
#else
  f = fopen(path, "rb");
#endif
  if (!f) {
    return false;
  }
  fclose(f);
  return true;
}

char *load_file(const char *path, iptr *size) {
  FILE* f = 0;
  size_t tell_size = 0;
  char *buffer = NULL;

#ifdef _MSC_VER
  fopen_s(&f, path, "rb");
#else
  f = fopen(path, "rb");
#endif
  if (!f) {
    return 0;
  }

  fseek(f, 0, SEEK_END);
  tell_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  buffer = (char *)malloc(tell_size+1);
  fread(buffer, 1, tell_size, f);
  buffer[tell_size] = 0;

  if (size)
    *size = (int)tell_size;

  fclose(f);
  return buffer;
}

Sprite *load_sprite_from_memory(const void *data, int size) {
  Sprite *sprite = NULL;
  json_value_s *json = NULL;

  json = json_parse(data, size);
  if (json) {

    struct json_object_s *json_obj = json_as_object(json);
    struct json_object_s *meta = json_as_object(json_get(json_obj, "meta"));
    struct json_value_s *tags_value = json_get(meta, "frameTags");
    struct json_array_s *tags = NULL;

    struct json_value_s *frames_value = json_get(json_obj, "frames");
    assert(frames_value->type == json_type_array && "Frames must be exported as an array, not a hashmap.");
    struct json_array_s *frames = json_as_array(frames_value);
    struct json_array_element_s *it = NULL;

    if (tags_value)
      tags = json_as_array(tags_value);

    sprite = LOADER_NEW(Sprite);

    for (it = frames->start;
        it; it = it->next)
    {
      struct json_object_s *f = json_as_object(it->value);
      struct json_object_s *frame = json_as_object(json_get(f, "frame"));
      struct json_object_s *source_size = json_as_object(json_get(f, "sourceSize"));
      float source_w = (float)json_as_number(json_get(source_size, "w"));
      float source_h = (float)json_as_number(json_get(source_size, "h"));
      float x = (float)json_as_number(json_get(frame, "x"));
      float y = (float)json_as_number(json_get(frame, "y"));
      int w = (int)json_as_number(json_get(frame, "w"));
      int h = (int)json_as_number(json_get(frame, "h"));
      float duration = (float)json_as_number(json_get(f, "duration")) * 0.001f;
      struct json_object_s *spriteSourceSize = json_as_object(json_get(f, "spriteSourceSize"));
      float ox = (float)json_as_number(json_get(spriteSourceSize, "x"));
      float oy = (float)json_as_number(json_get(spriteSourceSize, "y"));
      float ow = (float)json_as_number(json_get(spriteSourceSize, "w"));
      float oh = (float)json_as_number(json_get(spriteSourceSize, "h"));
      ox -= (w - ow)/2;
      oy -= (h - oh)/2;

      float quad[4] = {x,y, (float)w, (float)h};
      sprite_add_frame(sprite, quad, duration, (int)ox, (int)oy);
      sprite->width = (int)source_w;
      sprite->height = (int)source_h;
    }

    if (tags) {
      for (it = tags->start;
          it; it = it->next)
      {
        struct json_object_s *tag = json_as_object(it->value);

        const char *name = json_as_string(json_get(tag, "name"));
        int from = (int)json_as_number(json_get(tag, "from"));
        int to = (int)json_as_number(json_get(tag, "to"));

        sprite_add_anim(sprite, name, from, to);
      }
    }

    //sprite_add_anim(sprite, "<all-frames>", 0, sprite_get_num_frames(sprite)-1);
  }
  else {
    printf("Failed to parse sprite json.\n");
  }

  free(json);

  return sprite;
}

Sprite *load_sprite(const char *path) {
  Sprite *sprite = NULL;
  iptr size = 0;
  char *data = NULL;

  if (!str_ends_with(path, ".json"))
    return NULL;

  data = load_file(path, &size);
  if (!data) {
    printf("Cannot load '%s'.\n", path);
    return NULL;
  }

  sprite = load_sprite_from_memory(data, (int)size);
  free(data);
  return sprite;
}

enum {
  TEXTURE_DATA_VERSION_0 = 1,

  TEXTURE_DATA_VERSION_LATEST_PLUS_1,
  TEXTURE_DATA_VERSION_LATEST = TEXTURE_DATA_VERSION_LATEST_PLUS_1-1
};
#define TEXTURE_DATA_MAGIC "TEXD"

char *write_texture_data(Image_Data *img, int *size) {
  String_Builder sb_storage = {0};
  String_Builder *sb = &sb_storage;

  sb_write_magic(sb, TEXTURE_DATA_MAGIC);
  sb_write_uint32(sb, TEXTURE_DATA_VERSION_LATEST);
  sb_write_uint32(sb, img->width);
  sb_write_uint32(sb, img->height);

  sb_write(sb, img->pixels, img->width * img->height * sizeof(uint32_t));

  *size = sb->size;
  return sb_detach(sb);
}

static Image_Data load_image_data_impl(const char *path) {
  Image_Data ret = {0};
  iptr size = 0;
  char *data = NULL;
  int w = 0, h = 0, comps = 0;
  void *pixels = NULL;

  if (!str_ends_with(path, ".png"))
    return ret;

  data = load_file(path, &size);
  if (!data)
    return ret;

  pixels = stbi_load_from_memory((unsigned char *)data, size, &w, &h, &comps, 4);
  if (pixels) {
    ret.height = h;
    ret.width = w;
    ret.pixels = (uint8_t *)pixels;
  }
  free(data);

  return ret;
}

static bool load_texture_data(const char *path, Image_Data *img) {
  iptr size = 0;
  char *data = load_file(path, &size);
  uint32_t version = 0;
  Data_Reader dr_storage = make_data_reader(data, size);
  Data_Reader *dr = &dr_storage;
  uint32_t width = 0;
  uint32_t height = 0;
  int pixel_storage_size = 0;
  const char *pixels = NULL;

  ZERO(*img);

  if (!dr_read_magic(dr, TEXTURE_DATA_MAGIC))
    return read_error("[TEXTURE] Cannot read texture data. Magic mismatch.\n");

  if (!dr_read_uint32(dr, &version))
    read_error("[TEXTURE] Could not load version.\n");

  if (version > TEXTURE_DATA_VERSION_LATEST)
    return read_error("[TEXTURE] Version %u is greater than max supported version %u.\n", version, TEXTURE_DATA_VERSION_LATEST);

  if (!dr_read_uint32(dr, &width))
    read_error("[TEXTURE] Could not load width.\n");
  if (!dr_read_uint32(dr, &height))
    read_error("[TEXTURE] Could not load height.\n");

  pixel_storage_size = width * height * sizeof(uint32_t);
  if (!dr_read_data(dr, pixel_storage_size, &pixels))
    read_error("[TEXTURE] Could not read pixel data.\n");

  img->data_storage = data;
  img->pixels = (uint8_t *)pixels;
  img->width  = width;
  img->height = height;

  return true;
}

Image_Data load_image_data(const char *path) {
  Image_Data ret = {0};

#if CACHE_TEXD
  {
    uint64_t original_stamp = app_get_file_write_timestamp(path);
    if (!original_stamp)
      return ret;

    char baked_path[64];
    snprintf(baked_path, sizeof(baked_path), "%s.texd", path);
    uint64_t baked_stamp = app_get_file_write_timestamp(baked_path);
    if (baked_stamp < original_stamp) {
      Image_Data src = load_image_data_impl(path);
      if (!src.pixels)
        return ret;

      int size = 0;
      char *data = write_texture_data(&src, &size);
      if (data) {
        int size_written = app_write_file(baked_path, data, size);
        printf("%d bytes written to %s\n", size_written, baked_path);
        free(data);
      }

      ret = src;
    }
    else {
      Image_Data img = {0};
      if (load_texture_data(baked_path, &img)) {
        ret = img;
      }
    }
  }
#else
  ret = load_image_data_impl(path);
#endif

  return ret;
}

Texture *load_texture(const char *path) {
  Image_Data img = load_image_data(path);
  if (!img.pixels)
    return NULL;
  Texture *texture = gfx_new_texture(img.pixels, img.width, img.height);
  destroy_image_data(&img);
  return texture;
}

Texture *load_texture_from_memory(const void *data, int size) {
  int w = 0, h = 0, comps = 0;
  void *pixels = stbi_load_from_memory((const unsigned char *)data, size, &w, &h, &comps, 4);
  if (!pixels) return NULL;
  Texture *texture = gfx_new_texture(pixels, w, h);
  stbi_image_free(pixels);
  return texture;
}


Font *load_ttf_font_from_memory(const void *data, int size, int font_size) {
  Font *font = (Font *)calloc(1, sizeof(Font));
  if (!font_load_ttf(font, font_size, data, size)) {
    free(font);
    return NULL;
  }
  return font;
}

Font *load_ttf_font(const char *path, int font_size) {
  iptr size = 0;
  char *data = NULL;
  Font *font = NULL;

  if (!str_ends_with(path, ".ttf")) {
    printf("Cannot load TTF font '%s'. File must be .ttf\n", path);
    return NULL;
  }

  data = load_file(path, &size);
  if (!data)
    return NULL;

  font = load_ttf_font_from_memory(data, (int)size, font_size);
  free(data);
  return font;
}

Font *load_image_font_with_manifest(const char *path, int spacing) {
  const char *ext = ".png";
  if (!str_ends_with(path, ext))
    return NULL;

  Image_Data img = load_image_data(path);
  if (!img.pixels)
    return NULL;

  char manifest_file_name[128];
  snprintf(manifest_file_name, _countof(manifest_file_name), "%.*s.txt",
      (int)(str_len(path) - str_len(ext)), path);

  Font *ret = NULL;
  iptr manifest_size = 0;
  char *manifest = load_file(manifest_file_name, &manifest_size);
  if (manifest) {
    ret = LOADER_NEW(Font);
    font_load_image(ret, img.pixels, img.width, img.height, manifest, spacing);
    free(manifest);
  }

  destroy_image_data(&img);
  return ret;
}

Font *load_image_font(const char *path, const char *characters, int spacing) {
  Image_Data img = load_image_data(path);
  Font *font = NULL;
  if (!img.pixels)
    return NULL;

  font = LOADER_NEW(Font);
  font_load_image(font, img.pixels, img.width, img.height, characters, spacing);

  destroy_image_data(&img);
  return font;
}

Shader *load_shader_ps_only(const char *ps_path) {
  Shader *ret = NULL;
  iptr ps_size = 0;
  char *ps_data = NULL;

  //if (!str_ends_with(ps_path, ".glsl")) {
  //  printf("Shader must end with '.glsl'.\n");
  //  return NULL;
  //}

  ps_data = load_file(ps_path, &ps_size);
  if (ps_data) {
    ret = gfx_new_shader(ps_data, ps_size);
    free(ps_data);
  }

  return ret;
}

Shader *load_shader_full(const char *vs_path, const char *ps_path) {
  Shader *ret = NULL;
  iptr vs_size = 0;
  char *vs_data = NULL;

  if (!str_ends_with(vs_path, ".glsl")) {
    printf("Shader must end with '.glsl'.\n");
    return NULL;
  }
  if (!str_ends_with(ps_path, ".glsl")) {
    printf("Shader must end with '.glsl'.\n");
    return NULL;
  }

  vs_data = load_file(vs_path, &vs_size);
  if (vs_data) {
    iptr ps_size = 0;
    char *ps_data = load_file(ps_path, &ps_size);
    if (ps_data) {
      ret = gfx_new_shader_2(vs_data, vs_size, ps_data, ps_size);
      free(ps_data);
    }
    free(vs_data);
  }

  return ret;
}

void unload_sprite(Sprite *sprite) {
  sprite_destroy(sprite);
  free(sprite);
}

void unload_texture(Texture *texture) {
  gfx_free_texture(texture);
  ZERO(*texture);
}

#ifdef __cplusplus
} // extern "C"
#endif


