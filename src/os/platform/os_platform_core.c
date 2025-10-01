// helpers
internal OS_Handle os_zero_handle() {
    OS_Handle handle = zero_struct;
    return handle;
}

internal b8 os_is_handle_zero(OS_Handle handle) {
    return handle.v64[0] == 0 && handle.v64[1] == 0;
}