#pragma once

#define EMCONTROLS_MAX_CONTROLS 64

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

internal void emcontrols_init(Arena* arena);
internal void emcontrols_clear();
internal int  emcontrols_add(EMCONTROLS_Control button);

#if OS_WEB
    #include "wasm/emcontrols_wasm.h"
#endif