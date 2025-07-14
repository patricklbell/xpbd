// platform specific shared helpers
GLFWwindow* os_handle_to_glfw(OS_Handle handle) {
    return (GLFWwindow*)handle.v64;
}

// implementations
f64 os_now_seconds() {
    return glfwGetTime();
}

void os_init_gfx() {
    glfwInit();
}

void os_restore_gfx() {
    glfwTerminate();
}

OS_Handle os_open_window() {
    GLFWwindow* window = glfwCreateWindow(640, 480, "XPBD", NULL, NULL);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (!window) {
        return os_zero_handle();
    }

    return (OS_Handle) { .v64 = (u64)window };
}

void os_close_window(OS_Handle window) {
    glfwDestroyWindow(os_handle_to_glfw(window));
}

b8 os_window_has_close_event(OS_Handle window) {
    glfwPollEvents();
    return glfwWindowShouldClose(os_handle_to_glfw(window));
}

vec2_f32 os_window_size(OS_Handle window) {
    int w, h;
    glfwGetWindowSize(os_handle_to_glfw(window), &w, &h);

    return (vec2_f32){.x = (f32)w, .y = (f32)h};
}