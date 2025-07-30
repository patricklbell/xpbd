#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <float.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef s8       b8;
typedef s16      b16;
typedef s32      b32;
typedef s64      b64;
typedef float    f32;
typedef double   f64;

// helpers
#define Glue_(A,B) A##B
#define Glue(A,B) Glue_(A,B)

#define ArrayLength(a) (sizeof(a) / sizeof((a)[0]))

#define AlignPow2(x,b)     (((x) + (b) - 1)&(~((b) - 1)))
#define AlignDownPow2(x,b) ((x)&(~((b) - 1)))
#define AlignPadPow2(x,b)  ((0-(x)) & ((b) - 1))

#if COMPILER_MSVC
    #define AlignOf(T) __alignof(T)
#elif COMPILER_CLANG
    #define AlignOf(T) __alignof(T)
#elif COMPILER_GCC
    #define AlignOf(T) __alignof__(T)
#else
    #error AlignOf not defined for this compiler.
#endif

#if ARCH_64BIT
    #define IntFromPtr(ptr) ((u64)(ptr))
#elif ARCH_32BIT
    #define IntFromPtr(ptr) ((u32)(ptr))
#else
    #error Missing pointer-to-integer cast for this architecture.
#endif

#define Member(T,m)                 (((T*)0)->m)
#define OffsetOf(T,m)               IntFromPtr(&Member(T,m))
#define MemberFromOffset(T,ptr,off) (T)((((u8 *)ptr)+(off)))
#define CastFromMember(T,m,ptr)     (T*)(((u8*)ptr) - OffsetOf(T,m))

#if LANG_CPP
    #define zero_struct {}
#else
    #define zero_struct {0}
#endif

// unitsM, r_ogl
#define KB(n)  (((u64)(n)) << 10)
#define MB(n)  (((u64)(n)) << 20)
#define GB(n)  (((u64)(n)) << 30)
#define TB(n)  (((u64)(n)) << 40)

#define Thousand(n)   ((n)*1000)
#define Million(n)    ((n)*1000000)
#define Billion(n)    ((n)*1000000000)

// min, max, clamp
#define Min(A,B)     (((A)<(B))?(A):(B))
#define Max(A,B)     (((A)>(B))?(A):(B))
#define Clamp(X,A,B) (((X)<(A))?(A):((X)>(B))?(B):(X))

#if COMPILER_MSVC
    #define thread_static __declspec(thread)
#elif COMPILER_CLANG
    #define thread_static __thread
#elif COMPILER_GCC
    #define thread_static __thread
#else
    #error Thread static not defined for this compiler.
#endif

#if COMPILER_MSVC
    # define force_inline __forceinline
#elif COMPILER_CLANG
    #define force_inline inline __attribute__((always_inline))
#elif COMPILER_GCC
    #define force_inline inline __attribute__((always_inline))
#else
    #define force_inline
#endif

#if COMPILER_CLANG
    #define ENUM_CASE_UNUSED [[maybe_unused]]
#else
    #define ENUM_CASE_UNUSED
#endif

// asserts
#if COMPILER_MSVC
    #define Trap() __debugbreak()
#elif COMPILER_CLANG
    #define Trap() __builtin_trap()
#elif COMPILER_GCC
    #define Trap() __builtin_trap()
#else
    #error Unknown trap intrinsic for this compiler.
#endif

#define AssertAlways(x) do{if(!(x)) {Trap();}}while(0)
#if BUILD_DEBUG
    #define Assert(x) AssertAlways(x)
#else
    #define Assert(x) (void)(x)
#endif
#define InvalidPath        Assert(!"Invalid Path!")
#define NotImplemented     Assert(!"Not Implemented!")
#define StaticAssert(C, ID) static u8 Glue(ID, __LINE__)[(C)?1:-1]

// linked list macro helpers
#define CheckNil(p) ((p) == NULL)
#define SetNil(p) ((p) = NULL)

// singly-linked, doubly-headed lists (queues)
#define sllist_push_n(f,l,n,next) (CheckNil(f)?\
    ((f)=(l)=(n),SetNil((n)->next)):\
    ((l)->next=(n),(l)=(n),SetNil((n)->next)))
#define sllist_push_front_n(f,l,n,next) (CheckNil(f)?\
    ((f)=(l)=(n),SetNil((n)->next)):\
    ((n)->next=(f),(f)=(n)))
#define sllist_pop_n(f,l,next) ((f)==(l)?\
    (SetNil(f),SetNil(l)):\
    ((f)=(f)->next))

#define dllist_insert_np(f,l,p,n,next,prev) (CheckNil(f) ? \
    ((f) = (l) = (n), SetNil((n)->next), SetNil((n)->prev)) :\
    CheckNil(p) ? \
    ((n)->next = (f), (f)->prev = (n), (f) = (n), SetNil((n)->prev)) :\
    ((p)==(l)) ? \
    ((l)->next = (n), (n)->prev = (l), (l) = (n), SetNil( (n)->next)) :\
    (((!CheckNil(p) && CheckNil((p)->next)) ? (0) : ((p)->next->prev = (n))), ((n)->next = (p)->next), ((p)->next = (n)), ((n)->prev = (p))))
#define dllist_push_back_np(f,l,n,next,prev) dllist_insert_np(f,l,l,n,next,prev)
#define dllist_push_front_np(f,l,n,next,prev) dllist_insert_np(l,f,f,n,prev,next)
#define dllist_remove_np(f,l,n,next,prev) (((n) == (f) ? (f) = (n)->next : (0)),\
    ((n) == (l) ? (l) = (l)->prev : (0)),\
    (CheckNil((n)->prev) ? (0) :\
    ((n)->prev->next = (n)->next)),\
    (CheckNil((n)->next) ? (0) :\
    ((n)->next->prev = (n)->prev)))

#define stack_push_n(f,n,next) ((n)->next=(f), (f)=(n))
#define stack_pop_n(f,next) ((f)=(f)->next)

// doubly-linked-list helpers
#define dllist_insert(f,l,p,n) dllist_insert_np(f,l,p,n,next,prev)
#define dllist_push_back(f,l,n) dllist_push_back_np(f,l,n,next,prev)
#define dllist_push_front(f,l,n) dllist_push_front_np(f,l,n,next,prev)
#define dllist_remove(f,l,n) dllist_remove_np(f,l,n,next,prev)

// singly-linked, doubly-headed list helpers
#define sllist_push(f,l,n) sllist_push_n(f,l,n,next)
#define sllist_push_front(f,l,n) sllist_push_front_n(f,l,n,next)
#define sllist_pop(f,l) sllist_pop_n(f,l,next)

// singly-linked, singly-headed lists (stacks)
#define stack_push(f,n) stack_push_n(f,n,next)
#define stack_pop(f) stack_pop_n(f,next)

// loop helpers
#define EachIndex(var, length)          (int var = 0; var < length; var++)
#define EachElement(var, arr)           (int var = 0; var < ArrayLength(arr); var++)
#define EachList_N(node, type, f, n)    (type* node = f; node != NULL; node = node->n)
#define EachList(node, type, f)         EachList_N(node, type, f, next)

#define DeferCall(begin, end)           for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))
#define DeferResource(begin, end)       begin; for(int _i_ = 0; !_i_; _i_ += 1, (end))

// bit bashing
int count_ones_u64(u64 x);
int first_set_bit_u64(u64 x);