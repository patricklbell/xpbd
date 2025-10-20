#pragma once

#include "common/common_inc.h"
#include "os/os_inc.h"
#include "hashgrid/hashgrid.h"
#include "geo/geo.h"
#define PHYS_DBG_D_STEP 1
#include "physics/physics_inc.h"
#include "render/render_inc.h"
#include "draw/draw.h"
#include "mesh/mesh.h"
#include "input/input.h"
#include "vtk/vtk.h"
#include "dbgdraw/dbgdraw.h"
#include "emcontrols/emcontrols.h"

#include "demos_helpers.h"
#include "demos_main.h"

// header only
#include "tracing/tracing.h"

#define demos_hook internal

typedef struct DEMOS_CommonState DEMOS_CommonState;
struct DEMOS_CommonState {
    Arena* arena;
    
    OS_Handle window;
    R_Handle rwindow;
    
    b32 should_reset;
    b32 is_paused;
    b32 should_draw_dbg;
    b32 show_debug;
    b32 dont_show_frame;

    DEMOS_Camera camera;
    PHYS_World* w;
    f64 time;
};

// hooks implemented by each demo
demos_hook int  demos_init_hook(DEMOS_CommonState*);
demos_hook void demos_cleanup_hook(DEMOS_CommonState*);
demos_hook void demos_world_start_hook(PHYS_World*);
demos_hook void demos_world_end_hook(PHYS_World*);
demos_hook void demos_frame_hook(DEMOS_CommonState*);

// wrappers
internal void demos_world_start_wrapper(DEMOS_CommonState*);
internal void demos_world_end_wrapper(DEMOS_CommonState*);
internal force_inline void demos_frame_hook_wrapper(DEMOS_CommonState*);

// emcontrol callbacks
no_inline internal void demos_on_reset_button(void* data);