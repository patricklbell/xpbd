This repo contains the source for a physics library. The simulation uses
the method of eXtended Position Based Dynamics (XPBD) which works on positions directly 
which allows for real-time simulation. Accuracy is achieved by 
substepping (splitting a physics steps into many smaller steps). The API allows users to create
worlds and add entities to these worlds. A world can be stepped forward, queried and modified.

A collection of demos can be found online at https://patricklbell.github.io/xpbd/. The source
for each demo, located under each folder `src/demos/*`, acts as an informal documentation.

## Features
- Rigid bodies with discrete collision detection (with non-tunnelling guarantees)
    - Sphere
    - Concave polytope
- Constraints with substep independent compliance
    - Distance (eg. fixed, unilateral, offset)
    - Hinge
    - Swing
    - Twist
    - Lock orientations
    - Linear DOF (eg. prismatic)
- Softbodies (eg. soft ball or cloth)
    - Particle body with hashgrid collision detection
    - Edge distance constraints
    - Tetrahedral volume constraints
    - Internal pressure constraint
    - Self-collision
    - Collision with rigid bodies
- Raycasting
- Kinematic bodies
- Static and dynamic friction

## Platforms
- Linux x86/x64/ARM32/ARM64
- WebAssembly (but there are no bindings)

No special CPU instructions are required.

## Compiling
The build is managed through the `build.sh` script, refer to the help message for usage instructions. 
- Cross-compiles as C and C++
- gcc/g++/clang/clang++ recent enough to support C11
- Library only depends on libc and libm

## Demos
The graphics run on both Wayland and X11 and requires
X11 development libraries and support for EGL. Following the dependencies on 
[this page](https://www.glfw.org/docs/latest/compile_guide.html) should cover all the dependencies.
For reference, the build links `X11` and `Xext`.

The demos can be built for web by installing [Emscripten](https://emscripten.org/) 
and adding it to your path.

## Profiling
Support for profiling with [Tracy](https://github.com/wolfpld/tracy) can be included by adding `--trace`. A submodule is
included in the repo under `src/third_party/tracy` which needs to be 
initialised and is where you can build the profiling tools. Profiling
requires a C++ compiler.