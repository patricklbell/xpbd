#if COMPILER_GCC || COMPILER_CLANG
    int count_ones_u64(u64 x) {
        return __builtin_popcountll(x);
    }
#elif COMPILER_MSVC
    #include <intrin.h>
    int count_ones_u64(u64 x) {
        return __popcnt64(x);
    }
#else
    #error Compiler not supported.
#endif

#if COMPILER_GCC || COMPILER_CLANG
int first_set_bit_u64(u64 x) {
    Assert(x != 0);
    return __builtin_ctzll(x);
}
#elif COMPILER_MSVC
int first_set_bit_u64(u64 x) {
    Assert(x != 0);
    unsigned long index;
    _BitScanForward64(&index, x);  // MSVC intrinsic
    return (int)index;
}
#endif