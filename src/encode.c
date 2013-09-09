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
 * 29/08/13 12:55
 */

#include "encode.h"

SHARC_FORCE_INLINE SHARC_ENCODE_STATE sharc_encode_write_header(sharc_byte_buffer *restrict out, sharc_encode_state *restrict state) {
    if (out->position + sizeof(sharc_header) > out->size)
        return SHARC_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    if (!state->fileAttributes)
        state->totalWritten += sharc_header_write(out, SHARC_HEADER_ORIGIN_TYPE_STREAM, state->targetCompressionMode, state->blockType, NULL);
    else
        state->totalWritten += sharc_header_write(out, SHARC_HEADER_ORIGIN_TYPE_FILE, state->targetCompressionMode, state->blockType, state->fileAttributes);

    state->process = SHARC_ENCODE_PROCESS_WRITE_BLOCK_HEADER;

    return SHARC_ENCODE_STATE_READY;
}

SHARC_FORCE_INLINE SHARC_ENCODE_STATE sharc_encode_write_block_header(sharc_byte_buffer *restrict out, sharc_encode_state *restrict state) {
    if (out->position + sizeof(sharc_block_header) > out->size)
        return SHARC_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    state->currentCompressionMode = state->targetCompressionMode;

    if (state->dictionaryData.resetCycle)
        state->dictionaryData.resetCycle--;
    else {
        switch (state->targetCompressionMode) {
            case SHARC_COMPRESSION_MODE_DUAL_PASS:
                sharc_dictionary_resetCompressed(&state->dictionaryData.dictionary_b);

            case SHARC_COMPRESSION_MODE_FASTEST:
                sharc_dictionary_resetDirect(&state->dictionaryData.dictionary_a);
                break;

            case SHARC_COMPRESSION_MODE_COPY:
                break;
        }
        state->dictionaryData.resetCycle = SHARC_DICTIONARY_PREFERRED_RESET_CYCLE - 1;
    }

    state->efficiencyData.inStart = state->totalRead;
    state->efficiencyData.outStart = state->totalWritten;

    state->totalWritten += sharc_block_header_write(out);

    state->process = SHARC_ENCODE_PROCESS_WRITE_DATA;

    return SHARC_ENCODE_STATE_READY;
}

SHARC_FORCE_INLINE SHARC_ENCODE_STATE sharc_encode_write_block_footer(sharc_byte_buffer *restrict out, sharc_encode_state *restrict state) {
    if (out->position + sizeof(sharc_block_footer) > out->size)
        return SHARC_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    state->totalWritten += sharc_block_footer_write(out, 0);

    state->process = SHARC_ENCODE_PROCESS_WRITE_BLOCK_HEADER;

    return SHARC_ENCODE_STATE_READY;
}

SHARC_FORCE_INLINE SHARC_ENCODE_STATE sharc_encode_write_mode_marker(sharc_byte_buffer *restrict out, sharc_encode_state *restrict state) {
    if (out->position + sizeof(sharc_mode_marker) > out->size)
        return SHARC_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

    state->totalWritten += sharc_mode_marker_write(out, state->targetCompressionMode);

    state->process = SHARC_ENCODE_PROCESS_WRITE_DATA;

    return SHARC_ENCODE_STATE_READY;
}

SHARC_FORCE_INLINE void sharc_encode_update_totals(sharc_byte_buffer *restrict in, sharc_byte_buffer *restrict out, sharc_encode_state *restrict state, const uint_fast64_t inPositionBefore, const uint_fast64_t outPositionBefore) {
    state->totalRead += in->position - inPositionBefore;
    state->totalWritten += out->position - outPositionBefore;
}

SHARC_FORCE_INLINE SHARC_ENCODE_STATE sharc_encode_init(sharc_byte_buffer* workBuffer, sharc_encode_state *restrict state, const SHARC_COMPRESSION_MODE mode, const SHARC_ENCODE_OUTPUT_TYPE encodeType, const SHARC_BLOCK_TYPE blockType, const struct stat *fileAttributes) {
    switch (encodeType) {
        case SHARC_ENCODE_OUTPUT_TYPE_WITHOUT_HEADER:
        case SHARC_ENCODE_OUTPUT_TYPE_WITHOUT_HEADER_NOR_FOOTER:
            state->process = SHARC_ENCODE_PROCESS_WRITE_BLOCK_HEADER;
            break;
        default:
            state->process = SHARC_ENCODE_PROCESS_WRITE_HEADER;
            break;
    }
    state->targetCompressionMode = mode;
    state->currentCompressionMode = mode;
    state->blockType = blockType;
    state->fileAttributes = fileAttributes;

    state->totalRead = 0;
    state->totalWritten = 0;

    sharc_hash_encode_init(&state->hashEncodeState);
    state->dictionaryData.resetCycle = 0;

    state->workBuffer = workBuffer;

    return SHARC_ENCODE_STATE_READY;
}

SHARC_FORCE_INLINE SHARC_ENCODE_STATE sharc_encode_process(sharc_byte_buffer *restrict in, sharc_byte_buffer *restrict out, sharc_encode_state *restrict state, const sharc_bool flush) {
    SHARC_ENCODE_STATE encodeState;
    SHARC_HASH_ENCODE_STATE hashEncodeState;
    uint_fast64_t inPositionBefore;
    uint_fast64_t outPositionBefore;

    while (true) {
        switch (state->process) {
            case SHARC_ENCODE_PROCESS_WRITE_HEADER:
                if ((encodeState = sharc_encode_write_header(out, state)))
                    return encodeState;
                break;

            case SHARC_ENCODE_PROCESS_WRITE_BLOCK_HEADER:
                if ((encodeState = sharc_encode_write_block_header(out, state)))
                    return encodeState;
                break;

            case SHARC_ENCODE_PROCESS_WRITE_BLOCK_FOOTER:
                if ((encodeState = sharc_encode_write_block_footer(out, state)))
                    return encodeState;
                break;

            case SHARC_ENCODE_PROCESS_WRITE_MODE_MARKER:
                if ((encodeState = sharc_encode_write_mode_marker(out, state)))
                    return encodeState;
                break;

            case SHARC_ENCODE_PROCESS_WRITE_DATA:
                inPositionBefore = in->position;
                outPositionBefore = out->position;

                hashEncodeState = sharc_hash_encode_process(in, out, SHARC_HASH_XOR_MASK_DISPERSION, &state->dictionaryData.dictionary_a, &state->hashEncodeState, flush);
                sharc_encode_update_totals(in, out, state, inPositionBefore, outPositionBefore);

                switch (hashEncodeState) {
                    case SHARC_HASH_ENCODE_STATE_STALL_ON_INPUT_BUFFER:
                        return SHARC_ENCODE_STATE_STALL_ON_INPUT_BUFFER;

                    case SHARC_HASH_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER:
                        return SHARC_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER;

                    case SHARC_HASH_ENCODE_STATE_INFO_NEW_BLOCK:
                        if (state->blockType == SHARC_BLOCK_TYPE_DEFAULT)
                            state->process = SHARC_ENCODE_PROCESS_WRITE_BLOCK_FOOTER;
                        else
                            state->process = SHARC_ENCODE_PROCESS_WRITE_BLOCK_HEADER;
                        break;

                    case SHARC_HASH_ENCODE_STATE_INFO_EFFICIENCY_CHECK:
                        state->process = SHARC_ENCODE_PROCESS_WRITE_MODE_MARKER;
                        break;

                    case SHARC_HASH_ENCODE_STATE_FINISHED:
                        if (state->blockType == SHARC_BLOCK_TYPE_DEFAULT) if ((encodeState = sharc_encode_write_block_footer(out, state)))
                            return encodeState;
                        return SHARC_ENCODE_STATE_FINISHED;

                    case SHARC_HASH_ENCODE_STATE_READY:
                        break;

                    default:
                        return SHARC_ENCODE_STATE_ERROR;
                }
        }
    }
}

SHARC_FORCE_INLINE SHARC_ENCODE_STATE sharc_encode_finish(sharc_encode_state *restrict state) {
    if (sharc_hash_encode_finish(&state->hashEncodeState))
        return SHARC_ENCODE_STATE_ERROR;

    return SHARC_ENCODE_STATE_READY;
}