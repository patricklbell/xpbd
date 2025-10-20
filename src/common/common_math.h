#pragma once

typedef union vec2_f32 vec2_f32;
union vec2_f32 {
    struct {
        f32 x;
        f32 y;
    };

    f32 v[2];
};

internal vec2_f32 make_2f32(f32 x, f32 y);
internal vec2_f32 add_2f32(vec2_f32 a, vec2_f32 b);
internal vec2_f32 sub_2f32(vec2_f32 a, vec2_f32 b);
internal vec2_f32 mul_2f32(vec2_f32 a, f32 b);
internal vec2_f32 elmul_2f32(vec2_f32 a, vec2_f32 b);
internal f32 dot_2f32(vec2_f32 a, vec2_f32 b);
internal f32 length_2f32(vec2_f32 a);
internal vec2_f32 normalize_2f32(vec2_f32 a);

#define length2_2f32(a) dot_2f32(a,a)

typedef union vec3_f32 vec3_f32;
union vec3_f32 {
    struct {
        f32 x;
        f32 y;
        f32 z;
    };
    struct {
        vec2_f32 xy;
        f32 _z;
    };
    struct {
        f32 _x;
        vec2_f32 yz;
    };
    struct {
        f32 r;
        f32 g;
        f32 b;
    };
    struct {
        f32 h;
        f32 s;
        f32 l;
    };

    f32 v[3];
};

internal vec3_f32 make_3f32(f32 x, f32 y, f32 z);
internal vec3_f32 make_scale_3f32(f32 s);
internal vec3_f32 make_up_3f32();
internal vec3_f32 add_3f32(vec3_f32 a, vec3_f32 b);
internal vec3_f32 sub_3f32(vec3_f32 a, vec3_f32 b);
internal vec3_f32 mul_3f32(vec3_f32 a, f32 b);
internal vec3_f32 elmul_3f32(vec3_f32 a, vec3_f32 b);
internal vec3_f32 eldiv_3f32(vec3_f32 a, vec3_f32 b);
internal f32      dot_3f32(vec3_f32 a, vec3_f32 b);
internal f32      length_3f32(vec3_f32 a);
internal vec3_f32 normalize_3f32(vec3_f32 a);
internal vec3_f32 cross_3f32(vec3_f32 a, vec3_f32 b);
internal vec3_f32 reflect_3f32(vec3_f32 i, vec3_f32 n);
internal vec3_f32 lerp_3f32(vec3_f32 x, vec3_f32 y, f32 a);
internal vec3_f32 max_3f32(vec3_f32 a, vec3_f32 b);
internal vec3_f32 min_3f32(vec3_f32 a, vec3_f32 b);
internal vec3_f32 addscl_3f32(vec3_f32 a, f32 b);

#define length2_3f32(a) dot_3f32(a,a)

typedef union vec4_f32 vec4_f32;
union vec4_f32 {
    struct {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };
    struct {
        vec2_f32 xy;
        vec2_f32 zw;
    };
    struct {
        vec3_f32 xyz;
        f32 _w;
    };
    struct {
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

internal vec4_f32 make_angle_axis_quat(f64 t, vec3_f32 a);
internal vec4_f32 make_axis_quat(vec3_f32 a);
internal vec4_f32 make_identity_quat();
internal vec4_f32 inv_quat(vec4_f32 q);
internal vec3_f32 rot_quat(vec3_f32 a, vec4_f32 q);
internal vec4_f32 mul_quat(vec4_f32 q1, vec4_f32 q2);

internal vec4_f32 make_4f32(f32 x, f32 y, f32 z, f32 w);
internal vec4_f32 add_4f32(vec4_f32 a, vec4_f32 b);
internal vec4_f32 sub_4f32(vec4_f32 a, vec4_f32 b);
internal vec4_f32 mul_4f32(vec4_f32 a, f32 b);
internal vec4_f32 elmul_4f32(vec4_f32 a, vec4_f32 b);
internal vec4_f32 eldiv_4f32(vec4_f32 a, vec4_f32 b);
internal f32      dot_4f32(vec4_f32 a, vec4_f32 b);
internal f32      length_4f32(vec4_f32 a);
internal vec4_f32 normalize_4f32(vec4_f32 a);

#define length2_4f32(a) dot_4f32(a,a)

// @note all matrices are stored column major and multiplication with a column
// vector to the right
typedef union mat3x3_f32 mat3x3_f32;
union mat3x3_f32 {
    struct {
        vec3_f32 c1;
        vec3_f32 c2;
        vec3_f32 c3;
    };
    f32 v[3][3];
};

internal mat3x3_f32 make_diagonal_3x3f32(f32 d);
internal mat3x3_f32 make_scale_3x3f32(vec3_f32 s);
internal mat3x3_f32 make_rotate_3x3f32(vec4_f32 nq);
internal mat3x3_f32 add_3x3f32(mat3x3_f32 a, mat3x3_f32 b);
internal mat3x3_f32 sub_3x3f32(mat3x3_f32 a, mat3x3_f32 b);
internal vec3_f32   mul_3x3f32(mat3x3_f32 a, vec3_f32 b);
internal vec3_f32   mullhs_3x3f32(vec3_f32 a, mat3x3_f32 b);
internal mat3x3_f32 matmul_3x3f32(mat3x3_f32 a, mat3x3_f32 b);
internal mat3x3_f32 scale_3x3f32(mat3x3_f32 a, f32 b);
internal mat3x3_f32 inv_3x3f32(mat3x3_f32 m);
internal mat3x3_f32 transpose_3x3f32(mat3x3_f32 m);

typedef union mat4x4_f32 mat4x4_f32;
union mat4x4_f32 {
    struct {
        vec4_f32 c1;
        vec4_f32 c2;
        vec4_f32 c3;
        vec4_f32 c4;
    };
    f32 v[4][4];
};

// @note assumes right handed coordinate system
internal mat4x4_f32 make_diagonal_4x4f32(f32 d);
internal mat4x4_f32 make_scale_4x4f32(vec3_f32 s);
internal mat4x4_f32 make_translate_4x4f32(vec3_f32 t);
internal mat4x4_f32 make_rotate_4x4f32(vec4_f32 nq);
internal mat4x4_f32 make_perspective_4x4f32(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);
internal mat4x4_f32 make_look_at_4x4f32(vec3_f32 eye, vec3_f32 center, vec3_f32 up);
internal mat4x4_f32 add_4x4f32(mat4x4_f32 a, mat4x4_f32 b);
internal mat4x4_f32 sub_4x4f32(mat4x4_f32 a, mat4x4_f32 b);
internal vec4_f32   mul_4x4f32(mat4x4_f32 a, vec4_f32 b);
internal vec4_f32   mullhs_4x4f32(vec4_f32 a, mat4x4_f32 b);
internal mat4x4_f32 matmul_4x4f32(mat4x4_f32 a, mat4x4_f32 b);
internal mat4x4_f32 scale_4x4f32(mat4x4_f32 a, f32 b);
internal mat4x4_f32 inv_4x4f32(mat4x4_f32 m);
internal mat4x4_f32 transpose_4x4f32(mat4x4_f32 m);

typedef union rect_f32 rect_f32;
union rect_f32 {
    struct {
        vec2_f32 tl;
        vec2_f32 br;
    };
    vec2_f32 v[2];
};

internal rect_f32 make_rect_f32(vec2_f32 tl, vec2_f32 br);

#define PI_F32           3.1415926535897932384626433832795028841971693993751058209749445923078164062f
#define PI_F64           3.1415926535897932384626433832795028841971693993751058209749445923078164062
#define EPSILON_F32      (1.1920929E-7f)
#define DegreesToRad(v)  (v * (PI_F32 / 180.f))

#define sqrt_f32(v)   sqrtf(v)
#define cbrt_f32(v)   cbrtf(v)
#define mod_f32(a, b) fmodf((a), (b))
#define pow_f32(b, e) powf((b), (e))
#define ceil_f32(v)   ceilf(v)
#define floor_f32(v)  (s32)floorf(v)
#define round_f32(v)  (s32)roundf(v)
#define abs_f32(v)    fabsf(v)
#define sin_f32(v)    sinf(v)
#define cos_f32(v)    cosf(v)
#define tan_f32(v)    tanf(v)
#define asin_f32(v)   asinf(v)
#define acos_f32(v)   acosf(v)
#define atan_f32(v)   atanf(v)
#define log_f32(v)    logf(v)
#define exp_f32(v)    expf(v)
#define sgn_f32(v)    (((v) < 0) ? -1.f : 1.f)
#define sgnnum_f32(v) ((v == 0) ? 0.f : (((v) < 0) ? -1.f : 1.f))
#define rand_f32()    ((f32)rand()/(f32)RAND_MAX)

internal f32 smoothstep_f32(f32 edge_0, f32 edge_1, f32 x);

#define sqrt_f64(v)   sqrt(v)
#define cbrt_f64(v)   cbrt(v)
#define mod_f64(a, b) fmod((a), (b))
#define pow_f64(b, e) pow((b), (e))
#define ceil_f64(v)   ceil(v)
#define floor_f64(v)  (s64)floor(v)
#define round_f64(v)  (s64)round(v)
#define abs_f64(v)    fabs(v)
#define sin_f64(v)    sin(v)
#define cos_f64(v)    cos(v)
#define tan_f64(v)    tan(v)
#define asin_f64(v)   asin(v)
#define acos_f64(v)   acos(v)
#define atan_f64(v)   atan(v)
#define log_f64(v)    log(v)
#define exp_f64(v)    exp(v)
#define sgn_f64(v)    (((v) < 0) ? -1. : 1.)
#define sgnnum_f64(v) ((v == 0) ? 0. : (((v) < 0) ? -1. : 1.))
#define rand_f64()    ((f64)rand()/(f64)RAND_MAX)

internal u64 hash_u64(u8* buffer, u64 size);