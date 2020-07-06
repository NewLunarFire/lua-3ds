#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <stdarg.h>

typedef struct Buffer {
    char* bytes;
    int size;
    int cur;
} Buffer;

static inline void buffer_print(Buffer* buf, const char* format, ...) {
    va_list ap;
    va_start(ap,format);
    
    buf->cur += vsnprintf(&(buf->bytes[buf->cur]), buf->size - buf->cur, format, ap);

    va_end(ap);
}

static inline void buffer_putc(Buffer* buf, char c) {
    if(buf->cur < buf->size) {
        buf->bytes[buf->cur] = c;
        buf->cur++;
    }
}

static inline void buffer_nputs(Buffer* buf, const char* str, int len) {
    int _len = buf->size - buf->cur;
    if(len < _len) {
        _len = len;
    }

    memcpy(&(buf->bytes[buf->cur]), str, _len);
    buf->cur += _len;
}

#endif