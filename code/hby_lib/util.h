#pragma once

#include "basic.h"
#include "vec.h"

#include <float.h>

#if defined(_MSC_VER)
#define UTIL_INLINE __forceinline
#else
#define UTIL_INLINE inline
#endif

#define UTIL_ENABLE_SIMD 1

#if UTIL_ENABLE_SIMD
#ifdef __AVX__
#include <intrin.h>
#define UTIL_SIMD_ACTUALLY_ENABLED
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

static const float TAU_F = (float)(6.283185307179586);
static const float PI_F = (float)(6.283185307179586 * 0.5);
static const float EPSILON = 0.0001f;

#define EQ_F(a, b) (fabsf((a) - (b)) <= EPSILON)

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define CLAMP(x, low, high) MAX(MIN((x), (high)), (low))

#define UTIL_VIGNET_SHADER " \
float vignette(vec2 uv) {\n \
  uv *=  1.0 - uv.yx;   /*vec2(1.0)- uv.yx; -> 1.-u.yx; Thanks FabriceNeyret!*/ \n \
  float vig = uv.x*uv.y * 15.0; /* multiply with sth for intensity*/\n \
  vig = pow(vig, 0.25); /* change pow for modifying the extend of the  vignette*/\n \
  return vig;\n \
}\n \
"

UTIL_INLINE static float to_deg(float rad) {
  return rad * 180.f / PI_F;
}
UTIL_INLINE static float to_rad(float deg) {
  return deg / 180.f * PI_F;
}

UTIL_INLINE static uint32_t vec4_pack_10_10_10_8(Vec4 v) {
  return
    ((uint32_t)(v.x * ((float)0x3ff)) << 2 ) |
    ((uint32_t)(v.y * ((float)0x3ff)) << 12) |
    ((uint32_t)(v.z * ((float)0x3ff)) << 22) |
    ((uint32_t)(v.w * ((float)0x3)));
}

Mat4 get_standard_transform2(Vec3 pos, Quaternion rot, Vec3 scale);
Mat4 get_standard_transform2_inverse(Vec3 pos, Quaternion rot, Vec3 scale);

Mat4 get_standard_transform(Vec3 pos, Quaternion rot, float scale);
Mat4 get_standard_transform_inverse(Vec3 pos, Quaternion rot, float scale);

Quaternion util_rotate_to_face(Vec3 origin, Vec3 target);

UTIL_INLINE static Vec3 get_mid_point3(Vec3 a, Vec3 b) {
  return add3(a, smul3(0.5f, sub3(b, a)));
} 

typedef struct Timerd {
  double now;
} Timerd;
typedef struct Timer {
  float now;
} Timer;
typedef struct Timer_Delta {
  double last;
  double now;
} Timer_Delta;

UTIL_INLINE static Timer_Delta timer_update(Timer *timer, float dt) {
  Timer_Delta ret = {0};
  ret.last = timer->now;
  timer->now += dt;
  ret.now = timer->now;
  return ret;
}

UTIL_INLINE static Timer_Delta timer_reverse(Timer *timer, float dt) {
  Timer_Delta ret = {0};
  ret.last = timer->now;
  timer->now -= dt;
  if (timer->now <= 0) timer->now = 0;
  ret.now = timer->now;
  return ret;
}

UTIL_INLINE static Timer_Delta timerd_update(Timerd *timer, double dt) {
  Timer_Delta ret = {0};
  ret.last = timer->now;
  timer->now += dt;
  ret.now = timer->now;
  return ret;
}

UTIL_INLINE static Timer_Delta timerd_reverse(Timerd *timer, double dt) {
  Timer_Delta ret = {0};
  ret.last = timer->now;
  timer->now -= dt;
  if (timer->now <= 0) timer->now = 0;
  ret.now = timer->now;
  return ret;
}

typedef struct Timeline {
  double mark;
  double t, last_t;
  double found_mark;
  unsigned char changed : 1;
  unsigned char found : 1;
} Timeline;

UTIL_INLINE static Timeline tl_make(double last_t, double t) {
  Timeline ret = {0};
  ret.t = t;
  ret.last_t = last_t;
  return ret;
}

UTIL_INLINE static unsigned char  tl_forward(Timeline *tl, double range) {
  double new_mark = tl->mark + range;
  unsigned char  ret = 0;
  tl->changed = 0;
  if (tl->t >= tl->mark && tl->t < new_mark) {
    ret = 1;
    tl->found = 1;
    tl->found_mark = new_mark;
    if (tl->last_t < tl->mark && tl->t >= tl->mark) {
      tl->changed = 1;
    }
  }
  tl->mark = new_mark;
  return ret;
}

UTIL_INLINE static unsigned char  tl_found_before(Timeline *tl) {
  return tl->found && tl->mark <= tl->found_mark;
}

UTIL_INLINE static unsigned char  tl_default(Timeline *tl) {
  return !tl->found;
}

UTIL_INLINE static bool util_isnan(float v) { return v != v; }
UTIL_INLINE static bool util_isnand(double v) { return v != v; }
UTIL_INLINE static float util_maxf(float a, float b) { return MAX(a, b); }
UTIL_INLINE static float util_minf(float a, float b) { return MIN(a, b); }
UTIL_INLINE static float util_max3f(float a, float b, float c) { return MAX(MAX(a, b), c); }
UTIL_INLINE static float util_min3f(float a, float b, float c) { return MIN(MIN(a, b), c); }

// Courtesy of N.Schilke's answer on Stack overflow
UTIL_INLINE static float find_closest_point_to_line(Vec2 p, Vec2 a, Vec2 b) {
  Vec2 ap = sub2(p, a);       //Vector from A to P
  Vec2 ab = sub2(b, a);       //Vector from A to B
  
  float magnitudeAB = ab.x*ab.x + ab.y*ab.y;   //Magnitude of AB vector (it's length squared)     
  float ABAPproduct = dot2(ap, ab);    //The DOT product of a_to_p and a_to_b     
  float t = ABAPproduct / magnitudeAB; //The normalized "distance" from a to your closest point  

  return t; 
}

typedef struct Plane {
  union {
    Vec4 vec4;
    struct {
      Vec3 normal;
      float dist_from_origin;
    };
  };
} Plane;

// This is wrong.
#if 0
//
// https://iquilezles.org/articles/intersectors/
//
// In case i forget again: To represent a plane as a Vec4,
// xyz is a normal vector, and w is a magnitude. Think of it
// as direction and distance from (0,0,0)
UTIL_INLINE static
float ray_cast_plane(Vec3 orig, Vec3 dir, Vec4 plane) {
#if 0

#else
  return -(dot3(orig,plane.xyz)+plane.w)/dot3(dir,plane.xyz);
#endif
}
#endif

UTIL_INLINE static
bool ray_cast_plane(Vec3 orig, Vec3 dir, Vec4 plane, float *out_t) {
  Vec3 center = smul3(plane.w, plane.xyz);
  Vec3 normal = plane.xyz;
  float denom = dot3(normal, dir);
  if (fabs(denom) > 0.0001f) // your favorite epsilon
  {
    float t = dot3(sub3(center, orig), normal) / denom;
    if (t >= 0) {
      *out_t = t;
      return true; // you might want to allow an epsilon here too
    }
  }
  return false;
}

Vec4 transform_plane(Mat4 transform, Vec4 plane) {
  Vec4 O = vec4_xyz_w(muls3(plane.xyz, plane.w), 1);
  Vec4 N = vec4_xyz_w(plane.xyz, 0);
  O = mat4_multiply_vec4(transform, O);
  N = mat4_multiply_vec4(mat4_transpose(mat4_inverse(transform)), N);
  return vec4_xyz_w(norm3(N.xyz), dot3(O.xyz, N.xyz));
}

UTIL_INLINE static float get_distance_from_point_to_plane(Vec3 p, Vec4 plane) {
  Vec3 to_point = sub3(muls3(plane.xyz, plane.w), p);
  return fabsf(dot3(to_point, plane.xyz));
}

UTIL_INLINE static Vec3 find_closest_point_on_plane(Vec3 p, Vec4 plane) {
  Vec3 point_on_plane = muls3(plane.xyz, plane.w);
  Vec3 plane_normal = plane.xyz;
  Vec3 to_point = sub3(p, point_on_plane);
  float to_point_projected_on_N = dot3(to_point, plane_normal);
  return add3(p, smul3(-to_point_projected_on_N, plane_normal));
}

UTIL_INLINE static bool is_point_on_positive_side_of_plane(Vec3 p0, Vec4 plane) {
  Vec3 to_plane_0 = sub3(p0, muls3(plane.xyz, plane.w));
  Vec3 to_plane_norm_0 = norm3(to_plane_0);
  float dot_plane_normal_0 = dot3(to_plane_norm_0, plane.xyz);
  return dot_plane_normal_0 >= 0 || length3(to_plane_0) < EPSILON;
}

UTIL_INLINE static bool clip_segment_against_plane(Vec3 *pp0, Vec3 *pp1, Vec4 plane) {
  Vec3 p0 = *pp0;
  Vec3 p1 = *pp1;

  bool visible0 = is_point_on_positive_side_of_plane(p0, plane);
  bool visible1 = is_point_on_positive_side_of_plane(p1, plane);

  if (visible0 && visible1)
    return true;

  if (!visible0 && !visible1) {
    return false;
  }

  bool clipped1 = false;
  bool swapped = false;
  if (visible1) {
    SWAP(&p0, &p1);
    swapped = true;
  }

  Vec3 dir = norm3(sub3(p1, p0));
  float dist = 0;
  if (!ray_cast_plane(p0, dir, plane, &dist)) {
    return false;
  }
  p1 = add3(p0, smul3(dist, dir));
  clipped1 = true;

  if (swapped) {
    SWAP(&p0, &p1);
    clipped1 = !clipped1;
  }
  *pp0 = p0;
  *pp1 = p1;

  return true;
}

/*
 * Courtesy of this article by Victor Li:
 *  https://viclw17.github.io/2018/07/16/raytracing-ray-sphere-intersection/
 *
 *  Thanks mate!
 */
UTIL_INLINE static
bool ray_sphere_intersect(Vec3 orig, Vec3 dir, Vec3 center, float radius, float *out_t) {
  Vec3 oc = sub3(orig, center);
  float a = dot3(dir, dir);
  float b = 2.0f * dot3(oc, dir);
  float c = dot3(oc,oc) - radius*radius;
  float discriminant = b*b - 4*a*c;
  if (discriminant<0) return false;
  if (out_t)
    *out_t = (-b - sqrtf(discriminant)) / (2.f*a);
  return true;
}

/*
 * This was copied and pasted from Scratchpixel
 *   https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
 *
 * Thanks for the code!
 * */
UTIL_INLINE static bool ray_triangle_intersect( 
    Vec3 orig, Vec3 dir, 
    Vec3 v0, Vec3 v1, Vec3 v2, 
    float *t, float *u, float *v) 
{
  const float kEpsilon = 0.0001f;
  float invDet;
  Vec3 tvec, qvec;

  Vec3 v0v1 = sub3(v1, v0); 
  Vec3 v0v2 = sub3(v2, v0);
  Vec3 pvec = cross3(dir, v0v2);
  float det = dot3(v0v1, pvec);
//#define RAY_CAST_CULLING
#ifdef RAY_CAST_CULLING 
  // if the determinant is negative the triangle is backfacing
  // if the determinant is close to 0, the ray misses the triangle
  if (det < kEpsilon) return false; 
#else 
  // ray and triangle are parallel if det is close to 0
  if (fabs(det) < kEpsilon) return false; 
#endif 

  invDet = 1.f / det; 

  tvec = sub3(orig, v0);
  *u = dot3(tvec, pvec) * invDet; 
  if (*u < 0 || *u > 1) return false; 

  qvec = cross3(tvec, v0v1); 
  *v = dot3(dir, qvec) * invDet; 
  if (*v < 0 || *u + *v > 1) return false; 

  *t = dot3(v0v2, qvec) * invDet; 

  return *t >= 0; 
}

/*
 * Courtesy of https://iquilezles.org/www/articles/intersectors/intersectors.htm
 */
UTIL_INLINE static
unsigned char  ray_capsule_intersect(Vec3 orig, Vec3 dir, Vec3 p0, Vec3 p1, float radius, float *out_t) {
  Vec3 ba = sub3(p1, p0);
  if (length3(ba) <= 0.001f) {
    return ray_sphere_intersect(orig, dir, p0, radius, out_t);
  }

  {
    Vec3  oa = sub3(orig, p0);
    float baba = dot3(ba,ba);
    float bard = dot3(ba,dir);
    float baoa = dot3(ba,oa);
    float rdoa = dot3(dir,oa);
    float oaoa = dot3(oa,oa);

    float a = baba      - bard*bard;
    float b = baba*rdoa - baoa*bard;
    float c = baba*oaoa - baoa*baoa - radius*radius*baba;
    float h = b*b - a*c;

    Vec3 oc;
    if( h>=0.0 )
    {
      float t = (-b-sqrtf(h))/a;
      float y = baoa + t*bard;
      // body
      if(y>0.0 && y<baba) {
        if (out_t) *out_t = t;
        return t >= 0;
      }
      // caps
      oc = (y<=0.0) ? oa : sub3(orig, p1);
      b = dot3(dir,oc);
      c = dot3(oc,oc) - radius*radius;
      h = b*b - c;
      if(h>0.0) {
        float cap_t = -b - sqrtf(h);
        if (out_t) *out_t = cap_t;
        return cap_t >= 0;
      }
    }
  }
  return 0;
}

typedef struct Cone {
  Vec3 pos, dir;
  float angle_in_degrees;
} Cone;

typedef struct Aabb {
  union {
    struct {
      Vec3 min_point, max_point;
    };
    struct {
      Vec3 min_p, max_p;
    };
    Vec3 p[2];
  };
} Aabb;

static UTIL_INLINE Vec3 aabb_get_mid_point(Aabb aabb) {
  return get_mid_point3(aabb.min_point, aabb.max_point);
}
static UTIL_INLINE float aabb_get_diameter(Aabb aabb) {
  return length3(sub3(aabb.min_point, aabb.max_point));
}
static UTIL_INLINE float aabb_get_radius(Aabb aabb) {
  return 0.5f * aabb_get_diameter(aabb);
}
static UTIL_INLINE bool aabb_intersect(Aabb aabb_a, Aabb aabb_b) {
  return
    aabb_a.min_point.x <= aabb_b.max_point.x && aabb_b.min_point.x <= aabb_a.max_point.x &&
    aabb_a.min_point.y <= aabb_b.max_point.y && aabb_b.min_point.y <= aabb_a.max_point.y &&
    aabb_a.min_point.z <= aabb_b.max_point.z && aabb_b.min_point.z <= aabb_a.max_point.z;
}


UTIL_INLINE static Cone get_cone_from_camera_pos_and_aabb(Vec3 camera_pos, Aabb aabb) {
  Vec3 mid_point = smul3(0.5f, add3(aabb.max_point, aabb.min_point));
  float aabb_radius = length3(sub3(aabb.max_point, aabb.min_point)) / 2;
  Vec3 aabb_diff = sub3(mid_point, camera_pos);
  Vec3 aabb_dir = norm3(aabb_diff);
  float dist_from_midpoint = length3(aabb_diff);
  float aabb_cone_angle_in_rads = atanf(aabb_radius / dist_from_midpoint);

  Cone cone = {0};
  cone.pos = camera_pos;
  cone.dir = aabb_dir;
  cone.angle_in_degrees = to_deg(aabb_cone_angle_in_rads);
  return cone;
}

UTIL_INLINE static bool is_aabb_visible_in_cone(Cone cone, Aabb aabb) {
  Cone aabb_cone = get_cone_from_camera_pos_and_aabb(cone.pos, aabb);
  float cos_angle = dot3(aabb_cone.dir, cone.dir);
  float angle_in_rads = acosf(cos_angle);
  return angle_in_rads < cone.angle_in_degrees + aabb_cone.angle_in_degrees;
}

/*
Transforming Axis-Aligned Bounding Boxes
by Jim Arvo
from "Graphics Gems", Academic Press, 1990
*/
static UTIL_INLINE Aabb transform_aabb(Mat4 world_transform, Aabb aabb) {
  float  a, b;
  float  Amin[3], Amax[3];
  float  Bmin[3], Bmax[3];
  int    i, j;

  /*Copy box A into a min array and a max array for easy reference.*/

  Amin[0] = (float)aabb.min_point.x;  Amax[0] = aabb.max_point.x;
  Amin[1] = (float)aabb.min_point.y;  Amax[1] = aabb.max_point.y;
  Amin[2] = (float)aabb.min_point.z;  Amax[2] = aabb.max_point.z;

  /* Take care of translation by beginning at T. */

  Bmin[0] = Bmax[0] = world_transform.data[12];
  Bmin[1] = Bmax[1] = world_transform.data[13];
  Bmin[2] = Bmax[2] = world_transform.data[14];

  /* Now find the extreme points by considering the product of the */
  /* min and max with each component of M.  */
                   
  for( j = 0; j < 3; j++ ) {
    for( i = 0; i < 3; i++ ) {
      a = (float)(world_transform.Elements[j][i] * Amin[j]);
      b = (float)(world_transform.Elements[j][i] * Amax[j]);
      if( a < b ) { 
        Bmin[i] += a; 
        Bmax[i] += b;
      }
      else { 
        Bmin[i] += b; 
        Bmax[i] += a;
      }
    }
  }

  /* Copy the result into the new box. */

  aabb.min_point.x = Bmin[0];  aabb.max_point.x = Bmax[0];
  aabb.min_point.y = Bmin[1];  aabb.max_point.y = Bmax[1];
  aabb.min_point.z = Bmin[2];  aabb.max_point.z = Bmax[2];
  return aabb;
}

UTIL_INLINE static bool is_aabb_in_projection_matrix(Aabb aabb, Mat4 proj_matrix) {
  // Don't use this this is wrong.
#if 1
  float inv_z = util_maxf(fabsf(aabb.max_point.z), fabsf(aabb.min_point.z));
  aabb.min_point = mat4_multiply_pos(proj_matrix, aabb.min_point);
  aabb.max_point = mat4_multiply_pos(proj_matrix, aabb.max_point);
  if ((aabb.min_point.x <=-inv_z && aabb.max_point.x <= -inv_z) ||
      (aabb.min_point.y <=-inv_z && aabb.max_point.y <= -inv_z) ||
      (aabb.min_point.z <= 0     && aabb.max_point.z <=  0    ) ||
      (aabb.min_point.x >= inv_z && aabb.max_point.x >=  inv_z) ||
      (aabb.min_point.y >= inv_z && aabb.max_point.y >=  inv_z) ||
      (aabb.min_point.z >= inv_z && aabb.max_point.z >=  inv_z)
    )
  {
    return false;
  }
  return true;
#else
  aabb = transform_aabb(aabb, proj_matrix);
#endif
}

// Got this from https://gdbooks.gitbooks.io/3dcollisions/content/Chapter3/raycast_aabb.html
// Thanks mate!
UTIL_INLINE
static unsigned char  ray_aabb_intersect(Vec3 orig, Vec3 dir, Aabb aabb, float *out_t) {
  dir = norm3(dir);
  {
    float t1 = (aabb.min_point.x - orig.x) / dir.x;
    float t2 = (aabb.max_point.x - orig.x) / dir.x;
    float t3 = (aabb.min_point.y - orig.y) / dir.y;
    float t4 = (aabb.max_point.y - orig.y) / dir.y;
    float t5 = (aabb.min_point.z - orig.z) / dir.z;
    float t6 = (aabb.max_point.z - orig.z) / dir.z;

    float tmin = MAX(MAX(MIN(t1, t2), MIN(t3, t4)), MIN(t5, t6));
    float tmax = MIN(MIN(MAX(t1, t2), MAX(t3, t4)), MAX(t5, t6));

    // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
    if (tmax < 0) {
      return 0;
    }

    // if tmin > tmax, ray doesn't intersect AABB
    if (tmin > tmax) {
      return 0;
    }

    if (tmin < 0.f) {
      if (out_t)
        *out_t = tmax;
      return 1;
    }

    if (out_t)
      *out_t = tmin;
  }
  return 1;
}

// P is a point on the plane
// N is the normal of the plane
// point is the point we want to test
UTIL_INLINE static Vec3 closest_point_on_plane(Vec3 P, Vec3 N, Vec3 point) {
  Vec3 point_on_plane = P;
  Vec3 to_point = sub3(point, P);
  float to_point_projected_on_N = dot3(to_point, N);
  return add3(point, smul3(-to_point_projected_on_N, N));
}

#define CLAMP01(x) CLAMP((x),0,1)
#define RGBA_TO_UINT32(r,g,b,a) (\
  ((uint32_t)(CLAMP01((float)a) * 255.0f) << 24) | \
  ((uint32_t)(CLAMP01((float)b) * 255.0f) << 16) | \
  ((uint32_t)(CLAMP01((float)g) * 255.0f) << 8 ) | \
  ((uint32_t)(CLAMP01((float)r) * 255.0f) << 0 ))

uint32_t rgba_to_uint32(float r, float g, float b, float a);

static UTIL_INLINE
Vec4 uint32_to_vec4(uint32_t col) {
#ifdef UTIL_SIMD_ACTUALLY_ENABLED
  __m128i val4 = _mm_set1_epi32 ((int)col);
  __m128i masked = _mm_and_si128(val4, _mm_set_epi32(0xff000000,0x00ff0000,0x0000ff00,0x000000ff));
  __m128i shifted = _mm_srlv_epi32(masked, _mm_set_epi32(24, 16, 8, 0));
  __m128 converted = _mm_cvtepi32_ps(shifted);
  // Simd div seems to be the fastest here. Tried simd mul, but that was inaccurate.
  // Simd mul with _m256 double was slightly slower.
  // Using normal C arithmetics with both double multiply and float divide was slower than thier simd counterparts.
  // Normal C arithmetics float multiply was similarly inaccurate.
  __m128 result = _mm_div_ps(converted, _mm_set_ps1(255.f));
  Vec4 ret;
  _mm_store_ps(ret.p, result);
  return ret;
#else
  Vec4 v;
  v.w = (float)((col & (0xff << 24)) >> 24) / 255.f; // 1/255
  v.z = (float)((col & (0xff << 16)) >> 16) / 255.f;
  v.y = (float)((col & (0xff <<  8)) >>  8) / 255.f;
  v.x = (float)((col & (0xff <<  0)) >>  0) / 255.f;
  return v;
#endif
}

static UTIL_INLINE
uint32_t vec4_to_uint32(Vec4 col) {
  return rgba_to_uint32(col.r, col.g, col.b, col.a);
}

static UTIL_INLINE
uint32_t uint32_set_alpha(uint32_t color, float alpha) {
  Vec4 v = uint32_to_vec4(color);
  v.a = alpha;
  return vec4_to_uint32(v);
}

static UTIL_INLINE
float uint32_get_alpha(uint32_t color) {
  Vec4 v = uint32_to_vec4(color);
  return v.a;
}

Vec3 inverse_barycentric_2d(Vec2 a, Vec2 b, Vec2 c, Vec2 pos);

UTIL_INLINE static Vec3 barycentric3(Vec3 b, Vec3 p0, Vec3 p1, Vec3 p2) {
  Vec3 q0 = smul3(b.x, p0);
  Vec3 q1 = smul3(b.y, p1);
  Vec3 q2 = smul3(b.z, p2);
  return add3(add3(q0, q1), q2);
}
UTIL_INLINE static Vec2 barycentric2(Vec3 b, Vec2 p0, Vec2 p1, Vec2 p2) {
  Vec2 q0 = smul2(b.x, p0);
  Vec2 q1 = smul2(b.y, p1);
  Vec2 q2 = smul2(b.z, p2);
  return add2(add2(q0, q1), q2);
}

float lerp(float low, float high, float t);
double lerpd(double low, double high, double t);
Vec2 lerp2(Vec2 low, Vec2 high, float t);
Vec3 lerp3(Vec3 low, Vec3 high, float t);
Vec4 lerp4(Vec4 low, Vec4 high, float t);
float ilerp(float low, float high, float t);

float ease_cos_hill(float t);
float ease_cos(float t);
float ease_in_expo(float t);
float ease_cos_pulse(float t, int num_pulses);
float pow2(float x);

float clamp(float x, float low, float high);
float clamp01(float x);
bool move_to_d(double *x, double tx, double delta);
bool move_to(float *x, float tx, float delta);
bool move_to2(struct Vec2 *v, struct Vec2 t, float delta);
bool move_to3(struct Vec3 *v, struct Vec3 t, float delta);
bool intersect_ray_with_circle(Vec2 center, float radius, Vec2 begin, Vec2 end, Vec2 *out_point);
bool intersect_ray_with_box(Vec4 box, Vec2 begin, Vec2 end, Vec2 *out_point);
UTIL_INLINE static bool intersect_lines(Vec2 a, Vec2 b, Vec2 c, Vec2 d, Vec2 *out_point) {
  Vec2 ab = sub2(b, a);
  Vec2 cd = sub2(d, c);
  Vec2 ca = sub2(a, c);

  // cross(ab, cd)
  float cross_ab_cd = ab.x * cd.y - ab.y * cd.x;
  if (cross_ab_cd == 0)
    return false;

  {
    float inv = 1.f / cross_ab_cd;
    // s = cross(ab, ca) / cross(ab, cd)
    float s = (ab.x * ca.y - ab.y * ca.x) * inv;
    // t = cross(cd, ca) / cross(ab, cd)
    float t = (cd.x * ca.y - cd.y * ca.x) * inv;

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
      if (out_point)
        *out_point = add2(a, smul2(t, ab));
      return true;
    }
  }
  return false;
}

float util_get_wave_from_timestamp(double t);
float util_get_wave_period_from_timestamp(double t, float period);
float util_get_wave_period_phase_from_timestamp(double t, float period, float phase);
float get_wave(void);
float get_wave_period(float period);
float get_wave_period_phase(float period, float phase);

UTIL_INLINE static float util_get_wave(void) { return get_wave(); }

UTIL_INLINE static float saturate(float t) {
  return clamp01(t);
}

#define ENDIAN_SWAP_UINT32(val) (  \
    (((val) & 0xff000000) >> 24) | \
    (((val) & 0x00ff0000) >> 8 ) | \
    (((val) & 0x0000ff00) << 8 ) | \
    (((val) & 0x000000ff) << 24)   \
  )

#define PHOTOSHOP_COLOR3(color) ENDIAN_SWAP_UINT32(((color) << 8) | 0xff)

UTIL_INLINE uint32_t endian_swap_uint32(uint32_t val) {
  return ENDIAN_SWAP_UINT32(val);
}

static UTIL_INLINE
uint32_t photoshop_color3(uint32_t color) {
  return PHOTOSHOP_COLOR3(color);
}
static UTIL_INLINE
uint32_t web_color3(uint32_t color) {
  return PHOTOSHOP_COLOR3(color);
}

static UTIL_INLINE Vec4 web_color_to_vec4(uint32_t color) {
  return uint32_to_vec4(PHOTOSHOP_COLOR3(color));
}

UTIL_INLINE static Vec3 closest_point_on_line_segment(Vec3 A, Vec3 B, Vec3 point) {
  Vec3 AB = sub3(B, A);
  float t = dot3(sub3(point, A), AB) / dot3(AB, AB);
  return add3(A, smul3(saturate(t), AB)); // saturate(t) can be written as: min((max(t, 0), 1)
}

//====================================
// Courtesy of this post:
//  https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
static UTIL_INLINE float triangle_sign(Vec2 p1, Vec2 p2, Vec2 p3) {
  return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}
static UTIL_INLINE bool point_in_triangle(Vec2 pt, Vec2 v1, Vec2 v2, Vec2 v3)
{
    const float kEpsilon = 0.01f;
    float d1, d2, d3;
    unsigned char  has_neg, has_pos;

    d1 = triangle_sign(pt, v1, v2);
    d2 = triangle_sign(pt, v2, v3);
    d3 = triangle_sign(pt, v3, v1);

    has_neg = (d1 <-kEpsilon) || (d2 <-kEpsilon) || (d3 <-kEpsilon);
    has_pos = (d1 > kEpsilon) || (d2 > kEpsilon) || (d3 > kEpsilon);

    return !(has_neg && has_pos);
}

static UTIL_INLINE bool point_in_box(Vec2 p, Vec4 box) {
  return
    p.x >= box.x && p.x <= box.x+box.width &&
    p.y >= box.y && p.y <= box.y+box.height;
}

static UTIL_INLINE bool point_in_box_vec2(Vec2 p, Vec2 box0, Vec2 box1) {
  return
    p.x >= box0.x && p.x <= box1.x &&
    p.y >= box0.y && p.y <= box1.y;
}

static UTIL_INLINE bool util_box_overlap(Vec4 a, Vec4 b) {
  return
    a.x <= b.x+b.width  && b.x <= a.x+a.width &&
    a.y <= b.y+b.height && b.y <= a.y+a.height;
}
static UTIL_INLINE bool util_box_overlap_strict(Vec4 a, Vec4 b) {
  return
    a.x < b.x+b.width  && b.x < a.x+a.width &&
    a.y < b.y+b.height && b.y < a.y+a.height;
}

// This is probably horrifically slow. Lucily, we're not using it for
// anything important (yet).
static UTIL_INLINE bool rectangle_intersect_triangle(Vec2 box0, Vec2 box1, Vec2 p0, Vec2 p1, Vec2 p2) {
  Vec2 bp[4] = {0};
  Vec2 tp[3] = {0};
  int i = 0;

  tp[0] = p0;
  tp[1] = p1;
  tp[2] = p2;

  bp[0] = vec2(box0.x, box0.y);
  bp[1] = vec2(box1.x, box0.y);
  bp[2] = vec2(box1.x, box1.y);
  bp[3] = vec2(box0.x, box1.y);

  if (point_in_triangle(bp[0], tp[0], tp[1], tp[2]) ||
    point_in_triangle(bp[1], tp[0], tp[1], tp[2]) ||
    point_in_triangle(bp[2], tp[0], tp[1], tp[2]) ||
    point_in_triangle(bp[3], tp[0], tp[1], tp[2])
    )
  {
    return true;
  }

  if (point_in_box_vec2(tp[0], box0, box1) ||
      point_in_box_vec2(tp[1], box0, box1) ||
      point_in_box_vec2(tp[2], box0, box1))
  {
    return true;
  }

  for (i = 0; i < _countof(bp); i++) {
    int j = 0;
    Vec2 b0 = bp[i];
    Vec2 b1 = bp[i+1 < _countof(bp) ? i+1 : 0];
    for (j = 0; j < _countof(tp); j++) {
      Vec2 t0 = tp[j];
      Vec2 t1 = tp[j+1 < _countof(tp) ? j+1 : 0];
      if (intersect_lines(b0, b1, t0, t1, NULL))
        return true;
    }
  }

  return false;
}

UTIL_INLINE static Vec3 quaternion_multiply_vec3(Quaternion rot, Vec3 dir) {
  return mat4_multiply_vec4(quaternion_to_mat4(rot), vec4_xyz_w(dir, 0)).xyz;
}

uint32_t multiply_color_uint32(uint32_t color0, uint32_t color1);

Vec3 rgb_to_hsv(float r, float g, float b);
Vec3 hsv_to_rgb(float h, float s, float v);
Vec3 rgb_vec3_to_hsv(Vec3 rgb);
Vec3 hsv_vec3_to_rgb(Vec3 rgb);
UTIL_INLINE static Vec3 lerp_color(Vec3 rgb0, Vec3 rgb1, float t) {
  Vec3 hsv0 = rgb_vec3_to_hsv(rgb0);
  Vec3 hsv1 = rgb_vec3_to_hsv(rgb1);
  return hsv_vec3_to_rgb(lerp3(hsv0, hsv1, t));
}

UTIL_INLINE static Vec3 modify_saturation(Vec3 color, float i) {
  color = rgb_to_hsv(color.r, color.g, color.b);
  color.y = i;
  return hsv_to_rgb(color.x, color.y, color.z);
}
UTIL_INLINE static Vec3 modify_brightness(Vec3 color, float i) {
  color = rgb_to_hsv(color.r, color.g, color.b);
  color.z = i;
  return hsv_to_rgb(color.x, color.y, color.z);
}

UTIL_INLINE static unsigned char  t_in_range(float t, float *test, float duration) {
  unsigned char  ret = t >= *test && t < *test + duration;
  *test += duration;
  return ret;
}

UTIL_INLINE static unsigned char  crossed_threshold(float t, float last_t, float threshold, unsigned char  *up) {
  if (t >= threshold && last_t < threshold) {
    *up = 1;
    return 1;
  }
  else if (t <= threshold && last_t > threshold) {
    *up = 0;
    return 1;
  }
  return 0;
}

char **util_list_files_in_directory(const char *dir);
void util_free_file_list(char **files);

void util_gfx_line_imm(Vec3 p0, Vec3 p1);
void util_gfx_debug_point_imm(Vec3 p);
void util_gfx_debug_aabb_imm(Aabb aabb);
void util_gfx_debug_box_imm(Vec3 p0, Vec3 p1);
void util_gfx_debug_box_transform_imm(Vec3 p0, Vec3 p1, Mat4 transform);

void util_gfx_line(Vec3 p0, Vec3 p1);
void util_gfx_debug_point(Vec3 p);
void util_gfx_debug_aabb(Aabb aabb);

#ifndef OPEN_SIMPLEX_NOISE_H__
#define OPEN_SIMPLEX_NOISE_H__

/*
 * OpenSimplex (Simplectic) Noise in C.
 * Ported to C from Kurt Spencer's java implementation by Stephen M. Cameron
 *
 * v1.1 (October 6, 2014) 
 * - Ported to C
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

#ifdef __cplusplus
	extern "C" {
#endif

/* You can override this to be float if you want by adding -DOSNFLOAT=float to CFLAGS
 * It is not as well tested with floats though.
 */
#ifndef OSNFLOAT
#define OSNFLOAT double
#endif

struct osn_context;

int open_simplex_noise(int64_t seed, struct osn_context **ctx);
void open_simplex_noise_free(struct osn_context *ctx);
int open_simplex_noise_init_perm(struct osn_context *ctx, int16_t p[], int nelements);
OSNFLOAT open_simplex_noise2(const struct osn_context *ctx, OSNFLOAT x, OSNFLOAT y);
OSNFLOAT open_simplex_noise3(const struct osn_context *ctx, OSNFLOAT x, OSNFLOAT y, OSNFLOAT z);
OSNFLOAT open_simplex_noise4(const struct osn_context *ctx, OSNFLOAT x, OSNFLOAT y, OSNFLOAT z, OSNFLOAT w);

#ifdef __cplusplus
	}
#endif

#endif

//====================================================================================
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

#define rnd_gamerand_shuffle_array(gamerand, ptr, length) do { \
  char tmp[sizeof(ptr[0])]; \
  for (int i = length; i > 0; i--) { \
    int i_r = rnd_gamerand_range((gamerand), 0, i-1); \
    memcpy(tmp,       &ptr[i_r],   sizeof(ptr[0])); \
    memcpy(&ptr[i_r], &ptr[i-1],   sizeof(ptr[0])); \
    memcpy(&ptr[i-1],   tmp,       sizeof(ptr[0])); \
  } \
} while (0)

typedef struct rnd_xorshift_t { RND_U64 state[ 2 ]; } rnd_xorshift_t;
void rnd_xorshift_seed( rnd_xorshift_t* xorshift, RND_U64 seed );
RND_U64 rnd_xorshift_next( rnd_xorshift_t* xorshift );
float rnd_xorshift_nextf( rnd_xorshift_t* xorshift );
int rnd_xorshift_range( rnd_xorshift_t* xorshift, int min, int max );

#endif /* rnd_h */

typedef struct Rng Rng;
struct Rng {
  rnd_gamerand_t impl;
};
UTIL_INLINE static void setup_rng(Rng *gen, int seed) {
  rnd_gamerand_seed(&gen->impl, (uint32_t)seed);
}
UTIL_INLINE static float randomf(Rng *gen) {
  return rnd_gamerand_nextf(&gen->impl);
}
UTIL_INLINE static unsigned randomu(Rng *gen) {
  return rnd_gamerand_next(&gen->impl);
}
UTIL_INLINE static int randomi_range(Rng *gen, int lo, int hi) {
  return rnd_gamerand_range(&gen->impl, lo, hi);
}

typedef struct {
  union {
    struct { float pitch, yaw, roll; };
    Vec3 vec;
  };
} Euler;
Euler euler_angle_from_dir(Vec3 dir);

UTIL_INLINE static float transform_scale_uniform(Mat4 transform, float s) {
  return length3(mat4_multiply_dir(transform, vec3(s,0,0)));
}

const static int32_t BAYER_DITHER_BUFFER[64] = {
  0,  32, 8,  40, 2,  34, 10, 42, /* 8x8 Bayer ordered dithering  */
  48, 16, 56, 24, 50, 18, 58, 26, /* pattern. Each input pixel    */
  12, 44, 4,  36, 14, 46, 6,  38, /* is scaled to the 0..63 range */
  60, 28, 52, 20, 62, 30, 54, 22, /* before looking in this table */
  3,  35, 11, 43, 1,  33, 9,  41, /* to determine the action.     */
  51, 19, 59, 27, 49, 17, 57, 25,
  15, 47, 7,  39, 13, 45, 5,  37,
  63, 31, 55, 23, 61, 29, 53, 21
};

#ifdef __cplusplus
} // extern "C"
#endif

