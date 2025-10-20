internal NTString8 make_ntstr8(char* data, u64 length) {
    return (NTString8) {
        .cstr = data,
        .length = length,
    };
}

internal b8 ntstr8_begins_with(NTString8 str, const char* prefix) {
    if (strlen(prefix) > str.length) {
        return false;
    }

    for EachIndex(i, strlen(prefix)) {
        if (str.cstr[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}