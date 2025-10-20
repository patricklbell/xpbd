#pragma once

typedef struct NTString8 NTString8;
struct NTString8 {
    char* cstr;
    u64 length;
};
StaticAssert(sizeof(char*) == sizeof(u8*), ntstr8_cstr_union);

internal NTString8   make_ntstr8(char* cstr, u64 length);
internal b8          ntstr8_begins_with(NTString8 str, const char* prefix);

#define ntstr8_lit(str)             make_ntstr8(str, sizeof(str) - 1)
#define ntstr8_lit_init(str)        {(char*)(str), sizeof(str) - 1}