#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include <stdlib.h>

// files
internal force_inline HANDLE os_handle_to_HANDLE(OS_Handle file);