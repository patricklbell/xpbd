#!/bin/bash
set -e

CC=${CC:-g++}
CFLAGS="${CFLAGS} -x c++ -I src -Wno-writable-strings -Wno-write-strings"
LDFLAGS="${LDFLAGS} -lm"
LDFLAGS_GFX="-lX11 -lXext"

BUILD_DIR="build"
BUILD_EXT=""
DATA_DIR="data"

SCRIPT_NAME="build.sh"

print_help() {
    cat << EOF
Usage: $SCRIPT_NAME [OPTION]... [TARGETS]...

Options:
  --debug         Debug build (default)
  --release       Release build
  --emcc          Build with emscripten
  --trace         Enable Tracy profiling
  --help          Display this help and exit

Targets:
  all             Build the library and all demos
  lib             Build only the library
  balls           Build balls demo
  hanging_boxes   Build hanging boxes demo
  softbody        Build softbody demo
  cloth           Build cloth demo
  pendulum        Build pendulum demo
  joints          Build joints demo
  sheet           Build sheet demo
  balloon         Build balloon demo

Environment variables:
  CC              C compiler to use (default: g++)
  CFLAGS          Additional compiler flags
  LDFLAGS         Additional linker flags
EOF
}

print_info() {
    echo "- Build directory: $BUILD_DIR"
    echo "- Compiler:        $CC"
    if [[ -n "$release" ]]; then
    echo "- Mode:            Release"
    else
    echo "- Mode:            Debug"
    fi
    if [[ -v trace ]]; then
    echo "- Tracy:           Enabled"
    else
    echo "- Tracy:           Disabled"
    fi
}

build_single_file() {
    mkdir -p "${BUILD_DIR}"
    
    local demo_name="$1"
    shift
    local required_data_files=("$@")
    local embed_args=""
    local main_file="src/demos/${demo_name}/main.c"

    # Handle data files
    if [ "$CC" = "em++" ]; then
        for data_file in "${required_data_files[@]}"; do
            if [ -f "${DATA_DIR}/${data_file}" ]; then
                embed_args+=" --embed-file ${DATA_DIR}/${data_file}"
            else
                echo "Warning: Data file ${DATA_DIR}/${data_file} not found"
            fi
        done
    else
        CFLAGS="${CFLAGS} -march=native"
    fi
    
    build_command="${CC} ${CFLAGS} ${main_file} ${LDFLAGS} ${LDFLAGS_GFX} ${embed_args} -o ${BUILD_DIR}/${demo_name}${BUILD_EXT}"
    echo "${build_command}"
    eval ${build_command}
}

build_all_demos() {
    build_single_file balls sphere.obj cube.obj
    build_single_file hanging_boxes cube.obj
    build_single_file softbody bunny.vtk
    build_single_file cloth cloth.vtk sphere.obj
    build_single_file pendulum cube.obj
    build_single_file joints cube.obj
    build_single_file sheet
    build_single_file balloon sphere.vtk cube.vtk
}

build_libs() {
    LIBRARY_NAME="libxpbd"
    if [[ -v emcc ]]; then
        LIBSFLAGS="src/bindings/embind.cpp -std=c++11 --bind -Xlinker --no-entry -o extra/js/${LIBRARY_NAME}.wasm"
    elif [[ -v pybind ]]; then
        LIBRARY_NAME="pyXPBD"
        LIBSFLAGS="src/bindings/pybind11.cpp -shared -std=c++11 -fPIC $(python3 -m pybind11 --includes) $(python3-config --includes) -o extra/python/${LIBRARY_NAME}$(python3 -m pybind11 --extension-suffix)"
    else
        LIBSFLAGS="-fPIC -Wl,-soname,xpbd -shared -o ${BUILD_DIR}/${LIBRARY_NAME}.so"
    fi
    build_command="${CC} ${CFLAGS} -DLIBRARY_NAME=${LIBRARY_NAME} src/lib/lib.c ${LIBSFLAGS} ${LDFLAGS}"
    echo "${build_command}"
    eval ${build_command}
}

# build_weblibs() {
#     if ! clang --version > /dev/null 2>&1; then
#         echo "Error: 'weblibs' requires clang"
#         return 1
#     fi
#     clang_version=$(clang --version | grep -oP '^clang version \K[0-9]+')
#     if [ "$clang_version" -lt 12 ]; then
#         echo "Error: 'weblibs' requires clang version 12 or higher (found $clang_version)"
#         return 1
#     fi

#     wasm_out=${BUILD_DIR}/lib.wasm
#     build_command=$(cat << EOF | tr -d '\n'
# clang -target wasm32
#  -isystemsrc/third_party/emscripten/llvm12+/include
#  -isystemsrc/third_party/emscripten/llvm12+/include/libc
#  -isystemsrc/third_party/emscripten/llvm12+/lib/libc/musl/include
#  -isystemsrc/third_party/emscripten/llvm12+/lib/libc/musl/arch/emscripten
#  -isystemsrc/third_party/emscripten/llvm12+/lib/libc/musl/arch/generic
#  -Xlinker src/third_party/emscripten/llvm12+/system.bc

#  -nostartfiles -nodefaultlibs -nostdinc -fno-threadsafe-statics
#  -DDISABLE_THREADS=1 -D__EMSCRIPTEN__ -D_LIBCPP_ABI_VERSION=2

#  -Xlinker --no-entry
#  -Xlinker --allow-undefined
#  -Xlinker --export=__wasm_call_ctors
 
#  ${CFLAGS} src/lib/lib.c
#  -o ${wasm_out}
# EOF
# )
#     echo "${build_command}"
#     eval ${build_command}

#     opt_command=$(cat << EOF | tr -d '\n'
# wasm-opt
#     --legalize-js-interface --low-memory-unused
#     -Os
#     ${wasm_out} -o ${wasm_out}
# EOF
# )
#     echo "${opt_command}"
#     eval ${opt_command}
# }

main() {
    # Parse options and filter flags
    actions=()
    for arg in "$@"; do
        case "$arg" in
            --release)   release=1;;
            --trace)     trace=1;;
            --debug)     debug=1;;
            --emcc)      emcc=1;;
            --pybind)    pybind=1;;
            --help)      print_help; exit 0;;
            *)           actions+=("$arg");;
        esac
    done
    if ! [[ -v release ]]; then debug=1; fi

    # Handle build mode
    if [[ -v release ]]; then
        CFLAGS="${CFLAGS} -s -O3 -DBUILD_DEBUG=0"
    elif [[ -v debug ]]; then
        CFLAGS="${CFLAGS} -g -O0 -DBUILD_DEBUG=1 -fno-omit-frame-pointer"
    fi
    if [[ -v trace ]]; then
        CFLAGS="${CFLAGS} -DTRACY_ENABLE src/third_party/tracy/public/TracyClient.cpp"
    fi
    if [[ -v emcc ]]; then
        CC=em++
        LDFLAGS="${LDFLAGS} -sINITIAL_MEMORY=1024mb -sALLOW_MEMORY_GROWTH -sTOTAL_STACK=512mb"
        LDFLAGS="${LDFLAGS} -sFETCH"
        LDFLAGS="${LDFLAGS} -sALLOW_TABLE_GROWTH -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','addFunction','removeFunction']"
        LDFLAGS="${LDFLAGS} -sEXPORT_ALL=1"
        LDFLAGS_GFX="-sFULL_ES3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sGL_SUPPORT_SIMPLE_ENABLE_EXTENSIONS"
        BUILD_DIR="docs/demos"
        BUILD_EXT=".html"
        CFLAGS="${CFLAGS} --shell-file docs/emcc-template.html --pre-js docs/emcc-pre.js"
    fi

    # Print build info
    print_info
    echo ""

    # Execute command
    for arg in "${actions}"; do
        case "$arg" in
            all)           build_all_demos
                           if ! [[ -v emcc ]]; then build_libs; fi;;
            libs)          build_libs;;
            balls)         build_single_file balls sphere.obj cube.obj;;
            hanging_boxes) build_single_file hanging_boxes cube.obj;;
            softbody)      build_single_file softbody bunny.vtk;;
            cloth)         build_single_file cloth cloth.vtk sphere.obj;;
            pendulum)      build_single_file pendulum cube.obj;;
            joints)        build_single_file joints cube.obj;;
            sheet)         build_single_file sheet;;
            balloon)       build_single_file balloon sphere.vtk cube.vtk;;
            "")            echo "Error: No command specified"
                           echo "Try '$SCRIPT_NAME --help' for more information."
                           exit 1;;
            *)             echo "Error: Unknown command '$arg'"
                           echo "Try '$SCRIPT_NAME --help' for more information."
                           exit 1;;
        esac
    done
}

main "$@"