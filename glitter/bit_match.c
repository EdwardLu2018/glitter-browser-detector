#include "bit_match.h"
#include "queue_buf.h"
#include "linked_list.h"
#include "common/math_util.h"
#include "lightanchor.h"
#include "lightanchor_detector.h"

// #define DEBUG

static inline uint8_t cyclic_lsl(uint8_t bits, uint8_t size)
{
    return (bits << 1) | ((bits >> (size - 1)) & 0x1);
}

int decode(lightanchor_detector_t *ld, lightanchor_t *candidate_curr)
{
#ifdef DEBUG
    printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" ",
            BYTE_TO_BINARY(candidate_curr->code>>8), BYTE_TO_BINARY(candidate_curr->code));
#endif

    uint8_t code_to_match;
    if (candidate_curr->valid)
    {
        code_to_match = candidate_curr->next_code;
#ifdef DEBUG
        printf(" "BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                BYTE_TO_BINARY(code_to_match>>8), BYTE_TO_BINARY(code_to_match));
#endif
        if (candidate_curr->code == code_to_match)
        {
            candidate_curr->next_code = cyclic_lsl(code_to_match, 8);
            candidate_curr->valid = 1;
        }
        else {
#ifdef DEBUG
            printf("==== LOST ====\n");
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" != ",
                    BYTE_TO_BINARY(candidate_curr->code>>8), BYTE_TO_BINARY(candidate_curr->code));
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" | ",
                    BYTE_TO_BINARY(code_to_match>>8), BYTE_TO_BINARY(code_to_match));
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                    BYTE_TO_BINARY(shifted>>8), BYTE_TO_BINARY(shifted));

            int j = candidate_curr->brightnesses.idx;
            for (int i = 0; i < BUF_SIZE; i++)
            {
                printf("%u ", candidate_curr->brightnesses.buf[j]);
                j = (j + 1) % BUF_SIZE;
            }
            puts("");
            printf("===============\n");
#endif
            candidate_curr->valid = 0;
            // candidate_curr->next_code = cyclic_lsl(code_to_match, 16);
        }
        return candidate_curr->valid;
    }
    else {
        for (int i = 0; i < zarray_size(ld->codes); i++)
        {
            glitter_code_t *code;
            zarray_get_volatile(ld->codes, i, &code);
            code_to_match = code->code;
#ifdef DEBUG
            printf(" "BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                    BYTE_TO_BINARY(code_to_match>>8), BYTE_TO_BINARY(code_to_match));
#endif
            if (candidate_curr->code == code_to_match)
            {
#ifdef DEBUG
                printf("==== MATCH ====\n");
                printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" == ",
                        BYTE_TO_BINARY(candidate_curr->code>>8), BYTE_TO_BINARY(candidate_curr->code));
                printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                        BYTE_TO_BINARY(code_to_match>>8), BYTE_TO_BINARY(code_to_match));
                printf("===============\n");
#endif
                candidate_curr->code = code_to_match;
                candidate_curr->match_code = code_to_match;
                candidate_curr->next_code = cyclic_lsl(code_to_match, 8);
                candidate_curr->valid = 1;
                return 1;
            }
        }
        candidate_curr->valid = 0;
        return 0;
    }
}
