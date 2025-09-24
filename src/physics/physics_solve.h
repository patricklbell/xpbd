#pragma once

// correction helpers
static f32  phys_body_inverse_inertia(PHYS_Body* b, vec3_f32 t_world);
static void phys_body_apply_linear_correction(PHYS_Body* b, vec3_f32 dp_world);
static void phys_body_apply_angular_correction(PHYS_Body* b, vec3_f32 dt_world);

// velocity correction
static void     phys_body_apply_linear_velocity_correction(PHYS_Body* b, vec3_f32 corr);
static void     phys_body_apply_angular_velocity_correction(PHYS_Body* b, vec3_f32 corr, vec3_f32 r);
static vec3_f32 phys_body_velocity_at_offset(PHYS_Body* b, vec3_f32 r);

static f32 phys_lagrange_delta_no_update(f32 C, f32 w, f32 alpha);
static f32 phys_update_lagrange_multiplier_return_delta(f32 C, f32 w, f32 alpha, f32* l);

static f32 phys_calculate_coeffcient(f32 x1, f32 x2, PHYS_CoefficientCalculation method);

static void phys_copy_indexed_buffer_to_polgyon(vec3_f32* in_points, u32* in_indices, GEO_Topology in_topology, GEO_Polygon* out_face);

// 
// constraints
// 
static void phys_constraint_solve_distance(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings);
static void phys_constraint_solve_advanced_distance(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings);
static void phys_constraint_solve_linear_dofs(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings);
static void phys_constraint_solve_volume(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings);
static void phys_constraint_solve_orientation(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings);
static void phys_constraint_solve_hinge(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings);
static void phys_constraint_solve_swing(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings);
static void phys_constraint_solve_twist(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings);

// solver
static void phys_constraint_solve(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings);

// 
// collision
// 
static void     phys_collision_apply_corrections(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2, vec3_f32 dC, f32 l);
static void     phys_collision_apply_velocity_corrections(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2, vec3_f32 dC, f32 l);
static f32      phys_collision_generalized_inverse_mass(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2, vec3_f32 dC);
static vec3_f32 phys_collision_total_velocity(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2);

static void         phys_collision_SAT_min_max(vec3_f32 axis, PHYS_Body* b, PHYS_Collider* c, f32* min, f32* max, u32* min_face, u32* max_face);
static GEO_Polygon  phys_collision_SAT_get_supporting_face(vec3_f32 penetration_axis, u32 f, PHYS_Body* b, PHYS_Collider* c);
static b32          phys_collision_SAT_check_axis(PHYS_CollisionCheck* out, vec3_f32 axis, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider* c1, PHYS_Collider* c2);

static b32 phys_collision_check_spheres(PHYS_CollisionCheck* out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider_Sphere* s1, PHYS_Collider_Sphere* s2);
static b32 phys_collision_check_polytopes(PHYS_CollisionCheck* out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider_Polytope* p1, PHYS_Collider_Polytope* p2);
static b32 phys_collision_check_polytope_sphere(PHYS_CollisionCheck* out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider_Polytope* p1, PHYS_Collider_Sphere* s2);

static void phys_collision_manifold_solve_narrow(PHYS_CollisionCheck* in_out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider* c1, PHYS_Collider* c2, PHYS_ConstraintSolveSettings settings, f32 static_friction, f32 dynamic_friction);
static void phys_collision_solve_narrow(PHYS_ConstraintSolveSettings settings, PHYS_Body* b1, PHYS_Body* b2, PHYS_CollisionCheck* check, f32 static_friction, f32 dynamic_friction);

// solver
static void phys_collision_solve(PHYS_collider_id id1, PHYS_collider_id id2, PHYS_ConstraintSolveSettings settings);

// 
// substep
// 
static void phys_world_substep(PHYS_World* w, f64 dt);

// 
// solve api
// 
void phys_world_step(PHYS_World* w, f64 dt);