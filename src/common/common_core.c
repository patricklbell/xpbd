#if COMPILER_GCC || COMPILER_CLANG
    internal u64 count_ones_u64(u64 x) {
        return (u64)__builtin_popcountll(x);
    }
    internal u64 first_set_bit_u64(u64 x) {
        Assert(x != 0);
        return (u64)__builtin_ctzll(x);
    }

#elif COMPILER_MSVC

    #include <intrin.h>
    
    internal u64 count_ones_u64(u64 x) {
        #if ARCH_X86
        return __popcnt((u32)(x)) + __popcnt((u32)(x >> 32));
        #else
        return __popcnt64(x);
        #endif
    }
    internal u64 first_set_bit_u64(u64 x) {
        Assert(x != 0);
        unsigned long index;
        _BitScanForward64(&index, x);  // MSVC intrinsic
        return (u64)index;
    }
    
#else
    #error Compiler not supported.
#endif