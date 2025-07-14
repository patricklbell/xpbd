#pragma once

typedef struct Ball Ball;
struct Ball {
    f32 radius;

    vec3_f32 position;
    vec3_f32 prev_position;
    vec3_f32 velocity;
};

typedef struct PhysicsState PhysicsState;
struct PhysicsState {
    Ball ball;
    f32 little_g;
};

void phys_step(PhysicsState* state, f32 dt);