#!/bin/bash

# Default configuration
CC=${CC:-gcc}
CFLAGS="${CFLAGS} -g -I src"
LDFLAGS="${LDFLAGS} -lm"
LDFLAGS_GFX="-lX11 -lXext"

BUILD_DIR="build"
BUILD_EXT=""
DATA_DIR="data"

# Common build function for demos
build_demo() {
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
    else
        for data_file in "${required_data_files[@]}"; do
            copy_data_file "$data_file"
        done
    fi
    
    echo "Building ${demo_name}"
    ${CC} ${CFLAGS} ${main_file} ${LDFLAGS} ${LDFLAGS_GFX} ${embed_args} -o ${BUILD_DIR}/${demo_name}${BUILD_EXT}
}

# Function to build data files
copy_data_file() {
    local file="$1"
    mkdir -p "${BUILD_DIR}/${DATA_DIR}"
    if [ -f "${DATA_DIR}/${file}" ]; then
        cp -f "${DATA_DIR}/${file}" "${BUILD_DIR}/${DATA_DIR}/${file}"
    else
        echo "Warning: Data file ${DATA_DIR}/${file} not found"
    fi
}

# Function to clean build directory
clean() {
    rm -rf "${BUILD_DIR}"/*
    rm -rf docs/demos/*
}

# Function to build all demos
build_demos() {
    build_demo "balls" "sphere.obj"
    build_demo "hanging_boxes" "cube.obj"
}

# Emscripten configuration
build_emcc() {
    CC="emcc"
    LDFLAGS="${LDFLAGS}"
    LDFLAGS_GFX="-sFETCH -sEXPORTED_FUNCTIONS=['_main','_os_gfx_wasm_resize_callback'] -sALLOW_BLOCKING_ON_MAIN_THREAD=1 -sFULL_ES3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sGL_SUPPORT_SIMPLE_ENABLE_EXTENSIONS"
    BUILD_DIR="docs/demos"
    BUILD_EXT=".html"
    CFLAGS="${CFLAGS} --shell-file docs/emcc-template.html --pre-js docs/emcc-pre.js -pthread -sINITIAL_MEMORY=1024mb -sALLOW_MEMORY_GROWTH=1 -sTOTAL_STACK=512mb"
    
    build_demos
}

# Main command handling
case "$1" in
    clean)
        clean
        ;;
    emcc)
        build_emcc
        ;;
    *)
        build_demos
        ;;
esac