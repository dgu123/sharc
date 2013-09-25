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
 * 28/08/13 18:57
 */

#include "chameleon_encode.h"

SHARC_FORCE_INLINE void sharc_hash_encode_writeToSignature(sharc_chameleon_encode_state *state) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    *(state->signature) |= ((uint64_t) 1) << state->shift;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    *(state->signature) |= ((uint64_t) 1) << ((56 - (state->shift & ~0x7)) + (state->shift & 0x7));
#endif
}

SHARC_FORCE_INLINE SHARC_KERNEL_ENCODE_STATE sharc_hash_encode_prepareNewBlock(sharc_byte_buffer *restrict out, sharc_chameleon_encode_state *restrict state, const uint_fast32_t minimumLookahead) {
    if (out->position + minimumLookahead > out->size)
        return SHARC_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    switch (state->signaturesCount) {
        case SHARC_PREFERRED_EFFICIENCY_CHECK_SIGNATURES:
            if (state->efficiencyChecked ^ 0x1) {
                state->efficiencyChecked = 1;
                return SHARC_KERNEL_ENCODE_STATE_INFO_EFFICIENCY_CHECK;
            }
            break;
        case SHARC_PREFERRED_BLOCK_SIGNATURES:
            state->signaturesCount = 0;
            state->efficiencyChecked = 0;
            return SHARC_KERNEL_ENCODE_STATE_INFO_NEW_BLOCK;
        default:
            break;
    }
    state->signaturesCount++;

    state->shift = 0;
    state->signature = (sharc_hash_signature *) (out->pointer + out->position);
    *state->signature = 0;
    out->position += sizeof(sharc_hash_signature);

    return SHARC_KERNEL_ENCODE_STATE_READY;
}

SHARC_FORCE_INLINE SHARC_KERNEL_ENCODE_STATE sharc_hash_encode_checkState(sharc_byte_buffer *restrict out, sharc_chameleon_encode_state *restrict state) {
    SHARC_KERNEL_ENCODE_STATE returnState;

    switch (state->shift) {
        case 64:
            if ((returnState = sharc_hash_encode_prepareNewBlock(out, state, SHARC_HASH_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD))) {
                state->process = SHARC_HASH_ENCODE_PROCESS_PREPARE_NEW_BLOCK;
                return returnState;
            }
            break;
        default:
            break;
    }

    return SHARC_KERNEL_ENCODE_STATE_READY;
}

SHARC_FORCE_INLINE void sharc_hash_encode_kernel(sharc_byte_buffer *restrict out, uint32_t *restrict hash, const uint32_t chunk, sharc_chameleon_encode_state *restrict state) {
    SHARC_CHAMELEON_HASH_ALGORITHM(*hash, SHARC_LITTLE_ENDIAN_32(chunk));
    sharc_dictionary_entry *found = &state->dictionary.entries[*hash];

    if (chunk ^ found->as_uint32_t) {
        found->as_uint32_t = chunk;
        *(uint32_t *) (out->pointer + out->position) = chunk;
        out->position += sizeof(uint32_t);
    } else {
        sharc_hash_encode_writeToSignature(state);
        *(uint16_t *) (out->pointer + out->position) = SHARC_LITTLE_ENDIAN_16(*hash);
        out->position += sizeof(uint16_t);
    }

    state->shift++;
}

SHARC_FORCE_INLINE void sharc_hash_encode_process_chunk(uint64_t *chunk, sharc_byte_buffer *restrict in, sharc_byte_buffer *restrict out, uint32_t *restrict hash, sharc_chameleon_encode_state *restrict state) {
    *chunk = *(uint64_t *) (in->pointer + in->position);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    sharc_hash_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif
    sharc_hash_encode_kernel(out, hash, (uint32_t) (*chunk >> 32), state);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    sharc_hash_encode_kernel(out, hash, (uint32_t) (*chunk & 0xFFFFFFFF), state);
#endif
    in->position += sizeof(uint64_t);
}

SHARC_FORCE_INLINE void sharc_hash_encode_process_span(uint64_t *chunk, sharc_byte_buffer *restrict in, sharc_byte_buffer *restrict out, uint32_t *restrict hash, sharc_chameleon_encode_state *restrict state) {
    sharc_hash_encode_process_chunk(chunk, in, out, hash, state);
    sharc_hash_encode_process_chunk(chunk, in, out, hash, state);
    sharc_hash_encode_process_chunk(chunk, in, out, hash, state);
    sharc_hash_encode_process_chunk(chunk, in, out, hash, state);
}

SHARC_FORCE_INLINE sharc_bool sharc_hash_encode_attempt_copy(sharc_byte_buffer *restrict out, sharc_byte *restrict origin, const uint_fast32_t count) {
    if (out->position + count <= out->size) {
        memcpy(out->pointer + out->position, origin, count);
        out->position += count;
        return false;
    }
    return true;
}

SHARC_FORCE_INLINE SHARC_KERNEL_ENCODE_STATE NAME(sharc_chameleon_encode_init)(sharc_chameleon_encode_state *state) {
    state->signaturesCount = 0;
    state->efficiencyChecked = 0;

    state->process = SHARC_HASH_ENCODE_PROCESS_PREPARE_NEW_BLOCK;

    return SHARC_KERNEL_ENCODE_STATE_READY;
}

SHARC_FORCE_INLINE SHARC_KERNEL_ENCODE_STATE NAME(sharc_chameleon_encode_process)(sharc_byte_buffer *restrict in, sharc_byte_buffer *restrict out, sharc_chameleon_encode_state *restrict state, const sharc_bool flush) {
    SHARC_KERNEL_ENCODE_STATE returnState;
    uint32_t hash;
    uint_fast64_t remaining;
    uint64_t chunk;

    if (in->size == 0)
        goto exit;

    const uint_fast64_t limit = in->size & ~0x1F;

    switch (state->process) {
        case SHARC_HASH_ENCODE_PROCESS_CHECK_STATE:
            if ((returnState = sharc_hash_encode_checkState(out, state)))
                return returnState;
            state->process = SHARC_HASH_ENCODE_PROCESS_DATA;
            break;

        case SHARC_HASH_ENCODE_PROCESS_PREPARE_NEW_BLOCK:
            if ((returnState = sharc_hash_encode_prepareNewBlock(out, state, SHARC_HASH_ENCODE_MINIMUM_OUTPUT_LOOKAHEAD)))
                return returnState;
            state->process = SHARC_HASH_ENCODE_PROCESS_DATA;
            break;

        case SHARC_HASH_ENCODE_PROCESS_DATA:
            if (in->size - in->position < 4 * sizeof(uint64_t))
                goto finish;
            while (true) {
                sharc_hash_encode_process_span(&chunk, in, out, &hash, state);
                if (in->position == limit) {
                    if (flush) {
                        state->process = SHARC_HASH_ENCODE_PROCESS_FINISH;
                        return SHARC_KERNEL_ENCODE_STATE_READY;
                    } else {
                        state->process = SHARC_HASH_ENCODE_PROCESS_CHECK_STATE;
                        return SHARC_KERNEL_ENCODE_STATE_STALL_ON_INPUT_BUFFER;
                    }
                }

                if ((returnState = sharc_hash_encode_checkState(out, state)))
                    return returnState;
            }

        case SHARC_HASH_ENCODE_PROCESS_FINISH:
            while (true) {
                while (state->shift ^ 64) {
                    if (in->size - in->position < sizeof(uint32_t))
                        goto finish;
                    else {
                        if (out->size - out->position < sizeof(uint32_t))
                            return SHARC_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;
                        sharc_hash_encode_kernel(out, &hash, *(uint32_t *) (in->pointer + in->position), state);
                        in->position += sizeof(uint32_t);
                    }
                }
                if (in->size - in->position < sizeof(uint32_t))
                    goto finish;
                else if ((returnState = sharc_hash_encode_prepareNewBlock(out, state, sizeof(sharc_hash_signature))))
                    return returnState;
            }
        finish:
            remaining = in->size - in->position;
            if (remaining > 0) {
                if (sharc_hash_encode_attempt_copy(out, in->pointer + in->position, (uint32_t) remaining))
                    return SHARC_KERNEL_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;
                in->position += remaining;
            }
        exit:
            state->process = SHARC_HASH_ENCODE_PROCESS_PREPARE_NEW_BLOCK;
            return SHARC_KERNEL_ENCODE_STATE_FINISHED;

        default:
            return SHARC_KERNEL_ENCODE_STATE_ERROR;
    }

    return
            SHARC_KERNEL_ENCODE_STATE_READY;
}

SHARC_FORCE_INLINE SHARC_KERNEL_ENCODE_STATE NAME(sharc_chameleon_encode_finish)(sharc_chameleon_encode_state *state) {
    return SHARC_KERNEL_ENCODE_STATE_READY;
}
