namespace XPBDPYBIND {
    class World;
    class Body {
    private:
        py::object world_obj;
        PHYS_Body* resolve();
        PHYS_body_id id;
    public:
        Body(World& w);
        ~Body();

        const PHYS_body_id& get_id() const {return id;}

        vec3_f32 get_position         () {return resolve()->position;}
        vec3_f32 get_linear_velocity  () {return resolve()->linear_velocity;}
        vec4_f32 get_rotation         () {return resolve()->rotation;}
        vec3_f32 get_angular_velocity () {return resolve()->angular_velocity;}
        bool     get_no_gravity       () {return resolve()->no_gravity;}
        f32      get_inv_mass         () {return resolve()->inv_mass;}

        void set_position         (vec3_f32    val) {resolve()->position=val;}
        void set_linear_velocity  (vec3_f32    val) {resolve()->linear_velocity=val;}
        void set_rotation         (vec4_f32    val) {resolve()->rotation=val;}
        void set_angular_velocity (vec3_f32    val) {resolve()->angular_velocity=val;}
        void set_no_gravity       (bool        val) {resolve()->no_gravity=val;}
        void set_inv_mass         (f32         val) {resolve()->inv_mass=val;}
    };

    class DistanceConstraint {
    private:
        py::object world_obj, body1_obj, body2_obj;
        PHYS_Constraint* resolve();
        PHYS_constraint_id id;
    public:
        DistanceConstraint(World& w, Body& body1, Body& body2);
        ~DistanceConstraint();

        const PHYS_constraint_id& get_id() const {return id;}

        f32  get_l          () {return resolve()->l;}
        f32  get_compliance () {return resolve()->compliance;}
        f32  get_d          () {return resolve()->distance.d;}
        bool get_unilateral () {return resolve()->distance.unilateral;}

        void set_l          (f32  val) {resolve()->l=val;}
        void set_compliance (f32  val) {resolve()->compliance=val;}
        void set_d          (f32  val) {resolve()->distance.d=val;}
        void set_unilateral (bool val) {resolve()->distance.unilateral=val;}
    };

    class SphereCollider {
    private:
        py::object world_obj, body_obj;
        PHYS_Collider_Sphere* resolve();
        PHYS_collider_id id;
    public:
        SphereCollider(World& w, Body& body);
        ~SphereCollider();

        const PHYS_collider_id& get_id() const {return id;}

        f32  get_r() {return resolve()->base.r;}

        void set_r(f32 val) {resolve()->base.r=val;}
    };

    class World {
        friend Body;
        friend DistanceConstraint;
        friend SphereCollider;

    private:
        PHYS_World* w;
    public:
        World(const PHYS_WorldSettings& settings) {
            w = phys_make_world(settings);
            printf("created world\n");
        }
        ~World() {
            phys_world_cleanup(w);
            printf("cleaned up world\n");
        }

        std::unique_ptr<Body> add_body() {
            return std::unique_ptr<Body>(new Body{*this});
        }

        std::unique_ptr<DistanceConstraint> add_distance_constraint(std::shared_ptr<Body> b1, std::shared_ptr<Body> b2) {
            return std::unique_ptr<DistanceConstraint>(new DistanceConstraint{*this, *b1.get(), *b2.get()});
        }

        std::unique_ptr<SphereCollider> add_sphere_collider(std::shared_ptr<Body> body) {
            return std::unique_ptr<SphereCollider>(new SphereCollider{*this, *body.get()});
        }

        void step(f32 dt) {
            phys_world_step(w, dt);
        }
    };

}