/*
 * Centaurean Sharc
 * http://www.centaurean.com/sharc
 *
 * Copyright (c) 2013, Guillaume Voirin
 * All rights reserved.
 *
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation. For the terms of this
 * license, see http://www.gnu.org/licenses/gpl.html
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this library under a commercial
 * license, see http://www.centaurean.com/sharc for more
 * information.
 *
 * 18/09/13 14:57
 */

#include "argonaut_encode.h"

const primhufflook hl = {.lookup = HUFFMAN_PRIMARY_ENCODE};
const sechufflook shl = {.lookup = HUFFMAN_SECONDARY_ENCODE};
const wlhufflook wlhl = {.lookup = HUFFMAN_WORD_LENGTH_ENCODE};

SHARC_FORCE_INLINE SHARC_KERNEL_ENCODE_STATE sharc_argonaut_encode_check_next_output_block_memory_available(sharc_byte_buffer *restrict out, sharc_argonaut_encode_state *restrict state) {
    if (out->position + 64 > out->size)
        return SHARC_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    //state->blockShift = 0;

    return SHARC_KERNEL_ENCODE_STATE_READY;
}

SHARC_FORCE_INLINE void sharc_argonaut_encode_prepare_next_output_unit_even(sharc_byte_buffer *restrict out, sharc_argonaut_encode_state *restrict state) {
    state->shift = 0;

    state->output = (sharc_argonaut_output_unit *) (out->pointer + out->position);
    *(state->output) = 0;
    out->position += sizeof(sharc_argonaut_output_unit);
}

SHARC_FORCE_INLINE void sharc_argonaut_encode_prepare_next_output_unit(sharc_byte_buffer *restrict out, sharc_argonaut_encode_state *restrict state, const uint_fast64_t data, const uint_fast8_t bitSize) {
    state->shift = (uint8_t) (state->shift - SHARC_ARGONAUT_OUTPUT_UNIT_BIT_SIZE);

    state->output = (sharc_argonaut_output_unit *) (out->pointer + out->position);
    *(state->output) = (sharc_argonaut_output_unit) (data >> (bitSize - state->shift));
    out->position += sizeof(sharc_argonaut_output_unit);
}

SHARC_FORCE_INLINE void sharc_argonaut_encode_write_to_output(sharc_argonaut_encode_state *state, const uint_fast64_t value) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    *(state->output) |= value << state->shift;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    *(state->output) |= value << ((56 - (state->shift & ~0x7)) + (state->shift & 0x7));
#endif
}

SHARC_FORCE_INLINE void sharc_argonaut_encode_write_coded_separator(sharc_byte_buffer *restrict out, sharc_argonaut_encode_state *restrict state) {
    sharc_argonaut_encode_write_to_output(state, 1);

    state->shift++;
    if (state->shift == 0x40)
        sharc_argonaut_encode_prepare_next_output_unit_even(out, state);
}

SHARC_FORCE_INLINE void sharc_argonaut_encode_write(sharc_byte_buffer *restrict out, sharc_argonaut_encode_state *restrict state, const uint_fast64_t data, const uint_fast8_t bitSize) {
    sharc_argonaut_encode_write_to_output(state, data);

    state->shift += bitSize;
    if (state->shift & ~(SHARC_ARGONAUT_OUTPUT_UNIT_BIT_SIZE - 1))
        sharc_argonaut_encode_prepare_next_output_unit(out, state, data, bitSize);
}

SHARC_FORCE_INLINE void sharc_argonaut_encode_write_coded(sharc_byte_buffer *restrict out, sharc_argonaut_encode_state *restrict state, const sharc_argonaut_huffman_code *restrict code) {
    sharc_argonaut_encode_write(out, state, code->code, code->bitSize);
}

SHARC_FORCE_INLINE void sharc_argonaut_encode_process_letter(sharc_argonaut_dictionary_word *restrict word, const uint8_t *restrict letter, const uint8_t index, uint_fast32_t *restrict hash, sharc_argonaut_dictionary *restrict dictionary, uint8_t *restrict separator) {
    sharc_argonaut_dictionary_primary_entry *match = &dictionary->prim[*letter];

    const uint8_t rank = match->ranking;
    const uint8_t precedingRank = match->ranking - 1;

    *hash = *hash ^ *letter;
    *hash = *hash * SHARC_HASH_PRIME;

    const sharc_argonaut_huffman_code *huffmanCode = &hl.lookup[rank];
    word->letterCode[index] = huffmanCode;
    //word->compressedBitLength += huffmanCode->bitSize;

    match->durability++;
    sharc_argonaut_dictionary_primary_entry *preceding_match = dictionary->ranking.primary[precedingRank];
    if (preceding_match->durability < match->durability) {
        dictionary->ranking.primary[precedingRank] = match;
        dictionary->ranking.primary[rank] = preceding_match;
        match->ranking -= 1;
        if (!match->ranking)
            *separator = *letter;
        preceding_match->ranking += 1;
    }
}

SHARC_FORCE_INLINE uint8_t sharc_argonaut_encode_advance_to_separator(uint_fast8_t *restrict letter, uint_fast32_t *restrict hash, sharc_argonaut_dictionary_word *restrict word, const uint32_t xorMask, uint8_t *separator, sharc_argonaut_dictionary *restrict dictionary) {
    *hash = SHARC_HASH_OFFSET_BASIS ^ xorMask;
    if ((*letter = word->letters[0]) ^ *separator) {
        sharc_argonaut_encode_process_letter(word, letter, 0, hash, dictionary, separator);
        if ((*letter = word->letters[1]) ^ *separator) {
            sharc_argonaut_encode_process_letter(word, letter, 1, hash, dictionary, separator);
            if ((*letter = word->letters[2]) ^ *separator) {
                sharc_argonaut_encode_process_letter(word, letter, 2, hash, dictionary, separator);
                if ((*letter = word->letters[3]) ^ *separator) {
                    sharc_argonaut_encode_process_letter(word, letter, 3, hash, dictionary, separator);
                    if ((*letter = word->letters[4]) ^ *separator) {
                        sharc_argonaut_encode_process_letter(word, letter, 4, hash, dictionary, separator);
                        if ((*letter = word->letters[5]) ^ *separator) {
                            sharc_argonaut_encode_process_letter(word, letter, 5, hash, dictionary, separator);
                            if ((*letter = word->letters[6]) ^ *separator) {
                                sharc_argonaut_encode_process_letter(word, letter, 6, hash, dictionary, separator);
                                if ((*letter = word->letters[7]) ^ *separator) {
                                    sharc_argonaut_encode_process_letter(word, letter, 7, hash, dictionary, separator);
                                    return 8;
                                } else
                                    return 7;
                            } else
                                return 6;
                        } else
                            return 5;
                    } else
                        return 4;
                } else
                    return 3;
            } else
                return 2;
        } else
            return 1;
    } else
        return 0;
}

SHARC_FORCE_INLINE uint8_t sharc_argonaut_encode_advance_to_separator_limited(uint_fast8_t start, uint_fast8_t stop, uint_fast8_t *restrict letter, uint_fast32_t *restrict hash, sharc_argonaut_dictionary_word *restrict word, const uint32_t xorMask, uint8_t *separator, sharc_argonaut_dictionary *restrict dictionary) {
    if (!start)
        *hash = SHARC_HASH_OFFSET_BASIS ^ xorMask;
    if (start ^ stop && (*letter = word->letters[start]) ^ *separator) {
        start++;
        sharc_argonaut_encode_process_letter(word, letter, 0, hash, dictionary, separator);
        if (start ^ stop && (*letter = word->letters[start]) ^ *separator) {
            start++;
            sharc_argonaut_encode_process_letter(word, letter, 1, hash, dictionary, separator);
            if (start ^ stop && (*letter = word->letters[start]) ^ *separator) {
                start++;
                sharc_argonaut_encode_process_letter(word, letter, 2, hash, dictionary, separator);
                if (start ^ stop && (*letter = word->letters[start]) ^ *separator) {
                    start++;
                    sharc_argonaut_encode_process_letter(word, letter, 3, hash, dictionary, separator);
                    if (start ^ stop && (*letter = word->letters[start]) ^ *separator) {
                        start++;
                        sharc_argonaut_encode_process_letter(word, letter, 4, hash, dictionary, separator);
                        if (start ^ stop && (*letter = word->letters[start]) ^ *separator) {
                            start++;
                            sharc_argonaut_encode_process_letter(word, letter, 5, hash, dictionary, separator);
                            if (start ^ stop && (*letter = word->letters[start]) ^ *separator) {
                                start++;
                                sharc_argonaut_encode_process_letter(word, letter, 6, hash, dictionary, separator);
                                if (start ^ stop && (*letter = word->letters[start]) ^ *separator) {
                                    sharc_argonaut_encode_process_letter(word, letter, 7, hash, dictionary, separator);
                                    return 8;
                                } else
                                    return 7;
                            } else
                                return 6;
                        } else
                            return 5;
                    } else
                        return 4;
                } else
                    return 3;
            } else
                return 2;
        } else
            return 1;
    } else
        return 0;
}

SHARC_FORCE_INLINE SHARC_KERNEL_ENCODE_STATE sharc_argonaut_encode_goto_next_word(sharc_byte_buffer *restrict in, sharc_byte_buffer *restrict out, sharc_argonaut_dictionary *restrict dictionary, sharc_argonaut_encode_state *restrict state, uint8_t *separator) {
    //state->count[7]++;
    uint_fast64_t start = in->position;
    while (*(in->pointer + in->position) == *separator) {
        if (unlikely(out->position == out->size))
            return SHARC_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;
        sharc_argonaut_encode_write_coded_separator(out, state);
        in->position++;
        if (unlikely(in->position == in->size))
            return SHARC_KERNEL_ENCODE_STATE_STALL_ON_INPUT_BUFFER;
    }
    uint_fast64_t separatorCount = in->position - start;
    dictionary->ranking.primary[0]->durability += separatorCount;
    //state->count[0] += separatorCount;

    return SHARC_KERNEL_ENCODE_STATE_READY;
}

SHARC_FORCE_INLINE uint_fast64_t sharc_argonaut_encode_words_differ(sharc_argonaut_dictionary_word *restrict a, sharc_argonaut_dictionary_word *restrict b) {
    if (a->length != b->length)
        return true;
    //return a->as_uint64_t ^ b->as_uint64_t;
    const uint8_t limit = a->length;
    for (uint8_t compared = 0; compared ^ limit; compared++)
        if (a->letters[compared] ^ b->letters[compared])
            return true;
    return false;

}

SHARC_FORCE_INLINE SHARC_KERNEL_ENCODE_STATE sharc_argonaut_encode_process_word(sharc_byte_buffer *restrict in, sharc_byte_buffer *restrict out, uint_fast8_t *letter, uint_fast32_t *restrict hash, const uint32_t xorMask, sharc_argonaut_dictionary *restrict dictionary, sharc_argonaut_encode_state *restrict state, uint8_t *separator) {
    while ((*letter = *(in->pointer + in->position)) ^ *separator) {
        state->word.letters[state->word.length] = *letter;
        sharc_argonaut_encode_process_letter(&state->word, letter, state->word.length, hash, dictionary, separator);
        in->position ++;
        state->word.length ++;
        if(in->position == in->size)
            return SHARC_KERNEL_ENCODE_STATE_STALL_ON_INPUT_BUFFER;
        if(state->word.length == SHARC_ARGONAUT_DICTIONARY_WORD_MAX_LETTERS)
            break;
    }
    //sharc_argonaut_dictionary_word word;
    /*if (unlikely(state->partialWord.length)) {
        word = state->partialWord;
        word.as_uint64_t |= ((*(uint64_t *) (in->pointer + in->position)) >> state->partialWord.length);
        const uint8_t addedLength = sharc_argonaut_encode_advance_to_separator_limited(state->partialWord.length, SHARC_ARGONAUT_DICTIONARY_WORD_MAX_LETTERS, letter, hash, &word, xorMask, separator, dictionary);
        word.length += addedLength;
        in->position += addedLength;
        state->partialWord.length = 0;
    } else {
        word.as_uint64_t = *(uint64_t *) (in->pointer + in->position);
        if (likely(in->size - in->position < SHARC_ARGONAUT_DICTIONARY_WORD_MAX_LETTERS)) {
            word.length = sharc_argonaut_encode_advance_to_separator(letter, hash, &word, xorMask, separator, dictionary);
        } else {
            word.length = sharc_argonaut_encode_advance_to_separator_limited(0, (uint8_t) (in->size - in->position), letter, hash, &word, xorMask, separator, dictionary);
            word.as_uint64_t &= ((((uint64_t) 1) << (word.length << 3)) - 1);
            if (word.length == in->size - in->position) {
                state->partialWord = word;
                return SHARC_KERNEL_ENCODE_STATE_STALL_ON_INPUT_BUFFER;
            }
        }
        in->position += word.length;
    }

    if (word.length ^ SHARC_ARGONAUT_DICTIONARY_WORD_MAX_LETTERS)
        word.as_uint64_t &= ((((uint64_t) 1) << (word.length << 3)) - 1);*/

    const uint8_t bits = 16;
    uint_fast32_t shash = (uint_fast32_t) ((((*hash >> bits) ^ *hash) & ((1 << bits) - 1)));
    sharc_dictionary_qentry *match = &dictionary->quad[shash];
    if (sharc_argonaut_encode_words_differ(&state->word, &match->word) /*word.as_uint64_t ^ match->word.as_uint64_t*/) {
        if (match->durability)
            match->durability--;
        else {
            //match->word.as_uint64_t = word.as_uint64_t;
            //match->word.length = word.length;
            match->word = state->word;
        }
        // todo code word (3) + length
        sharc_argonaut_encode_write(out, state, 0, 3);
        sharc_argonaut_encode_write_coded(out, state, &wlhl.lookup[state->word.length - 1]);
        const uint8_t limit = state->word.length;
        for (uint_fast8_t i = 0; i ^ limit; i++)
            sharc_argonaut_encode_write_coded(out, state, state->word.letterCode[i]);
        //state->count[2]++;
        //state->length[match->word.length - 1] ++;
    } else {
        if (match->ranked) {
            // todo dict code (3)
            sharc_argonaut_encode_write(out, state, 0, 2);
            sharc_argonaut_encode_write_coded(out, state, &shl.lookup[match->ranking]);
            //state->bitCount += shl.lookup[secondaryHit->ranking - 1].bitSize;
            //state->count[3]++;
        } else {
            // todo dict code (2)
            sharc_argonaut_encode_write(out, state, 0, 3);
            sharc_argonaut_encode_write(out, state, shash, 16);
            //state->bitCount += bits + 2;
            //state->count[4]++;
        }

        match->durability++;
        if (match->ranked) {
            if (match->ranking) {
                const uint16_t preceding = match->ranking - 1;
                sharc_dictionary_qentry *preceding_match = dictionary->ranking.top[preceding];
                if (preceding_match->durability < match->durability) {
                    dictionary->ranking.top[preceding] = match;
                    dictionary->ranking.top[match->ranking] = preceding_match;
                    match->ranking -= 1;
                    preceding_match->ranking += 1;
                }
            }
        } else {
            const uint16_t preceding = SHARC_ARGONAUT_DICTIONARY_SECONDARY_RANKS - 1;
            sharc_dictionary_qentry *preceding_match = dictionary->ranking.top[preceding];
            if (preceding_match->durability < match->durability) {
                dictionary->ranking.top[preceding] = match;
                match->ranking = preceding;
                match->ranked = true;
                preceding_match->ranking = 0;
                preceding_match->ranked = false;
            }
        }
    }

    state->word.length = 0;

    return SHARC_KERNEL_ENCODE_STATE_READY;
}

SHARC_FORCE_INLINE SHARC_KERNEL_ENCODE_STATE sharc_argonaut_encode_init(sharc_argonaut_encode_state *state, sharc_argonaut_dictionary *restrict dictionary) {
    state->efficiencyChecked = 0;

    state->process = SHARC_ARGONAUT_ENCODE_PROCESS_PREPARE_OUTPUT;

    for (uint16_t i = 0; i < SHARC_ARGONAUT_DICTIONARY_SECONDARY_RANKS; i++) {
        dictionary->ranking.top[i] = &dictionary->quad[i];
        dictionary->quad[i].ranking = i;
        dictionary->quad[i].ranked = true;
    }
    for (uint16_t i = 0; i < (1 << 8); i++)
        dictionary->prim[i].letter = (uint8_t) (i);

    for (uint16_t i = 0; i < SHARC_ARGONAUT_DICTIONARY_PRIMARY_RANKS; i++) {
        dictionary->ranking.primary[i] = &dictionary->prim[i];
        dictionary->prim[i].ranking = (uint8_t) i;
    }

    return SHARC_KERNEL_ENCODE_STATE_READY;
}

SHARC_FORCE_INLINE SHARC_KERNEL_ENCODE_STATE sharc_argonaut_encode_process(sharc_byte_buffer *restrict in, sharc_byte_buffer *restrict out, const uint32_t xorMask, sharc_argonaut_dictionary *restrict dictionary, sharc_argonaut_encode_state *restrict state, const sharc_bool flush) {
    SHARC_KERNEL_ENCODE_STATE returnState;
    uint_fast8_t letter;
    uint_fast32_t hash;

    uint8_t separator = dictionary->ranking.primary[0]->letter;

    if (in->size == 0)
        goto exit;

    switch (state->process) {
        case SHARC_ARGONAUT_ENCODE_PROCESS_PREPARE_OUTPUT:
            sharc_argonaut_encode_prepare_next_output_unit_even(out, state);
            state->process = SHARC_ARGONAUT_ENCODE_PROCESS_GOTO_NEXT_WORD;

        case SHARC_ARGONAUT_ENCODE_PROCESS_GOTO_NEXT_WORD:
        goto_next_word:
            returnState = sharc_argonaut_encode_goto_next_word(in, out, dictionary, state, &separator);
            if (returnState) {
                if (flush) {
                    if (returnState == SHARC_KERNEL_ENCODE_STATE_STALL_ON_INPUT_BUFFER) {
                        state->process = SHARC_ARGONAUT_ENCODE_PROCESS_FINISH;
                        return SHARC_KERNEL_ENCODE_STATE_READY;
                    }
                } else
                    return returnState;
            }
            state->process = SHARC_ARGONAUT_ENCODE_PROCESS_CHECK_AVAILABLE_MEMORY;

        case SHARC_ARGONAUT_ENCODE_PROCESS_CHECK_AVAILABLE_MEMORY:
            if ((returnState = sharc_argonaut_encode_check_next_output_block_memory_available(out, state)))
                return returnState;
            //sharc_argonaut_encode_prepare_next_output_unit_even(out, state);
            state->process = SHARC_ARGONAUT_ENCODE_PROCESS_WORD;

        case SHARC_ARGONAUT_ENCODE_PROCESS_WORD:
            returnState = sharc_argonaut_encode_process_word(in, out, &letter, &hash, xorMask, dictionary, state, &separator);
            if (returnState) {
                if (flush) {
                    if (returnState == SHARC_KERNEL_ENCODE_STATE_STALL_ON_INPUT_BUFFER) {
                        state->process = SHARC_ARGONAUT_ENCODE_PROCESS_FINISH;
                        return SHARC_KERNEL_ENCODE_STATE_READY;
                    }
                } else
                    return returnState;
            }
            state->process = SHARC_ARGONAUT_ENCODE_PROCESS_GOTO_NEXT_WORD;
            goto goto_next_word;

        case SHARC_ARGONAUT_ENCODE_PROCESS_FINISH:
            for (uint8_t i = 0; i < 8; i++)
                printf("%llu, ", state->count[i]);
            printf("\n");
            for (uint8_t i = 0; i < 8; i++)
                printf("%llu, ", state->length[i]);
            printf("\n");
        exit:
            state->process = SHARC_ARGONAUT_ENCODE_PROCESS_GOTO_NEXT_WORD;
            return SHARC_KERNEL_ENCODE_STATE_FINISHED;

        default:
            return SHARC_KERNEL_ENCODE_STATE_ERROR;
    }
}

SHARC_FORCE_INLINE SHARC_KERNEL_ENCODE_STATE sharc_argonaut_encode_finish(sharc_argonaut_encode_state *state) {
    return SHARC_KERNEL_ENCODE_STATE_READY;
}
