#pragma once

// platform specific backend
#if OS_WINDOWING_SYSTEM == OS_WINDOWING_SYSTEM_WAYLAND
    #include "egl/render_opengl_egl.h"
#elif OS_WINDOWING_SYSTEM == OS_WINDOWING_SYSTEM_WASM
    #include "wasm/render_opengl_wasm.h"
#else
    // @todo WINAPI -> wgl
    // @todo XWINDOWS -> glx
    // @todo LINUX -> detect
    #error Unsupported windowing system.
#endif

#if !defined(R_OGL_USES_ES)
    #define R_OGL_USES_ES 0
#endif

void r_ogl_os_init();
void r_ogl_os_cleanup();
void r_ogl_os_window_swap(OS_Handle window, R_Handle rwindow);

static GLuint   r_ogl_handle_to_buffer(R_Handle handle);
static void     r_ogl_handle_set_buffer(R_Handle* handle, GLuint buffer);
static u32      r_ogl_handle_to_size(R_Handle handle);
static void     r_ogl_handle_set_size(R_Handle* handle, u32 size);

static GLuint r_ogl_temp_buffer(u64 size);

typedef struct R_OGL_BufferChain R_OGL_BufferChain;
struct R_OGL_BufferChain {
    R_OGL_BufferChain* next;
    GLuint buffer;
};

typedef struct R_OGL_State R_OGL_State;
struct R_OGL_State
{
    Arena *per_frame_arena;
    
    GLuint shared_vao;
    GLuint programs[R_Mesh3DMaterial_COUNT];

    GLuint shared_buffer;
    u64 shared_buffer_size;
    R_OGL_BufferChain* buffer_free_chain;
};

thread_static R_OGL_State r_ogl_state = zero_struct;

// mappings @todo codegen / macros
static const GLenum r_ogl_topology_mode[] = {
    // R_VertexTopology_ZERO
    GL_TRIANGLES,
    // R_VertexTopology_Points
    GL_POINTS,
    // R_VertexTopology_Lines
    GL_LINES,
    // R_VertexTopology_LineStrip
    GL_LINE_STRIP,
    // R_VertexTopology_Triangles
    GL_TRIANGLES,
    // R_VertexTopology_TriangleStrip
    GL_TRIANGLE_STRIP,
    // R_VertexTopology_Quads
    GL_QUADS,
};

typedef struct OGL_ResourceKindMetadata OGL_ResourceKindMetadata;
struct OGL_ResourceKindMetadata {
    GLenum usage;
};
static const OGL_ResourceKindMetadata r_ogl_resource_kind[] = {
    // R_ResourceKind_Static
    { .usage = GL_STATIC_DRAW },
    // R_ResourceKind_Dynamic
    { .usage = GL_DYNAMIC_DRAW },
    // R_ResourceKind_Stream
    { .usage = GL_STREAM_DRAW },
};

typedef struct OGL_ResourceHintMetadata OGL_ResourceHintMetadata;
struct OGL_ResourceHintMetadata {
    GLenum target;
};
static const OGL_ResourceHintMetadata r_ogl_resource_hint[] = {
    // R_ResourceHint_Array
    { .target = GL_ARRAY_BUFFER },
    // R_ResourceHint_Indices
    { .target = GL_ELEMENT_ARRAY_BUFFER },
};

typedef struct R_OGL_VertexAttribute R_OGL_VertexAttribute;
struct R_OGL_VertexAttribute {
    GLuint location;
    GLint size;
    R_VertexFlag flag;
    GLenum type;
    GLboolean normalized;
    NTString8 name;
};

typedef struct R_OGL_InstanceAttribute R_OGL_InstanceAttribute;
struct R_OGL_InstanceAttribute {
    GLuint location;
    GLint size;
    void* offset;
    GLenum type;
    GLboolean normalized;
    NTString8 name;
};

// 
// materials
//
#if R_OGL_USES_ES
    #define R_OGL_SHADER_PREAMBLE   "#version 300 es\nprecision mediump float;\n"
#else
    #define R_OGL_SHADER_PREAMBLE   "#version 330 core\n"
#endif

// lambertian
static const NTString8 r_ogl_lambertian_vertex_shader_src = ntstr8_lit_init(
    R_OGL_SHADER_PREAMBLE
    "layout (location = 0) in vec3 in_position;"
    "layout (location = 1) in vec3 in_normal;"
    "layout (location = 10) in mat4 in_model;"
    "layout (location = 14) in vec3 in_color;"
    ""
    "out vec3 vs_normal;"
    "out vec3 vs_color;"
    ""
    "uniform mat4 u_view;"
    "uniform mat4 u_projection;"
    ""
    "void main() {"
    "   vs_normal = (in_model*vec4(in_normal, 0.)).xyz;"
    "   vs_color = in_color;"
    ""
    "   gl_Position = u_projection*u_view*in_model*vec4(in_position, 1.0);"
    "}"
);

static const NTString8 r_ogl_lambertian_fragment_shader_src = ntstr8_lit_init(
    R_OGL_SHADER_PREAMBLE
    "in vec3 vs_normal;"
    "in vec3 vs_color;"
    ""
    "out vec4 out_color;"
    ""
    "void main() {"
    "   vec3 albedo = vs_color;"
    "   vec3 i = -normalize(vec3(1., -1., -1.));"
    ""
    "   vec3 n = normalize(vs_normal);"
    "   float idotn = max(dot(i, n), 0.);"
    "   float ambient = 0.3;"
    "   float subsurface = 0.1 * (1.0 - idotn) * 0.5;"
    "   vec3 Lr = ((1.-ambient)*idotn + subsurface + ambient)*albedo;"
    "   out_color = vec4(Lr, 1.0);"
    "}"
);

static const R_OGL_VertexAttribute r_ogl_lambertian_shader_vertex_attributes[] = {
    { .location = 0, .size = sizeof(vec3_f32)/sizeof(f32), .flag = R_VertexFlag_P, .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_position") },
    { .location = 1, .size = sizeof(vec3_f32)/sizeof(f32), .flag = R_VertexFlag_N, .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_normal"  ) },
};

static const R_OGL_InstanceAttribute r_ogl_lambertian_shader_instance_attributes[] = {
    { .location = 10, .size = sizeof(Member(R_Mesh3DInstance, transform.c1))/sizeof(f32), .offset = &Member(R_Mesh3DInstance, transform.c1), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_model") },
    { .location = 11, .size = sizeof(Member(R_Mesh3DInstance, transform.c2))/sizeof(f32), .offset = &Member(R_Mesh3DInstance, transform.c2), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_model") },
    { .location = 12, .size = sizeof(Member(R_Mesh3DInstance, transform.c3))/sizeof(f32), .offset = &Member(R_Mesh3DInstance, transform.c3), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_model") },
    { .location = 13, .size = sizeof(Member(R_Mesh3DInstance, transform.c4))/sizeof(f32), .offset = &Member(R_Mesh3DInstance, transform.c4), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_model") },
    { .location = 14, .size = sizeof(Member(R_Mesh3DInstance, color       ))/sizeof(f32), .offset = &Member(R_Mesh3DInstance, color       ), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_color") },
};

// dieletric pbr
static const NTString8 r_ogl_dieletric_pbr_vertex_shader_src = ntstr8_lit_init(
    R_OGL_SHADER_PREAMBLE
    "layout (location = 0) in vec3 in_position;"
    "layout (location = 1) in vec3 in_normal;"
    "layout (location = 10) in mat4 in_model;"
    "layout (location = 14) in vec4 in_albedo_roughness;"
    "layout (location = 15) in vec3 in_specular;"
    ""
    "out vec3 vs_normal;"
    "out vec3 vs_position;"
    "out vec3 vs_albedo;"
    "out float vs_roughness;"
    "out vec3 vs_specular;"
    ""
    "uniform mat4 u_view;"
    "uniform mat4 u_projection;"
    ""
    "void main() {"
    "   vs_normal = (in_model*vec4(in_normal, 0.)).xyz;"
    "   vec4 world_position = in_model*vec4(in_position, 1.0);"
    "   vs_position = world_position.xyz;"
    "   vs_albedo = in_albedo_roughness.xyz;"
    "   vs_roughness = in_albedo_roughness.w;"
    "   vs_specular = in_specular;"
    ""
    "   gl_Position = u_projection*u_view*world_position;"
    "}"
);

static const NTString8 r_ogl_dieletric_pbr_fragment_shader_src = ntstr8_lit_init(
    R_OGL_SHADER_PREAMBLE
    "#define PI      3.141592f\n"
    "#define EPSILON 1.19209e-07\n"
    ""
    "in vec3 vs_normal;"
    "in vec3 vs_position;"
    "in vec3 vs_albedo;"
    "in float vs_roughness;"
    "in vec3 vs_specular;"
    ""
    "out vec4 out_color;"
    ""
    "uniform mat4 u_view;"
    ""
    "vec3 F_Schlick(float cLdotH, vec3 F0) {"
    "    return F0 + (1.0 - F0)*pow(1.0 - cLdotH, 5.0);"
    "}"
    ""
    "float G2_GGX_corr(float alpha, float cLdotN, float cVdotN) {"
    "    float nom = cLdotN*cVdotN;"
    "    float denom = mix(2.0*nom, cLdotN + cVdotN, alpha) + EPSILON;"
    "    return nom / denom;"
    "}"
    ""
    "float D_GGX(float alpha, float HdotN) {"
    "    float a2 = alpha*alpha;"
    "    float c2 = HdotN*HdotN;"
    ""
    "    float nom = a2;"
    "    float denom = PI*(c2*(a2 - 1.0) + 1.0)*(c2*(a2 - 1.0) + 1.0);"
    "    return nom / denom;"
    "}"
    ""
    "vec3 L_r(vec3 N, vec3 V, float cVdotN, float alpha, vec3 albedo, vec3 specular, vec3 L, float incident_radiance) {"
    "    vec3 H = normalize(V + L);"
    "    float cLdotN = max(dot(L, N), 0.0);"
    "    float cLdotH = max(dot(L, H), 0.0);"
    "    float HdotN = dot(H, N);"
    ""
    "    vec3 F = F_Schlick(cLdotH, specular);"
    "    float D = D_GGX(alpha, HdotN);"
    "    float G2 = G2_GGX_corr(alpha, cLdotN, cVdotN);"
    ""
    "    vec3 brdf_s = F*D*G2 / (4.0*cLdotN*cVdotN + EPSILON);"
    "    vec3 brdf_d = (1.0 - F)*albedo / PI;"
    ""
    "    return (brdf_s + brdf_d)*incident_radiance*cLdotN;"
    "}"
    ""
    "vec3 reinhard_tonemap(vec3 x) {"
    "   return x / (x + vec3(1.0));"
    "}"
    ""
    "vec3 gamma_correction(vec3 mapped) {"
    "   const float gamma = 2.2;"
    "   return pow(mapped, vec3(1.0 / gamma));"
    "}"
    ""
    "void main() {"
    "   vec3 E = -transpose(mat3(u_view)) * u_view[3].xyz;"
    "   vec3 N = normalize(vs_normal);"
    "   vec3 V = normalize(E - vs_position);"
    "   float cVdotN = max(dot(V, N), 0.0);"
    "   float alpha = vs_roughness*vs_roughness;"
    ""
    "   vec3 sun_direction = normalize(vec3(-1.0,1.0,1.0));"
    "   float sun_radiance = 10.0;"
    "   vec3 radiance = L_r(N, V, cVdotN, alpha, vs_albedo, vs_specular, sun_direction, sun_radiance);"
    ""
    "   float ambient = 0.1;"
    "   radiance += ambient*vs_albedo / PI;"
    ""
    "   out_color = vec4(gamma_correction(reinhard_tonemap(radiance)), 1.0);"
    "}"
);

static const R_OGL_VertexAttribute r_ogl_dieletric_pbr_shader_vertex_attributes[] = {
    { .location = 0, .size = sizeof(vec3_f32)/sizeof(f32), .flag = R_VertexFlag_P, .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_position") },
    { .location = 1, .size = sizeof(vec3_f32)/sizeof(f32), .flag = R_VertexFlag_N, .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_normal"  ) },
};

static const R_OGL_InstanceAttribute r_ogl_dieletric_pbr_shader_instance_attributes[] = {
    { .location = 10, .size = sizeof(Member(R_PBRMesh3DInstance, transform.c1))/sizeof(f32), .offset = &Member(R_PBRMesh3DInstance, transform.c1), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_model") },
    { .location = 11, .size = sizeof(Member(R_PBRMesh3DInstance, transform.c2))/sizeof(f32), .offset = &Member(R_PBRMesh3DInstance, transform.c2), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_model") },
    { .location = 12, .size = sizeof(Member(R_PBRMesh3DInstance, transform.c3))/sizeof(f32), .offset = &Member(R_PBRMesh3DInstance, transform.c3), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_model") },
    { .location = 13, .size = sizeof(Member(R_PBRMesh3DInstance, transform.c4))/sizeof(f32), .offset = &Member(R_PBRMesh3DInstance, transform.c4), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_model") },
    { .location = 14, .size = sizeof(Member(R_PBRMesh3DInstance, albedo_roughness))/sizeof(f32), .offset = &Member(R_PBRMesh3DInstance, albedo_roughness), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_albedo_roughness") },
    { .location = 15, .size = sizeof(Member(R_PBRMesh3DInstance, specular        ))/sizeof(f32), .offset = &Member(R_PBRMesh3DInstance, specular        ), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_specular"        ) },
};

// debug
static const NTString8 r_ogl_debug_vertex_shader_src = ntstr8_lit_init(
    R_OGL_SHADER_PREAMBLE
    "layout (location = 0) in vec3 in_position;"
    "layout (location = 1) in vec3 in_color;"
    "layout (location = 10) in mat4 in_model;"
    ""
    "out vec3 vs_color;"
    ""
    "uniform mat4 u_view;"
    "uniform mat4 u_projection;"
    ""
    "void main() {"
    "   vs_color = in_color;"
    ""
    "   gl_Position = u_projection*u_view*in_model*vec4(in_position, 1.0);"
    "}"
);

static const NTString8 r_ogl_debug_fragment_shader_src = ntstr8_lit_init(
    R_OGL_SHADER_PREAMBLE
    "in vec3 vs_color;"
    ""
    "out vec4 out_color;"
    ""
    "void main() {"
    "   out_color = vec4(vs_color, 1.0);"
    "}"
);

static const R_OGL_VertexAttribute r_ogl_debug_shader_vertex_attributes[] = {
    { .location = 0, .size = sizeof(vec3_f32)/sizeof(f32), .flag = R_VertexFlag_P, .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_position") },
    { .location = 1, .size = sizeof(vec3_f32)/sizeof(f32), .flag = R_VertexFlag_C, .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_color"   ) },
};

static const R_OGL_InstanceAttribute r_ogl_debug_shader_instance_attributes[] = {
    { .location = 10, .size = sizeof(Member(R_Mesh3DInstance, transform.c1))/sizeof(f32), .offset = &Member(R_Mesh3DInstance, transform.c1), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_model") },
    { .location = 11, .size = sizeof(Member(R_Mesh3DInstance, transform.c2))/sizeof(f32), .offset = &Member(R_Mesh3DInstance, transform.c2), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_model") },
    { .location = 12, .size = sizeof(Member(R_Mesh3DInstance, transform.c3))/sizeof(f32), .offset = &Member(R_Mesh3DInstance, transform.c3), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_model") },
    { .location = 13, .size = sizeof(Member(R_Mesh3DInstance, transform.c4))/sizeof(f32), .offset = &Member(R_Mesh3DInstance, transform.c4), .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_model") },
};

// debug splat
static const NTString8 r_ogl_splat_vertex_shader_src = ntstr8_lit_init(
    R_OGL_SHADER_PREAMBLE
    "layout (location = 0) in vec3 in_position;"
    "layout (location = 1) in vec3 in_color;"
    "layout (location = 2) in vec2 in_radius;"
    ""
    "out vec3 vs_color;"
    "out vec2 vs_radius;"
    ""
    "uniform mat4 u_view;"
    ""
    "void main() {"
    "   vs_color = in_color;"
    "   vs_radius = in_radius;"
    ""
    "   gl_Position = u_view*vec4(in_position, 1.0);"
    "}"
);

static const NTString8 r_ogl_splat_geometry_shader_src = ntstr8_lit_init(
    R_OGL_SHADER_PREAMBLE
    "layout(points) in;"
    "layout(triangle_strip, max_vertices = 4) out;"
    ""
    "in vec3 vs_color[];"
    "in vec2 vs_radius[];"
    ""
    "out vec3 gs_color;"
    "out vec2 gs_texcoord;"
    ""
    "uniform mat4 u_projection;"
    ""
    "void main() {"
    "    vec4 center = gl_in[0].gl_Position;"
    ""
    "    vec4 positions[4];"
    "    positions[0] = center + vec4(-vs_radius[0].x, -vs_radius[0].y, 0.0, 0.0);"
    "    positions[1] = center + vec4( vs_radius[0].x, -vs_radius[0].y, 0.0, 0.0);"
    "    positions[2] = center + vec4(-vs_radius[0].x,  vs_radius[0].y, 0.0, 0.0);"
    "    positions[3] = center + vec4( vs_radius[0].x,  vs_radius[0].y, 0.0, 0.0);"
    ""
    "    vec2 texcoords[4];"
    "    texcoords[0] = vec2(-1.0, -1.0);"
    "    texcoords[1] = vec2( 1.0, -1.0);"
    "    texcoords[2] = vec2(-1.0,  1.0);"
    "    texcoords[3] = vec2( 1.0,  1.0);"
    ""
    "    for (int i = 0; i < 4; i++) {"
    "        gl_Position = u_projection*positions[i];"
    "        gs_color = vs_color[0];"
    "        gs_texcoord = texcoords[i];"
    "        EmitVertex();"
    "    }"
    ""
    "    EndPrimitive();"
    "}"
);

static const NTString8 r_ogl_splat_fragment_shader_src = ntstr8_lit_init(
    R_OGL_SHADER_PREAMBLE
    "in vec3 gs_color;"
    "in vec2 gs_texcoord;"
    ""
    "out vec4 out_color;"
    ""
    "const float u_blend = 0.1;"
    ""
    "void main() {"
    "   float d = length(gs_texcoord);"
    "   float alpha = smoothstep(1.0, 0.0, (d-1.0)/u_blend + 0.5);"
    "   out_color = vec4(gs_color, alpha);"
    "}"
);

static const R_OGL_VertexAttribute r_ogl_splat_shader_vertex_attributes[] = {
    { .location = 0, .size = sizeof(vec3_f32)/sizeof(f32), .flag = R_VertexFlag_P, .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_position") },
    { .location = 1, .size = sizeof(vec3_f32)/sizeof(f32), .flag = R_VertexFlag_C, .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_color"   ) },
    { .location = 2, .size = sizeof(vec2_f32)/sizeof(f32), .flag = R_VertexFlag_T, .type = GL_FLOAT, .normalized = GL_FALSE, .name = ntstr8_lit_init("in_radius"  ) },
};

static const R_OGL_InstanceAttribute r_ogl_splat_shader_instance_attributes[] = {};

typedef struct R_OGL_ProgramDefinition R_OGL_ProgramDefinition;
struct R_OGL_ProgramDefinition {
    R_Mesh3DMaterial material;

    NTString8 vertex_shader_src;
    NTString8 fragment_shader_src;
    NTString8 geometry_shader_src;

    const R_OGL_VertexAttribute* vertex_attributes;
    int vertex_attribute_count;

    const R_OGL_InstanceAttribute* instance_attributes;
    int instance_attribute_count;

    b32 disable_depth_test;
};

static const R_OGL_ProgramDefinition r_ogl_programs_definitions[R_Mesh3DMaterial_COUNT] = {
    {
        .material = R_Mesh3DMaterial_Lambertian,
        .vertex_shader_src = r_ogl_lambertian_vertex_shader_src,
        .fragment_shader_src = r_ogl_lambertian_fragment_shader_src,
        .vertex_attributes = r_ogl_lambertian_shader_vertex_attributes,
        .vertex_attribute_count = ArrayLength(r_ogl_lambertian_shader_vertex_attributes),
        .instance_attributes = r_ogl_lambertian_shader_instance_attributes,
        .instance_attribute_count = ArrayLength(r_ogl_lambertian_shader_instance_attributes),
        .disable_depth_test = false,
    },
    {
        .material = R_Mesh3DMaterial_DieletricPBR,
        .vertex_shader_src = r_ogl_dieletric_pbr_vertex_shader_src,
        .fragment_shader_src = r_ogl_dieletric_pbr_fragment_shader_src,
        .vertex_attributes = r_ogl_dieletric_pbr_shader_vertex_attributes,
        .vertex_attribute_count = ArrayLength(r_ogl_dieletric_pbr_shader_vertex_attributes),
        .instance_attributes = r_ogl_dieletric_pbr_shader_instance_attributes,
        .instance_attribute_count = ArrayLength(r_ogl_dieletric_pbr_shader_instance_attributes),
        .disable_depth_test = false,
    },
    {
        .material = R_Mesh3DMaterial_Debug,
        .vertex_shader_src = r_ogl_debug_vertex_shader_src,
        .fragment_shader_src = r_ogl_debug_fragment_shader_src,
        .vertex_attributes = r_ogl_debug_shader_vertex_attributes,
        .vertex_attribute_count = ArrayLength(r_ogl_debug_shader_vertex_attributes),
        .instance_attributes = r_ogl_debug_shader_instance_attributes,
        .instance_attribute_count = ArrayLength(r_ogl_debug_shader_instance_attributes),
        .disable_depth_test = true,
    },
    {
        .material = R_Mesh3DMaterial_Splat,
        #if R_OGL_USES_ES
        .vertex_shader_src = r_ogl_debug_vertex_shader_src,
        .fragment_shader_src = r_ogl_debug_fragment_shader_src,
        .vertex_attributes = r_ogl_debug_shader_vertex_attributes,
        .vertex_attribute_count = ArrayLength(r_ogl_debug_shader_vertex_attributes),
        .instance_attributes = r_ogl_debug_shader_instance_attributes,
        .instance_attribute_count = ArrayLength(r_ogl_debug_shader_instance_attributes),
        #else
        .vertex_shader_src = r_ogl_splat_vertex_shader_src,
        .fragment_shader_src = r_ogl_splat_fragment_shader_src,
        .geometry_shader_src = r_ogl_splat_geometry_shader_src,
        .vertex_attributes = r_ogl_splat_shader_vertex_attributes,
        .vertex_attribute_count = ArrayLength(r_ogl_splat_shader_vertex_attributes),
        .instance_attributes = r_ogl_splat_shader_instance_attributes,
        .instance_attribute_count = ArrayLength(r_ogl_splat_shader_instance_attributes),
        #endif
        .disable_depth_test = true,
    },
};