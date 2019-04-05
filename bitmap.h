#ifndef __BITMAP_H
#define __BITMAP_H

//#define BM_1
//#define BM_2
#define BM_4
#define BUILTIN_FUNC

/*
count set bits:
https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
https://www.geeksforgeeks.org/count-set-bits-in-an-integer/

If you want the fastest way, you will need to use non-portable methods.

Windows/MSVC:

_BitScanForward()
_BitScanReverse()
__popcnt()
GCC:

__builtin_ffs()
__builtin_ctz()
__builtin_clz()
__builtin_popcount()
These typically map directly to native hardware instructions. So it doesn't get much faster than these.

But since there's no C/C++ functionality for them, they're only accessible via compiler intrinsics.

*/

#if defined BM_4
typedef uint32_t bm_t;
#define BM_UNIT         (sizeof(bm_t) * 8)
#define BM_OFST         (5) // 8: 3, 16: 4, 32: 5
#elif defined BM_2
typedef uint16_t bm_t;
#define BM_UNIT         (sizeof(bm_t) * 8)
#define BM_OFST         (4) // 8: 3, 16: 4, 32: 5
#elif defined BM_1
typedef uint8_t bm_t;
#define BM_UNIT         (sizeof(bm_t) * 8)
#define BM_OFST         (3) // 8: 3, 16: 4, 32: 5
#endif

bool bit_get(bm_t *bitmap, int index);

void bit_set(bm_t *bitmap, int index);

void bit_clr(bm_t *bitmap, int index);

int bit_count_ones(bm_t *bitmap, int index);

int bit_ffs(bm_t *bitmap, int size);

/* find the nth set */
int bit_fns(bm_t *bitmap, int size, int n);

void bit_xor(bm_t *des, bm_t *src, int size);

bool bit_is_all_clear(bm_t *bitmap, int size);

void bit_clear_all(bm_t *bitmap, int size);

#endif // __BITMAP_H
