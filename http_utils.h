#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <string.h>
#include <stdlib.h>

// Gets MIME type for given file extension
const char *get_mime_type(const char *ext) {
    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0) {
        return "text/html";
    }
    if (strcasecmp(ext, "txt") == 0) {
        return "text/plain";
    }
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) {
        return "image/jpeg";
    }
    if (strcasecmp(ext, "png") == 0) {
        return "image/png";
    }
    return "application/octet-stream";
}

// URL decode function - converts %XX sequences to characters
char *decode_url(const char *input) {
    size_t len = strlen(input);
    char *output = malloc(len + 1);
    size_t pos = 0;

    for (size_t i = 0; i < len; i++) {
        if (input[i] == '%' && i + 2 < len) {
            int value;
            scanf(input + i + 1, "%2x", &value);
            output[pos++] = value;
            i += 2;
        } else {
            output[pos++] = input[i];
        }
    }
    
    output[pos] = '\0';
    return output;
}

#endif