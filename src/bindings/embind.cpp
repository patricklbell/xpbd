#include <emscripten/bind.h>
#include "lib/lib.h"

#undef internal
#undef global
EMSCRIPTEN_BINDINGS(LIBRARY_NAME) {
    emscripten::function("phys_collider_layers_overlap", &phys_collider_layers_overlap);
}