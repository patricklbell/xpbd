#pragma once

typedef void (*EMCONTROLS_Control_Callback)(void* data);
typedef void (*EMCONTROLS_Control_SliderCallback)(f32 value, void* data);

typedef enum EMCONTROLS_ControlType {
    EMCONTROLS_ControlType_Button = 0,
    EMCONTROLS_ControlType_Slider,
} EMCONTROLS_ControlType;

typedef struct EMCONTROLS_Control EMCONTROLS_Control;
struct EMCONTROLS_Control {
    EMCONTROLS_ControlType type;

    NTString8 label;
    void* data;

    EMCONTROLS_Control_Callback on_press;

    EMCONTROLS_Control_SliderCallback on_slider;
    f32 slider_value; 
    f32 slider_min;
    f32 slider_max;
};

#define EMCONTROLS_MAX_CONTROLS 64

typedef struct EMCONTROLS_ThreadCtx EMCONTROLS_ThreadCtx;
struct EMCONTROLS_ThreadCtx {
    Arena* arena;

    EMCONTROLS_Control controls[EMCONTROLS_MAX_CONTROLS];
    int count;
};

global EMCONTROLS_ThreadCtx* emcontrols_ctx = NULL;

internal void emcontrols_init(Arena* arena);
internal void emcontrols_clear();

internal int  emcontrols_add(EMCONTROLS_Control button);
internal void emcontrols_update(int id, EMCONTROLS_Control button);