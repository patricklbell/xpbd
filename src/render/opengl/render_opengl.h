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

void    r_ogl_os_init();
void    r_ogl_os_cleanup();
void    r_ogl_os_window_swap(OS_Handle window, R_Handle rwindow);

static GLuint  r_ogl_handle_to_buffer(R_Handle handle);
static u32     r_ogl_handle_to_size(R_Handle handle);
static GLuint  r_ogl_temp_buffer(u64 size);

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
    GLuint program;

    GLuint shared_buffer;
    u64 shared_buffer_size;
    R_OGL_BufferChain* buffer_free_chain;
};

thread_static R_OGL_State r_ogl_state = zero_struct;

// mappings @todo codegen / macros
typedef struct OGL_ResourceKindMetadata OGL_ResourceKindMetadata;
struct OGL_ResourceKindMetadata {
    GLenum usage;
};
static const OGL_ResourceKindMetadata r_ogl_resource_kind[] = {
    // R_ResourceKind_Static
    (OGL_ResourceKindMetadata) { .usage = GL_STATIC_DRAW },
    // R_ResourceKind_Dynamic
    (OGL_ResourceKindMetadata) { .usage = GL_DYNAMIC_DRAW },
    // R_ResourceKind_Stream
    (OGL_ResourceKindMetadata) { .usage = GL_STREAM_DRAW },
};

typedef struct OGL_ResourceHintMetadata OGL_ResourceHintMetadata;
struct OGL_ResourceHintMetadata {
    GLenum target;
};
static const OGL_ResourceHintMetadata r_ogl_resource_hint[] = {
    // R_ResourceHint_Array
    (OGL_ResourceHintMetadata) { .target = GL_ARRAY_BUFFER },
    // R_ResourceHint_Indices
    (OGL_ResourceHintMetadata) { .target = GL_ELEMENT_ARRAY_BUFFER },
};

typedef struct R_OGL_Attribute R_OGL_Attribute;
struct R_OGL_Attribute {
    GLuint location;
    GLenum type;
    GLint size;
    GLboolean normalized;
    void* offset;
    NTString8 name;
};


static const NTString8 r_ogl_vertex_shader_src = ntstr8_lit_init(
    #if R_OGL_USES_ES
    "#version 300 es\n"
    "precision mediump float;"
    #else
    "#version 330 core\n"
    #endif
    ""
    "layout (location = 0) in vec3 in_position;"
    "layout (location = 1) in vec3 in_normal;"
    "layout (location = 10) in mat4 in_model;"
    ""
    "out vec3 vs_normal;"
    ""
    "uniform mat4 u_view;"
    "uniform mat4 u_projection;"
    ""
    "void main() {"
    "   vs_normal = (in_model*vec4(in_normal, 0.)).xyz;"
    ""
    "   gl_Position = u_projection*u_view*in_model*vec4(in_position, 1.);"
    "}"
);

static const NTString8 r_ogl_fragment_shader_src = ntstr8_lit_init(
    #if R_OGL_USES_ES
    "#version 300 es\n"
    "precision mediump float;"
    #else
    "#version 330 core\n"
    #endif
    ""
    "in vec3 vs_normal;"
    ""
    "out vec4 out_color;"
    ""
    "void main() {"
    "   vec3 albedo = vec3(0.4, 0.4, 0.4);"
    "   vec3 i = -normalize(vec3(1., -1., -1.));"
    ""
    "   vec3 n = normalize(vs_normal);"
    "   float idotn = clamp(dot(i, n), 0., 1.);"
    "   float ambient = 0.1;"
    "   vec3 Lr = ((1.-ambient)*idotn + ambient)*albedo;"
    "   out_color = vec4(Lr, 1.);"
    "}"
);

// @todo x-macro
static const R_OGL_Attribute r_ogl_shader_vertex_attributes[] = {
    (R_OGL_Attribute) { .location = 0, .type = GL_FLOAT, .size = sizeof(Member(R_VertexLayout, position))/sizeof(f32), .normalized = GL_FALSE, .offset = &Member(R_VertexLayout, position), .name = ntstr8_lit_init("in_position") },
    (R_OGL_Attribute) { .location = 1, .type = GL_FLOAT, .size = sizeof(Member(R_VertexLayout, normal  ))/sizeof(f32), .normalized = GL_FALSE, .offset = &Member(R_VertexLayout, normal  ), .name = ntstr8_lit_init("in_normal"  ) },
};

static const R_OGL_Attribute r_ogl_shader_instance_attributes[] = {
    (R_OGL_Attribute) { .location = 10, .type = GL_FLOAT, .size = sizeof(vec4_f32)/sizeof(f32), .normalized = GL_FALSE, .offset = &Member(R_Mesh3DInst, inst_transform.c1), .name = ntstr8_lit_init("in_mvp") },
    (R_OGL_Attribute) { .location = 11, .type = GL_FLOAT, .size = sizeof(vec4_f32)/sizeof(f32), .normalized = GL_FALSE, .offset = &Member(R_Mesh3DInst, inst_transform.c2), .name = ntstr8_lit_init("in_mvp") },
    (R_OGL_Attribute) { .location = 12, .type = GL_FLOAT, .size = sizeof(vec4_f32)/sizeof(f32), .normalized = GL_FALSE, .offset = &Member(R_Mesh3DInst, inst_transform.c3), .name = ntstr8_lit_init("in_mvp") },
    (R_OGL_Attribute) { .location = 13, .type = GL_FLOAT, .size = sizeof(vec4_f32)/sizeof(f32), .normalized = GL_FALSE, .offset = &Member(R_Mesh3DInst, inst_transform.c4), .name = ntstr8_lit_init("in_mvp") },
};