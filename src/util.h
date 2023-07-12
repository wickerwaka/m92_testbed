#if !defined(UTIL_H)
#define UTIL_H 1

#include <stdint.h>

static inline void memset(void *ptr, int c, size_t len)
{
    uint8_t *p = (uint8_t *)ptr;
    while( len )
    {
        *p = c;
        p++;
        len--;
    }
}

#endif