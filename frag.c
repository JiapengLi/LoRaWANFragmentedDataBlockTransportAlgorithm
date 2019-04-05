#include "frag.h"

//#define FRAGDBG(x...)       printf(x)
#define FRAGDBG(x...)
#define FRAGLOG(x...)       printf(x)
//#define DEBUG

static bool is_power2(uint32_t num)
{
	return (num != 0) && ((num & (num-1)) == 0);
}

static uint32_t prbs23(uint32_t x)
{
    uint32_t b0, b1;
    b0 = (x & 0x00000001);
    b1 = (x & 0x00000020) >> 5;
    return (x >> 1) + ((b0 ^ b1) << 22);
}

/*
n: FragIndex
m: NubFrag
buf: length is greater than m
*/
static int matrix_line(uint8_t *buf, int n, int m)
{
    int mm, x, nbCoeff, r;

    mm = 0;
    if (is_power2(m)) {
        mm = 1;
    }
    mm += m;

    x = 1 + (1001 * n);

    for (nbCoeff = 0; nbCoeff < (m/2); nbCoeff++) {
        r = (1 << 16);
        while (r >= m) {
            x = prbs23(x);
            r = x % mm;
        }
        buf[r] = 1;
    }

    return 0;
}

static int matrix_line_bm(bm_t *bm, int n, int m)
{
    int mm, x, nbCoeff, r;

    bit_clear_all(bm, m);

    mm = 0;
    if (is_power2(m)) {
        mm = 1;
    }
    mm += m;

    x = 1 + (1001 * n);

    for (nbCoeff = 0; nbCoeff < (m/2); nbCoeff++) {
        r = (1 << 16);
        while (r >= m) {
            x = prbs23(x);
            r = x % mm;
        }
        bit_set(bm, r);
    }

    return 0;
}

static int buf_xor(uint8_t *des, uint8_t *src, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        des[i] ^= src[i];
    }
    return 0;
}

/*
unit: m
*/
int frag_enc(frag_enc_t *obj, uint8_t *buf, int len, int unit, int cr)
{
    int i, j, k;
    int num, maxlen;
    uint8_t *mline;
    uint8_t *rline;

    if ((len % unit) != 0) {
        return -1;
    }

    num = len / unit;
    maxlen = len + cr * unit + num * cr;
    if (maxlen > obj->maxlen) {
        FRAGLOG("maxlen: %d, input buffer: %d", maxlen, obj->maxlen);
        return -2;
    }

    obj->unit = unit;
    obj->num = num;
    obj->cr = cr;
    obj->line = obj->dt;
    obj->rline = obj->dt + len;
    obj->mline = obj->dt + len + cr * unit;

    memcpy(obj->dt + 0, buf, len);
    mline = obj->mline;
    rline = obj->rline;
    for (i = 0; i < cr; i++, mline += num, rline += unit) {
        matrix_line(mline, i + 1, num);
        for (j = 0; j < num; j++) {
            if (mline[j] == 1) {
                for (k = 0; k < unit; k++) {
                    rline[k] ^= buf[j * unit + k];
                }
            }
        }
    }
    return 0;
}

#define ALIGN4(x)           (x) = (((x) + 0x03) & ~0x03)
int frag_dec_init(frag_dec_t *obj)
{
    int i, j;

    i = 0;

    /* TODO: check if obj->cfg.dt is aligned */
    memset(obj->cfg.dt, 0, obj->cfg.maxlen);

    ALIGN4(i);
    obj->lost_frm_bm = (bm_t *)(obj->cfg.dt + i);
    i += (obj->cfg.nb + BM_UNIT - 1) / BM_UNIT * sizeof(bm_t);

    ALIGN4(i);
    obj->lost_frm_matrix_bm = (bm_t *)(obj->cfg.dt + i);
    i += (obj->cfg.tolerence * obj->cfg.tolerence + BM_UNIT - 1) / BM_UNIT * sizeof(bm_t);

    ALIGN4(i);
    obj->matched_lost_frm_bm0 = (bm_t *)(obj->cfg.dt + i);
    i += (obj->cfg.tolerence + BM_UNIT - 1) / BM_UNIT * sizeof(bm_t);

    ALIGN4(i);
    obj->matched_lost_frm_bm1 = (bm_t *)(obj->cfg.dt + i);
    i += (obj->cfg.tolerence + BM_UNIT - 1) / BM_UNIT * sizeof(bm_t);

    ALIGN4(i);
    obj->matrix_line_bm = (bm_t *)(obj->cfg.dt + i);
    i += (obj->cfg.nb + BM_UNIT - 1) / BM_UNIT * sizeof(bm_t);

    ALIGN4(i);
    obj->row_data_buf = obj->cfg.dt + i;
    i += obj->cfg.size;

    ALIGN4(i);
    obj->xor_row_data_buf = obj->cfg.dt + i;
    i += obj->cfg.size;

    ALIGN4(i);
    if (i > obj->cfg.maxlen) {
        return -1;
    }

    /* set all frame lost, from 0 to nb-1 */
    obj->lost_frm_count = obj->cfg.nb;
    for (j = 0; j < obj->cfg.nb; j++) {
        bit_set(obj->lost_frm_bm, j);
    }

    obj->filled_lost_frm_count = 0;
    obj->finished = false;

    return i;
}

void frag_dec_frame_received(frag_dec_t *obj, uint16_t index)
{
    if (bit_get(obj->lost_frm_bm, index)) {
        bit_clr(obj->lost_frm_bm, index);
        obj->lost_frm_count--;
        /* TODO: check and update other maps */
    }
}

void frag_dec_flash_wr(frag_dec_t *obj, uint16_t index, uint8_t *buf)
{
    obj->cfg.fwr_func(index * obj->cfg.size, buf, obj->cfg.size);
    #ifdef DEBUG
    FRAGDBG("-> index %d, ", index);
    frag_dec_log_buf(buf, obj->cfg.size);
    #endif
}

void frag_dec_flash_rd(frag_dec_t *obj, uint16_t index, uint8_t *buf)
{
    obj->cfg.frd_func(index * obj->cfg.size, buf, obj->cfg.size);
    #ifdef DEBUG
    FRAGDBG("<- index %d, ", index);
    frag_dec_log_buf(buf, obj->cfg.size);
    #endif
}

void frag_dec_lost_frm_matrix_save(frag_dec_t *obj, uint16_t lindex, bm_t *map, int len)
{
    int i, offset;

    offset = lindex * len;
    for (i = 0; i < len; i++) {
        if (bit_get(map, i)) {
            bit_set(obj->lost_frm_matrix_bm, offset + i);
        } else {
            bit_clr(obj->lost_frm_matrix_bm, offset + i);
        }
    }
}

void frag_dec_lost_frm_matrix_load(frag_dec_t *obj, uint16_t lindex, bm_t *map, int len)
{
    int i, offset;
    offset = lindex * len;
    for (i = 0; i < len; i++) {
        if (bit_get(obj->lost_frm_matrix_bm, offset + i)) {
            bit_set(map, i);
        } else {
            bit_clr(map, i);
        }
    }
}

bool frag_dec_lost_frm_matrix_is_diagonal(frag_dec_t *obj, uint16_t lindex, int len)
{
    return bit_get(obj->lost_frm_matrix_bm, lindex * len + lindex);
}

/* fcnt from 1 to nb */
int frag_dec(frag_dec_t *obj, uint16_t fcnt, uint8_t *buf, int len)
{
    int i, j;
    int index, unmatched_frame_cnt;
    int lost_frame_index, frame_index, frame_index1;
    bool no_info;

    if (obj->finished) {
        return obj->lost_frm_count;
    }

    if (len != obj->cfg.size) {
        return FRAG_DEC_ERR_INVALID_FRAME;
    }

    /* clear all temporary bm and buf */
    bit_clear_all(obj->matched_lost_frm_bm0, obj->lost_frm_count);
    bit_clear_all(obj->matched_lost_frm_bm1, obj->lost_frm_count);

    /* back up input data so that not to mess input data */
    memcpy(obj->xor_row_data_buf, buf, obj->cfg.size);

    index = fcnt - 1;
    if (index < obj->cfg.nb) {
        /* uncoded frames */
        /* mark new received frame */
        frag_dec_frame_received(obj, index);
        /* save data to flash */
        frag_dec_flash_wr(obj, index, buf);
        /* if no frame lost finish decode process */
        if (obj->lost_frm_count == 0) {
            obj->finished = true;
            return obj->lost_frm_count;
        }
    } else {
        /* coded frames start processing, lost_frm_count is now frozen and should be not changed (!!!) */
        /* too many packets are lost, it is not possible to reconstruct data block */
        if (obj->lost_frm_count > obj->cfg.tolerence) {
            return FRAG_DEC_ERR_TOO_MANY_FRAME_LOST;
        }
        unmatched_frame_cnt = 0;
        matrix_line_bm(obj->matrix_line_bm, index - obj->cfg.nb + 1, obj->cfg.nb);
        for (i = 0; i < obj->cfg.nb; i++) {
            if (bit_get(obj->matrix_line_bm, i) == true) {
                if (bit_get(obj->lost_frm_bm, i) == false) {
                    /* coded frame is matched one received uncoded frame */
                    frag_dec_flash_rd(obj, i, obj->row_data_buf);
                    buf_xor(obj->xor_row_data_buf, obj->row_data_buf, obj->cfg.size);
                } else {
                    /* coded frame is not matched one received uncoded frame */
                    /* matched_lost_frm_bm0 index is the nth lost frame */
                    bit_set(obj->matched_lost_frm_bm0, bit_count_ones(obj->lost_frm_bm, i) - 1);
                    unmatched_frame_cnt++;
                }
            }
        }
        if (unmatched_frame_cnt <= 0) {
            return FRAG_DEC_ONGOING;
        }

#ifdef DEBUG
        FRAGDBG("matrix_line_bm: %d, ", index);
        frag_dec_log_bits(obj->matrix_line_bm, obj->cfg.nb);
        FRAGDBG("matched_lost_frm_bm0: ");
        frag_dec_log_bits(obj->matched_lost_frm_bm0, obj->lost_frm_count);
#endif
        /* obj->matched_lost_frm_bm0 now saves new coded frame which excludes all received frames content */
        /* start to diagonal obj->matched_lost_frm_bm0 */
        no_info = false;
        do {
            lost_frame_index = bit_ffs(obj->matched_lost_frm_bm0, obj->lost_frm_count);
            frame_index = bit_fns(obj->lost_frm_bm, obj->cfg.nb, lost_frame_index + 1);
            if (frame_index == -1) {
                FRAGLOG("matched_lost_frm_bm0: ");
                frag_dec_log_bits(obj->matched_lost_frm_bm0, obj->lost_frm_count);
                FRAGLOG("lost_frm_bm: ");
                frag_dec_log_bits(obj->lost_frm_bm, obj->cfg.nb);
                FRAGLOG("frame_index: %d, lost_frame_index: %d\n", frame_index, lost_frame_index);
            }
#ifdef DEBUG
            FRAGDBG("matched_lost_frm_bm0: ");
            frag_dec_log_bits(obj->matched_lost_frm_bm0, obj->lost_frm_count);
            FRAGDBG("lost_frm_bm: ");
            frag_dec_log_bits(obj->lost_frm_bm, obj->cfg.nb);
            FRAGDBG("frame_index: %d, lost_frame_index: %d\n", frame_index, lost_frame_index);
#endif
            if (frag_dec_lost_frm_matrix_is_diagonal(obj, lost_frame_index, obj->lost_frm_count) == false) {
                break;
            }

            frag_dec_lost_frm_matrix_load(obj, lost_frame_index, obj->matched_lost_frm_bm1, obj->lost_frm_count);
            bit_xor(obj->matched_lost_frm_bm0, obj->matched_lost_frm_bm1, obj->lost_frm_count);
            frag_dec_flash_rd(obj, frame_index, obj->row_data_buf);
            buf_xor(obj->xor_row_data_buf, obj->row_data_buf, obj->cfg.size);
            if (bit_is_all_clear(obj->matched_lost_frm_bm0, obj->lost_frm_count)) {
                no_info = true;
                break;
            }
        } while (1);
        if (!no_info) {
            /* current frame contains new information, save it */
            frag_dec_lost_frm_matrix_save(obj, lost_frame_index, obj->matched_lost_frm_bm0, obj->lost_frm_count);
            frag_dec_flash_wr(obj, frame_index, obj->xor_row_data_buf);
            obj->filled_lost_frm_count++;
        }
        if (obj->filled_lost_frm_count == obj->lost_frm_count) {
            /* all frame content is received, now to reconstruct the whole frame */
            if (obj->lost_frm_count > 1) {
                for (i = (obj->lost_frm_count - 2); i >= 0; i--) {
                    frame_index = bit_fns(obj->lost_frm_bm, obj->cfg.nb, i + 1);
                    frag_dec_flash_rd(obj, frame_index, obj->xor_row_data_buf);
                    for (j = (obj->lost_frm_count  - 1); j > i; j--) {
                        frag_dec_lost_frm_matrix_load(obj, i, obj->matched_lost_frm_bm1, obj->lost_frm_count);
                        frag_dec_lost_frm_matrix_load(obj, j, obj->matched_lost_frm_bm0, obj->lost_frm_count);
                        if (bit_get(obj->matched_lost_frm_bm1, j)) {
                            frame_index1 = bit_fns(obj->lost_frm_bm, obj->cfg.nb, j + 1);
                            frag_dec_flash_rd(obj, frame_index1, obj->row_data_buf);
                            bit_xor(obj->matched_lost_frm_bm1, obj->matched_lost_frm_bm0, obj->lost_frm_count);
                            buf_xor(obj->xor_row_data_buf, obj->row_data_buf, obj->cfg.size);
                            frag_dec_lost_frm_matrix_save(obj, i, obj->matched_lost_frm_bm1, obj->lost_frm_count);
                        }
                    }
                    frag_dec_flash_wr(obj, frame_index, obj->xor_row_data_buf);
                }
            }
            obj->finished = true;
            return obj->lost_frm_count;
        }
    }
    /* process ongoing */
    return FRAG_DEC_ONGOING;
}

void frag_dec_log_buf(uint8_t *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        FRAGLOG("%02X, ", buf[i]);
    }
    FRAGLOG("\n");
}


void frag_dec_log_bits(bm_t *bitmap, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (bit_get(bitmap, i)) {
            FRAGLOG("1 ");
        } else {
            FRAGLOG("0 ");
        }
    }
    FRAGLOG("\n");
}

void frag_dec_log_matrix_bits(bm_t *bitmap, int len)
{
    int i, j;
    for (i = 0; i < len; i++) {
        for (j = 0; j < len; j++) {
            if (bit_get(bitmap, i * len + j)) {
                FRAGLOG("1 ");
            } else {
                FRAGLOG("0 ");
            }
        }
        FRAGLOG("\n");
    }
    if (i == 0) {
        FRAGLOG("\n");
    }
}


void frag_dec_log(frag_dec_t *obj)
{
    int i, j;
    FRAGLOG("Decode %s\n", obj->finished?"ok":"ng");
    for (i = 0; i < obj->cfg.nb; i++) {
        frag_dec_flash_rd(obj, i, obj->row_data_buf);
        for (j = 0; j < obj->cfg.size; j++) {
            FRAGLOG("%02X ", obj->row_data_buf[j]);
        }
        FRAGLOG("\n");
    }
#if 0
    FRAGLOG("filled_lost_frm_bm: (%d) ", obj->lost_frm_count);
    frag_dec_log_bits(obj->filled_lost_frm_bm, obj->lost_frm_count);

    FRAGLOG("lost_frm_matrix_bm: (%d) \n", obj->lost_frm_count);
    frag_dec_log_matrix_bits(obj->lost_frm_matrix_bm, obj->lost_frm_count);
#endif
}
