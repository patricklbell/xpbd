vec2_f32 add_2f32(vec2_f32 a, vec2_f32 b)       { return (vec2_f32) {.x = a.x + b.x,.y = a.y + b.y}; }
vec2_f32 sub_2f32(vec2_f32 a, vec2_f32 b)       { return (vec2_f32) {.x = a.x - b.x,.y = a.y - b.y}; }
vec2_f32 mul_2f32(vec2_f32 a, f32 b)            { return (vec2_f32) {.x = a.x*b,.y = a.y*b}; }
f32      dot_2f32(vec2_f32 a, vec2_f32 b)       { return a.x*b.x + a.y*b.y; }
f32      length_2f32(vec2_f32 a)                { return sqrt_f32(a.x*a.x + a.y*a.y); }
vec2_f32 normalize_2f32(vec2_f32 a)             { f32 l = length_2f32(a); return (vec2_f32) {.x = a.x/l,.y = a.y/l}; }

vec3_f32 add_3f32(vec3_f32 a, vec3_f32 b)       { return (vec3_f32) {.x = a.x + b.x,.y = a.y + b.y,.z = a.z + b.z}; }
vec3_f32 sub_3f32(vec3_f32 a, vec3_f32 b)       { return (vec3_f32) {.x = a.x - b.x,.y = a.y - b.y,.z = a.z - b.z}; }
vec3_f32 mul_3f32(vec3_f32 a, f32 b)            { return (vec3_f32) {.x = a.x*b,.y = a.y*b,.z = a.z*b}; }
f32      dot_3f32(vec3_f32 a, vec3_f32 b)       { return a.x*b.x + a.y*b.y + a.z*b.z; }
f32      length_3f32(vec3_f32 a)                { return sqrt_f32(a.x*a.x + a.y*a.y + a.z*a.z); }
vec3_f32 normalize_3f32(vec3_f32 a)             { f32 l = length_3f32(a); return (vec3_f32) {.x = a.x/l,.y = a.y/l,.z = a.z/l}; }
vec3_f32 cross_3f32(vec3_f32 a, vec3_f32 b)     { return (vec3_f32) {.x = a.y*b.z - a.z*b.y,.y = a.x*b.z - a.z*b.x,.z = a.x*b.y - a.y*b.z}; }
vec3_f32 reflect_3f32(vec3_f32 i, vec3_f32 n)   { return sub_3f32(i, mul_3f32(n, 2.0*dot_3f32(i, n))); }

vec4_f32 make_quat_4f32(f32 t, vec3_f32 a)      { f32 st = sin(t/2), ct = cos(t/2); return (vec4_f32) {.x = st*a.x,.y = st*a.y,.z = st*a.z,.w = ct};}
vec4_f32 add_4f32(vec4_f32 a, vec4_f32 b)       { return (vec4_f32) {.x = a.x + b.x,.y = a.y + b.y,.z = a.z + b.z,.w = a.w + b.w}; }
vec4_f32 sub_4f32(vec4_f32 a, vec4_f32 b)       { return (vec4_f32) {.x = a.x - b.x,.y = a.y - b.y,.z = a.z - b.z,.w = a.w - b.w}; }
vec4_f32 mul_4f32(vec4_f32 a, f32 b)            { return (vec4_f32) {.x = a.x*b,.y = a.y*b,.z = a.z*b,.w = a.w*b}; }
f32      dot_4f32(vec4_f32 a, vec4_f32 b)       { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
f32      length_4f32(vec4_f32 a)                { return sqrt_f32(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w); }
vec4_f32 normalize_4f32(vec4_f32 a)             { f32 l = length_4f32(a); return (vec4_f32) {.x = a.x/l,.y = a.y/l,.z = a.z/l,.w = a.w/l}; }

mat4x4_f32 make_diagonal_4x4f32(f32 d) {
    return (mat4x4_f32) {.v = {
        {d  , 0.f, 0.f, 0.f},
        {0.f, d  , 0.f, 0.f},
        {0.f, 0.f, d  , 0.f},
        {0.f, 0.f, 0.f, d  },
    }};
}
mat4x4_f32 make_scale_4x4f32(vec3_f32 s) {
    return (mat4x4_f32) {.v = {
        {s.x, 0.f, 0.f, 0.f},
        {0.f, s.y, 0.f, 0.f},
        {0.f, 0.f, s.z, 0.f},
        {0.f, 0.f, 0.f, 1.f},
    }};
}
mat4x4_f32 make_translate_4x4f32(vec3_f32 t) {
    return (mat4x4_f32) {.v = {
        { 1.f, 0.f, 0.f, 0.f},
        { 0.f, 1.f, 0.f, 0.f},
        { 0.f, 0.f, 1.f, 0.f},
        {-t.x,-t.y,-t.z, 1.f},
    }};
}
mat4x4_f32 make_rotate_4x4f32(vec4_f32 nq) {
    return (mat4x4_f32) {.v = {
        {1.0f - 2.0f*nq.y*nq.y - 2.0f*nq.z*nq.z, 2.0f*nq.x*nq.y - 2.0f*nq.z*nq.w,        2.0f*nq.x*nq.z + 2.0f*nq.y*nq.w,           0.0f},
        {2.0f*nq.x*nq.y + 2.0f*nq.z*nq.w,        1.0f - 2.0f*nq.x*nq.x - 2.0f*nq.z*nq.z, 2.0f*nq.y*nq.z - 2.0f*nq.x*nq.w,           0.0f},
        {2.0f*nq.x*nq.z - 2.0f*nq.y*nq.w,        2.0f*nq.y*nq.z + 2.0f*nq.x*nq.w,        1.0f - 2.0f*nq.x*nq.x - 2.0f*nq.y*nq.y,    0.0f},
        {0.0f,                                   0.0f,                                   0.0f,                                      1.0f},
    }}; // @todo transpose?
}
mat4x4_f32 make_perspective_4x4f32(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z) {
    f32 tan_half_fov = tan_f32(fov / 2.f);

    mat4x4_f32 result = make_diagonal_4x4f32(0.f);
    result.c[0].r[0] = 1.f / (aspect_ratio*tan_half_fov);
    result.c[1].r[1] = 1.f / (tan_half_fov);
    result.c[2].r[2] = - (far_z + near_z) / (far_z - near_z);
    result.c[2].r[3] = - 1.f;
    result.c[3].r[2] = - (2.f * far_z * near_z) / (far_z - near_z);
    return result;
}
mat4x4_f32 make_look_at_4x4f32(vec3_f32 eye, vec3_f32 center, vec3_f32 up) {
    vec3_f32 f = normalize_3f32(sub_3f32(center, eye));
    vec3_f32 s = normalize_3f32(cross_3f32(f, up));
    vec3_f32 u = cross_3f32(s, f);

    mat4x4_f32 result = make_diagonal_4x4f32(1.0f);
    result.c[0].r[0] = s.x;
    result.c[1].r[0] = s.y;
    result.c[2].r[0] = s.z;
    result.c[0].r[1] = u.x;
    result.c[1].r[1] = u.y;
    result.c[2].r[1] = u.z;
    result.c[0].r[2] =-f.x;
    result.c[1].r[2] =-f.y;
    result.c[2].r[2] =-f.z;
    result.c[3].r[0] =-dot_3f32(s, eye);
    result.c[3].r[1] =-dot_3f32(u, eye);
    result.c[3].r[2] = dot_3f32(f, eye);
    return result;
}
mat4x4_f32 add_4x4f32(mat4x4_f32 a, mat4x4_f32 b) {
    return (mat4x4_f32) {.v = {
        {a.v[0][0]+b.v[0][0], a.v[0][1]+b.v[0][1], a.v[0][2]+b.v[0][2], a.v[0][3]+b.v[0][3]},
        {a.v[1][0]+b.v[1][0], a.v[1][1]+b.v[1][1], a.v[1][2]+b.v[1][2], a.v[1][3]+b.v[1][3]},
        {a.v[2][0]+b.v[2][0], a.v[2][1]+b.v[2][1], a.v[2][2]+b.v[2][2], a.v[2][3]+b.v[2][3]},
        {a.v[3][0]+b.v[3][0], a.v[3][1]+b.v[3][1], a.v[3][2]+b.v[3][2], a.v[3][3]+b.v[3][3]},
    }};
}
mat4x4_f32 sub_4x4f32(mat4x4_f32 a, mat4x4_f32 b) {
    return (mat4x4_f32) {.v = {
        {a.v[0][0]-b.v[0][0], a.v[0][1]-b.v[0][1], a.v[0][2]-b.v[0][2], a.v[0][3]-b.v[0][3]},
        {a.v[1][0]-b.v[1][0], a.v[1][1]-b.v[1][1], a.v[1][2]-b.v[1][2], a.v[1][3]-b.v[1][3]},
        {a.v[2][0]-b.v[2][0], a.v[2][1]-b.v[2][1], a.v[2][2]-b.v[2][2], a.v[2][3]-b.v[2][3]},
        {a.v[3][0]-b.v[3][0], a.v[3][1]-b.v[3][1], a.v[3][2]-b.v[3][2], a.v[3][3]-b.v[3][3]},
    }};
}
mat4x4_f32 mul_4x4f32(mat4x4_f32 a, mat4x4_f32 b){
    mat4x4_f32 result = {0};
    for(int j = 0; j < 4; j++) {
        for(int i = 0; i < 4; i++) {
            result.v[i][j] = (a.v[0][j]*b.v[i][0] +
                              a.v[1][j]*b.v[i][1] +
                              a.v[2][j]*b.v[i][2] +
                              a.v[3][j]*b.v[i][3]);
        }
    }
    return result;
}
mat4x4_f32 transpose_4x4f32(mat4x4_f32 a) {
    return (mat4x4_f32) {.v = {
        {a.v[0][0], a.v[1][0], a.v[2][0], a.v[3][0]},
        {a.v[0][1], a.v[1][1], a.v[2][1], a.v[3][1]},
        {a.v[0][2], a.v[1][2], a.v[2][2], a.v[3][2]},
        {a.v[0][3], a.v[1][3], a.v[2][3], a.v[3][3]},
    }};
}

rect_f32 make_rect_f32(vec2_f32 tl, vec2_f32 br) {
    return (rect_f32) { .tl = tl, .br = br };
}

u64 hash_u64(u8* buffer, u64 size) {
  u64 result = 5381;
  for(u64 i = 0; i < size; i++) {
    result = ((result << 5) + result) + buffer[i];
  }
  return result;
}