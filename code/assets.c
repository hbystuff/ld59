#include "assets.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#define ASSETS_MANIFEST_PATH "data/manifest.txt"

typedef struct Asset {
  char    *path;
  void    *data;
  int      size;
  Texture *cached_texture;
  Sprite  *cached_sprite;
} Asset;

typedef enum Assets_State {
  ASSETS_STATE_IDLE,
  ASSETS_STATE_FETCHING_MANIFEST,
  ASSETS_STATE_FETCHING_FILES,
  ASSETS_STATE_DONE,
  ASSETS_STATE_FAILED,
} Assets_State;

static struct {
  Assets_State state;
  Asset       *items;
  int          loaded_count;
  int          total_count;
  char        *err;
} assets_;

static void assets_on_file_ok_(void *arg, void *data, int size) {
  Asset *a = (Asset *)arg;
  a->data = malloc(size);
  memcpy(a->data, data, size);
  a->size = size;
  assets_.loaded_count++;
  printf("Loaded %s. %d Bytes\n", a->path, a->size);
  if (assets_.loaded_count >= assets_.total_count) {
    assets_.state = ASSETS_STATE_DONE;
  }
}

static void assets_on_file_err_(void *arg) {
  Asset *a = (Asset *)arg;
  fprintf(stderr, "[assets] failed to load '%s'\n", a->path);
  assets_.state = ASSETS_STATE_FAILED;
  assets_.err = a->path;
}

static void assets_parse_manifest_(const char *text, int size) {
  const char *p = text;
  const char *end = text + size;
  while (p < end) {
    const char *line_start = p;
    while (p < end && *p != '\n' && *p != '\r') p++;
    int len = (int)(p - line_start);
    while (len > 0 && (line_start[len-1] == ' ' || line_start[len-1] == '\t')) len--;
    while (len > 0 && (*line_start == ' ' || *line_start == '\t')) { line_start++; len--; }
    if (len > 0 && line_start[0] != '#') {
      Asset a = {0};
      a.path = (char *)malloc(len + 1);
      memcpy(a.path, line_start, len);
      a.path[len] = 0;
      arr_add(assets_.items, a);
    }
    while (p < end && (*p == '\n' || *p == '\r')) p++;
  }
  assets_.total_count = (int)arr_size(assets_.items);
}

#ifdef __EMSCRIPTEN__
static void assets_on_manifest_ok_(void *arg, void *data, int size) {
  assets_parse_manifest_((const char *)data, size);
  if (assets_.total_count == 0) {
    assets_.state = ASSETS_STATE_DONE;
    return;
  }
  assets_.state = ASSETS_STATE_FETCHING_FILES;
  for (int i = 0; i < assets_.total_count; i++) {
    Asset *a = &assets_.items[i];
    emscripten_async_wget_data(a->path, a, assets_on_file_ok_, assets_on_file_err_);
  }
}

static void assets_on_manifest_err_(void *arg) {
  fprintf(stderr, "[assets] failed to load manifest\n");
  assets_.state = ASSETS_STATE_FAILED;
  assets_.err = "manifest";
}
#endif

void assets_setup(void) {
  assets_.state = ASSETS_STATE_FETCHING_MANIFEST;
#ifdef __EMSCRIPTEN__
  emscripten_async_wget_data(ASSETS_MANIFEST_PATH, NULL, assets_on_manifest_ok_, assets_on_manifest_err_);
#else
  iptr size = 0;
  char *text = load_file(ASSETS_MANIFEST_PATH, &size);
  if (!text) { assets_.state = ASSETS_STATE_FAILED; assets_.err = "manifest"; return; }
  assets_parse_manifest_(text, (int)size);
  free(text);
  assets_.state = ASSETS_STATE_FETCHING_FILES;
  for (int i = 0; i < assets_.total_count; i++) {
    Asset *a = &assets_.items[i];
    iptr fsize = 0;
    char *fdata = load_file(a->path, &fsize);
    if (!fdata) { assets_.state = ASSETS_STATE_FAILED; assets_.err = a->path; return; }
    a->data = fdata;
    a->size = (int)fsize;
    assets_.loaded_count++;
  }
  assets_.state = ASSETS_STATE_DONE;
#endif
}

bool  assets_is_done(void)   { return assets_.state == ASSETS_STATE_DONE; }
bool  assets_is_failed(void) { return assets_.state == ASSETS_STATE_FAILED; }

float assets_progress(void) {
  if (assets_.total_count <= 0) return 0.f;
  return (float)assets_.loaded_count / (float)assets_.total_count;
}

static Asset *assets_find_(const char *path) {
  for (int i = 0; i < (int)arr_size(assets_.items); i++) {
    if (0 == strcmp(assets_.items[i].path, path)) return &assets_.items[i];
  }
  return NULL;
}

const char *assets_get_file(const char *path, int *out_size) {
  Asset *a = assets_find_(path);
  if (!a || !a->data) { if (out_size) *out_size = 0; return NULL; }
  if (out_size) *out_size = a->size;
  return (const char *)a->data;
}

Texture *assets_get_texture(const char *path) {
  Asset *a = assets_find_(path);
  if (!a || !a->data) return NULL;
  if (!a->cached_texture) a->cached_texture = load_texture_from_memory(a->data, a->size);
  return a->cached_texture;
}

Sprite *assets_get_sprite(const char *path) {
  Asset *a = assets_find_(path);
  if (!a || !a->data) return NULL;
  if (!a->cached_sprite) a->cached_sprite = load_sprite_from_memory(a->data, a->size);
  return a->cached_sprite;
}

Font *assets_get_ttf_font(const char *path, int font_size) {
  Asset *a = assets_find_(path);
  if (!a || !a->data) return NULL;
  return load_ttf_font_from_memory(a->data, a->size, font_size);
}

Audio_Source assets_get_audio_source(const char *path) {
  Audio_Source null_src = {0};
  Asset *a = assets_find_(path);
  if (!a || !a->data) return null_src;
  return aud_get_source_from_memory(a->path, a->data, a->size);
}
