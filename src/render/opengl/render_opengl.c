// platform specific backend
#if OS_WINDOWING_SYSTEM == OS_WINDOWING_SYSTEM_WAYLAND
    #include "egl/render_opengl_egl.c"
#elif OS_WINDOWING_SYSTEM == OS_WINDOWING_SYSTEM_WASM
    #include "wasm/render_opengl_wasm.c"
#else
    // @todo WINAPI -> wgl
    // @todo XWINDOWS -> glx
    // @todo LINUX -> detect
    #error Unsupported windowing system.
#endif

// helpers
static GLuint r_ogl_handle_to_buffer(R_Handle handle) {
    return handle.v32[0];
}

static u32 r_ogl_handle_to_size(R_Handle handle) {
    return handle.v32[1];
}

static GLuint r_ogl_temp_buffer(u64 size) {
    GLuint buffer = r_ogl_state.shared_buffer;
    if (size > r_ogl_state.shared_buffer_size) {    
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);

        // add to free list for cleanup at end of frame
        R_OGL_BufferChain* n = push_array(r_ogl_state.per_frame_arena, R_OGL_BufferChain, 1);
        n->buffer = buffer;
        stack_push(r_ogl_state.buffer_free_chain, n);
    }
    return buffer;
}

static void r_ogl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
  fprintf(stderr, "[OpenGL] %.*s\n", (int)length, message);
}

// render implementation section
void r_init() {
    r_ogl_os_init();

    r_ogl_state.per_frame_arena = arena_alloc();

    glGenVertexArrays(1, &r_ogl_state.shared_vao);
    glBindVertexArray(r_ogl_state.shared_vao);

    // build and compile our shader programs
    for EachElement(program_i, r_ogl_programs_definitions) {
        const R_OGL_ProgramDefinition* program_def = &r_ogl_programs_definitions[program_i];

        int success;
        char log[512];

        // vertex shader
        const char* vertex_src[] = { program_def->vertex_shader_src.cstr };
        GLint vertex_src_lens[] = { (GLint)program_def->vertex_shader_src.length };
        GLuint vertex_id = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_id, 1, vertex_src, vertex_src_lens);
        glCompileShader(vertex_id);
        glGetShaderiv(vertex_id, GL_COMPILE_STATUS, &success);
        glGetShaderInfoLog(vertex_id, ArrayLength(log), NULL, log); // @todo debug
        if (!success) {
            fprintf(stderr, "[Vertex shader %d] %s\n", program_i, log);
        }
    
        // fragment shader
        const char* fragment_src[] = { program_def->fragment_shader_src.cstr };
        GLint fragment_src_lens[] = { (GLint)program_def->fragment_shader_src.length };
        GLuint fragment_id = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_id, 1, fragment_src, fragment_src_lens);
        glCompileShader(fragment_id);
        glGetShaderiv(fragment_id, GL_COMPILE_STATUS, &success);
        glGetShaderInfoLog(fragment_id, ArrayLength(log), NULL, log); // @todo debug
        if (!success) {
            fprintf(stderr, "[Fragment shader %d] %s\n", program_i, log);
        }

        // enable attributes
        for EachIndex(attribute_i, program_def->vertex_attribute_count) {
            const R_OGL_VertexAttribute* attribute = &program_def->vertex_attributes[attribute_i];
            glEnableVertexAttribArray(attribute->location);
        }
        for EachIndex(attribute_i, program_def->instance_attribute_count) {
            const R_OGL_InstanceAttribute* attribute = &program_def->instance_attributes[attribute_i];
            glEnableVertexAttribArray(attribute->location);
        }
    
        // link shaders
        GLuint program = glCreateProgram();
        glAttachShader(program, vertex_id);
        glAttachShader(program, fragment_id);
        glLinkProgram(program);
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        glGetProgramInfoLog(program, ArrayLength(log), NULL, log); // @todo debug
        if (!success) {
            fprintf(stderr, "[Linking shader %d] %s\n", program_i, log);
        }
        glDeleteShader(vertex_id);
        glDeleteShader(fragment_id);

        r_ogl_state.programs[program_i] = program;
    }
    
    r_ogl_state.shared_buffer_size = KB(64);
    glGenBuffers(1, &r_ogl_state.shared_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, r_ogl_state.shared_buffer);
    glBufferData(GL_ARRAY_BUFFER, r_ogl_state.shared_buffer_size, 0, GL_DYNAMIC_DRAW);

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#if BUILD_DEBUG && !OS_WEB
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(r_ogl_debug_message_callback, 0);
#endif
}

void r_cleanup() {
    r_ogl_os_cleanup();
}

void r_window_begin_frame(OS_Handle window, R_Handle rwindow) {
    r_os_select_window(window, rwindow);
    vec2_f32 window_size = os_gfx_window_size(window);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, (GLsizei)window_size.x, (GLsizei)window_size.y);
}

void r_window_end_frame(OS_Handle window, R_Handle rwindow) {
    for EachList(n, R_OGL_BufferChain, r_ogl_state.buffer_free_chain) {
        glDeleteBuffers(1, &n->buffer);
    }
    r_ogl_state.buffer_free_chain = NULL;
    arena_clear(r_ogl_state.per_frame_arena);

    r_ogl_os_window_swap(window, rwindow);
}

R_Handle r_buffer_alloc(R_ResourceKind kind, R_ResourceHint hint, u32 size, void *data) {
    Assert(kind >= 0 && kind < ArrayLength(r_ogl_resource_kind));
    Assert(hint >= 0 && hint < ArrayLength(r_ogl_resource_hint));

    R_Handle buffer;
    glGenBuffers(1, &buffer.v32[0]);
    glBindBuffer(r_ogl_resource_hint[hint].target, buffer.v32[0]);
    glBufferData(r_ogl_resource_hint[hint].target, size, data, r_ogl_resource_kind[kind].usage);

    buffer.v32[1] = size;

    return buffer;
}

void r_buffer_load(R_Handle handle, u32 offset, u32 size, void *data) {
    GLuint buffer = r_ogl_handle_to_buffer(handle);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}

void r_buffer_release(R_Handle handle) {
    GLuint buffer = r_ogl_handle_to_buffer(handle);
    glDeleteBuffers(1, &buffer);
}

void r_submit(OS_Handle window, R_PassList *passes) {
    for EachList(pass_n, R_PassNode, passes->first) {
        R_Pass* pass = &pass_n->v;
        switch (pass->kind) {
            case R_PassKind_3D: {
                R_PassParams_3D* params = pass->params_3d;
                
                glViewport(params->viewport.tl.x, params->viewport.tl.y, params->viewport.br.x, params->viewport.br.y);
                glScissor(params->clip.tl.x, params->clip.tl.y, params->clip.br.x, params->clip.br.y);
                glEnable(GL_SCISSOR_TEST);
                glEnable(GL_CULL_FACE);

                R_BatchGroup3DMap* batch_groups = &params->mesh_batches;
                for EachIndex(slot, batch_groups->slots_count) {
                    for EachList(batch_group_n, R_BatchGroup3DMapNode, batch_groups->slots[slot]) {
                        R_BatchList* batches = &batch_group_n->batches;
                        R_BatchGroup3DParams* group_params = &batch_group_n->params;

                        // load program
                        GLuint program = r_ogl_state.programs[group_params->mesh_material];
                        glUseProgram(program);
                        glUniformMatrix4fv(glGetUniformLocation(program, "u_projection"), 1, GL_FALSE, &params->projection.v[0][0]);
                        glUniformMatrix4fv(glGetUniformLocation(program, "u_view"), 1, GL_FALSE, &params->view.v[0][0]);

                        const R_OGL_ProgramDefinition* program_def = &r_ogl_programs_definitions[group_params->mesh_material];

                        if (program_def->disable_depth_test) {
                            glDisable(GL_DEPTH_TEST);
                        } else {
                            glEnable(GL_DEPTH_TEST);
                        }

                        // bind group data
                        {
                            GLuint bind_index = 0;
                            glBindBuffer(GL_ARRAY_BUFFER, r_ogl_handle_to_buffer(group_params->mesh_vertices));
                            for EachIndex(i, program_def->vertex_attribute_count) {
                                const R_OGL_VertexAttribute* attribute = &program_def->vertex_attributes[i];
                                glEnableVertexAttribArray(attribute->location);
                                u64 offset = r_vertex_offset(group_params->mesh_flags, attribute->flag);
                                u64 stride = r_vertex_stride(group_params->mesh_flags, attribute->flag);
                                glVertexAttribPointer(attribute->location, attribute->size, attribute->type, attribute->normalized, stride, (void*)offset);
                            }
                        }

                        // allocate and bind per-instance data
                        u64 byte_offset = 0;
                        {
                            // fill buffer with instance data
                            glBindBuffer(GL_ARRAY_BUFFER, r_ogl_temp_buffer(batches->bytes_count));
                            for EachList(batch, R_BatchNode, batches->first) {
                                glBufferSubData(GL_ARRAY_BUFFER, byte_offset, batch->v.bytes_count, batch->v.v);
                                byte_offset += batch->v.bytes_count;
                            }

                            // bind instance data to shader attributes
                            for EachIndex(i, program_def->instance_attribute_count) {
                                const R_OGL_InstanceAttribute* attribute = &program_def->instance_attributes[i];
                                glEnableVertexAttribArray(attribute->location);
                                glVertexAttribPointer(attribute->location, attribute->size, attribute->type, attribute->normalized, batches->bytes_per_inst, attribute->offset);
                                glVertexAttribDivisor(attribute->location, 1);
                            }
                        }

                        if (r_ogl_handle_to_buffer(group_params->mesh_indices)) {
                            // bind element buffer
                            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_ogl_handle_to_buffer(group_params->mesh_indices));
    
                            // draw instances
                            glDrawElementsInstanced(r_ogl_topology_mode[group_params->mesh_topology], r_ogl_handle_to_size(group_params->mesh_indices)/sizeof(u32), GL_UNSIGNED_INT, 0, byte_offset/batches->bytes_per_inst);
                        } else {
                            glDrawArraysInstanced(r_ogl_topology_mode[group_params->mesh_topology], 0, r_ogl_handle_to_size(group_params->mesh_vertices)/r_vertex_size(group_params->mesh_flags), byte_offset/batches->bytes_per_inst);
                        }
                    }
                }

                glDisable(GL_DEPTH_TEST);
                glDisable(GL_CULL_FACE);
                glDisable(GL_SCISSOR_TEST);

                break;
            }
        }
    }
}