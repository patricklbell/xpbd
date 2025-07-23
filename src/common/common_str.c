NTString8 make_ntstr8(u8* data, u64 length) {
    return (NTString8) {
        .data = data,
        .length = length,
    };
}

b8 ntstr8_begins_with(NTString8 str, const char* prefix) {
    if (strlen(prefix) > str.length) {
        return 0;
    }

    for EachIndex(i, strlen(prefix)) {
        if (str.data[i] != prefix[i]) {
            return 0;
        }
    }
    return 1;
}