b8 str_begins_with(NTString8 str, const char* prefix) {
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