#pragma once

void        assets_setup(void);
bool        assets_is_done(void);
bool        assets_is_failed(void);
float       assets_progress(void);

const char  *assets_get_file(const char *path, int *out_size);
Texture     *assets_get_texture(const char *path);
Sprite      *assets_get_sprite(const char *path);
Font        *assets_get_ttf_font(const char *path, int font_size);
Audio_Source assets_get_audio_source(const char *path);
