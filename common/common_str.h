#pragma once

typedef struct NTString8 NTString8;
struct NTString8 {
    const char* data;
    u64 length;
};

b8 str_begins_with(NTString8 str, const char* prefix);

#define str_8(str) ((NTString8){ .data = str, .length = strlen(str) })