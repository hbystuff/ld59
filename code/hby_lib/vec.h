#pragma once

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#if defined(_MSC_VER)
#define VEC_INLINE __forceinline
#else
#define VEC_INLINE inline
#endif

VEC_INLINE static float hby_vec_maxf(float a, float b) { return MAX(a, b); }
VEC_INLINE static float hby_vec_minf(float a, float b) { return MIN(a, b); }

typedef struct Vec2d {
  union {
    struct { double x, y; };
    double p[2];
  };
} Vec2d;

typedef struct Vec2 {
  union {
    struct { float x, y; };
    float p[2];
  };
} Vec2;
typedef Vec2 float2;

VEC_INLINE static Vec2 vec2(float x, float y) { Vec2 ret = {x, y}; return ret; }
VEC_INLINE static float size2(Vec2 v) { return sqrtf(v.x*v.x + v.y*v.y); }
VEC_INLINE static float length2(Vec2 v) { return sqrtf(v.x*v.x + v.y*v.y); }
VEC_INLINE static Vec2 norm2(Vec2 v) { float size = size2(v); Vec2 r = { v.x/size,v.y/size }; return r; }
VEC_INLINE static Vec2 abs2(Vec2 v) { Vec2 r = { fabsf(v.x), fabsf(v.y), }; return r; }
VEC_INLINE static Vec2 floor2(Vec2 v) { Vec2 r = { floorf(v.x), floorf(v.y) }; return r; }
VEC_INLINE static int eq2(Vec2 a, Vec2 b) { return a.x == b.x && a.y == b.y; }
VEC_INLINE static float dot2(Vec2 a, Vec2 b) { return a.x*b.x + a.y*b.y; }
VEC_INLINE static Vec2 add2(Vec2 a, Vec2 b) { Vec2 r = {a.x+b.x, a.y+b.y}; return r;}
VEC_INLINE static Vec2 sub2(Vec2 a, Vec2 b) { Vec2 r = {a.x-b.x, a.y-b.y}; return r;}
VEC_INLINE static Vec2 div2(Vec2 a, Vec2 b) { Vec2 r = {a.x/b.x, a.y/b.y}; return r;}
VEC_INLINE static Vec2 mul2(Vec2 a, Vec2 b) { Vec2 r = {a.x*b.x, a.y*b.y}; return r;}
VEC_INLINE static Vec2 smul2(float s, Vec2 v) { Vec2 r = {v.x*s, v.y*s}; return r;}
VEC_INLINE static Vec2 muls2(Vec2 v, float s) { Vec2 r = {v.x*s, v.y*s}; return r;}
VEC_INLINE static Vec2 divs2(Vec2 v, float s) { Vec2 r = {v.x/s, v.y/s}; return r;}
VEC_INLINE static Vec2 neg2(Vec2 v) { Vec2 r = {-v.x, -v.y}; return r;}
VEC_INLINE static float dist2(Vec2 a, Vec2 b) { return size2(sub2(a, b));}
VEC_INLINE static float distance2(Vec2 v, Vec2 w) { return length2(sub2(v, w)); }
VEC_INLINE static Vec2 norm_sub2(Vec2 a, Vec2 b) { return norm2(sub2(a, b));}
VEC_INLINE static Vec2 from_angle2(float angle) { Vec2 r; angle = angle * 3.1415926f / 180.f; r.x=cosf(angle); r.y=sinf(angle); return r; }
VEC_INLINE static Vec2 min2(Vec2 a, Vec2 b) { Vec2 ret = { hby_vec_minf(a.x,b.x),hby_vec_minf(a.y,b.y) }; return ret; }
VEC_INLINE static Vec2 max2(Vec2 a, Vec2 b) { Vec2 ret = { hby_vec_maxf(a.x,b.x),hby_vec_maxf(a.y,b.y) }; return ret; }
VEC_INLINE static Vec2 rotate2(Vec2 a, float angle) {
  Vec2 result;
  angle = angle * 3.1415926f / 180.f;
  result.x = cosf(angle)*a.x - sinf(angle)*a.y;
  result.y = sinf(angle)*a.x + cosf(angle)*a.y;
  return result;
}
VEC_INLINE static float angle2(Vec2 a) {
  Vec2 n = norm2(a);
  return atan2f(n.y, n.x) / 3.1415926f * 180;
}

typedef struct Vec3 {
  union {
    struct { float x, y, z; };
    Vec2 xy;
    struct { float x_; Vec2 yz; };

    struct { float r, g, b; };
    Vec2 rg;
    struct { float r_; Vec2 gb; };

    float p[3];
    float data[3];
  };
} Vec3;
typedef Vec3 float3;

VEC_INLINE static Vec3 vec3(float x, float y, float z) { Vec3 v = {x,y,z}; return v; }
VEC_INLINE static Vec3 v3_xy_z(Vec2 xy, float z) { Vec3 v = {0}; v.xy = xy; v.z = z; return v; }
VEC_INLINE static Vec3 vec3_xy_z(Vec2 xy, float z) { Vec3 v = {0}; v.xy = xy; v.z = z; return v; }
VEC_INLINE static float size3(Vec3 v) { return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z); }
VEC_INLINE static float length3(Vec3 v) { return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z); }
VEC_INLINE static Vec3 norm3(Vec3 v) { float size = size3(v); Vec3 r = { v.x/size,v.y/size,v.z/size }; return r; }
VEC_INLINE static Vec3 v3norm(float x, float y, float z) { return norm3(vec3(x,y,z)); }
VEC_INLINE static Vec3 add3(Vec3 v, Vec3 w) { Vec3 r = {v.x+w.x, v.y+w.y, v.z+w.z}; return r;}
VEC_INLINE static Vec3 sub3(Vec3 v, Vec3 w) { Vec3 r = {v.x-w.x, v.y-w.y, v.z-w.z}; return r;}
VEC_INLINE static Vec3 div3(Vec3 v, Vec3 w) { Vec3 r = {v.x/w.x, v.y/w.y, v.z/w.z}; return r;}
VEC_INLINE static Vec3 mul3(Vec3 v, Vec3 w) { Vec3 r = {v.x*w.x, v.y*w.y, v.z*w.z}; return r;}
VEC_INLINE static Vec3 smul3(float s, Vec3 v) { Vec3 r = {s*v.x, s*v.y, s*v.z}; return r;}
VEC_INLINE static Vec3 muls3(Vec3 v, float s) { Vec3 r = {v.x*s, v.y*s, v.z*s}; return r;}
VEC_INLINE static Vec3 divs3(Vec3 v, float s) { Vec3 r = {v.x/s, v.y/s, v.z/s}; return r;}
VEC_INLINE static Vec3 neg3(Vec3 v) { Vec3 r = {-v.x, -v.y, -v.z}; return r;}
VEC_INLINE static float dist3(Vec3 a, Vec3 b) { return size3(sub3(a, b));}
VEC_INLINE static float distance3(Vec3 v, Vec3 w) { return length3(sub3(v, w)); }
VEC_INLINE static Vec3 norm_sub3(Vec3 a, Vec3 b) { return norm3(sub3(a, b));}
VEC_INLINE static Vec3 abs3(Vec3 v) { Vec3 r = { fabsf(v.x), fabsf(v.y), fabsf(v.z), }; return r; }
VEC_INLINE static Vec3 max3(Vec3 a, Vec3 b) { Vec3 ret = { hby_vec_maxf(a.x,b.x),hby_vec_maxf(a.y,b.y),hby_vec_maxf(a.z,b.z), }; return ret; }
VEC_INLINE static float dot3(Vec3 VecOne, Vec3 VecTwo) {
  return (VecOne.x * VecTwo.x) + (VecOne.y * VecTwo.y) + (VecOne.z * VecTwo.z);
}
VEC_INLINE static Vec3 min3(Vec3 a, Vec3 b) { Vec3 ret = { hby_vec_minf(a.x,b.x),hby_vec_minf(a.y,b.y),hby_vec_minf(a.z,b.z) }; return ret; }
VEC_INLINE static Vec3 cross3(Vec3 VecOne, Vec3 VecTwo) {
  Vec3 Result;
  Result.x = (VecOne.y * VecTwo.z) - (VecOne.z * VecTwo.y);
  Result.y = (VecOne.z * VecTwo.x) - (VecOne.x * VecTwo.z);
  Result.z = (VecOne.x * VecTwo.y) - (VecOne.y * VecTwo.x);
  return (Result);
}
VEC_INLINE static Vec3 vec3_slerp(Vec3 a, Vec3 b, float t) {
  float omega = dot3(a, b);
  float inv_sin_omega = 1.f / sinf(omega);
  Vec3 aa = smul3( sinf((1-t) * omega) * inv_sin_omega, a );
  Vec3 bb = smul3( sinf(t*omega) * inv_sin_omega, b );
  return add3(aa, bb);
}


typedef struct Vec3d {
  union {
    struct { double x, y, z; };
    Vec2 xy;
    struct { double x_; Vec2 yz; };

    struct { double r, g, b; };
    Vec2 rg;
    struct { double r_; Vec2 gb; };

    double p[3];
    double data[3];
  };
} Vec3d;

VEC_INLINE static Vec3 floor3(Vec3 v) { Vec3 ret = { floorf(v.x), floorf(v.y), floorf(v.z), }; return ret; }
VEC_INLINE static Vec3d vec3d(double x, double y, double z) { Vec3d v = {x,y,z}; return v; }
VEC_INLINE static Vec3d vec3_to_vec3d(Vec3 v) { Vec3d ret = {0}; ret.x = v.x; ret.y = v.y; ret.z = v.z; return ret; }
VEC_INLINE static Vec3d make3d(double x, double y, double z) { Vec3d v = {x,y,z}; return v; }
VEC_INLINE static double size3d(Vec3d v) { return sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }
VEC_INLINE static double length3d(Vec3d v) { return sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }
VEC_INLINE static Vec3d norm3d(Vec3d v) { double size = size3d(v); Vec3d r = { v.x/size,v.y/size,v.z/size }; return r; }
VEC_INLINE static Vec3d add3d(Vec3d v, Vec3d w) { Vec3d r = {v.x+w.x, v.y+w.y, v.z+w.z}; return r;}
VEC_INLINE static Vec3d sub3d(Vec3d v, Vec3d w) { Vec3d r = {v.x-w.x, v.y-w.y, v.z-w.z}; return r;}
VEC_INLINE static Vec3d div3d(Vec3d v, Vec3d w) { Vec3d r = {v.x/w.x, v.y/w.y, v.z/w.z}; return r;}
VEC_INLINE static Vec3d mul3d(Vec3d v, Vec3d w) { Vec3d r = {v.x*w.x, v.y*w.y, v.z*w.z}; return r;}
VEC_INLINE static Vec3d smul3d(double s, Vec3d v) { Vec3d r = {s*v.x, s*v.y, s*v.z}; return r;}
VEC_INLINE static Vec3d muls3d(Vec3d v, double s) { Vec3d r = {v.x*s, v.y*s, v.z*s}; return r;}
VEC_INLINE static Vec3d divs3d(Vec3d v, double s) { Vec3d r = {v.x/s, v.y/s, v.z/s}; return r;}
VEC_INLINE static Vec3d neg3d(Vec3d v) { Vec3d r = {-v.x, -v.y, -v.z}; return r;}
VEC_INLINE static double dist3d(Vec3d a, Vec3d b) { return size3d(sub3d(a, b));}
VEC_INLINE static Vec3d norm_sub3d(Vec3d a, Vec3d b) { return norm3d(sub3d(a, b));}
VEC_INLINE static unsigned char eq3(Vec3 a, Vec3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
VEC_INLINE static Vec3d abs3d(Vec3d v) { Vec3d r = { fabs(v.x), fabs(v.y), fabs(v.z), }; return r; }
VEC_INLINE static double dot3d(Vec3d VecOne, Vec3d VecTwo) {
  return (VecOne.x * VecTwo.x) + (VecOne.y * VecTwo.y) + (VecOne.z * VecTwo.z);
}
VEC_INLINE static Vec3d cross3d(Vec3d VecOne, Vec3d VecTwo) {
  Vec3d Result;
  Result.x = (VecOne.y * VecTwo.z) - (VecOne.z * VecTwo.y);
  Result.y = (VecOne.z * VecTwo.x) - (VecOne.x * VecTwo.z);
  Result.z = (VecOne.x * VecTwo.y) - (VecOne.y * VecTwo.x);
  return (Result);
}


typedef struct Vec4 {
  union {
    struct { float x, y, z, w; };
    struct { float r, g, b, a; };
    struct { float red, green, blue, alpha; };
    Vec3 xyz;
    Vec3 rgb;
    struct { Vec2 xy; Vec2 zw; };
    struct { Vec2 rg; Vec2 ba; };
    struct { float x_; Vec3 yzw; };
    struct { float r_; Vec3 gba; };
    struct { float x__, y__, width, height; };
    struct { Vec2 pos, size; };
    float p[4];
  };
} Vec4;
typedef Vec4 float4;

VEC_INLINE static Vec4 floor4(Vec4 v) { Vec4 ret = { floorf(v.x), floorf(v.y), floorf(v.z), floorf(v.w), }; return ret; }
VEC_INLINE static Vec4 vec4(float x,float y,float z,float w) {Vec4 r = {x,y,z,w}; return r;}
VEC_INLINE static Vec4 vec4_xyz_w(Vec3 xyz,float w) {Vec4 r = {xyz.x,xyz.y,xyz.z,w}; return r;}
VEC_INLINE static Vec4 vec4_xy_zw(Vec2 xy,Vec2 zw) {Vec4 r = {xy.x,xy.y,zw.x,zw.y}; return r;}
VEC_INLINE static Vec4 vec4_xy_z_w(Vec2 xy,float z, float w) {Vec4 r = {xy.x,xy.y,z,w}; return r;}
VEC_INLINE static Vec4 make4(float x,float y,float z,float w) {Vec4 r = {x,y,z,w}; return r;}
VEC_INLINE static float size4(Vec4 v) { return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w); }
VEC_INLINE static Vec4 norm4(Vec4 v) { float size = size4(v); Vec4 r = { v.x/size,v.y/size,v.z/size,v.w/size }; return r; }
VEC_INLINE static Vec4 add4(Vec4 v, Vec4 w) { Vec4 r = {v.x+w.x, v.y+w.y, v.z+w.z, v.w+w.w}; return r;}
VEC_INLINE static Vec4 sub4(Vec4 v, Vec4 w) { Vec4 r = {v.x-w.x, v.y-w.y, v.z-w.z, v.w-w.w}; return r;}
VEC_INLINE static Vec4 div4(Vec4 v, Vec4 w) { Vec4 r = {v.x/w.x, v.y/w.y, v.z/w.z, v.w/w.w}; return r;}
VEC_INLINE static Vec4 mul4(Vec4 v, Vec4 w) { Vec4 r = {v.x*w.x, v.y*w.y, v.z*w.z, v.w*w.w}; return r;}
VEC_INLINE static Vec4 smul4(float s, Vec4 v) { Vec4 r = {v.x*s, v.y*s, v.z*s, v.w*s}; return r;}
VEC_INLINE static Vec4 muls4(Vec4 v, float s) { Vec4 r = {v.x*s, v.y*s, v.z*s, v.w*s}; return r;}
VEC_INLINE static Vec4 divs4(Vec4 v, float s) { Vec4 r = {v.x/s, v.y/s, v.z/s, v.w/s}; return r;}
VEC_INLINE static Vec4 neg4(Vec4 v) { Vec4 r = {-v.x, -v.y, -v.z, -v.w}; return r;}
VEC_INLINE static Vec3 rgb4(Vec4 v) { Vec3 r = {v.x, v.y, v.z}; return r; }
VEC_INLINE static float dot4(Vec4 VecOne, Vec4 VecTwo) {
  return (VecOne.x * VecTwo.x) + (VecOne.y * VecTwo.y) + (VecOne.z * VecTwo.z) + (VecOne.w * VecTwo.w);
}
VEC_INLINE static unsigned char eq4(Vec4 a, Vec4 b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }

VEC_INLINE static Vec4 min4(Vec4 a, Vec4 b) { Vec4 ret = { hby_vec_minf(a.x,b.x),hby_vec_minf(a.y,b.y),hby_vec_minf(a.z,b.z),hby_vec_minf(a.w,b.w) }; return ret; }
VEC_INLINE static Vec4 max4(Vec4 a, Vec4 b) { Vec4 ret = { hby_vec_maxf(a.x,b.x),hby_vec_maxf(a.y,b.y),hby_vec_maxf(a.z,b.z),hby_vec_maxf(a.w,b.w) }; return ret; }

#ifdef __cplusplus
VEC_INLINE static float length(Vec2 v) { return length2(v); }
VEC_INLINE static Vec2 normalize(Vec2 v) { return norm2(v); }
VEC_INLINE static Vec2 abs(Vec2 v) { return abs2(v); }
VEC_INLINE static Vec2 floor(Vec2 v) { return floor2(v); }
VEC_INLINE static int operator==(Vec2 a, Vec2 b) { return eq2(a, b); }
VEC_INLINE static float dot(Vec2 a, Vec2 b) { return dot2(a, b); }
VEC_INLINE static Vec2 operator+(Vec2 a, Vec2 b)  { return add2(a, b); }
VEC_INLINE static Vec2 operator-(Vec2 a, Vec2 b)  { return sub2(a, b); }
VEC_INLINE static Vec2 operator*(Vec2 a, Vec2 b)  { return mul2(a, b); }
VEC_INLINE static Vec2 operator*(float s, Vec2 v) { return smul2(s, v); }
VEC_INLINE static Vec2 operator*(Vec2 v, float s) { return muls2(v, s); }
VEC_INLINE static Vec2 operator/(Vec2 a, Vec2 b)  { return div2(a, b); }
VEC_INLINE static Vec2 operator/(Vec2 v, float s) { return divs2(v,s); }
VEC_INLINE static Vec2 operator-(Vec2 v) { return neg2(v); }
VEC_INLINE static Vec2 &operator+=(Vec2 &a, Vec2 b)  { a = a+b; return a; }
VEC_INLINE static Vec2 &operator-=(Vec2 &a, Vec2 b)  { a = a-b; return a; }
VEC_INLINE static Vec2 &operator*=(Vec2 &a, Vec2 b)  { a = a*b; return a; }
VEC_INLINE static Vec2 &operator*=(Vec2 &a, float s)  { a = a*s; return a; }
VEC_INLINE static Vec2 &operator/=(Vec2 &a, Vec2 b)  { a = a/b; return a; }
VEC_INLINE static Vec2 &operator/=(Vec2 &a, float s)  { a = a/s; return a; }

VEC_INLINE static float length(Vec3 v) { return length3(v); }
VEC_INLINE static Vec3 normalize(Vec3 v) { return norm3(v); }
VEC_INLINE static Vec3 abs(Vec3 v) { return abs3(v); }
VEC_INLINE static float dot(Vec3 a, Vec3 b) { return dot3(a, b); }
VEC_INLINE static Vec3 operator+(Vec3 a, Vec3 b)  { return add3(a, b); }
VEC_INLINE static Vec3 operator-(Vec3 a, Vec3 b)  { return sub3(a, b); }
VEC_INLINE static Vec3 operator*(Vec3 a, Vec3 b)  { return mul3(a, b); }
VEC_INLINE static Vec3 operator*(float s, Vec3 v) { return smul3(s, v); }
VEC_INLINE static Vec3 operator*(Vec3 v, float s) { return muls3(v, s); }
VEC_INLINE static Vec3 operator/(Vec3 a, Vec3 b)  { return div3(a, b); }
VEC_INLINE static Vec3 operator/(Vec3 v, float s) { return divs3(v,s); }
VEC_INLINE static Vec3 operator-(Vec3 v) { return neg3(v); }
VEC_INLINE static Vec3 &operator+=(Vec3 &a, Vec3 b)  { a = a+b; return a; }
VEC_INLINE static Vec3 &operator-=(Vec3 &a, Vec3 b)  { a = a-b; return a; }
VEC_INLINE static Vec3 &operator*=(Vec3 &a, Vec3 b)  { a = a*b; return a; }
VEC_INLINE static Vec3 &operator*=(Vec3 &a, float s)  { a = a*s; return a; }
VEC_INLINE static Vec3 &operator/=(Vec3 &a, Vec3 b)  { a = a/b; return a; }
VEC_INLINE static Vec3 &operator/=(Vec3 &a, float s)  { a = a/s; return a; }
VEC_INLINE static Vec3 project_vec3(Vec3 a, Vec3 b) { return dot(a,b) / dot(b,b) * b; }

VEC_INLINE static float length(Vec3d v) { return length3d(v); }
VEC_INLINE static Vec3d normalize(Vec3d v) { return norm3d(v); }
VEC_INLINE static Vec3d abs(Vec3d v) { return abs3d(v); }
VEC_INLINE static float dot(Vec3d a, Vec3d b) { return dot3d(a, b); }
VEC_INLINE static Vec3d operator+(Vec3d a, Vec3d b)  { return add3d(a, b); }
VEC_INLINE static Vec3d operator-(Vec3d a, Vec3d b)  { return sub3d(a, b); }
VEC_INLINE static Vec3d operator*(Vec3d a, Vec3d b)  { return mul3d(a, b); }
VEC_INLINE static Vec3d operator*(float s, Vec3d v) { return smul3d(s, v); }
VEC_INLINE static Vec3d operator*(Vec3d v, float s) { return muls3d(v, s); }
VEC_INLINE static Vec3d operator/(Vec3d a, Vec3d b)  { return div3d(a, b); }
VEC_INLINE static Vec3d operator/(Vec3d v, float s) { return divs3d(v,s); }
VEC_INLINE static Vec3d operator-(Vec3d v) { return neg3d(v); }
VEC_INLINE static Vec3d &operator+=(Vec3d &a, Vec3d b)  { a = a+b; return a; }
VEC_INLINE static Vec3d &operator-=(Vec3d &a, Vec3d b)  { a = a-b; return a; }
VEC_INLINE static Vec3d &operator*=(Vec3d &a, Vec3d b)  { a = a*b; return a; }
VEC_INLINE static Vec3d &operator*=(Vec3d &a, float s)  { a = a*s; return a; }
VEC_INLINE static Vec3d &operator/=(Vec3d &a, Vec3d b)  { a = a/b; return a; }
VEC_INLINE static Vec3d &operator/=(Vec3d &a, float s)  { a = a/s; return a; }

VEC_INLINE static Vec4 operator+(Vec4 a, Vec4 b)  { return add4(a, b); }
VEC_INLINE static Vec4 operator-(Vec4 a, Vec4 b)  { return sub4(a, b); }
VEC_INLINE static Vec4 operator*(Vec4 a, Vec4 b)  { return mul4(a, b); }
VEC_INLINE static Vec4 operator*(float s, Vec4 v) { return smul4(s, v); }
VEC_INLINE static Vec4 operator*(Vec4 v, float s) { return muls4(v, s); }
VEC_INLINE static Vec4 operator/(Vec4 a, Vec4 b)  { return div4(a, b); }
VEC_INLINE static Vec4 operator/(Vec4 v, float s) { return divs4(v,s); }
VEC_INLINE static Vec4 operator-(Vec4 v) { return neg4(v); }
VEC_INLINE static Vec4 &operator+=(Vec4 &a, Vec4 b)  { a = a+b; return a; }
VEC_INLINE static Vec4 &operator-=(Vec4 &a, Vec4 b)  { a = a-b; return a; }
VEC_INLINE static Vec4 &operator*=(Vec4 &a, Vec4 b)  { a = a*b; return a; }
VEC_INLINE static Vec4 &operator*=(Vec4 &a, float s)  { a = a*s; return a; }
VEC_INLINE static Vec4 &operator/=(Vec4 &a, Vec4 b)  { a = a/b; return a; }
VEC_INLINE static Vec4 &operator/=(Vec4 &a, float s)  { a = a/s; return a; }
#endif

typedef struct Vec2i16 {
  union {
    int16_t p[2];
    int16_t data[2];
    struct { int16_t x,y; };
  };
} Vec2i16;

VEC_INLINE static Vec2 vec2i16_to_vec2(Vec2i16 in) {
  Vec2 ret = {0};
  ret.x = (float)in.x;
  ret.y = (float)in.y;
  return ret;
}
VEC_INLINE static Vec2i16 vec2_to_vec2i16(Vec2 in) {
  Vec2i16 ret = {0};
  ret.x = (int16_t)in.x;
  ret.y = (int16_t)in.y;
  return ret;
}

typedef struct Vec4i16 {
  union {
    int16_t p[4];
    int16_t data[4];
    struct { int16_t x,y,z,w; };
  };
} Vec4i16;

VEC_INLINE static Vec4 vec4i16_to_vec4(Vec4i16 in) {
  Vec4 ret = {0};
  ret.x = (float)in.x;
  ret.y = (float)in.y;
  ret.z = (float)in.z;
  ret.w = (float)in.w;
  return ret;
}
VEC_INLINE static Vec4i16 vec4_to_vec4i16(Vec4 in) {
  Vec4i16 ret = {0};
  ret.x = (int16_t)in.x;
  ret.y = (int16_t)in.y;
  ret.z = (int16_t)in.z;
  ret.w = (int16_t)in.w;
  return ret;
}
VEC_INLINE static Vec4i16 vec4_to_vec4i16_normalized(Vec4 in) {
  float MAX_UINT16 = (float)(uint16_t)-1;
  Vec4i16 ret = {0};
  ret.x = (int16_t)(uint16_t)(in.x * MAX_UINT16);
  ret.y = (int16_t)(uint16_t)(in.y * MAX_UINT16);
  ret.z = (int16_t)(uint16_t)(in.z * MAX_UINT16);
  ret.w = (int16_t)(uint16_t)(in.w * MAX_UINT16);
  return ret;
}
VEC_INLINE static Vec4 vec4i16_normalized_to_vec4(Vec4i16 in) {
  float MAX_UINT16 = (float)(uint16_t)-1;
  Vec4 ret = {0};
  ret.x = (float)(uint16_t)in.x / MAX_UINT16;
  ret.y = (float)(uint16_t)in.y / MAX_UINT16;
  ret.z = (float)(uint16_t)in.z / MAX_UINT16;
  ret.w = (float)(uint16_t)in.w / MAX_UINT16;
  return ret;
}

typedef struct Mat4 {
  union {
    float data[16];
    float Elements[4][4];
    Vec4 Columns[4];
  };
} Mat4;

typedef struct Mat4x3 {
  float data[12];
} Mat4x3;

static VEC_INLINE Mat4x3 mat4_to_mat4x3(Mat4 mat) {
  Mat4x3 ret = {
    mat.data[0],  mat.data[1],  mat.data[2],
    mat.data[4],  mat.data[5],  mat.data[6],
    mat.data[8],  mat.data[9],  mat.data[10],
    mat.data[12], mat.data[13], mat.data[14],
  };
  return ret;
}

static VEC_INLINE Mat4 mat4x3_to_mat4(Mat4x3 mat) {
  const float *t = mat.data;
  Mat4 ret = {
    t[0],t[1], t[2], 0.f,
    t[3],t[4], t[5], 0.f,
    t[6],t[7], t[8], 0.f,
    t[9],t[10],t[11],1.f,
  };
  return ret;
}

const static Mat4 MAT4_IDENTITY = {
  1.f, 0.f, 0.f, 0.f,
  0.f, 1.f, 0.f, 0.f,
  0.f, 0.f, 1.f, 0.f,
  0.f, 0.f, 0.f, 1.f,
};

static VEC_INLINE Mat4 mat4_identity() {
  return MAT4_IDENTITY;
}

static VEC_INLINE void mat4_translate_post(Mat4 *m, float tx, float ty, float tz) {
  float *t = m->data;
  t[12] += t[0]*tx + t[4]*ty + t[8 ]*tz;
  t[13] += t[1]*tx + t[5]*ty + t[9 ]*tz;
  t[14] += t[2]*tx + t[6]*ty + t[10]*tz;
}

static VEC_INLINE void mat4_scale_post(Mat4 *m, float sx, float sy, float sz) {
  float *t = m->data;
  t[0]  *= sx;
  t[4]  *= sy;
  t[8]  *= sz;

  t[1]  *= sx;
  t[5]  *= sy;
  t[9]  *= sz;

  t[2]  *= sx;
  t[6]  *= sy;
  t[10] *= sz;
}

static VEC_INLINE void mat4_rotate_z_post(Mat4 *m, float r) {
  float *t = m->data;
  float Sin, Cos;
  float x,y,z,w;
  float a,b,e,f,i,j;

  r = r / 180.f * 3.1415926f;

  Sin = sinf(r);
  Cos = cosf(r);

  x = Cos;
  y = -Sin;
  z = Sin;
  w = Cos;

  a = t[0];
  b = t[4];
  e = t[1];
  f = t[5];
  i = t[2];
  j = t[6];

  t[0] = a*x+b*z;
  t[1] = e*x+f*z;
  t[2] = i*x+j*z;

  t[4] = a*y+b*w;
  t[5] = e*y+f*w;
  t[6] = i*y+j*w;
}

static VEC_INLINE void mat4_rotate_x_post(Mat4 *m, float r) {
  float *t = m->data;
  float Sin, Cos;
  float x,y,z,w;
  float b,c,f,g,j,k;

  r = r / 180.f * 3.1415926f;

  Sin = sinf(r);
  Cos = cosf(r);

  x = Cos;
  y = -Sin;
  z = Sin;
  w = Cos;

  b = t[4];
  c = t[8];
  f = t[5];
  g = t[9];
  j = t[6];
  k = t[10];

  t[4]  = b*x+c*z;
  t[5]  = f*x+g*z;
  t[6]  = j*x+k*z;

  t[8]  = b*y+c*w;
  t[9]  = f*y+g*w;
  t[10] = j*y+k*w;
}

static VEC_INLINE void mat4_rotate_y_post(Mat4 *m, float r) {
  float *t = m->data;
  float Sin, Cos;
  float x,y,z,w;
  float a,c,e,g,i,k;

  r = r / 180.f * 3.1415926f;

  Sin = sinf(r);
  Cos = cosf(r);

  x = Cos;
  y = Sin;
  z = -Sin;
  w = Cos;

  a = t[0];
  c = t[8];
  e = t[1];
  g = t[9];
  i = t[2];
  k = t[10];

  t[0]  = a*x+c*z;
  t[1]  = e*x+g*z;
  t[2]  = i*x+k*z;

  t[8]  = a*y+c*w;
  t[9]  = e*y+g*w;
  t[10] = i*y+k*w;
}

static VEC_INLINE Mat4 mat4_projection_orthographic(float width, float height, float near_, float far_, float zdir) {
  Mat4 ret;
  float *t = ret.data;
  float z_dir = 1;

  memset(t, 0, 16 * sizeof(float));
  t[0]  = 2.f / width;
  t[5]  = 2.f / height;
  t[10] = z_dir * 2.f / (far_ - near_);
  t[14] = -(far_ + near_) / (far_ - near_);
  t[15] = 1;

  return ret;
}

static VEC_INLINE Mat4 mat4_projection_perspective(float fov_x_deg, float fov_y_deg, float near_, float far_, float zdir) {
  Mat4 ret;
  float *t = ret.data;
  float px, py;
  float pz1, pz2, pw;

  memset(t, 0, 16 * sizeof(float));
  px = 1.0f / tanf(fov_x_deg * 0.5f * 3.1415926f / 180.f);
  py = 1.0f / tanf(fov_y_deg * 0.5f * 3.1415926f / 180.f);
  zdir = (zdir ? zdir : 1);
  pz1 = zdir * (far_ + near_) / (far_ - near_);
  pz2 = -2 * (far_ * near_) / (far_ - near_);
  pw = zdir;
  t[0]  = px;
  t[5]  = py;
  t[10] = pz1;
  t[11] = pw;
  t[14] = pz2;

  // px  0   0   0
  // 0   py  0   0
  // 0   0   pz1 pz2
  // 0   0   pw  0

  return ret;
}

static VEC_INLINE Mat4 mat4_rotate_y(float r) {
  Mat4 ret = mat4_identity();
  mat4_rotate_y_post(&ret, r);
  return ret;
}

static VEC_INLINE Mat4 mat4_rotate_z(float r) {
  Mat4 ret = mat4_identity();
  mat4_rotate_z_post(&ret, r);
  return ret;
}
static VEC_INLINE Mat4 mat4_rotate_x(float r) {
  Mat4 ret = mat4_identity();
  mat4_rotate_x_post(&ret, r);
  return ret;
}
static VEC_INLINE Mat4 mat4_translate(float tx, float ty, float tz) {
  Mat4 ret = mat4_identity();
  ret.data[12] = tx;
  ret.data[13] = ty;
  ret.data[14] = tz;
  return ret;
}
static VEC_INLINE Mat4 mat4_scale(float sx, float sy, float sz) {
  Mat4 ret = {0};
  ret.data[0] = sx;
  ret.data[5] = sy;
  ret.data[10] = sz;
  ret.data[15] = 1;
  return ret;
}

static VEC_INLINE Vec4 mat4_multiply_vec4(Mat4 Matrix, Vec4 Vector) {
  Vec4 Result;
  int Columns, Rows;
  for(Rows = 0; Rows < 4; ++Rows)
  {
    float Sum = 0;
    for(Columns = 0; Columns < 4; ++Columns)
    {
      Sum += Matrix.Elements[Columns][Rows] * Vector.p[Columns];
    }
    Result.p[Rows] = Sum;
  }
  return (Result);
}

static VEC_INLINE Vec3 mat4_multiply_pos(Mat4 Matrix, Vec3 pos) {
  return mat4_multiply_vec4(Matrix, vec4_xyz_w(pos, 1)).xyz;
}
static VEC_INLINE Vec3 mat4_multiply_dir(Mat4 Matrix, Vec3 dir) {
  return mat4_multiply_vec4(Matrix, vec4_xyz_w(dir, 0)).xyz;
}
static VEC_INLINE Vec3 mat4_mul_pos(Mat4 Matrix, Vec3 pos) {
  return mat4_multiply_pos(Matrix, pos);
}
static VEC_INLINE Vec3 mat4_mul_dir(Mat4 Matrix, Vec3 dir) {
  return mat4_multiply_dir(Matrix, dir);
}

static VEC_INLINE Mat4 mat4_mul(Mat4 left, Mat4 right) {
  float *a = left.data;
  float *b = right.data;
  Mat4 ret = {0};
  float *t = ret.data;
  t[0 ] = a[0 ]*b[0 ] + a[4 ]*b[1 ] + a[8 ]*b[2 ] + a[12]*b[3 ];
  t[1 ] = a[1 ]*b[0 ] + a[5 ]*b[1 ] + a[9 ]*b[2 ] + a[13]*b[3 ];
  t[2 ] = a[2 ]*b[0 ] + a[6 ]*b[1 ] + a[10]*b[2 ] + a[14]*b[3 ];
  t[3 ] = a[3 ]*b[0 ] + a[7 ]*b[1 ] + a[11]*b[2 ] + a[15]*b[3 ];

  t[4 ] = a[0 ]*b[4 ] + a[4 ]*b[5 ] + a[8 ]*b[6 ] + a[12]*b[7 ];
  t[5 ] = a[1 ]*b[4 ] + a[5 ]*b[5 ] + a[9 ]*b[6 ] + a[13]*b[7 ];
  t[6 ] = a[2 ]*b[4 ] + a[6 ]*b[5 ] + a[10]*b[6 ] + a[14]*b[7 ];
  t[7 ] = a[3 ]*b[4 ] + a[7 ]*b[5 ] + a[11]*b[6 ] + a[15]*b[7 ];

  t[8 ] = a[0 ]*b[8 ] + a[4 ]*b[9 ] + a[8 ]*b[10] + a[12]*b[11];
  t[9 ] = a[1 ]*b[8 ] + a[5 ]*b[9 ] + a[9 ]*b[10] + a[13]*b[11];
  t[10] = a[2 ]*b[8 ] + a[6 ]*b[9 ] + a[10]*b[10] + a[14]*b[11];
  t[11] = a[3 ]*b[8 ] + a[7 ]*b[9 ] + a[11]*b[10] + a[15]*b[11];

  t[12] = a[0 ]*b[12] + a[4 ]*b[13] + a[8 ]*b[14] + a[12]*b[15];
  t[13] = a[1 ]*b[12] + a[5 ]*b[13] + a[9 ]*b[14] + a[13]*b[15];
  t[14] = a[2 ]*b[12] + a[6 ]*b[13] + a[10]*b[14] + a[14]*b[15];
  t[15] = a[3 ]*b[12] + a[7 ]*b[13] + a[11]*b[14] + a[15]*b[15];
  return ret;
}

VEC_INLINE static Mat4 mat4_transpose(Mat4 Matrix) {
  Mat4 Result;
  int Columns, Rows;
  for(Columns = 0; Columns < 4; ++Columns) {
    for(Rows = 0; Rows < 4; ++Rows) {
      Result.Elements[Rows][Columns] = Matrix.Elements[Columns][Rows];
    }
  }
  return (Result);
}

#ifdef __cplusplus
static VEC_INLINE Mat4 operator*(Mat4 a, Mat4 b) { return mat4_mul(a, b); }
static VEC_INLINE Mat4 &operator*=(Mat4 &a, Mat4 b) { a = mat4_mul(a, b); return a; }
static VEC_INLINE Vec4 operator*(Mat4 a, Vec4 b) { return mat4_multiply_vec4(a, b); }
#endif

VEC_INLINE static Mat4 mat4_look_at(Vec3 Eye, Vec3 Center, Vec3 Up)
{
    Mat4 Result;

    Vec3 F = norm3(sub3(Center, Eye));
    Vec3 S = norm3(cross3(F, Up));
    Vec3 U = cross3(S, F);

    Result.Elements[0][0] = S.x;
    Result.Elements[0][1] = U.x;
    Result.Elements[0][2] = -F.x;
    Result.Elements[0][3] = 0.0f;

    Result.Elements[1][0] = S.y;
    Result.Elements[1][1] = U.y;
    Result.Elements[1][2] = -F.y;
    Result.Elements[1][3] = 0.0f;

    Result.Elements[2][0] = S.z;
    Result.Elements[2][1] = U.z;
    Result.Elements[2][2] = -F.z;
    Result.Elements[2][3] = 0.0f;

    Result.Elements[3][0] = -dot3(S, Eye);
    Result.Elements[3][1] = -dot3(U, Eye);
    Result.Elements[3][2] = dot3(F, Eye);
    Result.Elements[3][3] = 1.0f;

    return (Result);
}

// Courtesy of handmade math
VEC_INLINE Mat4 mat4_inverse(Mat4 Matrix) {
  Vec3 C01 = cross3(Matrix.Columns[0].xyz, Matrix.Columns[1].xyz);
  Vec3 C23 = cross3(Matrix.Columns[2].xyz, Matrix.Columns[3].xyz);
  Vec3 B10 = sub3(muls3(Matrix.Columns[0].xyz, Matrix.Columns[1].w), muls3(Matrix.Columns[1].xyz, Matrix.Columns[0].w));
  Vec3 B32 = sub3(muls3(Matrix.Columns[2].xyz, Matrix.Columns[3].w), muls3(Matrix.Columns[3].xyz, Matrix.Columns[2].w));

  float InvDeterminant = 1.0f / (dot3(C01, B32) + dot3(C23, B10));
  C01 = muls3(C01, InvDeterminant);
  C23 = muls3(C23, InvDeterminant);
  B10 = muls3(B10, InvDeterminant);
  B32 = muls3(B32, InvDeterminant);

  Mat4 Result;
  Result.Columns[0] = vec4_xyz_w(add3(cross3(Matrix.Columns[1].xyz, B32), muls3(C23, Matrix.Columns[1].w)), -dot3(Matrix.Columns[1].xyz, C23));
  Result.Columns[1] = vec4_xyz_w(sub3(cross3(B32, Matrix.Columns[0].xyz), muls3(C23, Matrix.Columns[0].w)), +dot3(Matrix.Columns[0].xyz, C23));
  Result.Columns[2] = vec4_xyz_w(add3(cross3(Matrix.Columns[3].xyz, B10), muls3(C01, Matrix.Columns[3].w)), -dot3(Matrix.Columns[3].xyz, C01));
  Result.Columns[3] = vec4_xyz_w(sub3(cross3(B10, Matrix.Columns[2].xyz), muls3(C01, Matrix.Columns[2].w)), +dot3(Matrix.Columns[2].xyz, C01));

  return mat4_transpose(Result);
}

typedef struct Quaternion {
  union {
    struct {
      float x,y,z,w;
    };
    struct {
      float X,Y,Z,W;
    };
    float p[4];
  };
} Quaternion;

VEC_INLINE static Quaternion quaternion_identity() {
  Quaternion ret;
  ret.x = 0;
  ret.y = 0;
  ret.z = 0;
  ret.w = 1;
  return ret;
}
VEC_INLINE static unsigned char quaternion_eq(Quaternion a, Quaternion b) {
  return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

VEC_INLINE static Vec3 quaternion_to_euler(Quaternion q) {
  float q0 = q.w;
  float q1 = q.x;
  float q2 = q.y;
  float q3 = q.z;
  float x = atan2f(2.f*(q0*q1 + q2*q3), (1.f - 2.f*(q1*q1 + q2*q2)));
  float y = asinf( 2.f*(q0*q2 - q3*q1));
  float z = atan2f(2.f*(q0*q3 + q1*q2), (1.f - 2.f*(q2*q2 + q3*q3)));
  Vec3 ret = {
    x * 180.f / 3.1415926f,
    y * 180.f / 3.1415926f,
    z * 180.f / 3.1415926f,
  };
  return ret;
}
VEC_INLINE static Quaternion euler_to_quaternion(Vec3 euler) {
  float x = euler.x * 3.1415926f / 180.f;
  float y = euler.y * 3.1415926f / 180.f;
  float z = euler.z * 3.1415926f / 180.f;
  float cx = cosf(x * 0.5f);
  float cy = cosf(y * 0.5f);
  float cz = cosf(z * 0.5f);
  float sx = sinf(x * 0.5f);
  float sy = sinf(y * 0.5f);
  float sz = sinf(z * 0.5f);
  Quaternion ret;
  ret.w = cx*cy*cz + sx*sy*sz;
  ret.x = sx*cy*cz - cx*sy*sz;
  ret.y = cx*sy*cz + sx*cy*sz;
  ret.z = cx*cy*sz - sx*sy*cz;
  return ret;
}
VEC_INLINE static Quaternion quaternion_multiply_scalar(Quaternion Left, float Multiplicative) {
    Quaternion Result;
    Result.X = Left.X * Multiplicative;
    Result.Y = Left.Y * Multiplicative;
    Result.Z = Left.Z * Multiplicative;
    Result.W = Left.W * Multiplicative;
    return (Result);
}
VEC_INLINE static Quaternion quaternion_div_scalar(Quaternion Left, float Dividend) {
    Quaternion Result;
    Result.X = Left.X / Dividend;
    Result.Y = Left.Y / Dividend;
    Result.Z = Left.Z / Dividend;
    Result.W = Left.W / Dividend;
    return (Result);
}

VEC_INLINE static float quaternion_dot(Quaternion Left, Quaternion Right) {
    float Result;
    Result = (Left.X * Right.X) + (Left.Y * Right.Y) + (Left.Z * Right.Z) + (Left.W * Right.W);
    return (Result);
}

VEC_INLINE static Quaternion quaternion_inverse(Quaternion Left) {
    Quaternion Result;

    Result.X = -Left.X;
    Result.Y = -Left.Y;
    Result.Z = -Left.Z;
    Result.W = Left.W;

    Result = quaternion_div_scalar(Result, (quaternion_dot(Left, Left)));

    return (Result);
}

VEC_INLINE static Quaternion normalize_quaternion(Quaternion Left) {
    Quaternion Result;

    float Length = sqrtf(quaternion_dot(Left, Left));
    Result = quaternion_div_scalar(Left, Length);

    return (Result);
}


VEC_INLINE static Mat4 quaternion_to_mat4(Quaternion Left) {
    Mat4 Result = {0};

    Quaternion NormalizedQuaternion = normalize_quaternion(Left);

    float XX, YY, ZZ,
          XY, XZ, YZ,
          WX, WY, WZ;

    XX = NormalizedQuaternion.X * NormalizedQuaternion.X;
    YY = NormalizedQuaternion.Y * NormalizedQuaternion.Y;
    ZZ = NormalizedQuaternion.Z * NormalizedQuaternion.Z;
    XY = NormalizedQuaternion.X * NormalizedQuaternion.Y;
    XZ = NormalizedQuaternion.X * NormalizedQuaternion.Z;
    YZ = NormalizedQuaternion.Y * NormalizedQuaternion.Z;
    WX = NormalizedQuaternion.W * NormalizedQuaternion.X;
    WY = NormalizedQuaternion.W * NormalizedQuaternion.Y;
    WZ = NormalizedQuaternion.W * NormalizedQuaternion.Z;

    Result.Elements[0][0] = 1.0f - 2.0f * (YY + ZZ);
    Result.Elements[0][1] = 2.0f * (XY + WZ);
    Result.Elements[0][2] = 2.0f * (XZ - WY);
    Result.Elements[0][3] = 0.0f;

    Result.Elements[1][0] = 2.0f * (XY - WZ);
    Result.Elements[1][1] = 1.0f - 2.0f * (XX + ZZ);
    Result.Elements[1][2] = 2.0f * (YZ + WX);
    Result.Elements[1][3] = 0.0f;

    Result.Elements[2][0] = 2.0f * (XZ + WY);
    Result.Elements[2][1] = 2.0f * (YZ - WX);
    Result.Elements[2][2] = 1.0f - 2.0f * (XX + YY);
    Result.Elements[2][3] = 0.0f;

    Result.Elements[3][0] = 0.0f;
    Result.Elements[3][1] = 0.0f;
    Result.Elements[3][2] = 0.0f;
    Result.Elements[3][3] = 1.0f;

    return (Result);
}

VEC_INLINE static Vec3 rotate_dir(Quaternion q, Vec3 dir) {
  return mat4_multiply_dir(quaternion_to_mat4(q), dir);
}

VEC_INLINE static Quaternion quaternion_axis(Vec3 axis, float deg) {
  Quaternion ret;

  float rad = deg * 3.1415926f / 180.f;

  Vec3 norm_axis = norm3(axis);
  float SineOfRotation = sinf(rad / 2.0f);

  Vec3 xyz = muls3(norm_axis, SineOfRotation);
  ret.x = xyz.x;
  ret.y = xyz.y;
  ret.z = xyz.z;

  ret.w = cosf(rad / 2.0f);

  return ret;
}

VEC_INLINE static Quaternion quaternion_y(float r) {
  return quaternion_axis(vec3(0,1,0), r);
}
VEC_INLINE static Quaternion quaternion_x(float r) {
  return quaternion_axis(vec3(1,0,0), r);
}
VEC_INLINE static Quaternion quaternion_z(float r) {
  return quaternion_axis(vec3(0,0,1), r);
}

VEC_INLINE Quaternion quaternion_multiply(Quaternion Left, Quaternion Right) {
    Quaternion Result;
    Result.x = (Left.x * Right.w) + (Left.y * Right.z) - (Left.z * Right.y) + (Left.w * Right.x);
    Result.y = (-Left.x * Right.z) + (Left.y * Right.w) + (Left.z * Right.x) + (Left.w * Right.y);
    Result.z = (Left.x * Right.y) - (Left.y * Right.x) + (Left.z * Right.w) + (Left.w * Right.z);
    Result.w = (-Left.x * Right.x) - (Left.y * Right.y) - (Left.z * Right.z) + (Left.w * Right.w);
    return (Result);
}
VEC_INLINE Quaternion quaternion_mul(Quaternion Left, Quaternion Right) {
  return quaternion_multiply(Left, Right);
}

#ifdef __cplusplus
VEC_INLINE static Quaternion operator*(Quaternion a, Quaternion b) {
  return quaternion_multiply(a, b);
}
VEC_INLINE static Quaternion operator*(Quaternion a, float scaler) {
  return quaternion_multiply_scalar(a, scaler);
}
#endif

VEC_INLINE static Vec3 mat4_get_pos(Mat4 m) {
  return vec3(m.data[12], m.data[13], m.data[14]);
}

// FIXME: This is almost certainly not correct.
VEC_INLINE static float mat4_get_scale(Mat4 m) {
  return length3(vec3(m.data[0], m.data[1], m.data[2]));
}

VEC_INLINE static Quaternion mat4_to_quaternion(Mat4 M)
{
    float T;
    Quaternion Q;
    if (M.Elements[2][2] < 0.0f) {
        if (M.Elements[0][0] > M.Elements[1][1]) {
            Quaternion r = {
              (T = 1 + M.Elements[0][0] - M.Elements[1][1] - M.Elements[2][2]),
              M.Elements[0][1] + M.Elements[1][0],
              M.Elements[2][0] + M.Elements[0][2],
              M.Elements[1][2] - M.Elements[2][1]
            };
            Q = r;
        } else {
            Quaternion r = {
                M.Elements[0][1] + M.Elements[1][0],
                (T = 1 - M.Elements[0][0] + M.Elements[1][1] - M.Elements[2][2]),
                M.Elements[1][2] + M.Elements[2][1],
                M.Elements[2][0] - M.Elements[0][2]
            };
            Q = r;
        }
    } else {
        if (M.Elements[0][0] < -M.Elements[1][1]) {
            Quaternion r = {
                M.Elements[2][0] + M.Elements[0][2],
                M.Elements[1][2] + M.Elements[2][1],
                (T = 1 - M.Elements[0][0] - M.Elements[1][1] + M.Elements[2][2]),
                M.Elements[0][1] - M.Elements[1][0]
            };
            Q = r;
        } else {
            Quaternion r = {
                M.Elements[1][2] - M.Elements[2][1],
                M.Elements[2][0] - M.Elements[0][2],
                M.Elements[0][1] - M.Elements[1][0],
                (T = 1 + M.Elements[0][0] + M.Elements[1][1] + M.Elements[2][2]),
            };
            Q = r;
        }
    }
    Q = quaternion_multiply_scalar(Q, (0.5f / sqrtf(T)));
    return Q;
}

VEC_INLINE Quaternion add_quaternion(Quaternion Left, Quaternion Right)
{
    Quaternion Result;

#if 0
    Result.InternalElementsSSE = _mm_add_ps(Left.InternalElementsSSE, Right.InternalElementsSSE);
#else

    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    Result.Z = Left.Z + Right.Z;
    Result.W = Left.W + Right.W;
#endif

    return (Result);
}

VEC_INLINE static float vec_lerp(float low, float high, float t) {
  t = t < 0 ? 0 : t;
  t = t > 1 ? 1 : t;
  return low + t * (high - low);
}

VEC_INLINE Quaternion quaternion_nlerp(Quaternion Left, Quaternion Right, float Time) {
  Quaternion Result;
#if 0
  __m128 ScalarLeft = _mm_set1_ps(1.0f - Time);
  __m128 ScalarRight = _mm_set1_ps(Time);
  __m128 SSEResultOne = _mm_mul_ps(Left.InternalElementsSSE, ScalarLeft);
  __m128 SSEResultTwo = _mm_mul_ps(Right.InternalElementsSSE, ScalarRight);
  Result.InternalElementsSSE = _mm_add_ps(SSEResultOne, SSEResultTwo);
#else
  Result.X = vec_lerp(Left.X, Right.X, Time);
  Result.Y = vec_lerp(Left.Y, Right.Y, Time);
  Result.Z = vec_lerp(Left.Z, Right.Z, Time);
  Result.W = vec_lerp(Left.W, Right.W, Time);
#endif
  Result = normalize_quaternion(Result);

  return (Result);
}

VEC_INLINE static Quaternion quaternion_slerp(Quaternion Left, Quaternion Right, float Time) {
  const float EPSILON = 0.0001f;
  Quaternion Result;
  Quaternion QuaternionLeft;
  Quaternion QuaternionRight;
  float Cos_Theta;
  float Angle;
  float S1, S2, Is;

  Cos_Theta = quaternion_dot(Left, Right);
  if (Cos_Theta < 0) {
    Right = quaternion_multiply_scalar(Right, -1);
    Cos_Theta *= -1;
  }
  if (Cos_Theta > 1.f - EPSILON) {
    return quaternion_nlerp(Left, Right, Time);
  }

  Angle = acosf(Cos_Theta);

  S1 = sinf((1.0f - Time) * Angle);
  S2 = sinf(Time * Angle);
  Is = 1.0f / sinf(Angle);

  QuaternionLeft = quaternion_multiply_scalar(Left, S1);
  QuaternionRight = quaternion_multiply_scalar(Right, S2);

  Result = add_quaternion(QuaternionLeft, QuaternionRight);
  Result = quaternion_multiply_scalar(Result, Is);

  return (Result);
}

VEC_INLINE static Quaternion get_shortest_quaternion_between(Quaternion a, Quaternion b) {
  if (quaternion_dot(a, b) < 0) {
    return quaternion_multiply(a, quaternion_inverse(quaternion_multiply_scalar(b, -1)));
  }
  else {
    return quaternion_multiply(a, quaternion_inverse(b));
  }
}

VEC_INLINE static Vec3 quaternion_rotate_vector(Quaternion q, Vec3 v) {
  Vec3 axis = {q.x, q.y, q.z};
  float angle = q.w;
  return 
    add3(
      add3(smul3(2.0f, smul3(dot3(axis, v), axis)),
           smul3((angle*angle - dot3(axis, axis)), v)),
      smul3(2.0f, smul3(angle, cross3(axis, v)))
    );
}

const static float VEC_EPSILON = 0.0001f;
#define VEC_EQ_F(a, b) (fabsf((a) - (b)) <= VEC_EPSILON)

VEC_INLINE static Quaternion get_quaternion_between(Vec3 p0, Vec3 p1) {
  Quaternion q = {0};
  float cos_theta = 0;

  p0 = norm3(p0);
  p1 = norm3(p1);
  cos_theta = dot3(p0, p1);
  if (!VEC_EQ_F(cos_theta, 1.f)) {
    Vec3 axis = {0};
    if (VEC_EQ_F(cos_theta, -1.f)) {
      axis = cross3(p0, vec3(1,0,0));
      if (VEC_EQ_F(length3(axis), 0.f)) {
        axis = cross3(p0, vec3(0,1,0));
      }
    }
    else {
      axis = cross3(p0, p1);
    }
    q.x = axis.x;
    q.y = axis.y;
    q.z = axis.z;
    q.w = 1.f + cos_theta;
    q = normalize_quaternion(q);
  }
  else {
    q = quaternion_identity();
  }
  return q;
}

static VEC_INLINE Mat4 mat4_post_mul_standard_transform(Mat4 m, Vec3 pos, Quaternion rot, float scale) {
  mat4_translate_post(&m, pos.x, pos.y, pos.z);
  m = mat4_mul(m, quaternion_to_mat4(rot));
  mat4_scale_post(&m, scale, scale, scale);
  return m;
}
static VEC_INLINE Mat4 mat4_post_mul_standard_transform2(Mat4 m, Vec3 pos, Quaternion rot, Vec3 scale) {
  mat4_translate_post(&m, pos.x, pos.y, pos.z);
  m = mat4_mul(m, quaternion_to_mat4(rot));
  mat4_scale_post(&m, scale.x, scale.y, scale.z);
  return m;
}

