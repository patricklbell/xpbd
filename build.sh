#!/bin/bash
set -e

# Default configuration
CC=${CC:-gcc}
CFLAGS="${CFLAGS} -I src"
LDFLAGS="${LDFLAGS} -lm"
LDFLAGS_GFX="-lX11 -lXext"

BUILD_DIR="build"
BUILD_EXT=""
DATA_DIR="data"

# Common build function for demos
build_demo() {
    mkdir -p "${BUILD_DIR}"
    
    local demo_name="$1"
    shift
    local required_data_files=("$@")
    local embed_args=""
    local main_file="src/demos/${demo_name}/main.c"

    # Handle data files
    if [ "$CC" = "emcc" ]; then
        for data_file in "${required_data_files[@]}"; do
            if [ -f "${DATA_DIR}/${data_file}" ]; then
                embed_args+=" --embed-file ${DATA_DIR}/${data_file}"
            else
                echo "Warning: Data file ${DATA_DIR}/${data_file} not found"
            fi
        done
    fi
    
    build_command="${CC} ${CFLAGS} ${main_file} ${LDFLAGS} ${LDFLAGS_GFX} ${embed_args} -o ${BUILD_DIR}/${demo_name}${BUILD_EXT}"
    echo ${build_command}
    eval ${build_command}
}

# Function to clean build directory
clean() {
    echo "cleaning ${BUILD_DIR} and docs/demos"
    rm -rf "${BUILD_DIR}"/*
    rm -rf docs/demos
}

# Function to build all demos
build_demo_wrapper() {
    declare -A dependencies=(
        [balls]="sphere.obj cube.obj"
        [hanging_boxes]="cube.obj"
        [softbody]="bunny.vtk"
        [cloth]="cloth.vtk sphere.obj"
        [pendulum]="cube.obj"
        [joints]="cube.obj"
    )

    if [[ -n "$1" && -n "${dependencies[$1]}" ]]; then
        build_demo "$1" ${dependencies[$1]}
    else
        for demo in "${!dependencies[@]}"; do
            build_demo "$demo" ${dependencies[$demo]}
        done
    fi
}

# Emscripten configuration
build_demo_emcc() {
    CC="emcc"
    LDFLAGS="${LDFLAGS} -sINITIAL_MEMORY=1024mb -sALLOW_MEMORY_GROWTH -sTOTAL_STACK=512mb"
    LDFLAGS="${LDFLAGS} -sFETCH"
    LDFLAGS="${LDFLAGS} -sALLOW_TABLE_GROWTH -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','addFunction','removeFunction']"
    LDFLAGS="${LDFLAGS} -sEXPORT_ALL=1"
    LDFLAGS_GFX="-sFULL_ES3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sGL_SUPPORT_SIMPLE_ENABLE_EXTENSIONS"
    BUILD_DIR="docs/demos"
    BUILD_EXT=".html"
    CFLAGS="${CFLAGS} --shell-file docs/emcc-template.html --pre-js docs/emcc-pre.js"
    
    build_demo_wrapper $1
}

# Lib configuration
build_libphys() {
    LIB_NAME="lib-xpbd.so"
    build_command="${CC} -fPIC -Wl,-soname,${LIB_NAME} -shared ${CFLAGS} src/libphys/libphys.c ${LDFLAGS} -o ${BUILD_DIR}/${LIB_NAME}"
    echo ${build_command}
    eval ${build_command}
}


# Filter flags
actions=()
for arg in "$@"; do
    case "$arg" in --release)   release=1;;
    *)                          actions+=("$arg");;
    esac
done

# Handle flags
if [[ -v $release ]];       then echo "[release mode]"; CFLAGS="${CFLAGS} -O3"; fi
if [[ ! -v $release ]];     then echo "[debug mode]"; CFLAGS="${CFLAGS} -g -O0"; fi

case "${actions[0]}" in
    clean)  clean;;
    lib)    build_libphys;;
    demo)   build_demo_wrapper "${actions[@]:1}";;
    emcc)   build_demo_emcc "${actions[@]:1}";;
esac