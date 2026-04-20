#pragma once

#include "vec.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUD_FILE_CALLBACK(NAME_) char *NAME_(const char *path, iptr *out_size)
typedef AUD_FILE_CALLBACK(Aud_File_Callback);

typedef struct Audio_Decoder { uint32_t value; } Audio_Decoder;
typedef struct Audio_Source  { uint32_t value; } Audio_Source;
typedef struct Audio_Splitter  { uint32_t value; } Audio_Splitter;
typedef struct Audio_Node { uint32_t value; uint32_t type; } Audio_Node;
typedef struct Audio_Mic_Device {
  char name[256];
  bool is_default;
} Audio_Mic_Device;
typedef struct Audio_Config {
  bool enable_mic_capture;
  int mic_device_index; // -1 uses the system default capture device.
  Aud_File_Callback *file_callback;
} Audio_Config;

static const Audio_Source  AUDIO_SOURCE_NULL;
static const Audio_Decoder AUDIO_DECODER_NULL;

void aud_setup(void);
void aud_setup_p(Audio_Config *conf);
int aud_mic_device_count(void);
int aud_get_default_mic_device_index(void);
bool aud_get_mic_device(int index, Audio_Mic_Device *out_device);
Audio_Decoder aud_get_decoder(const char *path);
Audio_Decoder aud_get_decoder_from_memory(const char *key, const void *data, int size);
Audio_Source aud_get_source(const char *path);
Audio_Source aud_get_source_from_memory(const char *key, const void *data, int size);
Audio_Source aud_get_source_from_decoder(Audio_Decoder decoder);
Audio_Source aud_new_source(void);

Audio_Node aud_source_as_node(Audio_Source src);
Audio_Node aud_splitter_as_node(Audio_Splitter src);
void aud_attach(Audio_Node from, int from_slot, Audio_Node to, int to_slot);
void aud_detach_output(Audio_Node from, int from_slot);

Audio_Splitter aud_new_splitter(int num_outputs);

void aud_set_group(Audio_Source src, uint8_t group);

void aud_set_group_volume(uint8_t group, float volume);
void aud_set_group_pitch(uint8_t group, float pitch);

bool aud_is_dead(Audio_Source src);
void aud_free_source(Audio_Source src);
void aud_free_decoder(Audio_Source src);
void aud_play_oneshot(const char *path, float volume);
void aud_play_oneshot_pan(const char *path, float volume, float pan);
void aud_play_oneshot_positioned(const char *path, float volume, Vec3 pos);
void aud_set_orphaned(Audio_Source src);
void aud_play(Audio_Source src);
void aud_set_looped(Audio_Source src, bool looped);
void aud_pause(Audio_Source src);
void aud_stop(Audio_Source src);
void aud_update(float dt);
void aud_update_3d(Vec3 listener_pos, Vec3 listener_forward, float dt);
void aud_set_pos(Audio_Source src, Vec3 pos);
void aud_set_pos_rel(Audio_Source src, Vec3 pos);
void aud_unset_pos(Audio_Source src);
Vec3 aud_get_pos(Audio_Source src);
float aud_get_volume(Audio_Source src);
void aud_set_pan(Audio_Source src, float pan);
void aud_set_volume(Audio_Source src, float volume);
void aud_set_master_volume(float volume);
void aud_set_master_pause(bool pause_all);
bool aud_is_playing(Audio_Source src);
void aud_fadeout(Audio_Source src);
Vec3 aud_get_listener_pos(void);
bool aud_is_mic_active(void);
float aud_get_mic_level(void);

float aud_get_length_in_seconds(Audio_Source src);
float aud_get_progress(Audio_Source src);
void aud_set_progress(Audio_Source src, float progress);

void aud_set_pitch(Audio_Source src, float pitch);

#ifdef __cplusplus
} // extern "C"
#endif
