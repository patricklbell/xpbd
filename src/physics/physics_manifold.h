typedef struct PHYS_ContactPairs PHYS_ContactPairs;
struct PHYS_ContactPairs {
    vec3_f32 bodies[2][GEO_MAX_CLIPPED_TOPOLOGY];
    u32 count;
};
internal void phys_manifold_add_contact_pair(PHYS_ContactPairs* pairs, vec3_f32 p1, vec3_f32 p2);

internal PHYS_ContactPairs phys_manifold_between_faces(vec3_f32 penetration_axis, f32 max_penetration, GEO_Polygon* face1, GEO_Polygon* face2);