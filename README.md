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
- Windows x86/x64
- Linux x86/x64/ARM32/ARM64
- WebAssembly (but there are no bindings)

No special CPU instructions are required.

## Compiling
The build is managed through the `build.bat` script on Windows and the `build.sh` script on Linux, refer to the respective help messages for usage instructions. 
- Cross-compiles as C and C++
- MSVC/gcc/g++/clang/clang++ recent enough to support C11
- The shared library only depends on libc, libm and a few core OS APIs
- The library can be statically built by including the `lib/lib.h` header and adding `lib/lib.c` as a translation unit

### Windows
Building on Windows requires either MSVC or clang to be installed and the command line to be correctly configured. 
This involves installing the C++ build tools for Visual Studio and running `build.bat` from the Native Tools command prompt. 
See [this article](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170) for more information on how to setup the Visual Studio command line tools.

## Demos
The demos should be run from the project directory as they look for files under `data/`. On web, the data files are packed into the artifacts.

### Linux
The demos support Wayland and X11. X11 compatibility may vary because OpenGL is provided by EGL rather than GLX.
On Debian and derivatives like Ubuntu and Linux Mint you will need the `libwayland-dev`, `libxkbcommon-dev` and the `xorg-dev` meta-package. These will pull in all other dependencies.
```
sudo apt install libwayland-dev libxkbcommon-dev xorg-dev
```
On Fedora and derivatives like Red Hat you will need the `wayland-devel`, `libxkbcommon-devel` and `libXi-devel` packages. These will pull in all other dependencies.
```
sudo dnf install wayland-devel libxkbcommon-devel libXi-devel
```

### Web
The demos can be built for web by installing [Emscripten](https://emscripten.org/) and adding it to your path. Installation instructions can be found on the Emscripten website.

## Profiling
Support for profiling with [Tracy](https://github.com/wolfpld/tracy) can be included by adding `--trace`. A submodule is
included in the repo under `src/third_party/tracy` which needs to be initialised and is 
where you can build the profiling tools. Profiling requires a C++ compiler, since MSVC does not support
compound literals in C++, profiling on Windows requires compiling with clang++.