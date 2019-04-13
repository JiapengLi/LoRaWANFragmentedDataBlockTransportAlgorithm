#ifndef __FRAGMENTATION_H
#define __FRAGMENTATION_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "bitmap.h"

/*
https://github.com/brocaar/lorawan/blob/master/applayer/fragmentation/encode.go
https://github.com/brocaar/lorawan/blob/master/applayer/fragmentation/encode_test.go

*/

#define FRAG_DEC_ONGOING                    (-1)
#define FRAG_DEC_ERR_INVALID_FRAME          (-2)
#define FRAG_DEC_ERR_TOO_MANY_FRAME_LOST    (-3)
#define FRAG_DEC_ERR_1                      (-4)
#define FRAG_DEC_ERR_2                      (-5)

typedef struct {
    uint8_t *dt;
    uint32_t maxlen;

    uint32_t unit;
    uint32_t num;
    uint32_t cr;
    uint8_t *line;
    uint8_t *mline;
    uint8_t *rline;
} frag_enc_t;

typedef int (*flash_rd_t)(uint32_t addr, uint8_t *buf, uint32_t len);
typedef int (*flash_wr_t)(uint32_t addr, uint8_t *buf, uint32_t len);

typedef struct {
    uint8_t *dt;
    uint32_t maxlen;
    uint16_t nb;
    uint8_t size;
    uint16_t tolerence;
    flash_rd_t frd_func;
    flash_wr_t fwr_func;
} frag_dec_cfg_t;

typedef enum {
    FRAG_DEC_STA_UNCODED,       // wait uncoded fragmentations
    FRAG_DEC_STA_CODED,         // wait coded fragmentations, uncoded frags are processed as coded ones
    FRAG_DEC_STA_DONE,
} frag_dec_sta_t;

/* bm is short for bitmap */
typedef struct {
    frag_dec_cfg_t cfg;

    frag_dec_sta_t sta;

    bm_t *lost_frm_bm;
    uint16_t lost_frm_count;
    bm_t *lost_frm_matrix_bm;
    uint16_t filled_lost_frm_count;

    /* temporary buffer */
    bm_t *matched_lost_frm_bm0;
    bm_t *matched_lost_frm_bm1;
    bm_t *matrix_line_bm;
    uint8_t *row_data_buf;
    uint8_t *xor_row_data_buf;
} frag_dec_t;

int frag_enc(frag_enc_t *obj, uint8_t *buf, int len, int unit, int cr);

int frag_dec_init(frag_dec_t *obj);
int frag_dec(frag_dec_t *obj, uint16_t fcnt, uint8_t *buf, int len);

void frag_dec_log_bits(bm_t *bitmap, int len);
void frag_dec_log_buf(uint8_t *buf, int len);
void frag_dec_log(frag_dec_t *obj);


#endif // __FRAGMENTATION_H
