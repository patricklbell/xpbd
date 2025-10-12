typedef union PHYS_ConstraintNodeValue {
    PHYS_Constraint constraint;
    PHYS_DependentConstraint dependent_constraint;
} PHYS_ConstraintNodeValue;

typedef struct PHYS_ConstraintNode PHYS_ConstraintNode;
struct PHYS_ConstraintNode {
    PHYS_ConstraintNode* prev;
    PHYS_ConstraintNode* next;
    PHYS_ConstraintNodeValue v;
    PHYS_constraint_id id;
};

typedef struct PHYS_ConstraintList PHYS_ConstraintList;
struct PHYS_ConstraintList {
    PHYS_ConstraintNode* first;
    PHYS_ConstraintNode* last;
};

#define PHYS_CONSTRAINT_MAP_DEFAULT_SLOTS_COUNT 40000
#define PHYS_DEPENDENT_CONSTRAINT_MAP_DEFAULT_SLOTS_COUNT 8
typedef struct PHYS_ConstraintMap PHYS_ConstraintMap;
struct PHYS_ConstraintMap {
    PHYS_ConstraintList* slots;
    u32 slots_count;
    u32 max_idx;
    PHYS_ConstraintNode* free_chain;
    u32 length;
};

internal PHYS_constraint_id phys_constraint_map_add_value(PHYS_ConstraintMap* map, Arena* arena, PHYS_ConstraintNodeValue v);
internal PHYS_ConstraintNode* phys_constraint_map_get_node(PHYS_ConstraintMap* map, u32 i);
internal PHYS_ConstraintNodeValue* phys_constraint_map_get_value(PHYS_ConstraintMap* map, PHYS_constraint_id id);
internal void phys_constraint_map_delete(PHYS_ConstraintMap* map, PHYS_constraint_id id);