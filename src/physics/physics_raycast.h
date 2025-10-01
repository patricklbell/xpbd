#pragma once

typedef struct PHYS_HitListData PHYS_HitListData;
struct PHYS_HitListData {
    f32 contact;
    union {
        u32 polytope_indice_idx;
    };
};

typedef struct PHYS_HitListNode PHYS_HitListNode;
struct PHYS_HitListNode {
    PHYS_HitListNode* next;
    PHYS_collider_id id;
    PHYS_HitListData data;
};

typedef struct PHYS_HitList PHYS_HitList;
struct PHYS_HitList {
    Arena* arena;
    PHYS_HitListNode* hits;
    u32 length;
};

// api
shared_function PHYS_HitList      phys_make_hit_list(Arena* arena);
shared_function void              phys_hit_list_add(PHYS_HitList* hl, PHYS_collider_id id, PHYS_HitListData data);
shared_function PHYS_HitListNode* phys_hit_list_closest(PHYS_HitList* hl);

shared_function void phys_raycast_collider(PHYS_World* w, PHYS_Collider* c, PHYS_collider_id id, vec3_f32 origin, vec3_f32 direction, PHYS_HitList* out_hits);
shared_function void phys_world_raycast(PHYS_World* w, vec3_f32 origin, vec3_f32 direction, PHYS_ColliderLayer layer, PHYS_HitList* out_hits);
