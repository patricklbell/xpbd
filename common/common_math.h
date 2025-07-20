#pragma once

typedef union vec2_f32 vec2_f32;
union vec2_f32 {
    struct {
        f32 x;
        f32 y;
    };

    f32 v[2];
};

vec2_f32 make_2f32(f32 x, f32 y);
vec2_f32 add_2f32(vec2_f32 a, vec2_f32 b);
vec2_f32 sub_2f32(vec2_f32 a, vec2_f32 b);
vec2_f32 mul_2f32(vec2_f32 a, f32 b);
vec2_f32 elmul_2f32(vec2_f32 a, vec2_f32 b);
f32 dot_2f32(vec2_f32 a, vec2_f32 b);
f32 length_2f32(vec2_f32 a);
vec2_f32 normalize_2f32(vec2_f32 a);

typedef union vec3_f32 vec3_f32;
union vec3_f32 {
    struct {
        f32 x;
        f32 y;
        f32 z;
    };
    union {
        vec2_f32 xy;
        f32 _z;
    };
    union {
        f32 _x;
        vec2_f32 yz;
    };
    struct {
        f32 r;
        f32 g;
        f32 b;
    };

    f32 v[3];
};

vec3_f32 make_3f32(f32 x, f32 y, f32 z);
vec3_f32 make_up_3f32();
vec3_f32 add_3f32(vec3_f32 a, vec3_f32 b);
vec3_f32 sub_3f32(vec3_f32 a, vec3_f32 b);
vec3_f32 mul_3f32(vec3_f32 a, f32 b);
vec3_f32 elmul_2f32(vec3_f32 a, vec3_f32 b);
f32 dot_3f32(vec3_f32 a, vec3_f32 b);
f32 length_3f32(vec3_f32 a);
vec3_f32 normalize_3f32(vec3_f32 a);
vec3_f32 cross_3f32(vec3_f32 a, vec3_f32 b);
vec3_f32 reflect_3f32(vec3_f32 i, vec3_f32 n);

typedef union vec4_f32 vec4_f32;
union vec4_f32 {
    struct {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };
    union {
        vec2_f32 xy;
        vec2_f32 zw;
    };
    union {
        vec3_f32 xyz;
        f32 _w;
    };
    union {
        f32 _x;
        vec3_f32 yzw;
    };
    struct {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    };

    f32 v[4];
};

vec4_f32 make_quat(f64 t, vec3_f32 a);
vec4_f32 inv_quat(vec4_f32 q);
vec3_f32 rot_quat(vec3_f32 a, vec4_f32 q);

vec4_f32 make_4f32(f32 x, f32 y, f32 z, f32 w);
vec4_f32 add_4f32(vec4_f32 a, vec4_f32 b);
vec4_f32 sub_4f32(vec4_f32 a, vec4_f32 b);
vec4_f32 mul_4f32(vec4_f32 a, f32 b);
vec4_f32 elmul_2f32(vec4_f32 a, vec4_f32 b);
f32 dot_4f32(vec4_f32 a, vec4_f32 b);
f32 length_4f32(vec4_f32 a);
vec4_f32 normalize_4f32(vec4_f32 a);

typedef union mat4x4_f32 mat4x4_f32;
union mat4x4_f32 {
    struct {
        vec4_f32 r1;
        vec4_f32 r2;
        vec4_f32 r3;
        vec4_f32 r4;
    };
    struct {
        f32 r[4];    
    } c[4];
    f32 v[4][4];
};

// @note assumes right handed coordinate system
mat4x4_f32 make_diagonal_4x4f32(f32 d);
mat4x4_f32 make_scale_4x4f32(vec3_f32 s);
mat4x4_f32 make_translate_4x4f32(vec3_f32 t);
mat4x4_f32 make_rotate_4x4f32(vec4_f32 nq);
mat4x4_f32 make_perspective_4x4f32(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);
mat4x4_f32 make_look_at_4x4f32(vec3_f32 eye, vec3_f32 center, vec3_f32 up);
mat4x4_f32 add_4x4f32(mat4x4_f32 a, mat4x4_f32 b);
mat4x4_f32 sub_4x4f32(mat4x4_f32 a, mat4x4_f32 b);
mat4x4_f32 mul_4x4f32(mat4x4_f32 a, mat4x4_f32 b);
mat4x4_f32 transpose_4x4f32(mat4x4_f32 a);

void decompose_4x4f32(mat4x4_f32 view, vec3_f32* f, vec3_f32* s, vec3_f32* u, vec3_f32* p);

typedef union rect_f32 rect_f32;
union rect_f32 {
    struct {
        vec2_f32 tl;
        vec2_f32 br;
    };
    vec2_f32 v[2];
};

rect_f32 make_rect_f32(vec2_f32 tl, vec2_f32 br);

#define PI               (3.1415926535897f)
#define DegreesToRad(v)  (v * (PI / 180.f))

#define sqrt_f32(v)   sqrtf(v)
#define cbrt_f32(v)   cbrtf(v)
#define mod_f32(a, b) fmodf((a), (b))
#define pow_f32(b, e) powf((b), (e))
#define ceil_f32(v)   ceilf(v)
#define floor_f32(v)  floorf(v)
#define round_f32(v)  roundf(v)
#define abs_f32(v)    fabsf(v)
#define sin_f32(v)    sinf(v)
#define cos_f32(v)    cosf(v)
#define tan_f32(v)    tanf(v)
#define rand_f32()    ((f32)rand()/(f32)RAND_MAX)

#define sqrt_f64(v)   sqrt(v)
#define cbrt_f64(v)   cbrt(v)
#define mod_f64(a, b) fmod((a), (b))
#define pow_f64(b, e) pow((b), (e))
#define ceil_f64(v)   ceil(v)
#define floor_f64(v)  floor(v)
#define round_f64(v)  round(v)
#define abs_f64(v)    fabs(v)
#define sin_f64(v)    sin(v)
#define cos_f64(v)    cos(v)
#define tan_f64(v)    tan(v)
#define rand_f64()    ((f64)rand()/(f64)RAND_MAX)

u64 hash_u64(u8* buffer, u64 size);