#if OS_WEB
    #include "wasm/emcontrols_wasm.c"
#else
    #include "stub/emcontrols_stub.c"
#endif