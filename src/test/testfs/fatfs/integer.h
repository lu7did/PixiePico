#ifndef _INTEGER
#define _INTEGER

#include <stdint.h>

typedef int8_t    INT8;
typedef uint8_t   UINT8;
typedef int16_t   INT16;
typedef uint16_t  UINT16;
typedef int32_t   INT32;
typedef uint32_t  UINT32;

typedef UINT32    DWORD;
typedef UINT16    WORD;
typedef UINT8     BYTE;
typedef unsigned int UINT;   // <-- IMPORTANT: match ff.h expectation (16 or 32 bits)

#endif
