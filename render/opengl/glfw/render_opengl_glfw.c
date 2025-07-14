void r_equip_window(OS_Handle window) {
    glfwMakeContextCurrent(os_handle_to_glfw(window));
}

void r_unequip_window() {
    glfwMakeContextCurrent(NULL);
}

void r_os_window_swap(OS_Handle window) {
    glfwSwapBuffers(os_handle_to_glfw(window));
}