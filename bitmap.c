#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "bitmap.h"

static int count_bits(uint32_t num);
static int find_first_set(uint32_t num);

bool bit_get(bm_t *bitmap, int index)
{
    return ((bitmap[index >> BM_OFST] & (1 << (index % BM_UNIT))) != 0);
}

void bit_set(bm_t *bitmap, int index)
{
    bitmap[index >> BM_OFST] |= ((bm_t)1 << (index % BM_UNIT));
}

void bit_clr(bm_t *bitmap, int index)
{
    bitmap[index >> BM_OFST] &= ~(1 << (index % BM_UNIT));
}

int bit_count_ones(bm_t *bitmap, int index)
{
    int i, cnt;
    bm_t val;

    cnt = 0;
    for (i = 0; i < (index >> BM_OFST); i++) {
        cnt += count_bits(bitmap[i]);
    }
    val = bitmap[index >> BM_OFST] & ((bm_t)(-1) >> (BM_UNIT - 1 - index % BM_UNIT));
    cnt += count_bits(val);
    return cnt;
}

int bit_ffs(bm_t *bitmap, int size)
{
#if 0
    int i, j;
    for (i = 0; i < ((size + BM_UNIT - 1) >> BM_OFST); i++) {
        for (j = 0; j < BM_UNIT; j++) {
            if (bitmap[i] & (1 << j)) {
                return ((i << BM_OFST) + j);
            }
        }
    }
    return -1;
#else
    int i;
    int len;

    len = ((size + BM_UNIT - 1) >> BM_OFST);
    for (i = 0; i < len; i++) {
        if (bitmap[i] != 0) {
            return (i << BM_OFST) + find_first_set(bitmap[i]);
        }
    }
    return -1;
#endif
}

/* find the nth set */
int bit_fns(bm_t *bitmap, int size, int n)
{
#if 0
    int i, j, cnt;
    cnt = 0;
    for (i = 0; i < ((size + BM_UNIT - 1) >> BM_OFST); i++) {
        for (j = 0; j < BM_UNIT; j++) {
            if (bitmap[i] & (1 << j)) {
                cnt++;
                if (cnt == n) {
                    return ((i << BM_OFST) + j);
                }
            }
        }
    }
    return -1;
#else
    int i, j, cnt;
    cnt = 0;
    for (i = 0; i < ((size + BM_UNIT - 1) >> BM_OFST); i++) {
        cnt += count_bits(bitmap[i]);
        if (cnt >= n) {
            for (j = BM_UNIT - 1; j >= 0; j--) {
                if (bitmap[i] & (1 << j)) {
                    if (cnt == n) {
                        return ((i << BM_OFST) + j);
                    }
                    cnt--;
                }
            }
        }
    }
    return -1;
#endif
}

void bit_xor(bm_t *des, bm_t *src, int size)
{
    int i;
    int len;

    len = ((size + BM_UNIT - 1) >> BM_OFST);
    for (i = 0; i < len; i++) {
        des[i] ^= src[i];
    }
}

bool bit_is_all_clear(bm_t *bitmap, int size)
{
    int i;
    int len;

    len = ((size + BM_UNIT - 1) >> BM_OFST);
    for (i = 0; i < len; i++) {
        if (bitmap[i]) {
            return false;
        }
    }
    return true;
}

void bit_clear_all(bm_t *bitmap, int size)
{
    int i;
    int len;

    len = ((size + BM_UNIT - 1) >> BM_OFST);
    for (i = 0; i < len; i++) {
        bitmap[i] = 0;
    }
}

int m2t_map(int x, int y, int m)
{
    if (x < y) {
        return -1;
    }
    /* doesn't check to speed up the process */
//    if ((x >= m) || (y >= m)) {
//        return -1;
//    }
    return (y + 1) * (m + m - y) / 2 - (m - x);
}

bool m2t_get(bm_t *m2tbm, int x, int y, int m)
{
    if (x < y) {
        return false;
    }
    return bit_get(m2tbm, (y + 1) * (m + m - y) / 2 - (m - x));
}

void m2t_set(bm_t *m2tbm, int x, int y, int m)
{
    if (x < y) {
        return;
    }
    bit_set(m2tbm, (y + 1) * (m + m - y) / 2 - (m - x));
}

void m2t_clr(bm_t *m2tbm, int x, int y, int m)
{
    if (x < y) {
        return;
    }
    bit_clr(m2tbm, (y + 1) * (m + m - y) / 2 - (m - x));
}

static const uint8_t num_to_bits[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
static int count_bits(uint32_t num)
{
#ifdef BUILTIN_FUNC
    return __builtin_popcount(num);
#else
    if (0 == num) {
        return num_to_bits[0];
    }
    return num_to_bits[num & 0xf] + count_bits(num >> 4);
#endif
}

int __rt_ffs(int value);
static int find_first_set(uint32_t num)
{
#ifdef BUILTIN_FUNC
    return __builtin_ffs(num) - 1;
#else
    return __rt_ffs(num) - 1;
#endif
}

const uint8_t __lowest_bit_bitmap[] =
{
    /* 00 */ 0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 10 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 20 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 30 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 40 */ 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 50 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 60 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 70 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 80 */ 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* 90 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* A0 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* B0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* C0 */ 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* D0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* E0 */ 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    /* F0 */ 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

/**
 * This function finds the first bit set (beginning with the least significant bit)
 * in value and return the index of that bit.
 *
 * Bits are numbered starting at 1 (the least significant bit).  A return value of
 * zero from any of these functions means that the argument was zero.
 *
 * @return return the index of the first bit set. If value is 0, then this function
 * shall return 0.
 */
int __rt_ffs(int value)
{
//    if (value == 0) return 0;
    if (value & 0xff)
        return __lowest_bit_bitmap[value & 0xff] + 1;

    if (value & 0xff00)
        return __lowest_bit_bitmap[(value & 0xff00) >> 8] + 9;

    if (value & 0xff0000)
        return __lowest_bit_bitmap[(value & 0xff0000) >> 16] + 17;

    return __lowest_bit_bitmap[(value & 0xff000000) >> 24] + 25;
}

