// clipping method between faces using Sutherland-Hodgman clipping.
// See https://research.ncl.ac.uk/game/mastersdegree/gametechnologies/previousinformation/physics5collisionmanifolds/
// 
// It is expected that face1 and face2 lie on colliders 1 and 2 respectively and
// that the penetration axis is relative to collider 1. The faces should also be
// chosen should containt the vertex furthest along the penetration axis and be
// best aligned with the penetration axis.
internal PHYS_ContactPairs phys_manifold_between_faces(vec3_f32 penetration_axis, f32 max_penetration, GEO_Polygon* face1, GEO_Polygon* face2) {
    PHYS_ContactPairs pairs = {.count=0};

    if (face1->topology == GEO_Topology_Point) {
        vec3_f32 penetration = mul_3f32(penetration_axis, max_penetration);
        phys_manifold_add_contact_pair(&pairs, face1->data[0], sub_3f32(face1->data[0], penetration));
        return pairs;
    }
    if (face2->topology == GEO_Topology_Point) {
        vec3_f32 penetration = mul_3f32(penetration_axis, max_penetration);
        phys_manifold_add_contact_pair(&pairs, add_3f32(face2->data[0], penetration), face2->data[0]);
        return pairs;
    }

    // no stable manifold can exist unless at least one face has an area and 
    // the other can intersect with this area
    if (
        Max(face1->topology, face2->topology) < GEO_Topology_Triangle ||
        Min(face1->topology, face2->topology) < GEO_Topology_Edge
    ) {
        return pairs;
    }
        
    u32 incident_pair_idx = 0;
    GEO_Polygon *incident = face1, *reference = face2;

    // ensure incident face can be clipped
    if (incident->topology < GEO_Topology_Triangle) {
        GEO_Polygon* tmp = incident; incident = reference; reference = tmp;
        incident_pair_idx ^= 1;
    }

    // ensure reference best aligns with penetration axis
    // @todo JoltPhysics does not seem to do this step?
    if (reference->topology >= GEO_Topology_Triangle && 
        abs_f32(dot_3f32(phys_polygon_normal_ccw(reference), penetration_axis)) < abs_f32(dot_3f32(phys_polygon_normal_ccw(incident), penetration_axis))
    ) {
        GEO_Polygon* tmp = incident; incident = reference; reference = tmp;
        incident_pair_idx ^= 1;
    }

    if (incident_pair_idx == 0)
        penetration_axis = mul_3f32(penetration_axis, -1.f);

    // clip incident face against reference
    GEO_Polygon clipped;
    if (reference->topology == GEO_Topology_Edge) {
        clipped = geo_clip_polygon_against_edge(incident, reference->data[0], reference->data[1], penetration_axis);
    } else {
        clipped = geo_clip_polygon_against_polygon(incident, reference, penetration_axis);
    }

    // final clipping to ensure clipped vertices lie behind the reference face
    vec3_f32 reference_normal, reference_origin = reference->data[0];
    if (reference->topology == GEO_Topology_Edge) {
        vec3_f32 d = sub_3f32(reference->data[1], reference->data[0]);
        reference_normal = cross_3f32(cross_3f32(d, penetration_axis), d);
    } else {
        reference_normal = phys_polygon_normal_ccw(reference);
    }

    f32 penetration_axis_dot_reference_normal = dot_3f32(penetration_axis, reference_normal);
    if (penetration_axis_dot_reference_normal == 0.f)
        return pairs; // @todo
    f32 penetration_axis_l = length_3f32(penetration_axis);

    Assert(clipped.topology >= 0 && clipped.topology < ArrayLength(pairs.bodies[0]));
    for EachIndex(i, clipped.topology) {
        vec3_f32 i_v = clipped.data[i]; // @note contact relative to reference

        // project incident face onto reference face in direction of penetration:
        // S: r_v = i_v + t*norm(penetration_axis)
        // P: (r_v - ref_origin) . ref_normal = 0
        f32 t_ = dot_3f32(sub_3f32(i_v, reference_origin), reference_normal) / penetration_axis_dot_reference_normal;
        f32 t = t_*penetration_axis_l;

        // ensure reference vertex is within collision region
        if (t < max_penetration) {
            vec3_f32 r_v = sub_3f32(i_v, mul_3f32(penetration_axis, t_));

            pairs.bodies[incident_pair_idx][pairs.count] = i_v;
            pairs.bodies[incident_pair_idx^1][pairs.count] = r_v;
            pairs.count++;
        }
    }

    return pairs;
}

internal void phys_manifold_add_contact_pair(PHYS_ContactPairs* pairs, vec3_f32 p1, vec3_f32 p2) {
    pairs->bodies[0][pairs->count] = p1;
    pairs->bodies[1][pairs->count] = p2;
    pairs->count++;
}
