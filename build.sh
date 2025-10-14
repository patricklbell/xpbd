#!/bin/bash
set -e

CC=${CC:-g++}
CFLAGS="${CFLAGS} -I src"
LDFLAGS="${LDFLAGS} -lm"
LDFLAGS_GFX="-lX11 -lXext"

BUILD_DIR="build"
BUILD_EXT=""
DATA_DIR="data"

VERSION="0.1.0"
SCRIPT_NAME="build.sh"

print_help() {
    cat << EOF
Usage: $SCRIPT_NAME [OPTION]... [COMMAND]...

Commands:
  demo [NAME]      Build demo(s). If NAME is specified, build only that demo,
                   otherwise build all demos
  emcc [NAME]      Build demo(s) for web using Emscripten
  lib              Build as a shared library
  clean            Remove build artifacts

Options:
  --release        Build in release mode (optimized)
  --help           Display this help and exit
  --version        Output version information and exit

Available demos:
  balls, hanging_boxes, softbody, cloth, pendulum, joints, balloon, sheet

Environment variables:
  CC              C compiler to use (default: gcc)
  CFLAGS          Additional compiler flags
  LDFLAGS         Additional linker flags

Examples:
  $SCRIPT_NAME demo              # Build all demos in debug mode
  $SCRIPT_NAME --release demo    # Build all demos in release mode
  $SCRIPT_NAME demo balls        # Build only the balls demo
  $SCRIPT_NAME emcc cloth        # Build cloth demo for web
  $SCRIPT_NAME lib               # Build shared library
  $SCRIPT_NAME clean             # Clean build artifacts
EOF
}

print_version() {
    echo "$SCRIPT_NAME $VERSION"
}

print_info() {
    echo "- Build directory: $BUILD_DIR"
    echo "- Data directory:  $DATA_DIR"
    echo "- Compiler:        $CC"
    if [[ -n "$release" ]]; then
    echo "- Mode:            Release"
    else
    echo "- Mode:            Debug"
    fi
    if [[ -v trace ]]; then
    echo "- Tracy:           Enabled"
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
    if [ "$CC" = "emcc" ]; then
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

build_demo() {
    declare -A dependencies=(
        [balls]="sphere.obj cube.obj"
        [hanging_boxes]="cube.obj"
        [softbody]="bunny.vtk"
        [cloth]="cloth.vtk sphere.obj"
        [pendulum]="cube.obj"
        [joints]="cube.obj"
        [sheet]=" "
        [balloon]="sphere.vtk cube.vtk"
    )

    if [[ -n "$1" && -n "${dependencies[$1]}" ]]; then
        build_single_file "$1" ${dependencies[$1]}
    else
        for demo in "${!dependencies[@]}"; do
            build_single_file "$demo" ${dependencies[$demo]}
        done
    fi
}

build_emcc() {
    CC="emcc"
    LDFLAGS="${LDFLAGS} -sINITIAL_MEMORY=1024mb -sALLOW_MEMORY_GROWTH -sTOTAL_STACK=512mb"
    LDFLAGS="${LDFLAGS} -sFETCH"
    LDFLAGS="${LDFLAGS} -sALLOW_TABLE_GROWTH -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','addFunction','removeFunction']"
    LDFLAGS="${LDFLAGS} -sEXPORT_ALL=1"
    LDFLAGS_GFX="-sFULL_ES3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sGL_SUPPORT_SIMPLE_ENABLE_EXTENSIONS"
    BUILD_DIR="docs/demos"
    BUILD_EXT=".html"
    CFLAGS="${CFLAGS} --shell-file docs/emcc-template.html --pre-js docs/emcc-pre.js"
    build_demo $1
}

build_lib() {
    LIB_NAME="lib.so"
    build_command="${CC} -fPIC -Wl,-soname,${LIB_NAME} -shared ${CFLAGS} src/lib/lib.c ${LDFLAGS} -o ${BUILD_DIR}/${LIB_NAME}"
    echo "${build_command}"
    eval ${build_command}
}

main() {
    # Parse options and filter flags
    actions=()
    for arg in "$@"; do
        case "$arg" in
            --release)   release=1;;
            --trace)     trace=1;;
            --debug)     debug=1;;
            --help)      print_help; exit 0;;
            --version)   print_version; exit 0;;
            *)           actions+=("$arg");;
        esac
    done

    # Handle build mode
    if [[ -v release ]]; then
        CFLAGS="${CFLAGS} -s -O3"
    elif [[ -v debug ]]; then
        CFLAGS="${CFLAGS} -g -O0"
    else
        CFLAGS="${CFLAGS} -g -O3"
    fi
    if [[ -v trace ]]; then
        CFLAGS="${CFLAGS} -fno-omit-frame-pointer -rdynamic -DTRACY_ENABLE src/third_party/tracy/public/TracyClient.cpp"
    fi

    # Print build info
    print_info
    echo ""

    # Execute command
    case "${actions[0]}" in
        lib)    build_lib;;
        demo)   build_demo "${actions[@]:1}";;
        emcc)   build_emcc "${actions[@]:1}";;
        "")     echo "Error: No command specified"
                echo "Try '$SCRIPT_NAME --help' for more information."
                exit 1;;
        *)      echo "Error: Unknown command '${actions[0]}'"
                echo "Try '$SCRIPT_NAME --help' for more information."
                exit 1;;
    esac
}

main "$@"