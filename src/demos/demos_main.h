#pragma once

#include "common/common_inc.h"
#include "os/os_inc.h"
#include "physics/physics_inc.h"
#include "render/render_inc.h"
#include "draw/draw.h"
#include "mesh/mesh.h"
#include "input/input.h"
#include "demos_helpers.h"
#include "demos_main.h"

typedef struct DEMOS_CommonState DEMOS_CommonState;
struct DEMOS_CommonState {
    Arena* arena;
    
    OS_Handle window;
    R_Handle rwindow;
    OS_Events events;
};

// hooks implemented by each demo
int demos_init_hook(DEMOS_CommonState*);
void demos_frame_hook(DEMOS_CommonState*);
void demos_shutdown_hook(DEMOS_CommonState*);