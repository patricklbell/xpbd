vec2_f32 make_2f32(f32 x, f32 y)                    { return (vec2_f32) {.x=x,.y=y}; }
vec2_f32 add_2f32(vec2_f32 a, vec2_f32 b)           { return (vec2_f32) {.x = a.x + b.x,.y = a.y + b.y}; }
vec2_f32 sub_2f32(vec2_f32 a, vec2_f32 b)           { return (vec2_f32) {.x = a.x - b.x,.y = a.y - b.y}; }
vec2_f32 mul_2f32(vec2_f32 a, f32 b)                { return (vec2_f32) {.x = a.x*b,.y = a.y*b}; }
vec2_f32 elmul_2f32(vec2_f32 a, vec2_f32 b)         { return (vec2_f32) {.x = a.x*b.x,.y = a.y*b.y}; }
f32      dot_2f32(vec2_f32 a, vec2_f32 b)           { return a.x*b.x + a.y*b.y; }
f32      length_2f32(vec2_f32 a)                    { return sqrt_f32(a.x*a.x + a.y*a.y); }
vec2_f32 normalize_2f32(vec2_f32 a)                 { f32 l = length_2f32(a); return (vec2_f32) {.x = a.x/l,.y = a.y/l}; }

vec3_f32 make_3f32(f32 x, f32 y, f32 z)             { return (vec3_f32) {.x=x,.y=y,.z=z}; }
vec3_f32 make_up_3f32()                             { return (vec3_f32) {.x=0,.y=1,.z=0}; }
vec3_f32 add_3f32(vec3_f32 a, vec3_f32 b)           { return (vec3_f32) {.x = a.x + b.x,.y = a.y + b.y,.z = a.z + b.z}; }
vec3_f32 sub_3f32(vec3_f32 a, vec3_f32 b)           { return (vec3_f32) {.x = a.x - b.x,.y = a.y - b.y,.z = a.z - b.z}; }
vec3_f32 mul_3f32(vec3_f32 a, f32 b)                { return (vec3_f32) {.x = a.x*b,.y = a.y*b,.z = a.z*b}; }
vec3_f32 elmul_3f32(vec3_f32 a, vec3_f32 b)         { return (vec3_f32) {.x = a.x*b.x,.y = a.y*b.y,.z = a.z*b.z}; }
f32      dot_3f32(vec3_f32 a, vec3_f32 b)           { return a.x*b.x + a.y*b.y + a.z*b.z; }
f32      length_3f32(vec3_f32 a)                    { return sqrt_f32(a.x*a.x + a.y*a.y + a.z*a.z); }
vec3_f32 normalize_3f32(vec3_f32 a)                 { f32 l = length_3f32(a); return (vec3_f32) {.x = a.x/l,.y = a.y/l,.z = a.z/l}; }
vec3_f32 cross_3f32(vec3_f32 a, vec3_f32 b)         { return (vec3_f32) {.x = a.y*b.z - a.z*b.y,.y = -(a.x*b.z - a.z*b.x),.z = a.x*b.y - a.y*b.x}; }
vec3_f32 reflect_3f32(vec3_f32 i, vec3_f32 n)       { return sub_3f32(i, mul_3f32(n, 2.0*dot_3f32(i, n))); }
vec3_f32 lerp_3f32(vec3_f32 x, vec3_f32 y, f32 a)   { return (vec3_f32) {.x = (1.f-a)*x.x + a*y.x,.y = (1.f-a)*x.y + a*y.y,.z = (1.f-a)*x.z + a*y.z}; }

vec4_f32 make_axis_angle_quat(f64 t, vec3_f32 a)    { f32 st = sin(t/2.), ct = cos(t/2.); return (vec4_f32) {.x = st*a.x,.y = st*a.y,.z = st*a.z,.w = ct};}
vec4_f32 make_axis_quat(vec3_f32 a)                 { return (vec4_f32) {.x = a.x,.y = a.y,.z = a.z,.w = 0.f};}
vec4_f32 make_identity_quat()                       { return (vec4_f32) {.x = 0.f,.y = 0.f,.z = 0.f,.w = 1.f};}
vec4_f32 inv_quat(vec4_f32 q)                       { return (vec4_f32) {.x =-q.x,.y =-q.y,.z =-q.z,.w = q.w};}
vec3_f32 rot_quat(vec3_f32 p, vec4_f32 q)           { return add_3f32(add_3f32(mul_3f32(p, q.w*q.w - dot_3f32(q.xyz, q.xyz)), mul_3f32(q.xyz, 2.f*dot_3f32(q.xyz, p))), mul_3f32(cross_3f32(q.xyz, p), 2.f*q.w));}
vec4_f32 mul_quat(vec4_f32 q1, vec4_f32 q2)         {
    return (vec4_f32) {
        .x = q1.x*q2.w + q1.w*q2.x + q1.y*q2.z - q1.z*q2.y,
        .y = q1.y*q2.w + q1.w*q2.y + q1.z*q2.x - q1.x*q2.z,
        .z = q1.z*q2.w + q1.w*q2.z + q1.x*q2.y - q1.y*q2.x,
        .w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z
    };
}

vec4_f32 make_4f32(f32 x, f32 y, f32 z, f32 w)      { return (vec4_f32) {.x=x,.y=y,.z=z,.w=w}; }
vec4_f32 add_4f32(vec4_f32 a, vec4_f32 b)           { return (vec4_f32) {.x = a.x + b.x,.y = a.y + b.y,.z = a.z + b.z,.w = a.w + b.w}; }
vec4_f32 sub_4f32(vec4_f32 a, vec4_f32 b)           { return (vec4_f32) {.x = a.x - b.x,.y = a.y - b.y,.z = a.z - b.z,.w = a.w - b.w}; }
vec4_f32 mul_4f32(vec4_f32 a, f32 b)                { return (vec4_f32) {.x = a.x*b,.y = a.y*b,.z = a.z*b,.w = a.w*b}; }
vec4_f32 elmul_4f32(vec4_f32 a, vec4_f32 b)         { return (vec4_f32) {.x = a.x*b.x,.y = a.y*b.y,.z = a.z*b.z,.w = a.w*b.w}; }
f32      dot_4f32(vec4_f32 a, vec4_f32 b)           { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
f32      length_4f32(vec4_f32 a)                    { return sqrt_f32(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w); }
vec4_f32 normalize_4f32(vec4_f32 a)                 { f32 l = length_4f32(a); return (vec4_f32) {.x = a.x/l,.y = a.y/l,.z = a.z/l,.w = a.w/l}; }

mat3x3_f32 make_diagonal_3x3f32(f32 d) {
    return (mat3x3_f32) {.v = {
        {d  , 0.f, 0.f},
        {0.f, d  , 0.f},
        {0.f, 0.f, d  },
    }};
}
mat3x3_f32 make_scale_3x3f32(vec3_f32 s) {
    return (mat3x3_f32) {.v = {
        {s.x, 0.f, 0.f},
        {0.f, s.y, 0.f},
        {0.f, 0.f, s.z},
    }};
}
mat3x3_f32 make_rotate_3x3f32(vec4_f32 nq) {
    return (mat3x3_f32) {.v = {
        {1.0f - 2.0f*nq.y*nq.y - 2.0f*nq.z*nq.z, 2.0f*nq.x*nq.y - 2.0f*nq.z*nq.w,        2.0f*nq.x*nq.z + 2.0f*nq.y*nq.w,      },
        {2.0f*nq.x*nq.y + 2.0f*nq.z*nq.w,        1.0f - 2.0f*nq.x*nq.x - 2.0f*nq.z*nq.z, 2.0f*nq.y*nq.z - 2.0f*nq.x*nq.w,      },
        {2.0f*nq.x*nq.z - 2.0f*nq.y*nq.w,        2.0f*nq.y*nq.z + 2.0f*nq.x*nq.w,        1.0f - 2.0f*nq.x*nq.x - 2.0f*nq.y*nq.y},
    }}; // @todo transpose?
}
mat3x3_f32 add_3x3f32(mat3x3_f32 a, mat3x3_f32 b) {
    return (mat3x3_f32) {.v = {
        {a.v[0][0]+b.v[0][0], a.v[0][1]+b.v[0][1], a.v[0][2]+b.v[0][2]},
        {a.v[1][0]+b.v[1][0], a.v[1][1]+b.v[1][1], a.v[1][2]+b.v[1][2]},
        {a.v[2][0]+b.v[2][0], a.v[2][1]+b.v[2][1], a.v[2][2]+b.v[2][2]},
    }};
}
mat3x3_f32 sub_3x3f32(mat3x3_f32 a, mat3x3_f32 b) {
    return (mat3x3_f32) {.v = {
        {a.v[0][0]-b.v[0][0], a.v[0][1]-b.v[0][1], a.v[0][2]-b.v[0][2]},
        {a.v[1][0]-b.v[1][0], a.v[1][1]-b.v[1][1], a.v[1][2]-b.v[1][2]},
        {a.v[2][0]-b.v[2][0], a.v[2][1]-b.v[2][1], a.v[2][2]-b.v[2][2]},
    }};
}
vec3_f32 mul_3x3f32(mat3x3_f32 a, vec3_f32 b) {
    vec3_f32 result = zero_struct;
    for(int i = 0; i < 3; i++) {
        result.v[i] = (a.v[0][i]*b.v[i] +
                       a.v[1][i]*b.v[i] +
                       a.v[2][i]*b.v[i]);
    }
    return result;
}
vec3_f32 mullhs_3x3f32(vec3_f32 a, mat3x3_f32 b) {
    vec3_f32 result = zero_struct;
    for(int i = 0; i < 3; i++) {
        result.v[i] = (b.v[i][0]*a.v[i] +
                       b.v[i][1]*a.v[i] +
                       b.v[i][2]*a.v[i]);
    }
    return result;
}
mat3x3_f32 matmul_3x3f32(mat3x3_f32 a, mat3x3_f32 b){
    mat3x3_f32 result = {0};
    for(int j = 0; j < 3; j++) {
        for(int i = 0; i < 3; i++) {
            result.v[i][j] = (a.v[0][j]*b.v[i][0] +
                              a.v[1][j]*b.v[i][1] +
                              a.v[2][j]*b.v[i][2]);
        }
    }
    return result;
}
mat3x3_f32 scale_3x3f32(mat3x3_f32 a, f32 b) {
    return (mat3x3_f32) {.v = {
        {a.v[0][0]*b, a.v[0][1]*b, a.v[0][2]*b},
        {a.v[1][0]*b, a.v[1][1]*b, a.v[1][2]*b},
        {a.v[2][0]*b, a.v[2][1]*b, a.v[2][2]*b},
    }};
}
mat3x3_f32 inv_3x3f32(mat3x3_f32 m) {
    f32 coef00 = m.v[1][1] * m.v[2][2] - m.v[2][1] * m.v[1][2];
    f32 coef01 = m.v[1][0] * m.v[2][2] - m.v[1][2] * m.v[2][0];
    f32 coef02 = m.v[1][0] * m.v[2][1] - m.v[1][1] * m.v[2][0];
    f32 coef10 = m.v[0][2] * m.v[2][1] - m.v[0][1] * m.v[2][2];
    f32 coef11 = m.v[0][0] * m.v[2][2] - m.v[0][2] * m.v[2][0];
    f32 coef12 = m.v[2][0] * m.v[0][1] - m.v[0][0] * m.v[2][1];
    f32 coef20 = m.v[0][1] * m.v[1][2] - m.v[0][2] * m.v[1][1];
    f32 coef21 = m.v[1][0] * m.v[0][2] - m.v[0][0] * m.v[1][2];
    f32 coef22 = m.v[0][0] * m.v[1][1] - m.v[1][0] * m.v[0][1];

    vec3_f32 fac0 = { coef00, coef10, coef20 };
    vec3_f32 fac1 = { coef01, coef11, coef21 };
    vec3_f32 fac2 = { coef02, coef12, coef22 };

    mat3x3_f32 inv;
    inv.c1 = fac0;
    inv.c2 = fac1;
    inv.c3 = fac2;

    vec3_f32 row0 = { m.v[0][0], m.v[0][1], m.v[0][2] };
    f32 det = row0.x*coef00 - row0.y*coef01 + row0.z*coef02;
    f32 one_over_det = 1.f / det;

    return scale_3x3f32(inv, one_over_det);
}
mat3x3_f32 transpose_3x3f32(mat3x3_f32 m) {
    return (mat3x3_f32) {.v = {
        {m.v[0][0], m.v[1][0], m.v[2][0]},
        {m.v[0][1], m.v[1][1], m.v[2][1]},
        {m.v[0][2], m.v[1][2], m.v[2][2]},
    }};
}

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
        { t.x, t.y, t.z, 1.f},
    }};
}
mat4x4_f32 make_rotate_4x4f32(vec4_f32 nq) {
    return (mat4x4_f32) {.v = {
        {1.0f - 2.0f*nq.y*nq.y - 2.0f*nq.z*nq.z, 2.0f*nq.x*nq.y + 2.0f*nq.z*nq.w,        2.0f*nq.x*nq.z - 2.0f*nq.y*nq.w,         0.0f},
        {2.0f*nq.x*nq.y - 2.0f*nq.z*nq.w,        1.0f - 2.0f*nq.x*nq.x - 2.0f*nq.z*nq.z, 2.0f*nq.y*nq.z + 2.0f*nq.x*nq.w,         0.0f},
        {2.0f*nq.x*nq.z + 2.0f*nq.y*nq.w,        2.0f*nq.y*nq.z - 2.0f*nq.x*nq.w,        1.0f - 2.0f*nq.x*nq.x - 2.0f*nq.y*nq.y,  0.0f},
        {0.0f,                                   0.0f,                                   0.0f,                                    1.0f},
    }};
}

mat4x4_f32 make_perspective_4x4f32(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z) {
    f32 tan_half_fov = tan_f32(fov / 2.f);

    mat4x4_f32 result = zero_struct;
    result.v[0][0] = 1.f / (aspect_ratio*tan_half_fov);
    result.v[1][1] = 1.f / (tan_half_fov);
    result.v[2][2] = - (far_z + near_z) / (far_z - near_z);
    result.v[2][3] = - 1.f;
    result.v[3][2] = - (2.f * far_z * near_z) / (far_z - near_z);
    return result;
}
mat4x4_f32 make_look_at_4x4f32(vec3_f32 eye, vec3_f32 center, vec3_f32 up) {
    vec3_f32 f = normalize_3f32(sub_3f32(center, eye));
    vec3_f32 s = normalize_3f32(cross_3f32(f, up));
    vec3_f32 u = cross_3f32(s, f);

    return (mat4x4_f32) {.v = {
        { s.x, u.x,-f.x, 0.f},
        { s.y, u.y,-f.y, 0.f},
        { s.z, u.z,-f.z, 0.f},
        {-dot_3f32(s, eye),-dot_3f32(u, eye),+dot_3f32(f, eye), 1.f},
    }};
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
vec4_f32 mul_4x4f32(mat4x4_f32 a, vec4_f32 b) {
    vec4_f32 result = zero_struct;
    for(int i = 0; i < 4; i++) {
        result.v[i] = (a.v[0][i]*b.v[i] +
                       a.v[1][i]*b.v[i] +
                       a.v[2][i]*b.v[i] +
                       a.v[3][i]*b.v[i]);
    }
    return result;
}
vec4_f32 mullhs_4x4f32(vec4_f32 a, mat4x4_f32 b) {
    vec4_f32 result = zero_struct;
    for(int i = 0; i < 3; i++) {
        result.v[i] = (b.v[i][0]*a.v[i] +
                       b.v[i][1]*a.v[i] +
                       b.v[i][2]*a.v[i] + 
                       b.v[i][3]*a.v[i]);
    }
    return result;
}
mat4x4_f32 matmul_4x4f32(mat4x4_f32 a, mat4x4_f32 b) {
    mat4x4_f32 result = zero_struct;
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
mat4x4_f32 scale_4x4f32(mat4x4_f32 a, f32 b) {
    return (mat4x4_f32) {.v = {
        {a.v[0][0]*b, a.v[0][1]*b, a.v[0][2]*b, a.v[0][3]*b},
        {a.v[1][0]*b, a.v[1][1]*b, a.v[1][2]*b, a.v[1][3]*b},
        {a.v[2][0]*b, a.v[2][1]*b, a.v[2][2]*b, a.v[2][3]*b},
        {a.v[3][0]*b, a.v[3][1]*b, a.v[3][2]*b, a.v[3][3]*b},
    }};
}
mat4x4_f32 inv_4x4f32(mat4x4_f32 m) {
    f32 coef00 = m.v[2][2] * m.v[3][3] - m.v[3][2] * m.v[2][3];
    f32 coef02 = m.v[1][2] * m.v[3][3] - m.v[3][2] * m.v[1][3];
    f32 coef03 = m.v[1][2] * m.v[2][3] - m.v[2][2] * m.v[1][3];
    f32 coef04 = m.v[2][1] * m.v[3][3] - m.v[3][1] * m.v[2][3];
    f32 coef06 = m.v[1][1] * m.v[3][3] - m.v[3][1] * m.v[1][3];
    f32 coef07 = m.v[1][1] * m.v[2][3] - m.v[2][1] * m.v[1][3];
    f32 coef08 = m.v[2][1] * m.v[3][2] - m.v[3][1] * m.v[2][2];
    f32 coef10 = m.v[1][1] * m.v[3][2] - m.v[3][1] * m.v[1][2];
    f32 coef11 = m.v[1][1] * m.v[2][2] - m.v[2][1] * m.v[1][2];
    f32 coef12 = m.v[2][0] * m.v[3][3] - m.v[3][0] * m.v[2][3];
    f32 coef14 = m.v[1][0] * m.v[3][3] - m.v[3][0] * m.v[1][3];
    f32 coef15 = m.v[1][0] * m.v[2][3] - m.v[2][0] * m.v[1][3];
    f32 coef16 = m.v[2][0] * m.v[3][2] - m.v[3][0] * m.v[2][2];
    f32 coef18 = m.v[1][0] * m.v[3][2] - m.v[3][0] * m.v[1][2];
    f32 coef19 = m.v[1][0] * m.v[2][2] - m.v[2][0] * m.v[1][2];
    f32 coef20 = m.v[2][0] * m.v[3][1] - m.v[3][0] * m.v[2][1];
    f32 coef22 = m.v[1][0] * m.v[3][1] - m.v[3][0] * m.v[1][1];
    f32 coef23 = m.v[1][0] * m.v[2][1] - m.v[2][0] * m.v[1][1];

    vec4_f32 fac0 = { coef00, coef00, coef02, coef03 };
    vec4_f32 fac1 = { coef04, coef04, coef06, coef07 };
    vec4_f32 fac2 = { coef08, coef08, coef10, coef11 };
    vec4_f32 fac3 = { coef12, coef12, coef14, coef15 };
    vec4_f32 fac4 = { coef16, coef16, coef18, coef19 };
    vec4_f32 fac5 = { coef20, coef20, coef22, coef23 };

    vec4_f32 vec0 = { m.v[1][0], m.v[0][0], m.v[0][0], m.v[0][0] };
    vec4_f32 vec1 = { m.v[1][1], m.v[0][1], m.v[0][1], m.v[0][1] };
    vec4_f32 vec2 = { m.v[1][2], m.v[0][2], m.v[0][2], m.v[0][2] };
    vec4_f32 vec3 = { m.v[1][3], m.v[0][3], m.v[0][3], m.v[0][3] };

    vec4_f32 inv0 = add_4f32(sub_4f32(elmul_4f32(vec1, fac0), elmul_4f32(vec2, fac1)), elmul_4f32(vec3, fac2));
    vec4_f32 inv1 = add_4f32(sub_4f32(elmul_4f32(vec0, fac0), elmul_4f32(vec2, fac3)), elmul_4f32(vec3, fac4));
    vec4_f32 inv2 = add_4f32(sub_4f32(elmul_4f32(vec0, fac1), elmul_4f32(vec1, fac3)), elmul_4f32(vec3, fac5));
    vec4_f32 inv3 = add_4f32(sub_4f32(elmul_4f32(vec0, fac2), elmul_4f32(vec1, fac4)), elmul_4f32(vec2, fac5));

    vec4_f32 sign_a = { +1, -1, +1, -1 };
    vec4_f32 sign_b = { -1, +1, -1, +1 };

    mat4x4_f32 inv;
    for(int i = 0; i < 4; i += 1) {
        inv.v[0][i] = inv0.v[i] * sign_a.v[i];
        inv.v[1][i] = inv1.v[i] * sign_b.v[i];
        inv.v[2][i] = inv2.v[i] * sign_a.v[i];
        inv.v[3][i] = inv3.v[i] * sign_b.v[i];
    }

    vec4_f32 row0 = { inv.v[0][0], inv.v[1][0], inv.v[2][0], inv.v[3][0] };
    vec4_f32 m0 = { m.v[0][0], m.v[0][1], m.v[0][2], m.v[0][3] };
    vec4_f32 dot0 = elmul_4f32(m0, row0);
    f32 dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);

    f32 one_over_det = 1.f / dot1;

    return scale_4x4f32(inv, one_over_det);
}
mat4x4_f32 transpose_4x4f32(mat4x4_f32 m) {
    return (mat4x4_f32) {.v = {
        {m.v[0][0], m.v[1][0], m.v[2][0], m.v[3][0]},
        {m.v[0][1], m.v[1][1], m.v[2][1], m.v[3][1]},
        {m.v[0][2], m.v[1][2], m.v[2][2], m.v[3][2]},
        {m.v[0][3], m.v[1][3], m.v[2][3], m.v[3][3]},
    }};
}

rect_f32 make_rect_f32(vec2_f32 tl, vec2_f32 br) {
    return (rect_f32) { .tl = tl, .br = br };
}

f32 smoothstep_f32(f32 edge_0, f32 edge_1, f32 x) {
    f32 t = Clamp((x - edge_0)/(edge_1 - edge_0), 0.f, 1.f);
    return t*t*(3.f - 2.f*t);
}

u64 hash_u64(u8* buffer, u64 size) {
  u64 result = 5381;
  for(u64 i = 0; i < size; i++) {
    result = ((result << 5) + result) + buffer[i];
  }
  return result;
}