#include "util.h"
#include "app.h"

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

float util_get_wave_period_phase_from_timestamp(double t, float period, float phase) {
  return (float)sin((t - phase) * 3.1415926 * 2 / period) * 0.5f + 0.5f;
}
float util_get_wave_period_from_timestamp(double t, float period) {
  return util_get_wave_period_phase_from_timestamp(t, period, 0);
}
float util_get_wave_from_timestamp(double t) {
  return util_get_wave_period_from_timestamp(t, 1);
}
float get_wave(void) {
  return get_wave_period(1);
}
float get_wave_period(float period) {
  return get_wave_period_phase(period, 0);
}
float get_wave_period_phase(float period, float phase) {
  return util_get_wave_period_phase_from_timestamp(app_last_time(), period, phase);
}

uint32_t rgba_to_uint32(float r, float g, float b, float a) {
  return
    ((uint32_t)(clamp01(a) * 255.0f) << 24) |
    ((uint32_t)(clamp01(b) * 255.0f) << 16) |
    ((uint32_t)(clamp01(g) * 255.0f) << 8 ) |
    ((uint32_t)(clamp01(r) * 255.0f) << 0 );
}

double clamp01d(double x) {
  if (x < 0) return 0;
  if (x > 1) return 1;
  return x;
}

double lerpd(double low, double high, double t) {
  t = clamp01d(t);
  return low + t * (high - low);
}

float lerp(float low, float high, float t) {
  t = clamp01(t);
  return low + t * (high - low);
}
Vec2 lerp2(Vec2 low, Vec2 high, float t) {
  return vec2(
    lerp(low.x, high.x, t),
    lerp(low.y, high.y, t)
  );
}
Vec3 lerp3(Vec3 low, Vec3 high, float t) {
  return vec3(
    lerp(low.x, high.x, t),
    lerp(low.y, high.y, t),
    lerp(low.z, high.z, t)
  );
}
Vec4 lerp4(Vec4 low, Vec4 high, float t) {
  return vec4(
    lerp(low.x, high.x, t),
    lerp(low.y, high.y, t),
    lerp(low.z, high.z, t),
    lerp(low.w, high.w, t)
  );
}

float ilerp(float low, float high, float t) {
  float ret = (t - low) / (high - low);
  return clamp01(ret);
}
float clamp(float x, float low, float high) {
  return x < low ? low : (x > high ? high : x);
}
float clamp01(float x) {
  return clamp(x, 0, 1);
}
bool move_to(float *x, float tx, float delta) {
  delta = fabsf(delta);
  if (*x < tx) {
    *x += delta;
    if (*x >= tx) {
      *x = tx;
      return true;
    }
  }
  else if (*x > tx) {
    *x -= delta;
    if (*x <= tx) {
      *x = tx;
      return true;
    }
  }
  return false;
}
bool move_to_d(double *x, double tx, double delta) {
  delta = fabs(delta);
  if (*x < tx) {
    *x += delta;
    if (*x >= tx) {
      *x = tx;
      return true;
    }
  }
  else if (*x > tx) {
    *x -= delta;
    if (*x <= tx) {
      *x = tx;
      return true;
    }
  }
  return false;
}

bool move_to2(Vec2 *v, Vec2 t, float delta) {
  bool ret = 1;
  Vec2 d = norm2(sub2(t, *v));
  int i;
  for (i = 0; i < 2; i++) {
    if (!move_to(&v->p[i], t.p[i], fabsf(d.p[i]) * delta))
      ret = 0;
  }
  return ret;
}

bool move_to3(Vec3 *v, Vec3 t, float delta) {
  bool ret = true;
  Vec3 d = norm3(sub3(t, *v));
  int i;
  for (i = 0; i < 3; i++) {
    if (!move_to(&v->p[i], t.p[i], fabsf(d.p[i]) * delta))
      ret = 0;
  }
  return ret;
}

bool intersect_ray_with_circle(Vec2 center, float radius, Vec2 begin, Vec2 end, Vec2 *out_point) {
  Vec2 d,f;
  float a,b,c, t1, t2, discriminant;

  if (eq2(begin, end))
    return false;

  d = sub2(end, begin);
  f = sub2(begin, center);
  a = dot2(d, d);
  b = 2 * dot2(f, d);
  c = dot2(f, f) - radius * radius;

  discriminant = b*b-4*a*c;
  if (discriminant < 0)
    return false;

  discriminant = sqrtf(discriminant);
  t1 = (-b - discriminant)/(2*a);
  t2 = (-b + discriminant)/(2*a);
  if (t1 >= 0 && t1 <= 1) {
    if (out_point) {
      *out_point = add2(begin, smul2(t1, d));
    }
    return true;
  }

  if (t2 >= 0 && t2 <= 1) {
    if (out_point) {
      *out_point = add2(begin, smul2(t2, d));
    }
    return true;
  }
  return false;
}

INLINE static unsigned char intersect_ray_with_box_impl(float ox, float dx, float min_x, float max_x, float *tmin, float *tmax, float *out_t) {
  float inv_d = 1.f / dx;

  float t0 = (min_x - ox) * inv_d;
  float t1 = (max_x - ox) * inv_d;

  if (inv_d < 0) {
    float tmp = t1;
    t1 = t0;
    t0 = tmp;
  }
 
  if (t0 > *tmin)
    *tmin = t0;

  if (t1 < *tmax) 
    *tmax = t1;
  
  if (*tmax <= *tmin)
    return 0;

  // Origin is inside the box.
  if (*tmin < 0) {
    return 0;
  }

  *out_t = *tmin;
  return 1;
}

bool intersect_ray_with_box(Vec4 box, Vec2 begin, Vec2 end, Vec2 *out_point) {
  Vec2 top_left = box.xy;
  Vec2 bottom_right = add2(box.xy, box.zw);

  bool succ = true;
  float tmin = 0;

  Vec2 dir = sub2(end, begin);
  float tmax = size2(dir);

  float tx, ty;

  if (tmax == 0)
    return false;

  dir = norm2(dir);

  tx = 0;
  ty = 0;

  succ = intersect_ray_with_box_impl(begin.x, dir.x, top_left.x, bottom_right.x, &tmin, &tmax, &tx);
  if (!succ)
    return false;

  succ = intersect_ray_with_box_impl(begin.y, dir.y, top_left.y, bottom_right.y, &tmin, &tmax, &ty);
  if (!succ)
    return false;

  if (out_point)
    *out_point = add2(begin, mul2(sub2(end, begin), vec2(tx, ty)));

  return true;
}


float ease_cos_hill(float t) {
  return cosf((t-0.5f) * TAU_F) * 0.5f + 0.5f;
}

float ease_cos(float t) {
  return cosf((t-1) * TAU_F*0.5f) * 0.5f + 0.5f;
}

float ease_in_expo(float t) {
  return t == 0 ? 0 : pow(2.f, 10.f * t - 10.f);
}

float ease_cos_pulse(float t, int num_pulses) {
  return -cos((t * num_pulses) * TAU_F) * 0.5 + 0.5;
}

float pow2(float x) {
  return x * x;
}

uint32_t multiply_color_uint32(uint32_t color0, uint32_t color1) {
  return vec4_to_uint32(mul4(uint32_to_vec4(color0), uint32_to_vec4(color1)));
}

Vec3 rgb_to_hsv(float r, float g, float b) {
  float c_max, c_min, hue, delta, value, deg60, saturation;

  r = CLAMP01(r);
  g = CLAMP01(g);
  b = CLAMP01(b);

  c_max = util_max3f(r, g, b);
  c_min = util_min3f(r, g, b);
  hue = 0;
  delta = c_max - c_min;
  value = c_max;

  deg60 = 60.f / 360.f;
  if (c_max == r) {
    hue = (g-b)/delta * deg60;
  }
  else if (c_max == g) {
    hue = ((b-r)/delta + 2) * deg60;
  }
  else if (c_max == b) {
    hue = ((r-g)/delta + 4) * deg60;
  }
  if (hue < 0) {
    hue = 1.f + hue;
  }
  else if (hue > 1) {
    hue = hue - 1.f;
  }
  else if (util_isnan(hue)) {
    hue = 0;
  }

  saturation = 0;
  if (c_max > 0) {
    saturation = delta / c_max;
  }

  return vec3(hue, saturation, value);
}

Vec3 rgb_vec3_to_hsv(Vec3 rgb) {
  return rgb_to_hsv(rgb.r, rgb.g, rgb.b);
}

Vec3 hsv_vec3_to_rgb(Vec3 hsv) {
  return hsv_to_rgb(hsv.x, hsv.y, hsv.z);
}

Vec3 hsv_to_rgb(float h, float s, float v) {
  Vec3 ret = {0};
  float hh, ff, p, q, t;
  long i;

  h = CLAMP01(h);
  s = CLAMP01(s);
  v = CLAMP01(v);

  hh = h * 360.f;
  if (hh >= 360.f) hh = 0.0;
  hh /= 60.0;

  i = (long)hh;
  ff = hh - i;
  p = v * (1.f - s);
  q = v * (1.f - (s * ff));
  t = v * (1.f - (s * (1.f - ff)));

  switch(i) {
  case 0:
    ret.r = v;
    ret.g = t;
    ret.b = p;
    break;
  case 1:
    ret.r = q;
    ret.g = v;
    ret.b = p;
    break;
  case 2:
    ret.r = p;
    ret.g = v;
    ret.b = t;
    break;

  case 3:
    ret.r = p;
    ret.g = q;
    ret.b = v;
    break;
  case 4:
    ret.r = t;
    ret.g = p;
    ret.b = v;
    break;
  case 5:
  default:
    ret.r = v;
    ret.g = p;
    ret.b = q;
    break;
  }

  return ret;
}

Vec3 inverse_barycentric_2d(Vec2 a2, Vec2 b2, Vec2 c2, Vec2 pos) {
  Vec3d a = vec3d(a2.x,a2.y,1.0);
  Vec3d b = vec3d(b2.x,b2.y,1.0);
  Vec3d c = vec3d(c2.x,c2.y,1.0);
  Vec3d p = vec3d(pos.x,pos.y,1.0);

  Vec3d bxc = cross3d(b,c);
  Vec3d axb = cross3d(a,b);
  Vec3d cxa = cross3d(c,a);
  double denom = dot3d(a,bxc);

  double alpha = dot3d(p,bxc);
  double beta  = dot3d(p,cxa);
  double gamma = dot3d(p,axb);

  alpha /= denom;
  beta  /= denom;
  gamma /= denom;

  return vec3((float)alpha,(float)beta,(float)gamma);
}

Euler euler_angle_from_dir(Vec3 dir) {
  Euler ret = {0};
  Vec3 dir_no_y = {0};

  float cos_pitch = clamp( dot3(dir, vec3(0, 1, 0)), -1, 1);
  ret.pitch = to_deg(acosf(cos_pitch)) - 90;

  dir_no_y = vec3(dir.x, 0, dir.z);
  if (length3(dir_no_y)) {
    float tan0, tan1;
    dir_no_y = norm3(dir_no_y);
    tan0 = atan2f(0, 1);
    tan1 = atan2f(dir.x, dir.z);
    ret.yaw = to_deg(tan1 - tan0);
  }

  return ret;
}

Quaternion util_rotate_to_face(Vec3 origin, Vec3 target) {
  Vec3 diff = sub3(target, origin);
  diff.y = 0;
  if (length3(diff) > EPSILON) {
    diff = norm3(diff);
    return quaternion_y(-to_deg(atan2(diff.z, diff.x)) - 90);
  }
  return quaternion_identity();
}

Mat4 get_standard_transform(Vec3 pos, Quaternion rot, float scale) {
  Mat4 ret = mat4_identity();
  mat4_translate_post(&ret, pos.x, pos.y, pos.z);
  ret = mat4_mul(ret, quaternion_to_mat4(rot));
  mat4_scale_post(&ret, scale, scale, scale);
  return ret;
}

Mat4 get_standard_transform_inverse(Vec3 pos, Quaternion rot, float scale) {
  Mat4 ret = mat4_identity();
  mat4_scale_post(&ret, 1.f/scale, 1.f/scale, 1.f/scale);
  ret = mat4_mul(ret, quaternion_to_mat4(quaternion_inverse(rot)));
  mat4_translate_post(&ret, -pos.x, -pos.y, -pos.z);
  return ret;
}

Mat4 get_standard_transform2(Vec3 pos, Quaternion rot, Vec3 scale) {
  Mat4 ret = mat4_identity();
  mat4_translate_post(&ret, pos.x, pos.y, pos.z);
  ret = mat4_mul(ret, quaternion_to_mat4(rot));
  mat4_scale_post(&ret, scale.x, scale.y, scale.z);
  return ret;
}

Mat4 get_standard_transform2_inverse(Vec3 pos, Quaternion rot, Vec3 scale) {
  Mat4 ret = mat4_identity();
  mat4_scale_post(&ret, 1.f/scale.x, 1.f/scale.y, 1.f/scale.z);
  ret = mat4_mul(ret, quaternion_to_mat4(quaternion_inverse(rot)));
  mat4_translate_post(&ret, -pos.x, -pos.y, -pos.z);
  return ret;
}

char **util_list_files_in_directory(const char *dir) {
  char **ret = NULL;
  App_File_Iterator it = {0};
  while (app_iterate_directory(dir, &it)) {
    arr_add(ret, str_copy(it.file_name));
  }
  return ret;
}

void util_free_file_list(char **files) {
  for (int i = 0; i < arr_len(files); i++)
    str_free(files[i]);
  arr_free(files);
}

void util_gfx_line_imm(Vec3 p0, Vec3 p1) {
  uint32_t color = gfx_get_color_u();
  gfx_imm_vertex_3d(p0.x, p0.y, p0.z, 0, 0, color);
  gfx_imm_vertex_3d(p1.x, p1.y, p1.z, 0, 0, color);
}
void util_gfx_line(Vec3 p0, Vec3 p1) {
  gfx_line_3d(p0.x, p0.y, p0.z, p1.x, p1.y, p1.z);
}
void util_gfx_debug_point_imm(Vec3 p) {
  float w = 4;
  util_gfx_line_imm(add3(p, vec3(0,0,w)), add3(p, vec3(0,0,-w)));
  util_gfx_line_imm(add3(p, vec3(0,w,0)), add3(p, vec3(0,-w,0)));
  util_gfx_line_imm(add3(p, vec3(w,0,0)), add3(p, vec3(-w,0,0)));
}
void util_gfx_debug_point(Vec3 p) {
  gfx_imm_begin_3d();
  util_gfx_debug_point_imm(p);
  gfx_imm_end();
  gfx_imm_draw(GFX_PRIMITIVE_LINES, NULL);
}

void util_gfx_debug_box_transform_imm(Vec3 p0, Vec3 p1, Mat4 transform) {
  float x0 = p0.x;
  float y0 = p0.y;
  float z0 = p0.z;
  float x1 = p1.x;
  float y1 = p1.y;
  float z1 = p1.z;

  // p4------p5
  // | \      |\
  // |  p0------p1
  // |  |     | |
  // p7-|----p6 |
  //   \|      \|
  //   p3-------p2

  Vec3 p[8] = {
    { x0, y1, z0 }, //p0
    { x1, y1, z0 }, //p1
    { x1, y0, z0 }, //p2
    { x0, y0, z0 }, //p3
    { x0, y1, z1 }, //p0
    { x1, y1, z1 }, //p1
    { x1, y0, z1 }, //p2
    { x0, y0, z1 }, //p3
  };
  for (int j = 0; j < _countof(p); j++) {
    p[j] = mat4_multiply_pos(transform, p[j]);
  }

  util_gfx_line_imm(p[0], p[1]);
  util_gfx_line_imm(p[1], p[2]);
  util_gfx_line_imm(p[2], p[3]);
  util_gfx_line_imm(p[3], p[0]);

  util_gfx_line_imm(p[4], p[5]);
  util_gfx_line_imm(p[5], p[6]);
  util_gfx_line_imm(p[6], p[7]);
  util_gfx_line_imm(p[7], p[4]);

  util_gfx_line_imm(p[0], p[4]);
  util_gfx_line_imm(p[1], p[5]);
  util_gfx_line_imm(p[2], p[6]);
  util_gfx_line_imm(p[3], p[7]);
}

void util_gfx_debug_box_imm(Vec3 p0, Vec3 p1) {
  Vec3 points[] = {
    vec3(p0.x, p0.y, p0.z),
    vec3(p1.x, p0.y, p0.z),

    vec3(p1.x, p0.y, p0.z),
    vec3(p1.x, p1.y, p0.z),

    vec3(p1.x, p1.y, p0.z),
    vec3(p0.x, p1.y, p0.z),

    vec3(p0.x, p1.y, p0.z),
    vec3(p0.x, p0.y, p0.z),

    //========================
    vec3(p0.x, p0.y, p1.z),
    vec3(p1.x, p0.y, p1.z),

    vec3(p1.x, p0.y, p1.z),
    vec3(p1.x, p1.y, p1.z),

    vec3(p1.x, p1.y, p1.z),
    vec3(p0.x, p1.y, p1.z),

    vec3(p0.x, p1.y, p1.z),
    vec3(p0.x, p0.y, p1.z),

    //========================
    vec3(p0.x, p0.y, p0.z),
    vec3(p0.x, p0.y, p1.z),

    vec3(p1.x, p0.y, p0.z),
    vec3(p1.x, p0.y, p1.z),

    vec3(p1.x, p1.y, p0.z),
    vec3(p1.x, p1.y, p1.z),

    vec3(p0.x, p1.y, p0.z),
    vec3(p0.x, p1.y, p1.z),
  };

  for (int j = 0; j+1 < _countof(points); j += 2) {
    util_gfx_line_imm(points[j+0], points[j+1]);
  }
}

void util_gfx_debug_aabb_imm(Aabb aabb) {
  Vec3 p0 = aabb.p[0];
  Vec3 p1 = aabb.p[1];
  util_gfx_debug_box_imm(p0, p1);
}
void util_gfx_debug_aabb(Aabb aabb) {
  gfx_imm_begin_3d();
  util_gfx_debug_aabb_imm(aabb);
  gfx_imm_end();
  gfx_imm_draw(GFX_PRIMITIVE_LINES, NULL);
}

/*
 * OpenSimplex (Simplectic) Noise in C.
 * Ported by Stephen M. Cameron from Kurt Spencer's java implementation
 * 
 * v1.1 (October 5, 2014)
 * - Added 2D and 4D implementations.
 * - Proper gradient sets for all dimensions, from a
 *   dimensionally-generalizable scheme with an actual
 *   rhyme and reason behind it.
 * - Removed default permutation array in favor of
 *   default seed.
 * - Changed seed-based constructor to be independent
 *   of any particular randomization library, so results
 *   will be the same when ported to other languages.
 */
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

//#include "open-simplex-noise.h"

#define STRETCH_CONSTANT_2D (-0.211324865405187)    /* (1 / sqrt(2 + 1) - 1 ) / 2; */
#define SQUISH_CONSTANT_2D  (0.366025403784439)     /* (sqrt(2 + 1) -1) / 2; */
#define STRETCH_CONSTANT_3D (-1.0 / 6.0)            /* (1 / sqrt(3 + 1) - 1) / 3; */
#define SQUISH_CONSTANT_3D  (1.0 / 3.0)             /* (sqrt(3+1)-1)/3; */
#define STRETCH_CONSTANT_4D (-0.138196601125011)    /* (1 / sqrt(4 + 1) - 1) / 4; */
#define SQUISH_CONSTANT_4D  (0.309016994374947)     /* (sqrt(4 + 1) - 1) / 4; */
	
#define NORM_CONSTANT_2D (47.0)
#define NORM_CONSTANT_3D (103.0)
#define NORM_CONSTANT_4D (30.0)
	
#define DEFAULT_SEED (0LL)

struct osn_context {
	int16_t *perm;
	int16_t *permGradIndex3D;
};

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif

/* 
 * Gradients for 2D. They approximate the directions to the
 * vertices of an octagon from the center.
 */
static const int8_t gradients2D[] = {
	 5,  2,    2,  5,
	-5,  2,   -2,  5,
	 5, -2,    2, -5,
	-5, -2,   -2, -5,
};

/*	
 * Gradients for 3D. They approximate the directions to the
 * vertices of a rhombicuboctahedron from the center, skewed so
 * that the triangular and square facets can be inscribed inside
 * circles of the same radius.
 */
static const signed char gradients3D[] = {
	-11,  4,  4,     -4,  11,  4,    -4,  4,  11,
	 11,  4,  4,      4,  11,  4,     4,  4,  11,
	-11, -4,  4,     -4, -11,  4,    -4, -4,  11,
	 11, -4,  4,      4, -11,  4,     4, -4,  11,
	-11,  4, -4,     -4,  11, -4,    -4,  4, -11,
	 11,  4, -4,      4,  11, -4,     4,  4, -11,
	-11, -4, -4,     -4, -11, -4,    -4, -4, -11,
	 11, -4, -4,      4, -11, -4,     4, -4, -11,
};

/*	
 * Gradients for 4D. They approximate the directions to the
 * vertices of a disprismatotesseractihexadecachoron from the center,
 * skewed so that the tetrahedral and cubic facets can be inscribed inside
 * spheres of the same radius.
 */
static const signed char gradients4D[] = {
	 3,  1,  1,  1,      1,  3,  1,  1,      1,  1,  3,  1,      1,  1,  1,  3,
	-3,  1,  1,  1,     -1,  3,  1,  1,     -1,  1,  3,  1,     -1,  1,  1,  3,
	 3, -1,  1,  1,      1, -3,  1,  1,      1, -1,  3,  1,      1, -1,  1,  3,
	-3, -1,  1,  1,     -1, -3,  1,  1,     -1, -1,  3,  1,     -1, -1,  1,  3,
	 3,  1, -1,  1,      1,  3, -1,  1,      1,  1, -3,  1,      1,  1, -1,  3,
	-3,  1, -1,  1,     -1,  3, -1,  1,     -1,  1, -3,  1,     -1,  1, -1,  3,
	 3, -1, -1,  1,      1, -3, -1,  1,      1, -1, -3,  1,      1, -1, -1,  3,
	-3, -1, -1,  1,     -1, -3, -1,  1,     -1, -1, -3,  1,     -1, -1, -1,  3,
	 3,  1,  1, -1,      1,  3,  1, -1,      1,  1,  3, -1,      1,  1,  1, -3,
	-3,  1,  1, -1,     -1,  3,  1, -1,     -1,  1,  3, -1,     -1,  1,  1, -3,
	 3, -1,  1, -1,      1, -3,  1, -1,      1, -1,  3, -1,      1, -1,  1, -3,
	-3, -1,  1, -1,     -1, -3,  1, -1,     -1, -1,  3, -1,     -1, -1,  1, -3,
	 3,  1, -1, -1,      1,  3, -1, -1,      1,  1, -3, -1,      1,  1, -1, -3,
	-3,  1, -1, -1,     -1,  3, -1, -1,     -1,  1, -3, -1,     -1,  1, -1, -3,
	 3, -1, -1, -1,      1, -3, -1, -1,      1, -1, -3, -1,      1, -1, -1, -3,
	-3, -1, -1, -1,     -1, -3, -1, -1,     -1, -1, -3, -1,     -1, -1, -1, -3,
};

static OSNFLOAT extrapolate2(const struct osn_context *ctx, int xsb, int ysb, OSNFLOAT dx, OSNFLOAT dy)
{
	const int16_t *perm = ctx->perm;
	int index = perm[(perm[xsb & 0xFF] + ysb) & 0xFF] & 0x0E;
	return gradients2D[index] * dx
		+ gradients2D[index + 1] * dy;
}
	
static OSNFLOAT extrapolate3(const struct osn_context *ctx, int xsb, int ysb, int zsb, OSNFLOAT dx, OSNFLOAT dy, OSNFLOAT dz)
{
	const int16_t *perm = ctx->perm;
	const int16_t *permGradIndex3D = ctx->permGradIndex3D;
	int index = permGradIndex3D[(perm[(perm[xsb & 0xFF] + ysb) & 0xFF] + zsb) & 0xFF];
	return gradients3D[index] * dx
		+ gradients3D[index + 1] * dy
		+ gradients3D[index + 2] * dz;
}
	
static OSNFLOAT extrapolate4(const struct osn_context *ctx, int xsb, int ysb, int zsb, int wsb, OSNFLOAT dx, OSNFLOAT dy, OSNFLOAT dz, OSNFLOAT dw)
{
	const int16_t *perm = ctx->perm;
	int index = perm[(perm[(perm[(perm[xsb & 0xFF] + ysb) & 0xFF] + zsb) & 0xFF] + wsb) & 0xFF] & 0xFC;
	return gradients4D[index] * dx
		+ gradients4D[index + 1] * dy
		+ gradients4D[index + 2] * dz
		+ gradients4D[index + 3] * dw;
}
	
static INLINE int fastFloor(OSNFLOAT x) {
	int xi = (int) x;
	return x < xi ? xi - 1 : xi;
}
	
static int allocate_perm(struct osn_context *ctx, int nperm, int ngrad)
{
	if (ctx->perm)
		free(ctx->perm);
	if (ctx->permGradIndex3D)
		free(ctx->permGradIndex3D);
	ctx->perm = (int16_t *) malloc(sizeof(*ctx->perm) * nperm); 
	if (!ctx->perm)
		return -ENOMEM;
	ctx->permGradIndex3D = (int16_t *) malloc(sizeof(*ctx->permGradIndex3D) * ngrad);
	if (!ctx->permGradIndex3D) {
		free(ctx->perm);
		return -ENOMEM;
	}
	return 0;
}
	
int open_simplex_noise_init_perm(struct osn_context *ctx, int16_t p[], int nelements)
{
	int i, rc;

	rc = allocate_perm(ctx, nelements, 256);
	if (rc)
		return rc;
	memcpy(ctx->perm, p, sizeof(*ctx->perm) * nelements);
		
	for (i = 0; i < 256; i++) {
		/* Since 3D has 24 gradients, simple bitmask won't work, so precompute modulo array. */
		ctx->permGradIndex3D[i] = (int16_t)((ctx->perm[i] % (ARRAYSIZE(gradients3D) / 3)) * 3);
	}
	return 0;
}

/*	
 * Initializes using a permutation array generated from a 64-bit seed.
 * Generates a proper permutation (i.e. doesn't merely perform N successive pair
 * swaps on a base array).  Uses a simple 64-bit LCG.
 */
int open_simplex_noise(int64_t seed, struct osn_context **ctx)
{
	int rc;
	int16_t source[256];
	int i;
	int16_t *perm;
	int16_t *permGradIndex3D;
	int r;

	*ctx = (struct osn_context *) malloc(sizeof(**ctx));
	if (!(*ctx))
		return -ENOMEM;
	(*ctx)->perm = NULL;
	(*ctx)->permGradIndex3D = NULL;

	rc = allocate_perm(*ctx, 256, 256);
	if (rc) {
		free(*ctx);
		return rc;
	}

	perm = (*ctx)->perm;
	permGradIndex3D = (*ctx)->permGradIndex3D;

	uint64_t seedU = seed;
	for (i = 0; i < 256; i++)
		source[i] = (int16_t) i;
	seedU = seedU * 6364136223846793005ULL + 1442695040888963407ULL;
	seedU = seedU * 6364136223846793005ULL + 1442695040888963407ULL;
	seedU = seedU * 6364136223846793005ULL + 1442695040888963407ULL;
	for (i = 255; i >= 0; i--) {
		seedU = seedU * 6364136223846793005ULL + 1442695040888963407ULL;
		r = (int)((seedU + 31) % (i + 1));
		if (r < 0)
			r += (i + 1);
		perm[i] = source[r];
		permGradIndex3D[i] = (short)((perm[i] % (ARRAYSIZE(gradients3D) / 3)) * 3);
		source[r] = source[i];
	}
	return 0;
}

void open_simplex_noise_free(struct osn_context *ctx)
{
	if (!ctx)
		return;
	if (ctx->perm) {
		free(ctx->perm);
		ctx->perm = NULL;	
	}
	if (ctx->permGradIndex3D) {
		free(ctx->permGradIndex3D);
		ctx->permGradIndex3D = NULL;
	}
	free(ctx);
}
	
/* 2D OpenSimplex (Simplectic) Noise. */
OSNFLOAT open_simplex_noise2(const struct osn_context *ctx, OSNFLOAT x, OSNFLOAT y)
{
	
	/* Place input coordinates onto grid. */
	OSNFLOAT stretchOffset = (x + y) * STRETCH_CONSTANT_2D;
	OSNFLOAT xs = x + stretchOffset;
	OSNFLOAT ys = y + stretchOffset;
		
	/* Floor to get grid coordinates of rhombus (stretched square) super-cell origin. */
	int xsb = fastFloor(xs);
	int ysb = fastFloor(ys);
		
	/* Skew out to get actual coordinates of rhombus origin. We'll need these later. */
	OSNFLOAT squishOffset = (xsb + ysb) * SQUISH_CONSTANT_2D;
	OSNFLOAT xb = xsb + squishOffset;
	OSNFLOAT yb = ysb + squishOffset;
		
	/* Compute grid coordinates relative to rhombus origin. */
	OSNFLOAT xins = xs - xsb;
	OSNFLOAT yins = ys - ysb;
		
	/* Sum those together to get a value that determines which region we're in. */
	OSNFLOAT inSum = xins + yins;

	/* Positions relative to origin point. */
	OSNFLOAT dx0 = x - xb;
	OSNFLOAT dy0 = y - yb;
		
	/* We'll be defining these inside the next block and using them afterwards. */
	OSNFLOAT dx_ext, dy_ext;
	int xsv_ext, ysv_ext;

	OSNFLOAT dx1;
	OSNFLOAT dy1;
	OSNFLOAT attn1;
	OSNFLOAT dx2;
	OSNFLOAT dy2;
	OSNFLOAT attn2;
	OSNFLOAT zins;
	OSNFLOAT attn0;
	OSNFLOAT attn_ext;

	OSNFLOAT value = 0;

	/* Contribution (1,0) */
	dx1 = dx0 - 1 - SQUISH_CONSTANT_2D;
	dy1 = dy0 - 0 - SQUISH_CONSTANT_2D;
	attn1 = 2 - dx1 * dx1 - dy1 * dy1;
	if (attn1 > 0) {
		attn1 *= attn1;
		value += attn1 * attn1 * extrapolate2(ctx, xsb + 1, ysb + 0, dx1, dy1);
	}

	/* Contribution (0,1) */
	dx2 = dx0 - 0 - SQUISH_CONSTANT_2D;
	dy2 = dy0 - 1 - SQUISH_CONSTANT_2D;
	attn2 = 2 - dx2 * dx2 - dy2 * dy2;
	if (attn2 > 0) {
		attn2 *= attn2;
		value += attn2 * attn2 * extrapolate2(ctx, xsb + 0, ysb + 1, dx2, dy2);
	}
		
	if (inSum <= 1) { /* We're inside the triangle (2-Simplex) at (0,0) */
		zins = 1 - inSum;
		if (zins > xins || zins > yins) { /* (0,0) is one of the closest two triangular vertices */
			if (xins > yins) {
				xsv_ext = xsb + 1;
				ysv_ext = ysb - 1;
				dx_ext = dx0 - 1;
				dy_ext = dy0 + 1;
			} else {
				xsv_ext = xsb - 1;
				ysv_ext = ysb + 1;
				dx_ext = dx0 + 1;
				dy_ext = dy0 - 1;
			}
		} else { /* (1,0) and (0,1) are the closest two vertices. */
			xsv_ext = xsb + 1;
			ysv_ext = ysb + 1;
			dx_ext = dx0 - 1 - 2 * SQUISH_CONSTANT_2D;
			dy_ext = dy0 - 1 - 2 * SQUISH_CONSTANT_2D;
		}
	} else { /* We're inside the triangle (2-Simplex) at (1,1) */
		zins = 2 - inSum;
		if (zins < xins || zins < yins) { /* (0,0) is one of the closest two triangular vertices */
			if (xins > yins) {
				xsv_ext = xsb + 2;
				ysv_ext = ysb + 0;
				dx_ext = dx0 - 2 - 2 * SQUISH_CONSTANT_2D;
				dy_ext = dy0 + 0 - 2 * SQUISH_CONSTANT_2D;
			} else {
				xsv_ext = xsb + 0;
				ysv_ext = ysb + 2;
				dx_ext = dx0 + 0 - 2 * SQUISH_CONSTANT_2D;
				dy_ext = dy0 - 2 - 2 * SQUISH_CONSTANT_2D;
			}
		} else { /* (1,0) and (0,1) are the closest two vertices. */
			dx_ext = dx0;
			dy_ext = dy0;
			xsv_ext = xsb;
			ysv_ext = ysb;
		}
		xsb += 1;
		ysb += 1;
		dx0 = dx0 - 1 - 2 * SQUISH_CONSTANT_2D;
		dy0 = dy0 - 1 - 2 * SQUISH_CONSTANT_2D;
	}
		
	/* Contribution (0,0) or (1,1) */
	attn0 = 2 - dx0 * dx0 - dy0 * dy0;
	if (attn0 > 0) {
		attn0 *= attn0;
		value += attn0 * attn0 * extrapolate2(ctx, xsb, ysb, dx0, dy0);
	}
	
	/* Extra Vertex */
	attn_ext = 2 - dx_ext * dx_ext - dy_ext * dy_ext;
	if (attn_ext > 0) {
		attn_ext *= attn_ext;
		value += attn_ext * attn_ext * extrapolate2(ctx, xsv_ext, ysv_ext, dx_ext, dy_ext);
	}
	
	return value / NORM_CONSTANT_2D;
}
	
/*
 * 3D OpenSimplex (Simplectic) Noise
 */
OSNFLOAT open_simplex_noise3(const struct osn_context *ctx, OSNFLOAT x, OSNFLOAT y, OSNFLOAT z)
{

	/* Place input coordinates on simplectic honeycomb. */
	OSNFLOAT stretchOffset = (x + y + z) * STRETCH_CONSTANT_3D;
	OSNFLOAT xs = x + stretchOffset;
	OSNFLOAT ys = y + stretchOffset;
	OSNFLOAT zs = z + stretchOffset;
	
	/* Floor to get simplectic honeycomb coordinates of rhombohedron (stretched cube) super-cell origin. */
	int xsb = fastFloor(xs);
	int ysb = fastFloor(ys);
	int zsb = fastFloor(zs);
	
	/* Skew out to get actual coordinates of rhombohedron origin. We'll need these later. */
	OSNFLOAT squishOffset = (xsb + ysb + zsb) * SQUISH_CONSTANT_3D;
	OSNFLOAT xb = xsb + squishOffset;
	OSNFLOAT yb = ysb + squishOffset;
	OSNFLOAT zb = zsb + squishOffset;
	
	/* Compute simplectic honeycomb coordinates relative to rhombohedral origin. */
	OSNFLOAT xins = xs - xsb;
	OSNFLOAT yins = ys - ysb;
	OSNFLOAT zins = zs - zsb;
	
	/* Sum those together to get a value that determines which region we're in. */
	OSNFLOAT inSum = xins + yins + zins;

	/* Positions relative to origin point. */
	OSNFLOAT dx0 = x - xb;
	OSNFLOAT dy0 = y - yb;
	OSNFLOAT dz0 = z - zb;
	
	/* We'll be defining these inside the next block and using them afterwards. */
	OSNFLOAT dx_ext0, dy_ext0, dz_ext0;
	OSNFLOAT dx_ext1, dy_ext1, dz_ext1;
	int xsv_ext0, ysv_ext0, zsv_ext0;
	int xsv_ext1, ysv_ext1, zsv_ext1;

	OSNFLOAT wins;
	int8_t c, c1, c2;
	int8_t aPoint, bPoint;
	OSNFLOAT aScore, bScore;
	int aIsFurtherSide;
	int bIsFurtherSide;
	OSNFLOAT p1, p2, p3;
	OSNFLOAT score;
	OSNFLOAT attn0, attn1, attn2, attn3, attn4, attn5, attn6;
	OSNFLOAT dx1, dy1, dz1;
	OSNFLOAT dx2, dy2, dz2;
	OSNFLOAT dx3, dy3, dz3;
	OSNFLOAT dx4, dy4, dz4;
	OSNFLOAT dx5, dy5, dz5;
	OSNFLOAT dx6, dy6, dz6;
	OSNFLOAT attn_ext0, attn_ext1;
	
	OSNFLOAT value = 0;
	if (inSum <= 1) { /* We're inside the tetrahedron (3-Simplex) at (0,0,0) */
		
		/* Determine which two of (0,0,1), (0,1,0), (1,0,0) are closest. */
		aPoint = 0x01;
		aScore = xins;
		bPoint = 0x02;
		bScore = yins;
		if (aScore >= bScore && zins > bScore) {
			bScore = zins;
			bPoint = 0x04;
		} else if (aScore < bScore && zins > aScore) {
			aScore = zins;
			aPoint = 0x04;
		}
		
		/* Now we determine the two lattice points not part of the tetrahedron that may contribute.
		   This depends on the closest two tetrahedral vertices, including (0,0,0) */
		wins = 1 - inSum;
		if (wins > aScore || wins > bScore) { /* (0,0,0) is one of the closest two tetrahedral vertices. */
			c = (bScore > aScore ? bPoint : aPoint); /* Our other closest vertex is the closest out of a and b. */
			
			if ((c & 0x01) == 0) {
				xsv_ext0 = xsb - 1;
				xsv_ext1 = xsb;
				dx_ext0 = dx0 + 1;
				dx_ext1 = dx0;
			} else {
				xsv_ext0 = xsv_ext1 = xsb + 1;
				dx_ext0 = dx_ext1 = dx0 - 1;
			}

			if ((c & 0x02) == 0) {
				ysv_ext0 = ysv_ext1 = ysb;
				dy_ext0 = dy_ext1 = dy0;
				if ((c & 0x01) == 0) {
					ysv_ext1 -= 1;
					dy_ext1 += 1;
				} else {
					ysv_ext0 -= 1;
					dy_ext0 += 1;
				}
			} else {
				ysv_ext0 = ysv_ext1 = ysb + 1;
				dy_ext0 = dy_ext1 = dy0 - 1;
			}

			if ((c & 0x04) == 0) {
				zsv_ext0 = zsb;
				zsv_ext1 = zsb - 1;
				dz_ext0 = dz0;
				dz_ext1 = dz0 + 1;
			} else {
				zsv_ext0 = zsv_ext1 = zsb + 1;
				dz_ext0 = dz_ext1 = dz0 - 1;
			}
		} else { /* (0,0,0) is not one of the closest two tetrahedral vertices. */
			c = (int8_t)(aPoint | bPoint); /* Our two extra vertices are determined by the closest two. */
			
			if ((c & 0x01) == 0) {
				xsv_ext0 = xsb;
				xsv_ext1 = xsb - 1;
				dx_ext0 = dx0 - 2 * SQUISH_CONSTANT_3D;
				dx_ext1 = dx0 + 1 - SQUISH_CONSTANT_3D;
			} else {
				xsv_ext0 = xsv_ext1 = xsb + 1;
				dx_ext0 = dx0 - 1 - 2 * SQUISH_CONSTANT_3D;
				dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_3D;
			}

			if ((c & 0x02) == 0) {
				ysv_ext0 = ysb;
				ysv_ext1 = ysb - 1;
				dy_ext0 = dy0 - 2 * SQUISH_CONSTANT_3D;
				dy_ext1 = dy0 + 1 - SQUISH_CONSTANT_3D;
			} else {
				ysv_ext0 = ysv_ext1 = ysb + 1;
				dy_ext0 = dy0 - 1 - 2 * SQUISH_CONSTANT_3D;
				dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_3D;
			}

			if ((c & 0x04) == 0) {
				zsv_ext0 = zsb;
				zsv_ext1 = zsb - 1;
				dz_ext0 = dz0 - 2 * SQUISH_CONSTANT_3D;
				dz_ext1 = dz0 + 1 - SQUISH_CONSTANT_3D;
			} else {
				zsv_ext0 = zsv_ext1 = zsb + 1;
				dz_ext0 = dz0 - 1 - 2 * SQUISH_CONSTANT_3D;
				dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_3D;
			}
		}

		/* Contribution (0,0,0) */
		attn0 = 2 - dx0 * dx0 - dy0 * dy0 - dz0 * dz0;
		if (attn0 > 0) {
			attn0 *= attn0;
			value += attn0 * attn0 * extrapolate3(ctx, xsb + 0, ysb + 0, zsb + 0, dx0, dy0, dz0);
		}

		/* Contribution (1,0,0) */
		dx1 = dx0 - 1 - SQUISH_CONSTANT_3D;
		dy1 = dy0 - 0 - SQUISH_CONSTANT_3D;
		dz1 = dz0 - 0 - SQUISH_CONSTANT_3D;
		attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1;
		if (attn1 > 0) {
			attn1 *= attn1;
			value += attn1 * attn1 * extrapolate3(ctx, xsb + 1, ysb + 0, zsb + 0, dx1, dy1, dz1);
		}

		/* Contribution (0,1,0) */
		dx2 = dx0 - 0 - SQUISH_CONSTANT_3D;
		dy2 = dy0 - 1 - SQUISH_CONSTANT_3D;
		dz2 = dz1;
		attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2;
		if (attn2 > 0) {
			attn2 *= attn2;
			value += attn2 * attn2 * extrapolate3(ctx, xsb + 0, ysb + 1, zsb + 0, dx2, dy2, dz2);
		}

		/* Contribution (0,0,1) */
		dx3 = dx2;
		dy3 = dy1;
		dz3 = dz0 - 1 - SQUISH_CONSTANT_3D;
		attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3;
		if (attn3 > 0) {
			attn3 *= attn3;
			value += attn3 * attn3 * extrapolate3(ctx, xsb + 0, ysb + 0, zsb + 1, dx3, dy3, dz3);
		}
	} else if (inSum >= 2) { /* We're inside the tetrahedron (3-Simplex) at (1,1,1) */
	
		/* Determine which two tetrahedral vertices are the closest, out of (1,1,0), (1,0,1), (0,1,1) but not (1,1,1). */
		aPoint = 0x06;
		aScore = xins;
		bPoint = 0x05;
		bScore = yins;
		if (aScore <= bScore && zins < bScore) {
			bScore = zins;
			bPoint = 0x03;
		} else if (aScore > bScore && zins < aScore) {
			aScore = zins;
			aPoint = 0x03;
		}
		
		/* Now we determine the two lattice points not part of the tetrahedron that may contribute.
		   This depends on the closest two tetrahedral vertices, including (1,1,1) */
		wins = 3 - inSum;
		if (wins < aScore || wins < bScore) { /* (1,1,1) is one of the closest two tetrahedral vertices. */
			c = (bScore < aScore ? bPoint : aPoint); /* Our other closest vertex is the closest out of a and b. */
			
			if ((c & 0x01) != 0) {
				xsv_ext0 = xsb + 2;
				xsv_ext1 = xsb + 1;
				dx_ext0 = dx0 - 2 - 3 * SQUISH_CONSTANT_3D;
				dx_ext1 = dx0 - 1 - 3 * SQUISH_CONSTANT_3D;
			} else {
				xsv_ext0 = xsv_ext1 = xsb;
				dx_ext0 = dx_ext1 = dx0 - 3 * SQUISH_CONSTANT_3D;
			}

			if ((c & 0x02) != 0) {
				ysv_ext0 = ysv_ext1 = ysb + 1;
				dy_ext0 = dy_ext1 = dy0 - 1 - 3 * SQUISH_CONSTANT_3D;
				if ((c & 0x01) != 0) {
					ysv_ext1 += 1;
					dy_ext1 -= 1;
				} else {
					ysv_ext0 += 1;
					dy_ext0 -= 1;
				}
			} else {
				ysv_ext0 = ysv_ext1 = ysb;
				dy_ext0 = dy_ext1 = dy0 - 3 * SQUISH_CONSTANT_3D;
			}

			if ((c & 0x04) != 0) {
				zsv_ext0 = zsb + 1;
				zsv_ext1 = zsb + 2;
				dz_ext0 = dz0 - 1 - 3 * SQUISH_CONSTANT_3D;
				dz_ext1 = dz0 - 2 - 3 * SQUISH_CONSTANT_3D;
			} else {
				zsv_ext0 = zsv_ext1 = zsb;
				dz_ext0 = dz_ext1 = dz0 - 3 * SQUISH_CONSTANT_3D;
			}
		} else { /* (1,1,1) is not one of the closest two tetrahedral vertices. */
			c = (int8_t)(aPoint & bPoint); /* Our two extra vertices are determined by the closest two. */
			
			if ((c & 0x01) != 0) {
				xsv_ext0 = xsb + 1;
				xsv_ext1 = xsb + 2;
				dx_ext0 = dx0 - 1 - SQUISH_CONSTANT_3D;
				dx_ext1 = dx0 - 2 - 2 * SQUISH_CONSTANT_3D;
			} else {
				xsv_ext0 = xsv_ext1 = xsb;
				dx_ext0 = dx0 - SQUISH_CONSTANT_3D;
				dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_3D;
			}

			if ((c & 0x02) != 0) {
				ysv_ext0 = ysb + 1;
				ysv_ext1 = ysb + 2;
				dy_ext0 = dy0 - 1 - SQUISH_CONSTANT_3D;
				dy_ext1 = dy0 - 2 - 2 * SQUISH_CONSTANT_3D;
			} else {
				ysv_ext0 = ysv_ext1 = ysb;
				dy_ext0 = dy0 - SQUISH_CONSTANT_3D;
				dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_3D;
			}

			if ((c & 0x04) != 0) {
				zsv_ext0 = zsb + 1;
				zsv_ext1 = zsb + 2;
				dz_ext0 = dz0 - 1 - SQUISH_CONSTANT_3D;
				dz_ext1 = dz0 - 2 - 2 * SQUISH_CONSTANT_3D;
			} else {
				zsv_ext0 = zsv_ext1 = zsb;
				dz_ext0 = dz0 - SQUISH_CONSTANT_3D;
				dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_3D;
			}
		}
		
		/* Contribution (1,1,0) */
		dx3 = dx0 - 1 - 2 * SQUISH_CONSTANT_3D;
		dy3 = dy0 - 1 - 2 * SQUISH_CONSTANT_3D;
		dz3 = dz0 - 0 - 2 * SQUISH_CONSTANT_3D;
		attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3;
		if (attn3 > 0) {
			attn3 *= attn3;
			value += attn3 * attn3 * extrapolate3(ctx, xsb + 1, ysb + 1, zsb + 0, dx3, dy3, dz3);
		}

		/* Contribution (1,0,1) */
		dx2 = dx3;
		dy2 = dy0 - 0 - 2 * SQUISH_CONSTANT_3D;
		dz2 = dz0 - 1 - 2 * SQUISH_CONSTANT_3D;
		attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2;
		if (attn2 > 0) {
			attn2 *= attn2;
			value += attn2 * attn2 * extrapolate3(ctx, xsb + 1, ysb + 0, zsb + 1, dx2, dy2, dz2);
		}

		/* Contribution (0,1,1) */
		dx1 = dx0 - 0 - 2 * SQUISH_CONSTANT_3D;
		dy1 = dy3;
		dz1 = dz2;
		attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1;
		if (attn1 > 0) {
			attn1 *= attn1;
			value += attn1 * attn1 * extrapolate3(ctx, xsb + 0, ysb + 1, zsb + 1, dx1, dy1, dz1);
		}

		/* Contribution (1,1,1) */
		dx0 = dx0 - 1 - 3 * SQUISH_CONSTANT_3D;
		dy0 = dy0 - 1 - 3 * SQUISH_CONSTANT_3D;
		dz0 = dz0 - 1 - 3 * SQUISH_CONSTANT_3D;
		attn0 = 2 - dx0 * dx0 - dy0 * dy0 - dz0 * dz0;
		if (attn0 > 0) {
			attn0 *= attn0;
			value += attn0 * attn0 * extrapolate3(ctx, xsb + 1, ysb + 1, zsb + 1, dx0, dy0, dz0);
		}
	} else { /* We're inside the octahedron (Rectified 3-Simplex) in between.
		        Decide between point (0,0,1) and (1,1,0) as closest */
		p1 = xins + yins;
		if (p1 > 1) {
			aScore = p1 - 1;
			aPoint = 0x03;
			aIsFurtherSide = 1;
		} else {
			aScore = 1 - p1;
			aPoint = 0x04;
			aIsFurtherSide = 0;
		}

		/* Decide between point (0,1,0) and (1,0,1) as closest */
		p2 = xins + zins;
		if (p2 > 1) {
			bScore = p2 - 1;
			bPoint = 0x05;
			bIsFurtherSide = 1;
		} else {
			bScore = 1 - p2;
			bPoint = 0x02;
			bIsFurtherSide = 0;
		}
		
		/* The closest out of the two (1,0,0) and (0,1,1) will replace the furthest out of the two decided above, if closer. */
		p3 = yins + zins;
		if (p3 > 1) {
			score = p3 - 1;
			if (aScore <= bScore && aScore < score) {
				// aScore = score; dead store
				aPoint = 0x06;
				aIsFurtherSide = 1;
			} else if (aScore > bScore && bScore < score) {
				// bScore = score; dead store
				bPoint = 0x06;
				bIsFurtherSide = 1;
			}
		} else {
			score = 1 - p3;
			if (aScore <= bScore && aScore < score) {
				// aScore = score; dead store
				aPoint = 0x01;
				aIsFurtherSide = 0;
			} else if (aScore > bScore && bScore < score) {
				// bScore = score; dead store
				bPoint = 0x01;
				bIsFurtherSide = 0;
			}
		}
		
		/* Where each of the two closest points are determines how the extra two vertices are calculated. */
		if (aIsFurtherSide == bIsFurtherSide) {
			if (aIsFurtherSide) { /* Both closest points on (1,1,1) side */

				/* One of the two extra points is (1,1,1) */
				dx_ext0 = dx0 - 1 - 3 * SQUISH_CONSTANT_3D;
				dy_ext0 = dy0 - 1 - 3 * SQUISH_CONSTANT_3D;
				dz_ext0 = dz0 - 1 - 3 * SQUISH_CONSTANT_3D;
				xsv_ext0 = xsb + 1;
				ysv_ext0 = ysb + 1;
				zsv_ext0 = zsb + 1;

				/* Other extra point is based on the shared axis. */
				c = (int8_t)(aPoint & bPoint);
				if ((c & 0x01) != 0) {
					dx_ext1 = dx0 - 2 - 2 * SQUISH_CONSTANT_3D;
					dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_3D;
					dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_3D;
					xsv_ext1 = xsb + 2;
					ysv_ext1 = ysb;
					zsv_ext1 = zsb;
				} else if ((c & 0x02) != 0) {
					dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_3D;
					dy_ext1 = dy0 - 2 - 2 * SQUISH_CONSTANT_3D;
					dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_3D;
					xsv_ext1 = xsb;
					ysv_ext1 = ysb + 2;
					zsv_ext1 = zsb;
				} else {
					dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_3D;
					dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_3D;
					dz_ext1 = dz0 - 2 - 2 * SQUISH_CONSTANT_3D;
					xsv_ext1 = xsb;
					ysv_ext1 = ysb;
					zsv_ext1 = zsb + 2;
				}
			} else { /* Both closest points on (0,0,0) side */

				/* One of the two extra points is (0,0,0) */
				dx_ext0 = dx0;
				dy_ext0 = dy0;
				dz_ext0 = dz0;
				xsv_ext0 = xsb;
				ysv_ext0 = ysb;
				zsv_ext0 = zsb;

				/* Other extra point is based on the omitted axis. */
				c = (int8_t)(aPoint | bPoint);
				if ((c & 0x01) == 0) {
					dx_ext1 = dx0 + 1 - SQUISH_CONSTANT_3D;
					dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_3D;
					dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_3D;
					xsv_ext1 = xsb - 1;
					ysv_ext1 = ysb + 1;
					zsv_ext1 = zsb + 1;
				} else if ((c & 0x02) == 0) {
					dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_3D;
					dy_ext1 = dy0 + 1 - SQUISH_CONSTANT_3D;
					dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_3D;
					xsv_ext1 = xsb + 1;
					ysv_ext1 = ysb - 1;
					zsv_ext1 = zsb + 1;
				} else {
					dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_3D;
					dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_3D;
					dz_ext1 = dz0 + 1 - SQUISH_CONSTANT_3D;
					xsv_ext1 = xsb + 1;
					ysv_ext1 = ysb + 1;
					zsv_ext1 = zsb - 1;
				}
			}
		} else { /* One point on (0,0,0) side, one point on (1,1,1) side */
			if (aIsFurtherSide) {
				c1 = aPoint;
				c2 = bPoint;
			} else {
				c1 = bPoint;
				c2 = aPoint;
			}

			/* One contribution is a permutation of (1,1,-1) */
			if ((c1 & 0x01) == 0) {
				dx_ext0 = dx0 + 1 - SQUISH_CONSTANT_3D;
				dy_ext0 = dy0 - 1 - SQUISH_CONSTANT_3D;
				dz_ext0 = dz0 - 1 - SQUISH_CONSTANT_3D;
				xsv_ext0 = xsb - 1;
				ysv_ext0 = ysb + 1;
				zsv_ext0 = zsb + 1;
			} else if ((c1 & 0x02) == 0) {
				dx_ext0 = dx0 - 1 - SQUISH_CONSTANT_3D;
				dy_ext0 = dy0 + 1 - SQUISH_CONSTANT_3D;
				dz_ext0 = dz0 - 1 - SQUISH_CONSTANT_3D;
				xsv_ext0 = xsb + 1;
				ysv_ext0 = ysb - 1;
				zsv_ext0 = zsb + 1;
			} else {
				dx_ext0 = dx0 - 1 - SQUISH_CONSTANT_3D;
				dy_ext0 = dy0 - 1 - SQUISH_CONSTANT_3D;
				dz_ext0 = dz0 + 1 - SQUISH_CONSTANT_3D;
				xsv_ext0 = xsb + 1;
				ysv_ext0 = ysb + 1;
				zsv_ext0 = zsb - 1;
			}

			/* One contribution is a permutation of (0,0,2) */
			dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_3D;
			dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_3D;
			dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_3D;
			xsv_ext1 = xsb;
			ysv_ext1 = ysb;
			zsv_ext1 = zsb;
			if ((c2 & 0x01) != 0) {
				dx_ext1 -= 2;
				xsv_ext1 += 2;
			} else if ((c2 & 0x02) != 0) {
				dy_ext1 -= 2;
				ysv_ext1 += 2;
			} else {
				dz_ext1 -= 2;
				zsv_ext1 += 2;
			}
		}

		/* Contribution (1,0,0) */
		dx1 = dx0 - 1 - SQUISH_CONSTANT_3D;
		dy1 = dy0 - 0 - SQUISH_CONSTANT_3D;
		dz1 = dz0 - 0 - SQUISH_CONSTANT_3D;
		attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1;
		if (attn1 > 0) {
			attn1 *= attn1;
			value += attn1 * attn1 * extrapolate3(ctx, xsb + 1, ysb + 0, zsb + 0, dx1, dy1, dz1);
		}

		/* Contribution (0,1,0) */
		dx2 = dx0 - 0 - SQUISH_CONSTANT_3D;
		dy2 = dy0 - 1 - SQUISH_CONSTANT_3D;
		dz2 = dz1;
		attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2;
		if (attn2 > 0) {
			attn2 *= attn2;
			value += attn2 * attn2 * extrapolate3(ctx, xsb + 0, ysb + 1, zsb + 0, dx2, dy2, dz2);
		}

		/* Contribution (0,0,1) */
		dx3 = dx2;
		dy3 = dy1;
		dz3 = dz0 - 1 - SQUISH_CONSTANT_3D;
		attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3;
		if (attn3 > 0) {
			attn3 *= attn3;
			value += attn3 * attn3 * extrapolate3(ctx, xsb + 0, ysb + 0, zsb + 1, dx3, dy3, dz3);
		}

		/* Contribution (1,1,0) */
		dx4 = dx0 - 1 - 2 * SQUISH_CONSTANT_3D;
		dy4 = dy0 - 1 - 2 * SQUISH_CONSTANT_3D;
		dz4 = dz0 - 0 - 2 * SQUISH_CONSTANT_3D;
		attn4 = 2 - dx4 * dx4 - dy4 * dy4 - dz4 * dz4;
		if (attn4 > 0) {
			attn4 *= attn4;
			value += attn4 * attn4 * extrapolate3(ctx, xsb + 1, ysb + 1, zsb + 0, dx4, dy4, dz4);
		}

		/* Contribution (1,0,1) */
		dx5 = dx4;
		dy5 = dy0 - 0 - 2 * SQUISH_CONSTANT_3D;
		dz5 = dz0 - 1 - 2 * SQUISH_CONSTANT_3D;
		attn5 = 2 - dx5 * dx5 - dy5 * dy5 - dz5 * dz5;
		if (attn5 > 0) {
			attn5 *= attn5;
			value += attn5 * attn5 * extrapolate3(ctx, xsb + 1, ysb + 0, zsb + 1, dx5, dy5, dz5);
		}

		/* Contribution (0,1,1) */
		dx6 = dx0 - 0 - 2 * SQUISH_CONSTANT_3D;
		dy6 = dy4;
		dz6 = dz5;
		attn6 = 2 - dx6 * dx6 - dy6 * dy6 - dz6 * dz6;
		if (attn6 > 0) {
			attn6 *= attn6;
			value += attn6 * attn6 * extrapolate3(ctx, xsb + 0, ysb + 1, zsb + 1, dx6, dy6, dz6);
		}
	}

	/* First extra vertex */
	attn_ext0 = 2 - dx_ext0 * dx_ext0 - dy_ext0 * dy_ext0 - dz_ext0 * dz_ext0;
	if (attn_ext0 > 0)
	{
		attn_ext0 *= attn_ext0;
		value += attn_ext0 * attn_ext0 * extrapolate3(ctx, xsv_ext0, ysv_ext0, zsv_ext0, dx_ext0, dy_ext0, dz_ext0);
	}

	/* Second extra vertex */
	attn_ext1 = 2 - dx_ext1 * dx_ext1 - dy_ext1 * dy_ext1 - dz_ext1 * dz_ext1;
	if (attn_ext1 > 0)
	{
		attn_ext1 *= attn_ext1;
		value += attn_ext1 * attn_ext1 * extrapolate3(ctx, xsv_ext1, ysv_ext1, zsv_ext1, dx_ext1, dy_ext1, dz_ext1);
	}
	
	return value / NORM_CONSTANT_3D;
}
	
/* 
 * 4D OpenSimplex (Simplectic) Noise.
 */
OSNFLOAT open_simplex_noise4(const struct osn_context *ctx, OSNFLOAT x, OSNFLOAT y, OSNFLOAT z, OSNFLOAT w)
{
	OSNFLOAT uins;
	OSNFLOAT dx1, dy1, dz1, dw1;
	OSNFLOAT dx2, dy2, dz2, dw2;
	OSNFLOAT dx3, dy3, dz3, dw3;
	OSNFLOAT dx4, dy4, dz4, dw4;
	OSNFLOAT dx5, dy5, dz5, dw5;
	OSNFLOAT dx6, dy6, dz6, dw6;
	OSNFLOAT dx7, dy7, dz7, dw7;
	OSNFLOAT dx8, dy8, dz8, dw8;
	OSNFLOAT dx9, dy9, dz9, dw9;
	OSNFLOAT dx10, dy10, dz10, dw10;
	OSNFLOAT attn0, attn1, attn2, attn3, attn4;
	OSNFLOAT attn5, attn6, attn7, attn8, attn9, attn10;
	OSNFLOAT attn_ext0, attn_ext1, attn_ext2;
	int8_t c, c1, c2;
	int8_t aPoint, bPoint;
	OSNFLOAT aScore, bScore;
	int aIsBiggerSide;
	int bIsBiggerSide;
	OSNFLOAT p1, p2, p3, p4;
	OSNFLOAT score;

	/* Place input coordinates on simplectic honeycomb. */
	OSNFLOAT stretchOffset = (x + y + z + w) * STRETCH_CONSTANT_4D;
	OSNFLOAT xs = x + stretchOffset;
	OSNFLOAT ys = y + stretchOffset;
	OSNFLOAT zs = z + stretchOffset;
	OSNFLOAT ws = w + stretchOffset;
	
	/* Floor to get simplectic honeycomb coordinates of rhombo-hypercube super-cell origin. */
	int xsb = fastFloor(xs);
	int ysb = fastFloor(ys);
	int zsb = fastFloor(zs);
	int wsb = fastFloor(ws);
	
	/* Skew out to get actual coordinates of stretched rhombo-hypercube origin. We'll need these later. */
	OSNFLOAT squishOffset = (xsb + ysb + zsb + wsb) * SQUISH_CONSTANT_4D;
	OSNFLOAT xb = xsb + squishOffset;
	OSNFLOAT yb = ysb + squishOffset;
	OSNFLOAT zb = zsb + squishOffset;
	OSNFLOAT wb = wsb + squishOffset;
	
	/* Compute simplectic honeycomb coordinates relative to rhombo-hypercube origin. */
	OSNFLOAT xins = xs - xsb;
	OSNFLOAT yins = ys - ysb;
	OSNFLOAT zins = zs - zsb;
	OSNFLOAT wins = ws - wsb;
	
	/* Sum those together to get a value that determines which region we're in. */
	OSNFLOAT inSum = xins + yins + zins + wins;

	/* Positions relative to origin point. */
	OSNFLOAT dx0 = x - xb;
	OSNFLOAT dy0 = y - yb;
	OSNFLOAT dz0 = z - zb;
	OSNFLOAT dw0 = w - wb;
	
	/* We'll be defining these inside the next block and using them afterwards. */
	OSNFLOAT dx_ext0, dy_ext0, dz_ext0, dw_ext0;
	OSNFLOAT dx_ext1, dy_ext1, dz_ext1, dw_ext1;
	OSNFLOAT dx_ext2, dy_ext2, dz_ext2, dw_ext2;
	int xsv_ext0, ysv_ext0, zsv_ext0, wsv_ext0;
	int xsv_ext1, ysv_ext1, zsv_ext1, wsv_ext1;
	int xsv_ext2, ysv_ext2, zsv_ext2, wsv_ext2;
	
	OSNFLOAT value = 0;
	if (inSum <= 1) { /* We're inside the pentachoron (4-Simplex) at (0,0,0,0) */

		/* Determine which two of (0,0,0,1), (0,0,1,0), (0,1,0,0), (1,0,0,0) are closest. */
		aPoint = 0x01;
		aScore = xins;
		bPoint = 0x02;
		bScore = yins;
		if (aScore >= bScore && zins > bScore) {
			bScore = zins;
			bPoint = 0x04;
		} else if (aScore < bScore && zins > aScore) {
			aScore = zins;
			aPoint = 0x04;
		}
		if (aScore >= bScore && wins > bScore) {
			bScore = wins;
			bPoint = 0x08;
		} else if (aScore < bScore && wins > aScore) {
			aScore = wins;
			aPoint = 0x08;
		}
		
		/* Now we determine the three lattice points not part of the pentachoron that may contribute.
		   This depends on the closest two pentachoron vertices, including (0,0,0,0) */
		uins = 1 - inSum;
		if (uins > aScore || uins > bScore) { /* (0,0,0,0) is one of the closest two pentachoron vertices. */
			c = (bScore > aScore ? bPoint : aPoint); /* Our other closest vertex is the closest out of a and b. */
			if ((c & 0x01) == 0) {
				xsv_ext0 = xsb - 1;
				xsv_ext1 = xsv_ext2 = xsb;
				dx_ext0 = dx0 + 1;
				dx_ext1 = dx_ext2 = dx0;
			} else {
				xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb + 1;
				dx_ext0 = dx_ext1 = dx_ext2 = dx0 - 1;
			}

			if ((c & 0x02) == 0) {
				ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
				dy_ext0 = dy_ext1 = dy_ext2 = dy0;
				if ((c & 0x01) == 0x01) {
					ysv_ext0 -= 1;
					dy_ext0 += 1;
				} else {
					ysv_ext1 -= 1;
					dy_ext1 += 1;
				}
			} else {
				ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
				dy_ext0 = dy_ext1 = dy_ext2 = dy0 - 1;
			}
			
			if ((c & 0x04) == 0) {
				zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
				dz_ext0 = dz_ext1 = dz_ext2 = dz0;
				if ((c & 0x03) != 0) {
					if ((c & 0x03) == 0x03) {
						zsv_ext0 -= 1;
						dz_ext0 += 1;
					} else {
						zsv_ext1 -= 1;
						dz_ext1 += 1;
					}
				} else {
					zsv_ext2 -= 1;
					dz_ext2 += 1;
				}
			} else {
				zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
				dz_ext0 = dz_ext1 = dz_ext2 = dz0 - 1;
			}
			
			if ((c & 0x08) == 0) {
				wsv_ext0 = wsv_ext1 = wsb;
				wsv_ext2 = wsb - 1;
				dw_ext0 = dw_ext1 = dw0;
				dw_ext2 = dw0 + 1;
			} else {
				wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb + 1;
				dw_ext0 = dw_ext1 = dw_ext2 = dw0 - 1;
			}
		} else { /* (0,0,0,0) is not one of the closest two pentachoron vertices. */
			c = (int8_t)(aPoint | bPoint); /* Our three extra vertices are determined by the closest two. */
			
			if ((c & 0x01) == 0) {
				xsv_ext0 = xsv_ext2 = xsb;
				xsv_ext1 = xsb - 1;
				dx_ext0 = dx0 - 2 * SQUISH_CONSTANT_4D;
				dx_ext1 = dx0 + 1 - SQUISH_CONSTANT_4D;
				dx_ext2 = dx0 - SQUISH_CONSTANT_4D;
			} else {
				xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb + 1;
				dx_ext0 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
				dx_ext1 = dx_ext2 = dx0 - 1 - SQUISH_CONSTANT_4D;
			}
			
			if ((c & 0x02) == 0) {
				ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
				dy_ext0 = dy0 - 2 * SQUISH_CONSTANT_4D;
				dy_ext1 = dy_ext2 = dy0 - SQUISH_CONSTANT_4D;
				if ((c & 0x01) == 0x01) {
					ysv_ext1 -= 1;
					dy_ext1 += 1;
				} else {
					ysv_ext2 -= 1;
					dy_ext2 += 1;
				}
			} else {
				ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
				dy_ext0 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
				dy_ext1 = dy_ext2 = dy0 - 1 - SQUISH_CONSTANT_4D;
			}
			
			if ((c & 0x04) == 0) {
				zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
				dz_ext0 = dz0 - 2 * SQUISH_CONSTANT_4D;
				dz_ext1 = dz_ext2 = dz0 - SQUISH_CONSTANT_4D;
				if ((c & 0x03) == 0x03) {
					zsv_ext1 -= 1;
					dz_ext1 += 1;
				} else {
					zsv_ext2 -= 1;
					dz_ext2 += 1;
				}
			} else {
				zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
				dz_ext0 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
				dz_ext1 = dz_ext2 = dz0 - 1 - SQUISH_CONSTANT_4D;
			}
			
			if ((c & 0x08) == 0) {
				wsv_ext0 = wsv_ext1 = wsb;
				wsv_ext2 = wsb - 1;
				dw_ext0 = dw0 - 2 * SQUISH_CONSTANT_4D;
				dw_ext1 = dw0 - SQUISH_CONSTANT_4D;
				dw_ext2 = dw0 + 1 - SQUISH_CONSTANT_4D;
			} else {
				wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb + 1;
				dw_ext0 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
				dw_ext1 = dw_ext2 = dw0 - 1 - SQUISH_CONSTANT_4D;
			}
		}

		/* Contribution (0,0,0,0) */
		attn0 = 2 - dx0 * dx0 - dy0 * dy0 - dz0 * dz0 - dw0 * dw0;
		if (attn0 > 0) {
			attn0 *= attn0;
			value += attn0 * attn0 * extrapolate4(ctx, xsb + 0, ysb + 0, zsb + 0, wsb + 0, dx0, dy0, dz0, dw0);
		}

		/* Contribution (1,0,0,0) */
		dx1 = dx0 - 1 - SQUISH_CONSTANT_4D;
		dy1 = dy0 - 0 - SQUISH_CONSTANT_4D;
		dz1 = dz0 - 0 - SQUISH_CONSTANT_4D;
		dw1 = dw0 - 0 - SQUISH_CONSTANT_4D;
		attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1 - dw1 * dw1;
		if (attn1 > 0) {
			attn1 *= attn1;
			value += attn1 * attn1 * extrapolate4(ctx, xsb + 1, ysb + 0, zsb + 0, wsb + 0, dx1, dy1, dz1, dw1);
		}

		/* Contribution (0,1,0,0) */
		dx2 = dx0 - 0 - SQUISH_CONSTANT_4D;
		dy2 = dy0 - 1 - SQUISH_CONSTANT_4D;
		dz2 = dz1;
		dw2 = dw1;
		attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2 - dw2 * dw2;
		if (attn2 > 0) {
			attn2 *= attn2;
			value += attn2 * attn2 * extrapolate4(ctx, xsb + 0, ysb + 1, zsb + 0, wsb + 0, dx2, dy2, dz2, dw2);
		}

		/* Contribution (0,0,1,0) */
		dx3 = dx2;
		dy3 = dy1;
		dz3 = dz0 - 1 - SQUISH_CONSTANT_4D;
		dw3 = dw1;
		attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3 - dw3 * dw3;
		if (attn3 > 0) {
			attn3 *= attn3;
			value += attn3 * attn3 * extrapolate4(ctx, xsb + 0, ysb + 0, zsb + 1, wsb + 0, dx3, dy3, dz3, dw3);
		}

		/* Contribution (0,0,0,1) */
		dx4 = dx2;
		dy4 = dy1;
		dz4 = dz1;
		dw4 = dw0 - 1 - SQUISH_CONSTANT_4D;
		attn4 = 2 - dx4 * dx4 - dy4 * dy4 - dz4 * dz4 - dw4 * dw4;
		if (attn4 > 0) {
			attn4 *= attn4;
			value += attn4 * attn4 * extrapolate4(ctx, xsb + 0, ysb + 0, zsb + 0, wsb + 1, dx4, dy4, dz4, dw4);
		}
	} else if (inSum >= 3) { /* We're inside the pentachoron (4-Simplex) at (1,1,1,1)
		Determine which two of (1,1,1,0), (1,1,0,1), (1,0,1,1), (0,1,1,1) are closest. */
		aPoint = 0x0E;
		aScore = xins;
		bPoint = 0x0D;
		bScore = yins;
		if (aScore <= bScore && zins < bScore) {
			bScore = zins;
			bPoint = 0x0B;
		} else if (aScore > bScore && zins < aScore) {
			aScore = zins;
			aPoint = 0x0B;
		}
		if (aScore <= bScore && wins < bScore) {
			bScore = wins;
			bPoint = 0x07;
		} else if (aScore > bScore && wins < aScore) {
			aScore = wins;
			aPoint = 0x07;
		}
		
		/* Now we determine the three lattice points not part of the pentachoron that may contribute.
		   This depends on the closest two pentachoron vertices, including (0,0,0,0) */
		uins = 4 - inSum;
		if (uins < aScore || uins < bScore) { /* (1,1,1,1) is one of the closest two pentachoron vertices. */
			c = (bScore < aScore ? bPoint : aPoint); /* Our other closest vertex is the closest out of a and b. */
			
			if ((c & 0x01) != 0) {
				xsv_ext0 = xsb + 2;
				xsv_ext1 = xsv_ext2 = xsb + 1;
				dx_ext0 = dx0 - 2 - 4 * SQUISH_CONSTANT_4D;
				dx_ext1 = dx_ext2 = dx0 - 1 - 4 * SQUISH_CONSTANT_4D;
			} else {
				xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb;
				dx_ext0 = dx_ext1 = dx_ext2 = dx0 - 4 * SQUISH_CONSTANT_4D;
			}

			if ((c & 0x02) != 0) {
				ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
				dy_ext0 = dy_ext1 = dy_ext2 = dy0 - 1 - 4 * SQUISH_CONSTANT_4D;
				if ((c & 0x01) != 0) {
					ysv_ext1 += 1;
					dy_ext1 -= 1;
				} else {
					ysv_ext0 += 1;
					dy_ext0 -= 1;
				}
			} else {
				ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
				dy_ext0 = dy_ext1 = dy_ext2 = dy0 - 4 * SQUISH_CONSTANT_4D;
			}
			
			if ((c & 0x04) != 0) {
				zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
				dz_ext0 = dz_ext1 = dz_ext2 = dz0 - 1 - 4 * SQUISH_CONSTANT_4D;
				if ((c & 0x03) != 0x03) {
					if ((c & 0x03) == 0) {
						zsv_ext0 += 1;
						dz_ext0 -= 1;
					} else {
						zsv_ext1 += 1;
						dz_ext1 -= 1;
					}
				} else {
					zsv_ext2 += 1;
					dz_ext2 -= 1;
				}
			} else {
				zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
				dz_ext0 = dz_ext1 = dz_ext2 = dz0 - 4 * SQUISH_CONSTANT_4D;
			}
			
			if ((c & 0x08) != 0) {
				wsv_ext0 = wsv_ext1 = wsb + 1;
				wsv_ext2 = wsb + 2;
				dw_ext0 = dw_ext1 = dw0 - 1 - 4 * SQUISH_CONSTANT_4D;
				dw_ext2 = dw0 - 2 - 4 * SQUISH_CONSTANT_4D;
			} else {
				wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb;
				dw_ext0 = dw_ext1 = dw_ext2 = dw0 - 4 * SQUISH_CONSTANT_4D;
			}
		} else { /* (1,1,1,1) is not one of the closest two pentachoron vertices. */
			c = (int8_t)(aPoint & bPoint); /* Our three extra vertices are determined by the closest two. */
			
			if ((c & 0x01) != 0) {
				xsv_ext0 = xsv_ext2 = xsb + 1;
				xsv_ext1 = xsb + 2;
				dx_ext0 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
				dx_ext1 = dx0 - 2 - 3 * SQUISH_CONSTANT_4D;
				dx_ext2 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
			} else {
				xsv_ext0 = xsv_ext1 = xsv_ext2 = xsb;
				dx_ext0 = dx0 - 2 * SQUISH_CONSTANT_4D;
				dx_ext1 = dx_ext2 = dx0 - 3 * SQUISH_CONSTANT_4D;
			}
			
			if ((c & 0x02) != 0) {
				ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb + 1;
				dy_ext0 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
				dy_ext1 = dy_ext2 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
				if ((c & 0x01) != 0) {
					ysv_ext2 += 1;
					dy_ext2 -= 1;
				} else {
					ysv_ext1 += 1;
					dy_ext1 -= 1;
				}
			} else {
				ysv_ext0 = ysv_ext1 = ysv_ext2 = ysb;
				dy_ext0 = dy0 - 2 * SQUISH_CONSTANT_4D;
				dy_ext1 = dy_ext2 = dy0 - 3 * SQUISH_CONSTANT_4D;
			}
			
			if ((c & 0x04) != 0) {
				zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb + 1;
				dz_ext0 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
				dz_ext1 = dz_ext2 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
				if ((c & 0x03) != 0) {
					zsv_ext2 += 1;
					dz_ext2 -= 1;
				} else {
					zsv_ext1 += 1;
					dz_ext1 -= 1;
				}
			} else {
				zsv_ext0 = zsv_ext1 = zsv_ext2 = zsb;
				dz_ext0 = dz0 - 2 * SQUISH_CONSTANT_4D;
				dz_ext1 = dz_ext2 = dz0 - 3 * SQUISH_CONSTANT_4D;
			}
			
			if ((c & 0x08) != 0) {
				wsv_ext0 = wsv_ext1 = wsb + 1;
				wsv_ext2 = wsb + 2;
				dw_ext0 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
				dw_ext1 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
				dw_ext2 = dw0 - 2 - 3 * SQUISH_CONSTANT_4D;
			} else {
				wsv_ext0 = wsv_ext1 = wsv_ext2 = wsb;
				dw_ext0 = dw0 - 2 * SQUISH_CONSTANT_4D;
				dw_ext1 = dw_ext2 = dw0 - 3 * SQUISH_CONSTANT_4D;
			}
		}

		/* Contribution (1,1,1,0) */
		dx4 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
		dy4 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
		dz4 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
		dw4 = dw0 - 3 * SQUISH_CONSTANT_4D;
		attn4 = 2 - dx4 * dx4 - dy4 * dy4 - dz4 * dz4 - dw4 * dw4;
		if (attn4 > 0) {
			attn4 *= attn4;
			value += attn4 * attn4 * extrapolate4(ctx, xsb + 1, ysb + 1, zsb + 1, wsb + 0, dx4, dy4, dz4, dw4);
		}

		/* Contribution (1,1,0,1) */
		dx3 = dx4;
		dy3 = dy4;
		dz3 = dz0 - 3 * SQUISH_CONSTANT_4D;
		dw3 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
		attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3 - dw3 * dw3;
		if (attn3 > 0) {
			attn3 *= attn3;
			value += attn3 * attn3 * extrapolate4(ctx, xsb + 1, ysb + 1, zsb + 0, wsb + 1, dx3, dy3, dz3, dw3);
		}

		/* Contribution (1,0,1,1) */
		dx2 = dx4;
		dy2 = dy0 - 3 * SQUISH_CONSTANT_4D;
		dz2 = dz4;
		dw2 = dw3;
		attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2 - dw2 * dw2;
		if (attn2 > 0) {
			attn2 *= attn2;
			value += attn2 * attn2 * extrapolate4(ctx, xsb + 1, ysb + 0, zsb + 1, wsb + 1, dx2, dy2, dz2, dw2);
		}

		/* Contribution (0,1,1,1) */
		dx1 = dx0 - 3 * SQUISH_CONSTANT_4D;
		dz1 = dz4;
		dy1 = dy4;
		dw1 = dw3;
		attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1 - dw1 * dw1;
		if (attn1 > 0) {
			attn1 *= attn1;
			value += attn1 * attn1 * extrapolate4(ctx, xsb + 0, ysb + 1, zsb + 1, wsb + 1, dx1, dy1, dz1, dw1);
		}

		/* Contribution (1,1,1,1) */
		dx0 = dx0 - 1 - 4 * SQUISH_CONSTANT_4D;
		dy0 = dy0 - 1 - 4 * SQUISH_CONSTANT_4D;
		dz0 = dz0 - 1 - 4 * SQUISH_CONSTANT_4D;
		dw0 = dw0 - 1 - 4 * SQUISH_CONSTANT_4D;
		attn0 = 2 - dx0 * dx0 - dy0 * dy0 - dz0 * dz0 - dw0 * dw0;
		if (attn0 > 0) {
			attn0 *= attn0;
			value += attn0 * attn0 * extrapolate4(ctx, xsb + 1, ysb + 1, zsb + 1, wsb + 1, dx0, dy0, dz0, dw0);
		}
	} else if (inSum <= 2) { /* We're inside the first dispentachoron (Rectified 4-Simplex) */
		aIsBiggerSide = 1;
		bIsBiggerSide = 1;
		
		/* Decide between (1,1,0,0) and (0,0,1,1) */
		if (xins + yins > zins + wins) {
			aScore = xins + yins;
			aPoint = 0x03;
		} else {
			aScore = zins + wins;
			aPoint = 0x0C;
		}
		
		/* Decide between (1,0,1,0) and (0,1,0,1) */
		if (xins + zins > yins + wins) {
			bScore = xins + zins;
			bPoint = 0x05;
		} else {
			bScore = yins + wins;
			bPoint = 0x0A;
		}
		
		/* Closer between (1,0,0,1) and (0,1,1,0) will replace the further of a and b, if closer. */
		if (xins + wins > yins + zins) {
			score = xins + wins;
			if (aScore >= bScore && score > bScore) {
				bScore = score;
				bPoint = 0x09;
			} else if (aScore < bScore && score > aScore) {
				aScore = score;
				aPoint = 0x09;
			}
		} else {
			score = yins + zins;
			if (aScore >= bScore && score > bScore) {
				bScore = score;
				bPoint = 0x06;
			} else if (aScore < bScore && score > aScore) {
				aScore = score;
				aPoint = 0x06;
			}
		}
		
		/* Decide if (1,0,0,0) is closer. */
		p1 = 2 - inSum + xins;
		if (aScore >= bScore && p1 > bScore) {
			bScore = p1;
			bPoint = 0x01;
			bIsBiggerSide = 0;
		} else if (aScore < bScore && p1 > aScore) {
			aScore = p1;
			aPoint = 0x01;
			aIsBiggerSide = 0;
		}
		
		/* Decide if (0,1,0,0) is closer. */
		p2 = 2 - inSum + yins;
		if (aScore >= bScore && p2 > bScore) {
			bScore = p2;
			bPoint = 0x02;
			bIsBiggerSide = 0;
		} else if (aScore < bScore && p2 > aScore) {
			aScore = p2;
			aPoint = 0x02;
			aIsBiggerSide = 0;
		}
		
		/* Decide if (0,0,1,0) is closer. */
		p3 = 2 - inSum + zins;
		if (aScore >= bScore && p3 > bScore) {
			bScore = p3;
			bPoint = 0x04;
			bIsBiggerSide = 0;
		} else if (aScore < bScore && p3 > aScore) {
			aScore = p3;
			aPoint = 0x04;
			aIsBiggerSide = 0;
		}
		
		/* Decide if (0,0,0,1) is closer. */
		p4 = 2 - inSum + wins;
		if (aScore >= bScore && p4 > bScore) {
			// bScore = p4; dead store
			bPoint = 0x08;
			bIsBiggerSide = 0;
		} else if (aScore < bScore && p4 > aScore) {
			// aScore = p4; dead store
			aPoint = 0x08;
			aIsBiggerSide = 0;
		}
		
		/* Where each of the two closest points are determines how the extra three vertices are calculated. */
		if (aIsBiggerSide == bIsBiggerSide) {
			if (aIsBiggerSide) { /* Both closest points on the bigger side */
				c1 = (int8_t)(aPoint | bPoint);
				c2 = (int8_t)(aPoint & bPoint);
				if ((c1 & 0x01) == 0) {
					xsv_ext0 = xsb;
					xsv_ext1 = xsb - 1;
					dx_ext0 = dx0 - 3 * SQUISH_CONSTANT_4D;
					dx_ext1 = dx0 + 1 - 2 * SQUISH_CONSTANT_4D;
				} else {
					xsv_ext0 = xsv_ext1 = xsb + 1;
					dx_ext0 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
					dx_ext1 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
				}
				
				if ((c1 & 0x02) == 0) {
					ysv_ext0 = ysb;
					ysv_ext1 = ysb - 1;
					dy_ext0 = dy0 - 3 * SQUISH_CONSTANT_4D;
					dy_ext1 = dy0 + 1 - 2 * SQUISH_CONSTANT_4D;
				} else {
					ysv_ext0 = ysv_ext1 = ysb + 1;
					dy_ext0 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
					dy_ext1 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
				}
				
				if ((c1 & 0x04) == 0) {
					zsv_ext0 = zsb;
					zsv_ext1 = zsb - 1;
					dz_ext0 = dz0 - 3 * SQUISH_CONSTANT_4D;
					dz_ext1 = dz0 + 1 - 2 * SQUISH_CONSTANT_4D;
				} else {
					zsv_ext0 = zsv_ext1 = zsb + 1;
					dz_ext0 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
					dz_ext1 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
				}
				
				if ((c1 & 0x08) == 0) {
					wsv_ext0 = wsb;
					wsv_ext1 = wsb - 1;
					dw_ext0 = dw0 - 3 * SQUISH_CONSTANT_4D;
					dw_ext1 = dw0 + 1 - 2 * SQUISH_CONSTANT_4D;
				} else {
					wsv_ext0 = wsv_ext1 = wsb + 1;
					dw_ext0 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
					dw_ext1 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
				}
				
				/* One combination is a permutation of (0,0,0,2) based on c2 */
				xsv_ext2 = xsb;
				ysv_ext2 = ysb;
				zsv_ext2 = zsb;
				wsv_ext2 = wsb;
				dx_ext2 = dx0 - 2 * SQUISH_CONSTANT_4D;
				dy_ext2 = dy0 - 2 * SQUISH_CONSTANT_4D;
				dz_ext2 = dz0 - 2 * SQUISH_CONSTANT_4D;
				dw_ext2 = dw0 - 2 * SQUISH_CONSTANT_4D;
				if ((c2 & 0x01) != 0) {
					xsv_ext2 += 2;
					dx_ext2 -= 2;
				} else if ((c2 & 0x02) != 0) {
					ysv_ext2 += 2;
					dy_ext2 -= 2;
				} else if ((c2 & 0x04) != 0) {
					zsv_ext2 += 2;
					dz_ext2 -= 2;
				} else {
					wsv_ext2 += 2;
					dw_ext2 -= 2;
				}
				
			} else { /* Both closest points on the smaller side */
				/* One of the two extra points is (0,0,0,0) */
				xsv_ext2 = xsb;
				ysv_ext2 = ysb;
				zsv_ext2 = zsb;
				wsv_ext2 = wsb;
				dx_ext2 = dx0;
				dy_ext2 = dy0;
				dz_ext2 = dz0;
				dw_ext2 = dw0;
				
				/* Other two points are based on the omitted axes. */
				c = (int8_t)(aPoint | bPoint);
				
				if ((c & 0x01) == 0) {
					xsv_ext0 = xsb - 1;
					xsv_ext1 = xsb;
					dx_ext0 = dx0 + 1 - SQUISH_CONSTANT_4D;
					dx_ext1 = dx0 - SQUISH_CONSTANT_4D;
				} else {
					xsv_ext0 = xsv_ext1 = xsb + 1;
					dx_ext0 = dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_4D;
				}
				
				if ((c & 0x02) == 0) {
					ysv_ext0 = ysv_ext1 = ysb;
					dy_ext0 = dy_ext1 = dy0 - SQUISH_CONSTANT_4D;
					if ((c & 0x01) == 0x01)
					{
						ysv_ext0 -= 1;
						dy_ext0 += 1;
					} else {
						ysv_ext1 -= 1;
						dy_ext1 += 1;
					}
				} else {
					ysv_ext0 = ysv_ext1 = ysb + 1;
					dy_ext0 = dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_4D;
				}
				
				if ((c & 0x04) == 0) {
					zsv_ext0 = zsv_ext1 = zsb;
					dz_ext0 = dz_ext1 = dz0 - SQUISH_CONSTANT_4D;
					if ((c & 0x03) == 0x03)
					{
						zsv_ext0 -= 1;
						dz_ext0 += 1;
					} else {
						zsv_ext1 -= 1;
						dz_ext1 += 1;
					}
				} else {
					zsv_ext0 = zsv_ext1 = zsb + 1;
					dz_ext0 = dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_4D;
				}
				
				if ((c & 0x08) == 0)
				{
					wsv_ext0 = wsb;
					wsv_ext1 = wsb - 1;
					dw_ext0 = dw0 - SQUISH_CONSTANT_4D;
					dw_ext1 = dw0 + 1 - SQUISH_CONSTANT_4D;
				} else {
					wsv_ext0 = wsv_ext1 = wsb + 1;
					dw_ext0 = dw_ext1 = dw0 - 1 - SQUISH_CONSTANT_4D;
				}
				
			}
		} else { /* One point on each "side" */
			if (aIsBiggerSide) {
				c1 = aPoint;
				c2 = bPoint;
			} else {
				c1 = bPoint;
				c2 = aPoint;
			}
			
			/* Two contributions are the bigger-sided point with each 0 replaced with -1. */
			if ((c1 & 0x01) == 0) {
				xsv_ext0 = xsb - 1;
				xsv_ext1 = xsb;
				dx_ext0 = dx0 + 1 - SQUISH_CONSTANT_4D;
				dx_ext1 = dx0 - SQUISH_CONSTANT_4D;
			} else {
				xsv_ext0 = xsv_ext1 = xsb + 1;
				dx_ext0 = dx_ext1 = dx0 - 1 - SQUISH_CONSTANT_4D;
			}
			
			if ((c1 & 0x02) == 0) {
				ysv_ext0 = ysv_ext1 = ysb;
				dy_ext0 = dy_ext1 = dy0 - SQUISH_CONSTANT_4D;
				if ((c1 & 0x01) == 0x01) {
					ysv_ext0 -= 1;
					dy_ext0 += 1;
				} else {
					ysv_ext1 -= 1;
					dy_ext1 += 1;
				}
			} else {
				ysv_ext0 = ysv_ext1 = ysb + 1;
				dy_ext0 = dy_ext1 = dy0 - 1 - SQUISH_CONSTANT_4D;
			}
			
			if ((c1 & 0x04) == 0) {
				zsv_ext0 = zsv_ext1 = zsb;
				dz_ext0 = dz_ext1 = dz0 - SQUISH_CONSTANT_4D;
				if ((c1 & 0x03) == 0x03) {
					zsv_ext0 -= 1;
					dz_ext0 += 1;
				} else {
					zsv_ext1 -= 1;
					dz_ext1 += 1;
				}
			} else {
				zsv_ext0 = zsv_ext1 = zsb + 1;
				dz_ext0 = dz_ext1 = dz0 - 1 - SQUISH_CONSTANT_4D;
			}
			
			if ((c1 & 0x08) == 0) {
				wsv_ext0 = wsb;
				wsv_ext1 = wsb - 1;
				dw_ext0 = dw0 - SQUISH_CONSTANT_4D;
				dw_ext1 = dw0 + 1 - SQUISH_CONSTANT_4D;
			} else {
				wsv_ext0 = wsv_ext1 = wsb + 1;
				dw_ext0 = dw_ext1 = dw0 - 1 - SQUISH_CONSTANT_4D;
			}

			/* One contribution is a permutation of (0,0,0,2) based on the smaller-sided point */
			xsv_ext2 = xsb;
			ysv_ext2 = ysb;
			zsv_ext2 = zsb;
			wsv_ext2 = wsb;
			dx_ext2 = dx0 - 2 * SQUISH_CONSTANT_4D;
			dy_ext2 = dy0 - 2 * SQUISH_CONSTANT_4D;
			dz_ext2 = dz0 - 2 * SQUISH_CONSTANT_4D;
			dw_ext2 = dw0 - 2 * SQUISH_CONSTANT_4D;
			if ((c2 & 0x01) != 0) {
				xsv_ext2 += 2;
				dx_ext2 -= 2;
			} else if ((c2 & 0x02) != 0) {
				ysv_ext2 += 2;
				dy_ext2 -= 2;
			} else if ((c2 & 0x04) != 0) {
				zsv_ext2 += 2;
				dz_ext2 -= 2;
			} else {
				wsv_ext2 += 2;
				dw_ext2 -= 2;
			}
		}
		
		/* Contribution (1,0,0,0) */
		dx1 = dx0 - 1 - SQUISH_CONSTANT_4D;
		dy1 = dy0 - 0 - SQUISH_CONSTANT_4D;
		dz1 = dz0 - 0 - SQUISH_CONSTANT_4D;
		dw1 = dw0 - 0 - SQUISH_CONSTANT_4D;
		attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1 - dw1 * dw1;
		if (attn1 > 0) {
			attn1 *= attn1;
			value += attn1 * attn1 * extrapolate4(ctx, xsb + 1, ysb + 0, zsb + 0, wsb + 0, dx1, dy1, dz1, dw1);
		}

		/* Contribution (0,1,0,0) */
		dx2 = dx0 - 0 - SQUISH_CONSTANT_4D;
		dy2 = dy0 - 1 - SQUISH_CONSTANT_4D;
		dz2 = dz1;
		dw2 = dw1;
		attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2 - dw2 * dw2;
		if (attn2 > 0) {
			attn2 *= attn2;
			value += attn2 * attn2 * extrapolate4(ctx, xsb + 0, ysb + 1, zsb + 0, wsb + 0, dx2, dy2, dz2, dw2);
		}

		/* Contribution (0,0,1,0) */
		dx3 = dx2;
		dy3 = dy1;
		dz3 = dz0 - 1 - SQUISH_CONSTANT_4D;
		dw3 = dw1;
		attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3 - dw3 * dw3;
		if (attn3 > 0) {
			attn3 *= attn3;
			value += attn3 * attn3 * extrapolate4(ctx, xsb + 0, ysb + 0, zsb + 1, wsb + 0, dx3, dy3, dz3, dw3);
		}

		/* Contribution (0,0,0,1) */
		dx4 = dx2;
		dy4 = dy1;
		dz4 = dz1;
		dw4 = dw0 - 1 - SQUISH_CONSTANT_4D;
		attn4 = 2 - dx4 * dx4 - dy4 * dy4 - dz4 * dz4 - dw4 * dw4;
		if (attn4 > 0) {
			attn4 *= attn4;
			value += attn4 * attn4 * extrapolate4(ctx, xsb + 0, ysb + 0, zsb + 0, wsb + 1, dx4, dy4, dz4, dw4);
		}
		
		/* Contribution (1,1,0,0) */
		dx5 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dy5 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dz5 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dw5 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
		attn5 = 2 - dx5 * dx5 - dy5 * dy5 - dz5 * dz5 - dw5 * dw5;
		if (attn5 > 0) {
			attn5 *= attn5;
			value += attn5 * attn5 * extrapolate4(ctx, xsb + 1, ysb + 1, zsb + 0, wsb + 0, dx5, dy5, dz5, dw5);
		}
		
		/* Contribution (1,0,1,0) */
		dx6 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dy6 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dz6 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dw6 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
		attn6 = 2 - dx6 * dx6 - dy6 * dy6 - dz6 * dz6 - dw6 * dw6;
		if (attn6 > 0) {
			attn6 *= attn6;
			value += attn6 * attn6 * extrapolate4(ctx, xsb + 1, ysb + 0, zsb + 1, wsb + 0, dx6, dy6, dz6, dw6);
		}

		/* Contribution (1,0,0,1) */
		dx7 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dy7 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dz7 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dw7 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
		attn7 = 2 - dx7 * dx7 - dy7 * dy7 - dz7 * dz7 - dw7 * dw7;
		if (attn7 > 0) {
			attn7 *= attn7;
			value += attn7 * attn7 * extrapolate4(ctx, xsb + 1, ysb + 0, zsb + 0, wsb + 1, dx7, dy7, dz7, dw7);
		}
		
		/* Contribution (0,1,1,0) */
		dx8 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dy8 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dz8 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dw8 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
		attn8 = 2 - dx8 * dx8 - dy8 * dy8 - dz8 * dz8 - dw8 * dw8;
		if (attn8 > 0) {
			attn8 *= attn8;
			value += attn8 * attn8 * extrapolate4(ctx, xsb + 0, ysb + 1, zsb + 1, wsb + 0, dx8, dy8, dz8, dw8);
		}
		
		/* Contribution (0,1,0,1) */
		dx9 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dy9 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dz9 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dw9 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
		attn9 = 2 - dx9 * dx9 - dy9 * dy9 - dz9 * dz9 - dw9 * dw9;
		if (attn9 > 0) {
			attn9 *= attn9;
			value += attn9 * attn9 * extrapolate4(ctx, xsb + 0, ysb + 1, zsb + 0, wsb + 1, dx9, dy9, dz9, dw9);
		}
		
		/* Contribution (0,0,1,1) */
		dx10 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dy10 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dz10 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dw10 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
		attn10 = 2 - dx10 * dx10 - dy10 * dy10 - dz10 * dz10 - dw10 * dw10;
		if (attn10 > 0) {
			attn10 *= attn10;
			value += attn10 * attn10 * extrapolate4(ctx, xsb + 0, ysb + 0, zsb + 1, wsb + 1, dx10, dy10, dz10, dw10);
		}
	} else { /* We're inside the second dispentachoron (Rectified 4-Simplex) */
		aIsBiggerSide = 1;
		bIsBiggerSide = 1;
		
		/* Decide between (0,0,1,1) and (1,1,0,0) */
		if (xins + yins < zins + wins) {
			aScore = xins + yins;
			aPoint = 0x0C;
		} else {
			aScore = zins + wins;
			aPoint = 0x03;
		}
		
		/* Decide between (0,1,0,1) and (1,0,1,0) */
		if (xins + zins < yins + wins) {
			bScore = xins + zins;
			bPoint = 0x0A;
		} else {
			bScore = yins + wins;
			bPoint = 0x05;
		}
		
		/* Closer between (0,1,1,0) and (1,0,0,1) will replace the further of a and b, if closer. */
		if (xins + wins < yins + zins) {
			score = xins + wins;
			if (aScore <= bScore && score < bScore) {
				bScore = score;
				bPoint = 0x06;
			} else if (aScore > bScore && score < aScore) {
				aScore = score;
				aPoint = 0x06;
			}
		} else {
			score = yins + zins;
			if (aScore <= bScore && score < bScore) {
				bScore = score;
				bPoint = 0x09;
			} else if (aScore > bScore && score < aScore) {
				aScore = score;
				aPoint = 0x09;
			}
		}
		
		/* Decide if (0,1,1,1) is closer. */
		p1 = 3 - inSum + xins;
		if (aScore <= bScore && p1 < bScore) {
			bScore = p1;
			bPoint = 0x0E;
			bIsBiggerSide = 0;
		} else if (aScore > bScore && p1 < aScore) {
			aScore = p1;
			aPoint = 0x0E;
			aIsBiggerSide = 0;
		}
		
		/* Decide if (1,0,1,1) is closer. */
		p2 = 3 - inSum + yins;
		if (aScore <= bScore && p2 < bScore) {
			bScore = p2;
			bPoint = 0x0D;
			bIsBiggerSide = 0;
		} else if (aScore > bScore && p2 < aScore) {
			aScore = p2;
			aPoint = 0x0D;
			aIsBiggerSide = 0;
		}
		
		/* Decide if (1,1,0,1) is closer. */
		p3 = 3 - inSum + zins;
		if (aScore <= bScore && p3 < bScore) {
			bScore = p3;
			bPoint = 0x0B;
			bIsBiggerSide = 0;
		} else if (aScore > bScore && p3 < aScore) {
			aScore = p3;
			aPoint = 0x0B;
			aIsBiggerSide = 0;
		}
		
		/* Decide if (1,1,1,0) is closer. */
		p4 = 3 - inSum + wins;
		if (aScore <= bScore && p4 < bScore) {
			// bScore = p4; dead store
			bPoint = 0x07;
			bIsBiggerSide = 0;
		} else if (aScore > bScore && p4 < aScore) {
			// aScore = p4; dead store
			aPoint = 0x07;
			aIsBiggerSide = 0;
		}
		
		/* Where each of the two closest points are determines how the extra three vertices are calculated. */
		if (aIsBiggerSide == bIsBiggerSide) {
			if (aIsBiggerSide) { /* Both closest points on the bigger side */
				c1 = (int8_t)(aPoint & bPoint);
				c2 = (int8_t)(aPoint | bPoint);
				
				/* Two contributions are permutations of (0,0,0,1) and (0,0,0,2) based on c1 */
				xsv_ext0 = xsv_ext1 = xsb;
				ysv_ext0 = ysv_ext1 = ysb;
				zsv_ext0 = zsv_ext1 = zsb;
				wsv_ext0 = wsv_ext1 = wsb;
				dx_ext0 = dx0 - SQUISH_CONSTANT_4D;
				dy_ext0 = dy0 - SQUISH_CONSTANT_4D;
				dz_ext0 = dz0 - SQUISH_CONSTANT_4D;
				dw_ext0 = dw0 - SQUISH_CONSTANT_4D;
				dx_ext1 = dx0 - 2 * SQUISH_CONSTANT_4D;
				dy_ext1 = dy0 - 2 * SQUISH_CONSTANT_4D;
				dz_ext1 = dz0 - 2 * SQUISH_CONSTANT_4D;
				dw_ext1 = dw0 - 2 * SQUISH_CONSTANT_4D;
				if ((c1 & 0x01) != 0) {
					xsv_ext0 += 1;
					dx_ext0 -= 1;
					xsv_ext1 += 2;
					dx_ext1 -= 2;
				} else if ((c1 & 0x02) != 0) {
					ysv_ext0 += 1;
					dy_ext0 -= 1;
					ysv_ext1 += 2;
					dy_ext1 -= 2;
				} else if ((c1 & 0x04) != 0) {
					zsv_ext0 += 1;
					dz_ext0 -= 1;
					zsv_ext1 += 2;
					dz_ext1 -= 2;
				} else {
					wsv_ext0 += 1;
					dw_ext0 -= 1;
					wsv_ext1 += 2;
					dw_ext1 -= 2;
				}
				
				/* One contribution is a permutation of (1,1,1,-1) based on c2 */
				xsv_ext2 = xsb + 1;
				ysv_ext2 = ysb + 1;
				zsv_ext2 = zsb + 1;
				wsv_ext2 = wsb + 1;
				dx_ext2 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
				dy_ext2 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
				dz_ext2 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
				dw_ext2 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
				if ((c2 & 0x01) == 0) {
					xsv_ext2 -= 2;
					dx_ext2 += 2;
				} else if ((c2 & 0x02) == 0) {
					ysv_ext2 -= 2;
					dy_ext2 += 2;
				} else if ((c2 & 0x04) == 0) {
					zsv_ext2 -= 2;
					dz_ext2 += 2;
				} else {
					wsv_ext2 -= 2;
					dw_ext2 += 2;
				}
			} else { /* Both closest points on the smaller side */
				/* One of the two extra points is (1,1,1,1) */
				xsv_ext2 = xsb + 1;
				ysv_ext2 = ysb + 1;
				zsv_ext2 = zsb + 1;
				wsv_ext2 = wsb + 1;
				dx_ext2 = dx0 - 1 - 4 * SQUISH_CONSTANT_4D;
				dy_ext2 = dy0 - 1 - 4 * SQUISH_CONSTANT_4D;
				dz_ext2 = dz0 - 1 - 4 * SQUISH_CONSTANT_4D;
				dw_ext2 = dw0 - 1 - 4 * SQUISH_CONSTANT_4D;
				
				/* Other two points are based on the shared axes. */
				c = (int8_t)(aPoint & bPoint);
				
				if ((c & 0x01) != 0) {
					xsv_ext0 = xsb + 2;
					xsv_ext1 = xsb + 1;
					dx_ext0 = dx0 - 2 - 3 * SQUISH_CONSTANT_4D;
					dx_ext1 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
				} else {
					xsv_ext0 = xsv_ext1 = xsb;
					dx_ext0 = dx_ext1 = dx0 - 3 * SQUISH_CONSTANT_4D;
				}
				
				if ((c & 0x02) != 0) {
					ysv_ext0 = ysv_ext1 = ysb + 1;
					dy_ext0 = dy_ext1 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
					if ((c & 0x01) == 0)
					{
						ysv_ext0 += 1;
						dy_ext0 -= 1;
					} else {
						ysv_ext1 += 1;
						dy_ext1 -= 1;
					}
				} else {
					ysv_ext0 = ysv_ext1 = ysb;
					dy_ext0 = dy_ext1 = dy0 - 3 * SQUISH_CONSTANT_4D;
				}
				
				if ((c & 0x04) != 0) {
					zsv_ext0 = zsv_ext1 = zsb + 1;
					dz_ext0 = dz_ext1 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
					if ((c & 0x03) == 0)
					{
						zsv_ext0 += 1;
						dz_ext0 -= 1;
					} else {
						zsv_ext1 += 1;
						dz_ext1 -= 1;
					}
				} else {
					zsv_ext0 = zsv_ext1 = zsb;
					dz_ext0 = dz_ext1 = dz0 - 3 * SQUISH_CONSTANT_4D;
				}
				
				if ((c & 0x08) != 0)
				{
					wsv_ext0 = wsb + 1;
					wsv_ext1 = wsb + 2;
					dw_ext0 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
					dw_ext1 = dw0 - 2 - 3 * SQUISH_CONSTANT_4D;
				} else {
					wsv_ext0 = wsv_ext1 = wsb;
					dw_ext0 = dw_ext1 = dw0 - 3 * SQUISH_CONSTANT_4D;
				}
			}
		} else { /* One point on each "side" */
			if (aIsBiggerSide) {
				c1 = aPoint;
				c2 = bPoint;
			} else {
				c1 = bPoint;
				c2 = aPoint;
			}
			
			/* Two contributions are the bigger-sided point with each 1 replaced with 2. */
			if ((c1 & 0x01) != 0) {
				xsv_ext0 = xsb + 2;
				xsv_ext1 = xsb + 1;
				dx_ext0 = dx0 - 2 - 3 * SQUISH_CONSTANT_4D;
				dx_ext1 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
			} else {
				xsv_ext0 = xsv_ext1 = xsb;
				dx_ext0 = dx_ext1 = dx0 - 3 * SQUISH_CONSTANT_4D;
			}
			
			if ((c1 & 0x02) != 0) {
				ysv_ext0 = ysv_ext1 = ysb + 1;
				dy_ext0 = dy_ext1 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
				if ((c1 & 0x01) == 0) {
					ysv_ext0 += 1;
					dy_ext0 -= 1;
				} else {
					ysv_ext1 += 1;
					dy_ext1 -= 1;
				}
			} else {
				ysv_ext0 = ysv_ext1 = ysb;
				dy_ext0 = dy_ext1 = dy0 - 3 * SQUISH_CONSTANT_4D;
			}
			
			if ((c1 & 0x04) != 0) {
				zsv_ext0 = zsv_ext1 = zsb + 1;
				dz_ext0 = dz_ext1 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
				if ((c1 & 0x03) == 0) {
					zsv_ext0 += 1;
					dz_ext0 -= 1;
				} else {
					zsv_ext1 += 1;
					dz_ext1 -= 1;
				}
			} else {
				zsv_ext0 = zsv_ext1 = zsb;
				dz_ext0 = dz_ext1 = dz0 - 3 * SQUISH_CONSTANT_4D;
			}
			
			if ((c1 & 0x08) != 0) {
				wsv_ext0 = wsb + 1;
				wsv_ext1 = wsb + 2;
				dw_ext0 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
				dw_ext1 = dw0 - 2 - 3 * SQUISH_CONSTANT_4D;
			} else {
				wsv_ext0 = wsv_ext1 = wsb;
				dw_ext0 = dw_ext1 = dw0 - 3 * SQUISH_CONSTANT_4D;
			}

			/* One contribution is a permutation of (1,1,1,-1) based on the smaller-sided point */
			xsv_ext2 = xsb + 1;
			ysv_ext2 = ysb + 1;
			zsv_ext2 = zsb + 1;
			wsv_ext2 = wsb + 1;
			dx_ext2 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
			dy_ext2 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
			dz_ext2 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
			dw_ext2 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
			if ((c2 & 0x01) == 0) {
				xsv_ext2 -= 2;
				dx_ext2 += 2;
			} else if ((c2 & 0x02) == 0) {
				ysv_ext2 -= 2;
				dy_ext2 += 2;
			} else if ((c2 & 0x04) == 0) {
				zsv_ext2 -= 2;
				dz_ext2 += 2;
			} else {
				wsv_ext2 -= 2;
				dw_ext2 += 2;
			}
		}
		
		/* Contribution (1,1,1,0) */
		dx4 = dx0 - 1 - 3 * SQUISH_CONSTANT_4D;
		dy4 = dy0 - 1 - 3 * SQUISH_CONSTANT_4D;
		dz4 = dz0 - 1 - 3 * SQUISH_CONSTANT_4D;
		dw4 = dw0 - 3 * SQUISH_CONSTANT_4D;
		attn4 = 2 - dx4 * dx4 - dy4 * dy4 - dz4 * dz4 - dw4 * dw4;
		if (attn4 > 0) {
			attn4 *= attn4;
			value += attn4 * attn4 * extrapolate4(ctx, xsb + 1, ysb + 1, zsb + 1, wsb + 0, dx4, dy4, dz4, dw4);
		}

		/* Contribution (1,1,0,1) */
		dx3 = dx4;
		dy3 = dy4;
		dz3 = dz0 - 3 * SQUISH_CONSTANT_4D;
		dw3 = dw0 - 1 - 3 * SQUISH_CONSTANT_4D;
		attn3 = 2 - dx3 * dx3 - dy3 * dy3 - dz3 * dz3 - dw3 * dw3;
		if (attn3 > 0) {
			attn3 *= attn3;
			value += attn3 * attn3 * extrapolate4(ctx, xsb + 1, ysb + 1, zsb + 0, wsb + 1, dx3, dy3, dz3, dw3);
		}

		/* Contribution (1,0,1,1) */
		dx2 = dx4;
		dy2 = dy0 - 3 * SQUISH_CONSTANT_4D;
		dz2 = dz4;
		dw2 = dw3;
		attn2 = 2 - dx2 * dx2 - dy2 * dy2 - dz2 * dz2 - dw2 * dw2;
		if (attn2 > 0) {
			attn2 *= attn2;
			value += attn2 * attn2 * extrapolate4(ctx, xsb + 1, ysb + 0, zsb + 1, wsb + 1, dx2, dy2, dz2, dw2);
		}

		/* Contribution (0,1,1,1) */
		dx1 = dx0 - 3 * SQUISH_CONSTANT_4D;
		dz1 = dz4;
		dy1 = dy4;
		dw1 = dw3;
		attn1 = 2 - dx1 * dx1 - dy1 * dy1 - dz1 * dz1 - dw1 * dw1;
		if (attn1 > 0) {
			attn1 *= attn1;
			value += attn1 * attn1 * extrapolate4(ctx, xsb + 0, ysb + 1, zsb + 1, wsb + 1, dx1, dy1, dz1, dw1);
		}
		
		/* Contribution (1,1,0,0) */
		dx5 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dy5 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dz5 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dw5 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
		attn5 = 2 - dx5 * dx5 - dy5 * dy5 - dz5 * dz5 - dw5 * dw5;
		if (attn5 > 0) {
			attn5 *= attn5;
			value += attn5 * attn5 * extrapolate4(ctx, xsb + 1, ysb + 1, zsb + 0, wsb + 0, dx5, dy5, dz5, dw5);
		}
		
		/* Contribution (1,0,1,0) */
		dx6 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dy6 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dz6 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dw6 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
		attn6 = 2 - dx6 * dx6 - dy6 * dy6 - dz6 * dz6 - dw6 * dw6;
		if (attn6 > 0) {
			attn6 *= attn6;
			value += attn6 * attn6 * extrapolate4(ctx, xsb + 1, ysb + 0, zsb + 1, wsb + 0, dx6, dy6, dz6, dw6);
		}

		/* Contribution (1,0,0,1) */
		dx7 = dx0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dy7 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dz7 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dw7 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
		attn7 = 2 - dx7 * dx7 - dy7 * dy7 - dz7 * dz7 - dw7 * dw7;
		if (attn7 > 0) {
			attn7 *= attn7;
			value += attn7 * attn7 * extrapolate4(ctx, xsb + 1, ysb + 0, zsb + 0, wsb + 1, dx7, dy7, dz7, dw7);
		}
		
		/* Contribution (0,1,1,0) */
		dx8 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dy8 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dz8 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dw8 = dw0 - 0 - 2 * SQUISH_CONSTANT_4D;
		attn8 = 2 - dx8 * dx8 - dy8 * dy8 - dz8 * dz8 - dw8 * dw8;
		if (attn8 > 0) {
			attn8 *= attn8;
			value += attn8 * attn8 * extrapolate4(ctx, xsb + 0, ysb + 1, zsb + 1, wsb + 0, dx8, dy8, dz8, dw8);
		}
		
		/* Contribution (0,1,0,1) */
		dx9 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dy9 = dy0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dz9 = dz0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dw9 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
		attn9 = 2 - dx9 * dx9 - dy9 * dy9 - dz9 * dz9 - dw9 * dw9;
		if (attn9 > 0) {
			attn9 *= attn9;
			value += attn9 * attn9 * extrapolate4(ctx, xsb + 0, ysb + 1, zsb + 0, wsb + 1, dx9, dy9, dz9, dw9);
		}
		
		/* Contribution (0,0,1,1) */
		dx10 = dx0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dy10 = dy0 - 0 - 2 * SQUISH_CONSTANT_4D;
		dz10 = dz0 - 1 - 2 * SQUISH_CONSTANT_4D;
		dw10 = dw0 - 1 - 2 * SQUISH_CONSTANT_4D;
		attn10 = 2 - dx10 * dx10 - dy10 * dy10 - dz10 * dz10 - dw10 * dw10;
		if (attn10 > 0) {
			attn10 *= attn10;
			value += attn10 * attn10 * extrapolate4(ctx, xsb + 0, ysb + 0, zsb + 1, wsb + 1, dx10, dy10, dz10, dw10);
		}
	}

	/* First extra vertex */
	attn_ext0 = 2 - dx_ext0 * dx_ext0 - dy_ext0 * dy_ext0 - dz_ext0 * dz_ext0 - dw_ext0 * dw_ext0;
	if (attn_ext0 > 0)
	{
		attn_ext0 *= attn_ext0;
		value += attn_ext0 * attn_ext0 * extrapolate4(ctx, xsv_ext0, ysv_ext0, zsv_ext0, wsv_ext0, dx_ext0, dy_ext0, dz_ext0, dw_ext0);
	}

	/* Second extra vertex */
	attn_ext1 = 2 - dx_ext1 * dx_ext1 - dy_ext1 * dy_ext1 - dz_ext1 * dz_ext1 - dw_ext1 * dw_ext1;
	if (attn_ext1 > 0)
	{
		attn_ext1 *= attn_ext1;
		value += attn_ext1 * attn_ext1 * extrapolate4(ctx, xsv_ext1, ysv_ext1, zsv_ext1, wsv_ext1, dx_ext1, dy_ext1, dz_ext1, dw_ext1);
	}

	/* Third extra vertex */
	attn_ext2 = 2 - dx_ext2 * dx_ext2 - dy_ext2 * dy_ext2 - dz_ext2 * dz_ext2 - dw_ext2 * dw_ext2;
	if (attn_ext2 > 0)
	{
		attn_ext2 *= attn_ext2;
		value += attn_ext2 * attn_ext2 * extrapolate4(ctx, xsv_ext2, ysv_ext2, zsv_ext2, wsv_ext2, dx_ext2, dy_ext2, dz_ext2, dw_ext2);
	}

	return value / NORM_CONSTANT_4D;
}
	

//==================================================================
//
// This here is pasted the public domain rnd.h library from
// mattiasgustavsson.
//
// Thanks, mate!
//
#define  RND_IMPLEMENTATION
/*
------------------------------------------------------------------------------
          Licensing information can be found at the end of the file.
------------------------------------------------------------------------------

rnd.h - v1.0 - Pseudo-random number generators for C/C++.

Do this:
    #define RND_IMPLEMENTATION
before you include this file in *one* C/C++ file to create the implementation.

Additional Contributors
  Jonatan Hedborg: unsigned int to normalized float conversion
*/

#ifndef rnd_h
#define rnd_h

#ifndef RND_U32
    #define RND_U32 unsigned int
#endif
#ifndef RND_U64
    #define RND_U64 unsigned long long
#endif

typedef struct rnd_pcg_t { RND_U64 state[ 2 ]; } rnd_pcg_t;
void rnd_pcg_seed( rnd_pcg_t* pcg, RND_U32 seed );
RND_U32 rnd_pcg_next( rnd_pcg_t* pcg );
float rnd_pcg_nextf( rnd_pcg_t* pcg );
int rnd_pcg_range( rnd_pcg_t* pcg, int min, int max );

typedef struct rnd_well_t { RND_U32 state[ 17 ]; } rnd_well_t;
void rnd_well_seed( rnd_well_t* well, RND_U32 seed );
RND_U32 rnd_well_next( rnd_well_t* well );
float rnd_well_nextf( rnd_well_t* well );
int rnd_well_range( rnd_well_t* well, int min, int max );

typedef struct rnd_gamerand_t { RND_U32 state[ 2 ]; } rnd_gamerand_t;
void rnd_gamerand_seed( rnd_gamerand_t* gamerand, RND_U32 seed );
RND_U32 rnd_gamerand_next( rnd_gamerand_t* gamerand );
float rnd_gamerand_nextf( rnd_gamerand_t* gamerand );
int rnd_gamerand_range( rnd_gamerand_t* gamerand, int min, int max );

typedef struct rnd_xorshift_t { RND_U64 state[ 2 ]; } rnd_xorshift_t;
void rnd_xorshift_seed( rnd_xorshift_t* xorshift, RND_U64 seed );
RND_U64 rnd_xorshift_next( rnd_xorshift_t* xorshift );
float rnd_xorshift_nextf( rnd_xorshift_t* xorshift );
int rnd_xorshift_range( rnd_xorshift_t* xorshift, int min, int max );

#endif /* rnd_h */

/**

rnd.h
=====

Pseudo-random number generators for C/C++.


Example
-------

A basic example showing how to use the PCG set of random functions.

    #define  RND_IMPLEMENTATION
    #include "rnd.h"

    #include <stdio.h> // for printf
    #include <time.h> // for time
    
    int main( int argc, char** argv ) {

        rnd_pcg_t pcg;
        rnd_pcg_seed( &pcg, 0u ); // initialize generator

        // print a handful of random integers
        // these will be the same on every run, as we 
        // seeded the rng with a fixed value
        for( int i = 0; i < 5; ++i ) {
            RND_U32 n = rnd_pcg_next( &pcg );
            printf( "%08x, ", n );
        }
        printf( "\n" );

        // reseed with a value which is different on each run
        time_t seconds;
        time( &seconds );
        rnd_pcg_seed( &pcg, (RND_U32) seconds ); 

        // print another handful of random integers
        // these will be different on every run
        for( int i = 0; i < 5; ++i ) {
            RND_U32 n = rnd_pcg_next( &pcg );
            printf( "%08x, ", n );
        }
        printf( "\n" );


        // print a handful of random floats
        for( int i = 0; i < 5; ++i ) {
            float f = rnd_pcg_nextf( &pcg );
            printf( "%f, ", f );
        }
        printf( "\n" );

        // print random integers in the range 1 to 6
        for( int i = 0; i < 15; ++i ) {
            int r = rnd_pcg_range( &pcg, 1, 6 );
            printf( "%d, ", r );
        }
        printf( "\n" );

        return 0;
    }


API Documentation
-----------------

rnd.h is a single-header library, and does not need any .lib files or other binaries, or any build scripts. To use it,
you just include rnd.h to get the API declarations. To get the definitions, you must include rnd.h from *one* single C 
or C++ file, and #define the symbol `RND_IMPLEMENTATION` before you do. 

The library is meant for general-purpose use, such as games and similar apps. It is not meant to be used for 
cryptography and similar use cases.


### Customization

rnd.h allows for specifying the exact type of 32 and 64 bit unsigned integers to be used in its API. By default, these
default to `unsigned int` and `unsigned long long`, but can be redefined by #defining RND_U32 and RND_U64 respectively
before including rnd.h. This is useful if you, for example, use the types from `<stdint.h>` in the rest of your program, 
and you want rnd.h to use compatible types. In this case, you would include rnd.h using the following code:

    #define RND_U32 uint32_t
    #define RND_U64 uint64_t
    #include "rnd.h"

Note that when customizing the data type, you need to use the same definition in every place where you include rnd.h, 
as it affect the declarations as well as the definitions.


### The generators

The library includes four different generators: PCG, WELL, GameRand and XorShift. They all have different 
characteristics, and you might want to use them for different things. GameRand is very fast, but does not give a great
distribution or period length. XorShift is the only one returning a 64-bit value. WELL is an improvement of the often
used Mersenne Twister, and has quite a large internal state. PCG is small, fast and has a small state. If you don't
have any specific reason, you may default to using PCG.

All generators expose their internal state, so it is possible to save this state and later restore it, to resume the 
random sequence from the same point.


#### PCG - Permuted Congruential Generator

PCG is a family of simple fast space-efficient statistically good algorithms for random number generation. Unlike many 
general-purpose RNGs, they are also hard to predict.

More information can be found here: 

http://www.pcg-random.org/


#### WELL - Well Equidistributed Long-period Linear

Random number generation, using the WELL algorithm by F. Panneton, P. L'Ecuyer and M. Matsumoto.
More information in the original paper: 

http://www.iro.umontreal.ca/~panneton/WELLRNG.html

This code is originally based on WELL512 C/C++ code written by Chris Lomont (published in Game Programming Gems 7) 
and placed in the public domain. 

http://lomont.org/Math/Papers/2008/Lomont_PRNG_2008.pdf


#### GameRand

Based on the random number generator by Ian C. Bullard:

http://www.redditmirror.cc/cache/websites/mjolnirstudios.com_7yjlc/mjolnirstudios.com/IanBullard/files/79ffbca75a75720f066d491e9ea935a0-10.html

GameRand is a random number generator based off an "Image of the Day" posted by Stephan Schaem. More information here:

http://www.flipcode.com/archives/07-15-2002.shtml


#### XorShift 

A random number generator of the type LFSR (linear feedback shift registers). This specific implementation uses the
XorShift+ variation, and returns 64-bit random numbers.

More information can be found here: 

https://en.wikipedia.org/wiki/Xorshift



rnd_pcg_seed
------------

    void rnd_pcg_seed( rnd_pcg_t* pcg, RND_U32 seed )

Initialize a PCG generator with the specified seed. The generator is not valid until it's been seeded.


rnd_pcg_next
------------

    RND_U32 rnd_pcg_next( rnd_pcg_t* pcg )

Returns a random number N in the range: 0 <= N <= 0xffffffff, from the specified PCG generator.


rnd_pcg_nextf
-------------

    float rnd_pcg_nextf( rnd_pcg_t* pcg )

Returns a random float X in the range: 0.0f <= X < 1.0f, from the specified PCG generator.


rnd_pcg_range
-------------

    int rnd_pcg_range( rnd_pcg_t* pcg, int min, int max )

Returns a random integer N in the range: min <= N <= max, from the specified PCG generator.


rnd_well_seed
-------------

    void rnd_well_seed( rnd_well_t* well, RND_U32 seed )

Initialize a WELL generator with the specified seed. The generator is not valid until it's been seeded.


rnd_well_next
-------------

    RND_U32 rnd_well_next( rnd_well_t* well )

Returns a random number N in the range: 0 <= N <= 0xffffffff, from the specified WELL generator.


rnd_well_nextf
--------------
    float rnd_well_nextf( rnd_well_t* well )

Returns a random float X in the range: 0.0f <= X < 1.0f, from the specified WELL generator.


rnd_well_range
--------------

    int rnd_well_range( rnd_well_t* well, int min, int max )

Returns a random integer N in the range: min <= N <= max, from the specified WELL generator.


rnd_gamerand_seed
-----------------

    void rnd_gamerand_seed( rnd_gamerand_t* gamerand, RND_U32 seed )

Initialize a GameRand generator with the specified seed. The generator is not valid until it's been seeded.


rnd_gamerand_next
-----------------

    RND_U32 rnd_gamerand_next( rnd_gamerand_t* gamerand )

Returns a random number N in the range: 0 <= N <= 0xffffffff, from the specified GameRand generator.


rnd_gamerand_nextf
------------------

    float rnd_gamerand_nextf( rnd_gamerand_t* gamerand )

Returns a random float X in the range: 0.0f <= X < 1.0f, from the specified GameRand generator.


rnd_gamerand_range
------------------

    int rnd_gamerand_range( rnd_gamerand_t* gamerand, int min, int max )

Returns a random integer N in the range: min <= N <= max, from the specified GameRand generator.


rnd_xorshift_seed
-----------------

    void rnd_xorshift_seed( rnd_xorshift_t* xorshift, RND_U64 seed )

Initialize a XorShift generator with the specified seed. The generator is not valid until it's been seeded.


rnd_xorshift_next
-----------------

    RND_U64 rnd_xorshift_next( rnd_xorshift_t* xorshift )

Returns a random number N in the range: 0 <= N <= 0xffffffffffffffff, from the specified XorShift generator.


rnd_xorshift_nextf
------------------

    float rnd_xorshift_nextf( rnd_xorshift_t* xorshift )

Returns a random float X in the range: 0.0f <= X < 1.0f, from the specified XorShift generator.


rnd_xorshift_range
------------------

    int rnd_xorshift_range( rnd_xorshift_t* xorshift, int min, int max )

Returns a random integer N in the range: min <= N <= max, from the specified XorShift generator.


*/


/*
----------------------
    IMPLEMENTATION
----------------------
*/

#ifdef RND_IMPLEMENTATION
#undef RND_IMPLEMENTATION

// Convert a randomized RND_U32 value to a float value x in the range 0.0f <= x < 1.0f. Contributed by Jonatan Hedborg
static float rnd_internal_float_normalized_from_uint32( RND_U32 value )
    {
    RND_U32 exponent = 127;
    RND_U32 mantissa = value >> 9;
    RND_U32 result = ( exponent << 23 ) | mantissa;
    float fresult = *(float*)( &result );
    return fresult - 1.0f;
    }


static RND_U32 rnd_internal_murmur3_avalanche32( RND_U32 h )
    {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
    }


static RND_U64 rnd_internal_murmur3_avalanche64( RND_U64 h )
    {
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    h ^= h >> 33;
    return h;
    }


void rnd_pcg_seed( rnd_pcg_t* pcg, RND_U32 seed )
    {
    RND_U64 value = ( ( (RND_U64) seed ) << 1ULL ) | 1ULL;
    value = rnd_internal_murmur3_avalanche64( value );
    pcg->state[ 0 ] = 0U;
    pcg->state[ 1 ] = ( value << 1ULL ) | 1ULL;
    rnd_pcg_next( pcg );
    pcg->state[ 0 ] += rnd_internal_murmur3_avalanche64( value );
    rnd_pcg_next( pcg );
    }


RND_U32 rnd_pcg_next( rnd_pcg_t* pcg )
    {
    RND_U64 oldstate = pcg->state[ 0 ];
    pcg->state[ 0 ] = oldstate * 0x5851f42d4c957f2dULL + pcg->state[ 1 ];
    {
      RND_U32 xorshifted = (RND_U32)( ( ( oldstate >> 18ULL)  ^ oldstate ) >> 27ULL );
      RND_U32 rot = (RND_U32)( oldstate >> 59ULL );
      return ( xorshifted >> rot ) | ( xorshifted << ( ( -(int) rot ) & 31 ) );
    }
    }


float rnd_pcg_nextf( rnd_pcg_t* pcg )
    {
    return rnd_internal_float_normalized_from_uint32( rnd_pcg_next( pcg ) );
    }


int rnd_pcg_range( rnd_pcg_t* pcg, int min, int max )
    {
    int const range = ( max - min ) + 1;
    if( range <= 0 ) return min;
    {
      int const value = (int) ( rnd_pcg_nextf( pcg ) * range );
      return min + value; 
    }
    }


void rnd_well_seed( rnd_well_t* well, RND_U32 seed )
    {
    RND_U32 value = rnd_internal_murmur3_avalanche32( ( seed << 1U ) | 1U );
    int i;
    well->state[ 16 ] = 0;
    well->state[ 0 ] = value ^ 0xf68a9fc1U;
    for( i = 1; i < 16; ++i ) 
        well->state[ i ] = ( 0x6c078965U * ( well->state[ i - 1 ] ^ ( well->state[ i - 1 ] >> 30 ) ) + i ); 
    }


RND_U32 rnd_well_next( rnd_well_t* well )
    {
    RND_U32 a = well->state[ well->state[ 16 ] ];
    RND_U32 c = well->state[ ( well->state[ 16 ] + 13 ) & 15 ];
    RND_U32 b = a ^ c ^ ( a << 16 ) ^ ( c << 15 );
    c = well->state[ ( well->state[ 16 ] + 9 ) & 15 ];
    c ^= ( c >> 11 );
    a = well->state[ well->state[ 16 ] ] = b ^ c;
    {
      RND_U32 d = a ^ ( ( a << 5 ) & 0xda442d24U );
      well->state[ 16 ] = (well->state[ 16 ] + 15 ) & 15;
      a = well->state[ well->state[ 16 ] ];
      well->state[ well->state[ 16 ] ] = a ^ b ^ d ^ ( a << 2 ) ^ ( b << 18 ) ^ ( c << 28 );
      return well->state[ well->state[ 16 ] ];
    }
    }


float rnd_well_nextf( rnd_well_t* well )
    {
    return rnd_internal_float_normalized_from_uint32( rnd_well_next( well ) );
    }


int rnd_well_range( rnd_well_t* well, int min, int max )
    {
    int const range = ( max - min ) + 1;
    if( range <= 0 ) return min;
    {
      int const value = (int) ( rnd_well_nextf( well ) * range );
      return min + value; 
    }
    }


void rnd_gamerand_seed( rnd_gamerand_t* gamerand, RND_U32 seed )
    {
    RND_U32 value = rnd_internal_murmur3_avalanche32( ( seed << 1U ) | 1U );
    gamerand->state[ 0 ] = value;
    gamerand->state[ 1 ] = value ^ 0x49616e42U;
    }


RND_U32 rnd_gamerand_next( rnd_gamerand_t* gamerand )
    {
    gamerand->state[ 0 ] = ( gamerand->state[ 0 ] << 16 ) + ( gamerand->state[ 0 ] >> 16 );
    gamerand->state[ 0 ] += gamerand->state[ 1 ];
    gamerand->state[ 1 ] += gamerand->state[ 0 ];
    return gamerand->state[ 0 ];
    }


float rnd_gamerand_nextf( rnd_gamerand_t* gamerand )
    {
    return rnd_internal_float_normalized_from_uint32( rnd_gamerand_next( gamerand ) );
    }


int rnd_gamerand_range( rnd_gamerand_t* gamerand, int min, int max )
    {
    int const range = ( max - min ) + 1;
    if( range <= 0 ) return min;
    {
      int const value = (int) ( rnd_gamerand_nextf( gamerand ) * range );
      return min + value; 
    }
    }


void rnd_xorshift_seed( rnd_xorshift_t* xorshift, RND_U64 seed )
    {
    RND_U64 value = rnd_internal_murmur3_avalanche64( ( seed << 1ULL ) | 1ULL );
    xorshift->state[ 0 ] = value;
    value = rnd_internal_murmur3_avalanche64( value );
    xorshift->state[ 1 ] = value;
    }


RND_U64 rnd_xorshift_next( rnd_xorshift_t* xorshift )
    {
    RND_U64 x = xorshift->state[ 0 ];
    RND_U64 const y = xorshift->state[ 1 ];
    xorshift->state[ 0 ] = y;
    x ^= x << 23;
    x ^= x >> 17;
    x ^= y ^ ( y >> 26 );
    xorshift->state[ 1 ] = x;
    return x + y;
    }


float rnd_xorshift_nextf( rnd_xorshift_t* xorshift )
    {
    return rnd_internal_float_normalized_from_uint32( (RND_U32)( rnd_xorshift_next( xorshift ) >> 32 ) );
    }


int rnd_xorshift_range( rnd_xorshift_t* xorshift, int min, int max )
    {
    int const range = ( max - min ) + 1;
    if( range <= 0 ) return min;
    {
      int const value = (int) ( rnd_xorshift_next( xorshift ) * range );
      return min + value; 
    }
   }



#endif /* RND_IMPLEMENTATION */

/*
revision history:
    1.0     first publicly released version 
*/

/*
------------------------------------------------------------------------------

This software is available under 2 licenses - you may choose the one you like.
Based on public domain implementation - original licenses can be found next to 
the relevant implementation sections of this file.

------------------------------------------------------------------------------

ALTERNATIVE A - MIT License

Copyright (c) 2016 Mattias Gustavsson

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.

------------------------------------------------------------------------------

ALTERNATIVE B - Public Domain (www.unlicense.org)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
software, either in source code form or as a compiled binary, for any purpose, 
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this 
software dedicate any and all copyright interest in the software to the public 
domain. We make this dedication for the benefit of the public at large and to 
the detriment of our heirs and successors. We intend this dedication to be an 
overt act of relinquishment in perpetuity of all present and future rights to 
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------------------------
*/


#ifdef __cplusplus
} // extern "C"
#endif

