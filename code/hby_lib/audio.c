#include "audio.h"
#include "basic.h"
#include "loader.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.c"    // Enables Vorbis decoding.

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#undef STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.c"

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "app.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct Audio_Group {
  uint8_t group;
  float volume;
  float pitch;
} Audio_Group;

typedef struct Audio_Source_Info {
  ma_sound sound;
  Audio_Source handle;
  Audio_Decoder decoder;
  int num_frames;
  ma_decoder ma_decoder_;
  Vec3 pos;
  float volume;
  float pitch;
  uint8_t group;
  uint8_t orphaned : 1;
  uint8_t paused   : 1;
  uint8_t fadeout  : 1;
  uint8_t positioned  : 1;
} Audio_Source_Info;

// ma_decoder turns out cannot be shared
// between different sources since seeking
// for every source for every frame causes
// internal perf problems.
typedef struct Audio_Decoder_Info {
  Audio_Decoder handle;
  int ref_count;
  char *path;
  void *data;
  size_t data_size;
} Audio_Decoder_Info;

typedef struct Audio_Splitter_Info {
  Audio_Splitter handle;
  ma_splitter_node splitter;
} Audio_Splitter_Info;

typedef struct Source_Info_Bucket {
  Audio_Source_Info sources[128];
  struct Source_Info_Bucket *next;
} Source_Info_Bucket;

typedef struct Audio_State {
  ma_context context;
  ma_engine engine;
  ma_device mic_device;
  struct Audio_Decoder_Info *decoders;

  uint32_t max_source_id;
  uint32_t max_decoder_id;
  uint32_t max_splitter_id;

  struct Source_Info_Bucket *first_bucket;
  int num_sources;
  int max_num_sources;

  bool pause_everything;

  struct Audio_Splitter_Info *splitters;

  Audio_Group *groups;
  int num_groups;

  bool context_ready;
  bool mic_ready;
  volatile float mic_level;
  volatile double mic_last_activity_time;

  Aud_File_Callback *file_callback;
} Audio_State;

static Audio_State aud_state_;

enum {
  AUD_SOURCE,
  AUD_SPLITTER,
};

enum {
  AUD_MIC_SAMPLE_RATE_ = 16000,
  AUD_MIC_CHANNELS_ = 1,
};

static const float AUD_MIC_ACTIVITY_THRESHOLD_ = 0.02f;
static const double AUD_MIC_ACTIVITY_WINDOW_ = 0.12;

static bool aud_get_mic_device_info_(int index, ma_device_info *out_info) {
  ma_device_info *capture_infos = NULL;
  ma_uint32 capture_count = 0;
  ma_result result = MA_SUCCESS;

  if (!aud_state_.context_ready || !out_info || index < 0) {
    return false;
  }

  result = ma_context_get_devices(&aud_state_.context, NULL, NULL, &capture_infos, &capture_count);
  if (result != MA_SUCCESS || (ma_uint32)index >= capture_count) {
    return false;
  }

  *out_info = capture_infos[index];
  return true;
}

int aud_mic_device_count(void) {
  ma_device_info *capture_infos = NULL;
  ma_uint32 capture_count = 0;
  ma_result result = MA_SUCCESS;

  if (!aud_state_.context_ready) {
    return 0;
  }

  result = ma_context_get_devices(&aud_state_.context, NULL, NULL, &capture_infos, &capture_count);
  if (result != MA_SUCCESS) {
    return 0;
  }

  return (int)capture_count;
}

int aud_get_default_mic_device_index(void) {
  ma_device_info *capture_infos = NULL;
  ma_uint32 capture_count = 0;
  ma_result result = MA_SUCCESS;

  if (!aud_state_.context_ready) {
    return -1;
  }

  result = ma_context_get_devices(&aud_state_.context, NULL, NULL, &capture_infos, &capture_count);
  if (result != MA_SUCCESS) {
    return -1;
  }

  for (ma_uint32 i = 0; i < capture_count; i++) {
    if (capture_infos[i].isDefault) {
      return (int)i;
    }
  }

  return capture_count > 0 ? 0 : -1;
}

bool aud_get_mic_device(int index, Audio_Mic_Device *out_device) {
  ma_device_info info = {0};
  if (!out_device || !aud_get_mic_device_info_(index, &info)) {
    return false;
  }

  ZERO(*out_device);
  snprintf(out_device->name, sizeof(out_device->name), "%s", info.name);
  out_device->is_default = info.isDefault != 0;
  return true;
}

Audio_Node aud_source_as_node(Audio_Source src) {
  Audio_Node node = { src.value, AUD_SOURCE, };
  return node;
}
Audio_Node aud_splitter_as_node(Audio_Splitter splitter) {
  Audio_Node node = { splitter.value, AUD_SPLITTER, };
  return node;
}

static ma_node *aud_get_as_ma_node(Audio_Node node);
void aud_attach(Audio_Node from, int from_slot, Audio_Node to, int to_slot) {
  ma_node *from_ma = aud_get_as_ma_node(from);
  if (!from_ma)
    return;
  ma_node *to_ma = aud_get_as_ma_node(to);
  if (!to_ma)
    return;

  // TODO: Detach if one of them is missing.

  // FIXME:
  // This crashes right now on ARM64 because somehow, the alignment in
  // ma_node_output_bus -> pInputNode is incorrect despite the compiler
  // attributes annotations. This causes _InterlockedCompareExchange64 to
  // crash. Try new miniaudio that's not 3 years old, see if it's been fixed
  // now.
  ma_result result = MA_SUCCESS;
  if ((result = ma_node_attach_output_bus(from_ma, from_slot, to_ma, to_slot))) {
    printf("ERROR: Failed to attach nodes. Error code: %d\n", (int)result);
  }
}

static Audio_Group *aud_get_group_(uint8_t group) {
  if (aud_state_.num_groups <= group) {
    int new_num_groups = (group+1);
    aud_state_.groups =
      (Audio_Group *)realloc(aud_state_.groups, new_num_groups*sizeof(aud_state_.groups[0]));
    for (int i = aud_state_.num_groups; i < new_num_groups; i++) {
      Audio_Group *group_info = &aud_state_.groups[i];
      ZERO(*group_info);
      group_info->group = i;
      group_info->volume = 1;
      group_info->pitch = 1;
    }
    aud_state_.num_groups = group+1;
  }
  return &aud_state_.groups[group];
}

static void aud_mic_data_callback_(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
  float peak = 0;
  const float *samples = (const float *)pInput;
  ma_uint32 sample_count = frameCount * AUD_MIC_CHANNELS_;
  (void)pOutput;
  (void)pDevice;

  if (!samples) {
    aud_state_.mic_level = 0;
    return;
  }

  for (ma_uint32 i = 0; i < sample_count; i++) {
    float sample = fabsf(samples[i]);
    if (sample > peak) {
      peak = sample;
    }
  }

  aud_state_.mic_level = peak;
  if (peak >= AUD_MIC_ACTIVITY_THRESHOLD_) {
    aud_state_.mic_last_activity_time = app_time();
  }
}

static void aud_mic_setup_(int device_index) {
  ma_device_config mic_config = ma_device_config_init(ma_device_type_capture);
  ma_result result = MA_SUCCESS;
  ma_device_info device_info = {0};

  mic_config.capture.format = ma_format_f32;
  mic_config.capture.channels = AUD_MIC_CHANNELS_;
  mic_config.sampleRate = AUD_MIC_SAMPLE_RATE_;
  mic_config.dataCallback = aud_mic_data_callback_;

  if (device_index >= 0) {
    if (!aud_get_mic_device_info_(device_index, &device_info)) {
      fprintf(stderr, "Failed to find microphone device at index %d\n", device_index);
      return;
    }
    mic_config.capture.pDeviceID = &device_info.id;
  }

  result = ma_device_init(&aud_state_.context, &mic_config, &aud_state_.mic_device);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "Failed to initialize microphone capture. Error: %d\n", (int)result);
    return;
  }

  result = ma_device_start(&aud_state_.mic_device);
  if (result != MA_SUCCESS) {
    fprintf(stderr, "Failed to start microphone capture. Error: %d\n", (int)result);
    ma_device_uninit(&aud_state_.mic_device);
    return;
  }

  aud_state_.mic_ready = true;
}

void aud_set_group_volume(uint8_t group, float volume) {
  aud_get_group_(group)->volume = volume;
}

void aud_set_group_pitch(uint8_t group, float pitch) {
  aud_get_group_(group)->pitch = pitch;
}

static Vec3 aud_ma_vec3_to_my_vec3_(ma_vec3f v) {
  Vec3 ret = {v.x, v.y, v.z};
  return ret;
}

Vec3 aud_get_listener_pos(void) {
  return aud_ma_vec3_to_my_vec3_(ma_engine_listener_get_position(&aud_state_.engine, 0));
}

void aud_detach_output(Audio_Node from, int from_slot) {
  ma_node *from_ma = aud_get_as_ma_node(from);
  if (!from_ma)
    return;
  ma_result result = MA_SUCCESS;
  if ((result = ma_node_detach_output_bus(from_ma, from_slot))) {
    printf("ERROR: Failed to detach nodes. Error code: %d\n", (int)result);
  }
}

static Audio_Source_Info *source_at(Audio_State *state, int i) {
  int bound = 0;
  Source_Info_Bucket *bucket;
  if (i >= aud_state_.num_sources)
    return NULL;

  for (bucket = aud_state_.first_bucket;
    bucket; bucket = bucket->next)
  {
    bound += _countof(bucket->sources);
    if (i < bound) {
      int index = i - (bound - _countof(bucket->sources));
      return &bucket->sources[index];
    }
  }
  return NULL;
}

static void deallocate_source(Audio_State *state, Audio_Source_Info *info) {
  int bound = 0;
  Source_Info_Bucket *bucket;
  for (bucket = aud_state_.first_bucket;
    bucket; bucket = bucket->next)
  {
    bound += _countof(bucket->sources);
    ZERO(*info);
  }
}

static void aud_setup_source_info_(Audio_Source_Info *info) {
  ZERO(*info);
  info->pitch = 1;
  info->volume = 1;
  info->paused = true;
}

static Audio_Source_Info *allocate_source(Audio_State *state) {
  int i;
  if (!aud_state_.first_bucket) {
    aud_state_.first_bucket = NEW(Source_Info_Bucket);
    aud_state_.max_num_sources = _countof(aud_state_.first_bucket->sources);
  }

  for (i = 0; i < aud_state_.num_sources; i++) {
    Audio_Source_Info *info = source_at(&aud_state_, i);
    if (!info->handle.value) {
      aud_setup_source_info_(info);
      return info;
    }
  }

  if (aud_state_.num_sources >= aud_state_.max_num_sources) {
    Source_Info_Bucket *last_bucket = aud_state_.first_bucket;
    while (last_bucket->next)
      last_bucket = last_bucket->next;
    last_bucket->next = NEW(Source_Info_Bucket);
    aud_state_.max_num_sources += _countof(aud_state_.first_bucket->sources);
  }

  {
    const int i = aud_state_.num_sources++;
    Audio_Source_Info *ret = source_at(&aud_state_, i);
    aud_setup_source_info_(ret);
    return ret;
  }
}

void aud_set_master_volume(float volume) {
  ma_engine_set_volume(&aud_state_.engine, volume);
}

void aud_set_master_pause(bool pause_all) {
  aud_state_.pause_everything = pause_all;
  for (int i = 0; i < aud_state_.num_sources; i++) {
    Audio_Source_Info *info = source_at(&aud_state_, i);
    if (!info->paused) {
      if (aud_state_.pause_everything) {
        ma_sound_stop(&info->sound);
      }
      else {
        ma_sound_start(&info->sound);
      }
    }
  }
}

enum {
  LISTENER_0 = 0,
  LISTENER_COUNT_,
};

static void free_callback(void *ptr, void *userdata) {
  free(ptr);
}
static void *realloc_callback(void *ptr, size_t new_size, void *userdata) {
  return realloc(ptr, new_size);
}
static void *malloc_callback(size_t new_size, void *userdata) {
  return malloc(new_size);
}

static void aud_experiment_(void);
static void aud_setup_with_config_(Audio_Config conf);

void aud_setup(void) {
  Audio_Config conf = {0};
  conf.mic_device_index = -1;
  aud_setup_with_config_(conf);
}

void aud_setup_p(Audio_Config *conf) {
  Audio_Config def = {0};
  def.mic_device_index = -1;
  if (!conf)
    conf = &def;
  aud_setup_with_config_(*conf);
}

static void aud_setup_with_config_(Audio_Config conf) {
  aud_state_.file_callback = conf.file_callback;

  int status;
  ma_result context_status = MA_SUCCESS;
  ma_allocation_callbacks allocationCallbacks = {0};
  ma_engine_config engine_conf = {0};

  allocationCallbacks.onFree = free_callback;
  allocationCallbacks.onRealloc = realloc_callback;
  allocationCallbacks.onMalloc = malloc_callback;

  conf.mic_device_index = conf.mic_device_index < -1 ? -1 : conf.mic_device_index;

  context_status = ma_context_init(NULL, 0, NULL, &aud_state_.context);
  if (context_status != MA_SUCCESS) {
    fprintf(stderr, "Failed to initialize audio context. Error: %d\n", (int)context_status);
    return;
  }
  aud_state_.context_ready = true;

  engine_conf = ma_engine_config_init();
  engine_conf.allocationCallbacks = allocationCallbacks;
  engine_conf.listenerCount = LISTENER_COUNT_;
  engine_conf.pContext = &aud_state_.context;

  status = ma_engine_init(&engine_conf, &aud_state_.engine);
  if (MA_SUCCESS == status) {
    ma_engine_listener_set_world_up(&aud_state_.engine, LISTENER_0, 0, 1, 0);
    ma_engine_set_volume(&aud_state_.engine, 1);
  } else {
    fprintf(stderr, "Failed to initialize audio system. Error: %d\n", status);
    ma_context_uninit(&aud_state_.context);
    aud_state_.context_ready = false;
    return;
  }

  if (conf.enable_mic_capture) {
    aud_mic_setup_(conf.mic_device_index);
  }

  aud_experiment_();
}

char *aud_load_file_internal_(const char *path, iptr *out_size) {
  if (aud_state_.file_callback) {
    return aud_state_.file_callback(path, out_size);
  }
  return load_file(path, out_size);
}

Audio_Decoder_Info *aud_add_decoder_info(const char *path) {
  int i = 0;
  Audio_Decoder_Info *ret = NULL;

  {
    iptr size = 0;
    char *data = aud_load_file_internal_(path, &size);
    if (data) {
      ret = arr_add_new(aud_state_.decoders);
      if (ret) {
        ma_decoder_config conf;
        ZERO(conf);
        ret->path = str_copy(path);
        ret->ref_count++;
        ret->handle.value = ++aud_state_.max_decoder_id;
        ret->data = data;
        ret->data_size = size;
      }
    }
  }

  return ret;
}

Audio_Decoder_Info *aud_get_or_add_decoder_info(const char *path) {
  int i;
  for (i = 0; i < arr_len(aud_state_.decoders); i++) {
    Audio_Decoder_Info *it = &aud_state_.decoders[i];
    if (str_eq(it->path, path)) {
      it->ref_count++;
      return it;
    }
  }
  return aud_add_decoder_info(path);
}

Audio_Decoder_Info *aud_get_or_add_decoder_info_from_memory(const char *key, const void *data, int size) {
  for (int i = 0; i < arr_len(aud_state_.decoders); i++) {
    Audio_Decoder_Info *it = &aud_state_.decoders[i];
    if (str_eq(it->path, key)) {
      it->ref_count++;
      return it;
    }
  }
  Audio_Decoder_Info *ret = arr_add_new(aud_state_.decoders);
  if (ret) {
    char *copy = (char *)malloc(size);
    memcpy(copy, data, size);
    ret->path = str_copy(key);
    ret->ref_count++;
    ret->handle.value = ++aud_state_.max_decoder_id;
    ret->data = copy;
    ret->data_size = size;
  }
  return ret;
}

static Audio_Decoder_Info *aud_find_decoder_info_(Audio_Decoder dec) {
  int i;
  for (i = 0; i < arr_len(aud_state_.decoders); i++) {
    Audio_Decoder_Info *it = &aud_state_.decoders[i];
    if (it->handle.value == dec.value) {
      return it;
    }
  }
  return NULL;
}

static void release_decoder_info(Audio_Decoder_Info *info) {
  assert(info->ref_count > 0);
  if (info->ref_count > 0) {
    info->ref_count--;
    if (info->ref_count == 0) {
      str_free(info->path);
      free(info->data);
      arr_remove_unordered(aud_state_.decoders, info - aud_state_.decoders);
    }
  }
}

static void release_decoder(Audio_Decoder dec) {
  Audio_Decoder_Info *info = NULL;
  int i = 0;

  for (i = 0; i < arr_len(aud_state_.decoders); i++) {
    Audio_Decoder_Info *it = &aud_state_.decoders[i];
    if (it->handle.value == dec.value) {
      info = it;
      break;
    }
  }

  if (info) {
    release_decoder_info(info);
  }
}

static Audio_Source_Info *aud_add_source_info_from_decoder_info(Audio_Decoder_Info *decoder_info) {
  Audio_Source_Info *ret = NULL;
  Audio_Source_Info *source_info =  NULL;
  ma_result result = MA_SUCCESS;
  if (!decoder_info)
    return ret;

  source_info = allocate_source(&aud_state_);
  if (!source_info)
    return ret;

  if (source_info) {
    source_info->handle.value = ++aud_state_.max_source_id;
    source_info->decoder = decoder_info->handle;
    source_info->paused = 1;

    ma_decoder_config ma_decoder_config_ ={0};
    result = ma_decoder_init_memory(decoder_info->data, decoder_info->data_size, &ma_decoder_config_, &source_info->ma_decoder_);
    if (result == MA_SUCCESS) {
      ma_sound_set_volume(&source_info->sound, 1);
      ma_sound_config conf = ma_sound_config_init();
      conf.pDataSource = &source_info->ma_decoder_;
      conf.flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
      if (MA_SUCCESS == ma_sound_init_ex(&aud_state_.engine, &conf, &source_info->sound)) {
        ret = source_info;
        ma_sound_set_volume(&source_info->sound, 1);

        {
          ma_uint64 num_frames = 0;
          ma_decoder_get_length_in_pcm_frames(&source_info->ma_decoder_, &num_frames);

          // HACK. Miniaudio can't figure out the vorbis length for push mode. Figure it out once here.
          if (num_frames == 0) {
            int err = 0;
            stb_vorbis *vorbis = stb_vorbis_open_memory(
                (const unsigned char *)decoder_info->data, decoder_info->data_size, &err, NULL);
            if (vorbis) {
              num_frames = stb_vorbis_stream_length_in_samples(vorbis);
              stb_vorbis_close(vorbis);
            }
          }

          source_info->num_frames = (uint32_t)num_frames;
        }
      }
    }
  }

  if (!ret) {
    if (source_info) {
      deallocate_source(&aud_state_, source_info);
    }
    release_decoder_info(decoder_info);
  }

  return ret;
}

static Audio_Source_Info *aud_add_source_info_from_path(const char *path) {
  Audio_Source_Info *ret = NULL;
  Audio_Decoder_Info *decoder_info = aud_get_or_add_decoder_info(path);
  if (!decoder_info)
    return ret;

  return aud_add_source_info_from_decoder_info(decoder_info);
}


Audio_Source aud_new_source(void) {
  Audio_Source ret = {0};
  Audio_Source_Info *source_info = allocate_source(&aud_state_);
  source_info->handle.value = ++aud_state_.max_source_id;
  ma_sound_config conf = ma_sound_config_init();
  conf.flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
  if (MA_SUCCESS == ma_sound_init_ex(&aud_state_.engine, &conf, &source_info->sound)) {
    ret = source_info->handle;
    ma_sound_set_volume(&source_info->sound, 1);
  }
  else {
    deallocate_source(&aud_state_, source_info);
  }
  return ret;
}

Audio_Source aud_get_source_from_decoder(Audio_Decoder decoder) {
  Audio_Source ret = {0};
  Audio_Decoder_Info *decoder_info = aud_find_decoder_info_(decoder);
  Audio_Source_Info *info = aud_add_source_info_from_decoder_info(decoder_info);
  if (info) {
    decoder_info->ref_count++;
    ret = info->handle;
  }
  return ret;
}

Audio_Source aud_get_source(const char *path) {
  Audio_Source ret = {0};
  Audio_Source_Info *info = aud_add_source_info_from_path(path);
  if (info)
    ret = info->handle;
  return ret;
}

Audio_Decoder aud_get_decoder(const char *path) {
  Audio_Decoder_Info *info = aud_get_or_add_decoder_info(path);
  Audio_Decoder ret = {0};
  if (info) {
    ret = info->handle;
  }
  return ret;
}

Audio_Decoder aud_get_decoder_from_memory(const char *key, const void *data, int size) {
  Audio_Decoder ret = {0};
  Audio_Decoder_Info *info = aud_get_or_add_decoder_info_from_memory(key, data, size);
  if (info) ret = info->handle;
  return ret;
}

Audio_Source aud_get_source_from_memory(const char *key, const void *data, int size) {
  Audio_Source ret = {0};
  Audio_Decoder_Info *decoder_info = aud_get_or_add_decoder_info_from_memory(key, data, size);
  if (!decoder_info) return ret;
  Audio_Source_Info *info = aud_add_source_info_from_decoder_info(decoder_info);
  if (info) ret = info->handle;
  return ret;
}

static Audio_Source_Info *aud_find_source_info_(Audio_Source src) {
  if (!src.value)
    return NULL;
  for (int i = 0; i < aud_state_.num_sources; i++) {
    Audio_Source_Info *it = source_at(&aud_state_, i);
    if (it->handle.value && src.value == it->handle.value) {
      if (it->orphaned)
        return NULL;
      return it;
    }
  }
  return NULL;
}

static uint64_t aud_source_info_get_sample_rate_(Audio_Source_Info *info) {
  return info->ma_decoder_.outputSampleRate;
}
static float aud_source_info_get_length_in_seconds_(Audio_Source_Info *info) {
  return (float)((double)info->num_frames / (double)aud_source_info_get_sample_rate_(info));
}

void aud_set_group(Audio_Source src, uint8_t group) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    info->group = group;
  }
}

float aud_get_length_in_seconds(Audio_Source src) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    return aud_source_info_get_length_in_seconds_(info);
  }
  return 0;
}

float aud_get_progress(Audio_Source src) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    float length = aud_source_info_get_length_in_seconds_(info);
    ma_uint64 cur_frame = 0;
    ma_result result = ma_sound_get_cursor_in_pcm_frames(&info->sound, &cur_frame);
    if (result == MA_SUCCESS) {
      double cur_time = (double)cur_frame / (double)aud_source_info_get_sample_rate_(info);
      return cur_time / length;
    }
  }
  return 0;
}

void aud_set_progress(Audio_Source src, float progress) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    uint64_t progress_in_frame = (uint64_t)(progress * (double)info->num_frames);
    ma_sound_seek_to_pcm_frame(&info->sound, progress_in_frame);
  }
}

void aud_set_pitch(Audio_Source src, float pitch) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    info->pitch = pitch;
    ma_sound_set_pitch(&info->sound, pitch * aud_get_group_(info->group)->pitch);
  }
}

void aud_set_volume(Audio_Source src, float volume) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    info->volume = volume;
    ma_sound_set_volume(&info->sound, info->volume * aud_get_group_(info->group)->volume);
  }
}

float aud_get_volume(Audio_Source src) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    return info->volume;
  }
  return 0;
}

void aud_set_pan(Audio_Source src, float pan) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    ma_sound_set_spatialization_enabled(&info->sound, 0);
    ma_sound_set_pan(&info->sound, pan);
  }
}

void aud_unset_pos(Audio_Source src) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    ma_sound_set_spatialization_enabled(&info->sound, 0);
  }
}

void aud_set_pos_rel_impl(Audio_Source_Info *info, Vec3 pos) {
  ma_sound_set_spatialization_enabled(&info->sound, 1);
  ma_sound_set_positioning(&info->sound, ma_positioning_relative);
  ma_sound_set_position(&info->sound, pos.x, pos.y, pos.z);
}

void aud_set_pos_rel(Audio_Source src, Vec3 pos) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    aud_set_pos_rel_impl(info, pos);
  }
}

void aud_set_pos(Audio_Source src, Vec3 pos) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    ma_sound_set_spatialization_enabled(&info->sound, 1);
    ma_sound_set_positioning(&info->sound, ma_positioning_absolute);
    ma_sound_set_position(&info->sound, pos.x, pos.y, pos.z);
  }
}

Vec3 aud_get_pos(Audio_Source src) {
  Vec3 ret = {0};
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    ma_vec3f v = ma_sound_get_position(&info->sound);
    ret = vec3(v.x, v.y, v.z);
  }
  return ret;
}

bool aud_is_playing(Audio_Source src) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info && !info->orphaned) {
    return !info->paused && ma_sound_is_playing(&info->sound);
  }
  return true;
}

void aud_play(Audio_Source src) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info && !info->orphaned) {
    info->paused = 0;
    if (!aud_state_.pause_everything) {
      ma_sound_start(&info->sound);
    }
  }
}

bool aud_is_dead(Audio_Source src) {
  return !aud_find_source_info_(src);
}

void aud_free_source(Audio_Source src) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    info->orphaned = 1;
    ma_sound_stop(&info->sound);
  }
}

void aud_set_orphaned(Audio_Source src) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    info->orphaned = 1;
  }
}

void aud_fadeout(Audio_Source src) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    info->orphaned = 1;
    info->fadeout  = 1;
  }
}

Audio_Source_Info *aud_play_oneshot_impl_(const char *path, float volume) {
  Audio_Source_Info *info = aud_add_source_info_from_path(path);
  if (info) {
    info->orphaned = 1;
    info->paused = 0;
    info->volume = volume;
    ma_sound_set_volume(&info->sound, info->volume);
  }
  return info;
}

void aud_play_oneshot_positioned(const char *path, float volume, Vec3 pos) {
  Audio_Source_Info *info = aud_play_oneshot_impl_(path, volume);
  if (info) {
    info->positioned = true;
    info->pos = pos;
    ma_sound_set_spatialization_enabled(&info->sound, 1);
    ma_sound_set_positioning(&info->sound, ma_positioning_absolute);
    ma_sound_set_position(&info->sound, pos.x, pos.y, pos.z);
    ma_sound_start(&info->sound);
  }
}

void aud_play_oneshot_pan(const char *path, float volume, float pan) {
  Audio_Source_Info *info = aud_play_oneshot_impl_(path, volume);
  if (info) {
    ma_sound_set_pan(&info->sound, pan);
    ma_sound_start(&info->sound);
  }
}

void aud_play_oneshot(const char *path, float volume) {
  Audio_Source_Info *info = aud_play_oneshot_impl_(path, volume);
  if (info) {
    ma_sound_start(&info->sound);
  }
}

void aud_update(float dt) {
  aud_update_3d(vec3(0,0,0), vec3(0,0,1), dt);
}

void aud_update_3d(Vec3 listener_pos, Vec3 listener_forward, float dt) {
  int i = 0;
  // miniaudio uses right handed coordinates. Reverse the listener direction since there's
  // no easy way to change the config without me getting confused every time I upgrade the lib.
  listener_forward = neg3(listener_forward);

  ma_engine_listener_set_position(&aud_state_.engine,  LISTENER_0, listener_pos.x, listener_pos.y, listener_pos.z);
  ma_engine_listener_set_direction(&aud_state_.engine, LISTENER_0, listener_forward.x, listener_forward.y, listener_forward.z);

  for (i = 0; i < aud_state_.num_sources;) {
    bool remove = false;
    Audio_Source_Info *info = source_at(&aud_state_, i);
    if (!info->handle.value) {
      i++;
      continue;
    }

    if (info->fadeout) {
      info->volume -= dt / 0.5f;
      if (info->volume <= 0) {
        info->volume = 0;
        ma_sound_stop(&info->sound);
      }
    }

    Audio_Group *group = aud_get_group_(info->group);
    float volume = info->volume * group->volume;
    float pitch = info->pitch * group->pitch;

    ma_sound_set_volume(&info->sound, volume);
    ma_sound_set_pitch(&info->sound, pitch);

    if (info->orphaned) {
      if (info->volume == 0 || !ma_sound_is_playing(&info->sound))
      {
        remove = true;
      }
    }

    if (remove) {
      ma_sound_uninit(&info->sound);
      //my_data_source_uninit(&info->data_source);
      ma_decoder_uninit(&info->ma_decoder_);
      release_decoder(info->decoder);
      deallocate_source(&aud_state_, info);
    }
    else {
      i++;
    }
  }
}

static Audio_Splitter_Info *aud_find_splitter_info_(Audio_Splitter splitter) {
  for (int i = 0; i < arr_len(aud_state_.splitters); i++) {
    if (aud_state_.splitters[i].handle.value == splitter.value)
      return &aud_state_.splitters[i];
  }
  return NULL;
}

Audio_Splitter aud_new_splitter(int num_outputs) {
  Audio_Splitter ret = {0};
  Audio_Splitter_Info *info = arr_add_new(aud_state_.splitters);
  info->handle.value = ++aud_state_.max_splitter_id;
  ma_node_graph *node_graph = ma_engine_get_node_graph(&aud_state_.engine);

  ma_splitter_node_config splitter_node_config = ma_splitter_node_config_init(2);
  splitter_node_config.outputBusCount = num_outputs;
  ma_result result = MA_SUCCESS;
  if ((result = ma_splitter_node_init(node_graph, &splitter_node_config, NULL, &info->splitter))) {
    arr_remove_unordered(aud_state_.splitters, info - aud_state_.splitters);
    return ret;
  }
  ret = info->handle;
  return ret;
}

static void aud_experiment_(void) {
  return;

  ma_result status = MA_SUCCESS;
  ma_engine *eng = &aud_state_.engine;
  ma_node_graph *node_graph = ma_engine_get_node_graph(eng);

  static ma_sound sound_nodes[20] = {0};
  for (int i = 0; i < _countof(sound_nodes); i++) {
    ma_sound_config conf = ma_sound_config_init();
    conf.channelsIn = 2;
    //conf.flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
    if ((status = ma_sound_init_ex(eng, &conf, &sound_nodes[i]))) {
      printf("Create output sound failed. status: %d\n", status);
      return;
    }

    float pos[3] = {0.f, 40.f, -400.f * i};
    for (int j = 0; j < _countof(pos); j++) pos[j] *= 0.07;
    ma_sound_set_position(&sound_nodes[i], pos[0], pos[1], pos[2]);
  }


  static ma_splitter_node splitter_node = {0};
  {
    ma_splitter_node_config splitter_node_config = ma_splitter_node_config_init(2);
    splitter_node_config.outputBusCount = _countof(sound_nodes);
    if ((status = ma_splitter_node_init(node_graph, &splitter_node_config, NULL, &splitter_node))) {
      printf("Creating splitter failed. status: %d\n", status);
      return;
    }

    for (int i = 0; i < _countof(sound_nodes); i++) {
      if ((status = ma_node_attach_output_bus(&splitter_node, i, &sound_nodes[i], 0))) {
        printf("Connect split to output sound failed. status: %d\n", status);
        return;
      }
    }
  }

  static ma_sound sound_source = {0};
  {
    ma_sound_config conf = ma_sound_config_init();
    conf.pFilePath = "data/audio/music_box.ogg";
    conf.channelsIn = 2;
    conf.flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
    if ((status = ma_sound_init_ex(eng, &conf, &sound_source))) {
      printf("Create sound source failed. status: %d\n", status);
      return;
    }
    ma_sound_start(&sound_source);
    ma_node_attach_output_bus(&sound_source, 0, &splitter_node, 0);
  }
}

void aud_stop(Audio_Source src) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info && !info->orphaned) {
    ma_sound_stop(&info->sound);
    if (!info->paused) {
      ma_sound_seek_to_pcm_frame(&info->sound, 0);
    }
    info->paused = 1;
  }
}

void aud_pause(Audio_Source src) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info && !info->orphaned) {
    info->paused = 1;
    ma_sound_stop(&info->sound);
  }
}

void aud_set_looped(Audio_Source src, bool looped) {
  Audio_Source_Info *info = aud_find_source_info_(src);
  if (info) {
    ma_sound_set_looping(&info->sound, looped);
  }
}

bool aud_is_mic_active(void) {
  if (!aud_state_.mic_ready) {
    return false;
  }
  return app_time() - aud_state_.mic_last_activity_time <= AUD_MIC_ACTIVITY_WINDOW_;
}

float aud_get_mic_level(void) {
  if (!aud_state_.mic_ready) {
    return 0;
  }
  return aud_state_.mic_level;
}

static ma_node *aud_get_as_ma_node(Audio_Node node) {
  if (!node.value)
    return NULL;
  switch (node.type) {
    case AUD_SOURCE:
    {
      Audio_Source src = { node.value };
      Audio_Source_Info *info = aud_find_source_info_(src);
      if (info) {
        return &info->sound;
      }
    } break;
    case AUD_SPLITTER:
    {
      Audio_Splitter splitter = { node.value };
      Audio_Splitter_Info *info = aud_find_splitter_info_(splitter);
      if (info) {
        return &info->splitter;
      }
    } break;
  }
  return NULL;
}


#ifdef __cplusplus
} // extern "C"
#endif
