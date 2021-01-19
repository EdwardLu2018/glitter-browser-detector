#include "bit_match.h"
#include "queue_buf.h"
#include "common/math_util.h"
#include "lightanchor.h"
#include "lightanchor_detector.h"

// #define DEBUG

#define EVEN_MASK       0xaaaa
#define ODD_MASK        0x5555

static inline size_t cyclic_lsl(size_t bits, size_t size)
{
    return (bits << 1) | ((bits >> (size - 1)) & 0x1);
}

uint16_t double_bits(uint8_t bits)
{
    uint16_t res = 0;
    for (int i = 0; i < 8; i++)
    {
        res |= ((bits & 0x1) << 1 | (bits & 0x1)) << (2*i);
        bits >>= 1;
    }
    return res;
}

size_t hamming_dist(size_t a, size_t b)
{
    size_t c = a ^ b;
    unsigned int hamm = 0;
    while (c) {
        ++hamm;
        c &= (c-1);
    }
    return hamm;
}

static int match_even_odd(uint16_t a, uint16_t b)
{
    uint16_t a_even = a & EVEN_MASK, a_odd = a & ODD_MASK;
    uint16_t b_even = b & EVEN_MASK, b_odd = b & ODD_MASK;
    return (a_even == b_even) || (a_odd == b_odd);
}

int match(lightanchor_detector_t *ld, lightanchor_t *candidate_curr)
{
#ifdef DEBUG
    printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" ",
            BYTE_TO_BINARY(candidate_curr->code>>8), BYTE_TO_BINARY(candidate_curr->code));
#endif
    uint16_t match_code;
    if (candidate_curr->valid > 0) {
        match_code = candidate_curr->next_code;
#ifdef DEBUG
        printf(" "BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                BYTE_TO_BINARY(match_code>>8), BYTE_TO_BINARY(match_code));
#endif
        if (match_even_odd(candidate_curr->code, match_code)) {
            candidate_curr->next_code = cyclic_lsl(match_code, 16);
            candidate_curr->valid++;
            return 1;
        }
        else {
#ifdef DEBUG
            printf("==== LOST ====\n");
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" != ",
                    BYTE_TO_BINARY(candidate_curr->code>>8), BYTE_TO_BINARY(candidate_curr->code));
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                    BYTE_TO_BINARY(match_code>>8), BYTE_TO_BINARY(match_code));

            int j = candidate_curr->brightnesses.idx;
            for (int i = 0; i < BUF_SIZE; i++) {
                printf("%u ", candidate_curr->brightnesses.buf[j]);
                j = (j + 1) % BUF_SIZE;
            }
            puts("");
            printf("===============\n");
#endif
            candidate_curr->next_code = 0;
            candidate_curr->valid = 0;
            return 0;
        }
    }
    else {
        match_code = ld->code;
#ifdef DEBUG
        printf(" "BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                        BYTE_TO_BINARY(match_code>>8), BYTE_TO_BINARY(match_code));
#endif
        if (match_even_odd(candidate_curr->code, match_code)) {
#ifdef DEBUG
            printf("==== MATCH ====\n");
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" == ",
                    BYTE_TO_BINARY(candidate_curr->code>>8), BYTE_TO_BINARY(candidate_curr->code));
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                    BYTE_TO_BINARY(match_code>>8), BYTE_TO_BINARY(match_code));
            printf("===============\n");
#endif
            candidate_curr->code = match_code;
            candidate_curr->next_code = cyclic_lsl(match_code, 16);
            candidate_curr->valid++;
            return 1;
        }
        return 0;
    }
}