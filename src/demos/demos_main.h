#pragma once

#include "common/common_inc.h"
#include "os/os_inc.h"
#include "hashgrid/hashgrid.h"
#include "physics/physics_inc.h"
#include "render/render_inc.h"
#include "draw/draw.h"
#include "mesh/mesh.h"
#include "input/input.h"
#include "vtk/vtk.h"
#include "dbgdraw/dbgdraw.h"
#include "geo/geo.h"
#if OS_WEB
    #include "emcontrols/emcontrols.h"
#endif

#include "demos_helpers.h"
#include "demos_main.h"

typedef struct DEMOS_CommonState DEMOS_CommonState;
struct DEMOS_CommonState {
    Arena* arena;
    
    OS_Handle window;
    R_Handle rwindow;
    OS_Events events;

    b32 should_reset;
};

// hooks implemented by each demo
int  demos_persistent_init_hook(DEMOS_CommonState*);
void demos_state_init_hook();
void demos_frame_hook(DEMOS_CommonState*);
void demos_state_cleanup_hook();
void demos_persistent_cleanup_hook(DEMOS_CommonState*);