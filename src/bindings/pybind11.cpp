#include <pybind11/pybind11.h>
#include "lib/lib.h"

namespace py = pybind11;

#include "pybind11.hpp"
namespace XPBDPYBIND {
    // 
    // Body
    // 
    PHYS_Body* Body::resolve() {
        World& world = world_obj.cast<World&>();
        return phys_world_resolve_body(world.w, id);
    }
    Body::Body(World& w) {
        this->world_obj = py::cast(w);
        this->id = phys_world_add_body(w.w, (PHYS_Body){});
        printf("added body %d\n", this->id);
    }
    Body::~Body() {
        World& world = world_obj.cast<World&>();
        phys_world_remove_body(world.w, id);
        printf("removed body %d\n", id);
    }

    // 
    // DistanceConstraint
    // 
    PHYS_Constraint* DistanceConstraint::resolve() {
        World& world = world_obj.cast<World&>();
        return phys_world_resolve_constraint(world.w, id);
    }
    DistanceConstraint::DistanceConstraint(World& w, Body& body1, Body& body2) {
        this->world_obj = py::cast(w);
        this->body1_obj = py::cast(body1);
        this->body2_obj = py::cast(body2);
        this->id = phys_world_add_constraint(w.w, (PHYS_Constraint){
            .type = PHYS_ConstraintType_Distance,
            .distance = {
                .body1 = body1.get_id(),
                .body2 = body2.get_id(),
            }
        });
        printf("added distance constraint %d\n", this->id);
    }
    DistanceConstraint::~DistanceConstraint() {
        World& w = world_obj.cast<World&>();
        phys_world_remove_constraint(w.w, id);
        printf("removed distance constraint %d\n", id);
    }

    // 
    // SphereCollider
    // 
    PHYS_Collider_Sphere* SphereCollider::resolve() {
        World& world = world_obj.cast<World&>();
        return &phys_world_resolve_collider(world.w, id)->sphere;
    }
    SphereCollider::SphereCollider(World& w, Body& body) {
        this->world_obj = py::cast(w);
        this->body_obj = py::cast(body);
        this->id = phys_world_add_collider(w.w, (PHYS_Collider){
            .base = {
                .type = PHYS_ColliderType_Sphere,
                .p = body.get_id(),
            }
        });
        printf("added sphere collider %d\n", this->id);
    }
    SphereCollider::~SphereCollider() {
        World& w = world_obj.cast<World&>();
        phys_world_remove_collider(w.w, id);
        printf("removed sphere collider %d\n", id);
    }
}

PYBIND11_MODULE(LIBRARY_NAME, m, py::mod_gil_not_used()) {
    m.doc() = "XPBD Physics";

    py::class_<vec3_f32>(m, "vec3", py::buffer_protocol())
        .def(py::init([](py::buffer b) {
            py::buffer_info info = b.request();
            if (!info.item_type_is_equivalent_to<f32>())
                throw std::runtime_error("Incompatible format: expected an f32 array!");
            if (info.ndim != 1)
                throw std::runtime_error("Incompatible buffer dimension!");
            if (info.shape[0] != 3)
                throw std::runtime_error("Incompatible shape!");

            return (vec3_f32*)info.ptr;
        }))
        .def_buffer([](vec3_f32 &v) -> py::buffer_info {
            return py::buffer_info(
                v.v,
                sizeof(f32),
                py::format_descriptor<f32>::format(),
                1,
                { ArrayLength(v.v) },
                { sizeof(f32) }
            );
        });
    py::class_<vec4_f32>(m, "vec4", py::buffer_protocol())
        .def(py::init([](py::buffer b) {
            py::buffer_info info = b.request();
            if (!info.item_type_is_equivalent_to<f32>())
                throw std::runtime_error("Incompatible format: expected an f32 array!");
            if (info.ndim != 1)
                throw std::runtime_error("Incompatible buffer dimension!");
            if (info.shape[0] != 4)
                throw std::runtime_error("Incompatible shape!");

            return (vec4_f32*)info.ptr;
        }))
        .def_buffer([](vec4_f32 &v) -> py::buffer_info {
            return py::buffer_info(
                v.v,
                sizeof(f32),
                py::format_descriptor<f32>::format(),
                1,
                { ArrayLength(v.v) },
                { sizeof(f32) }
            );
        });
    py::class_<PHYS_WorldSettings>(m, "WorldSettings")
        .def_readwrite("substeps", &PHYS_WorldSettings::substeps)
        .def_readwrite("little_g", &PHYS_WorldSettings::little_g)
        .def(py::init<>());
    py::class_<XPBDPYBIND::World>(m, "World")
        .def(py::init<const PHYS_WorldSettings&>())
        .def("add_body", &XPBDPYBIND::World::add_body)
        .def("add_distance_constraint", &XPBDPYBIND::World::add_distance_constraint)
        .def("add_sphere_collider", &XPBDPYBIND::World::add_sphere_collider)
        .def("step", &XPBDPYBIND::World::step);
    py::classh<XPBDPYBIND::Body>(m, "Body")
        .def("get_position", &XPBDPYBIND::Body::get_position)
        .def("set_position", &XPBDPYBIND::Body::set_position)
        .def("get_linear_velocity", &XPBDPYBIND::Body::get_linear_velocity)
        .def("set_linear_velocity", &XPBDPYBIND::Body::set_linear_velocity)
        .def("get_rotation", &XPBDPYBIND::Body::get_rotation)
        .def("set_rotation", &XPBDPYBIND::Body::set_rotation)
        .def("get_angular_velocity", &XPBDPYBIND::Body::get_angular_velocity)
        .def("set_angular_velocity", &XPBDPYBIND::Body::set_angular_velocity)
        .def("get_no_gravity", &XPBDPYBIND::Body::get_no_gravity)
        .def("set_no_gravity", &XPBDPYBIND::Body::set_no_gravity)
        .def("get_inv_mass", &XPBDPYBIND::Body::get_inv_mass)
        .def("set_inv_mass", &XPBDPYBIND::Body::set_inv_mass);
    py::classh<XPBDPYBIND::DistanceConstraint>(m, "DistanceConstraint")
        .def("get_l", &XPBDPYBIND::DistanceConstraint::get_l)
        .def("get_compliance", &XPBDPYBIND::DistanceConstraint::get_compliance)
        .def("get_d", &XPBDPYBIND::DistanceConstraint::get_d)
        .def("get_unilateral", &XPBDPYBIND::DistanceConstraint::get_unilateral)
        .def("set_l", &XPBDPYBIND::DistanceConstraint::set_l)
        .def("set_compliance", &XPBDPYBIND::DistanceConstraint::set_compliance)
        .def("set_d", &XPBDPYBIND::DistanceConstraint::set_d)
        .def("set_unilateral", &XPBDPYBIND::DistanceConstraint::set_unilateral);
    py::classh<XPBDPYBIND::SphereCollider>(m, "SphereCollider")
        .def("get_r", &XPBDPYBIND::SphereCollider::get_r)
        .def("set_r", &XPBDPYBIND::SphereCollider::set_r);

    // @todo
    // phys_collider_layers_overlap
    // phys_collider_layers_overlap

    // phys_world_add_fixed_point
    // phys_world_remove_rigid_body
    // phys_world_add_ball
    // phys_world_add_box
    // phys_world_add_box_boundary
    // phys_world_remove_box_boundary
    // phys_world_add_softbody
    // phys_world_remove_softbody
    // phys_world_add_cloth
    // phys_world_add_sheet
    // phys_world_remove_cloth

    // phys_make_hit_list
    // phys_hit_list_add
    // phys_hit_list_closest
    // phys_raycast_collider
    // phys_world_raycast
}