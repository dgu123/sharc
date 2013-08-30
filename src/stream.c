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
 * 26/08/13 23:18
 */

#include "stream.h"

SHARC_FORCE_INLINE SHARC_STREAM_STATE sharc_stream_prepare(sharc_stream * restrict stream, char* restrict in, const uint32_t availableIn, char* restrict out, const uint32_t availableOut) {
    sharc_byte_buffer_encapsulate(&stream->in, (sharc_byte*)in, availableIn);
    sharc_byte_buffer_encapsulate(&stream->out, (sharc_byte*)out, availableOut);

    return SHARC_STREAM_STATE_OK;
}

SHARC_FORCE_INLINE SHARC_STREAM_STATE sharc_stream_compress_init(sharc_stream * stream, SHARC_COMPRESSION_MODE compressionMode) {
    if (sharc_encode_init(&stream->out, &stream->internal_state.internal_state, compressionMode, SHARC_ENCODE_TYPE_WITH_HEADER))
        return SHARC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;

    return SHARC_STREAM_STATE_OK;
}

SHARC_FORCE_INLINE SHARC_STREAM_STATE sharc_stream_check_conformity(sharc_stream *stream) {
    if ((uint32_t) &stream->in.pointer & 0x3)
        return SHARC_STREAM_STATE_ERROR_INPUT_BUFFER_NOT_PROPERLY_ALIGNED;

    if ((uint32_t) &stream->out.pointer & 0x1)
        return SHARC_STREAM_STATE_ERROR_OUTPUT_BUFFER_NOT_PROPERLY_ALIGNED;

    if(stream->out.size < SHARC_STREAM_MINIMUM_OUT_BUFFER_SIZE)
        return SHARC_STREAM_STATE_ERROR_OUTPUT_BUFFER_TOO_SMALL;

    return SHARC_STREAM_STATE_OK;
}

SHARC_FORCE_INLINE SHARC_STREAM_STATE sharc_stream_compress_continue(sharc_stream *stream) {
    SHARC_STREAM_STATE streamState = sharc_stream_check_conformity(stream);
    if(streamState)
        return streamState;

    if (stream->out.size & 0x3)
        return SHARC_STREAM_STATE_ERROR_INPUT_BUFFER_SIZE_NOT_MULTIPLE_OF_4;

    SHARC_ENCODE_STATE returnState = sharc_encode_continue(&stream->in, &stream->out, &stream->internal_state.internal_state);
    stream->in_total_read = stream->internal_state.internal_state.totalRead;
    stream->out_total_written = stream->internal_state.internal_state.totalWritten;

    switch (returnState) {
        case SHARC_ENCODE_STATE_STALL_INPUT_BUFFER:
            return SHARC_STREAM_STATE_STALL_ON_INPUT_BUFFER;

        case SHARC_ENCODE_STATE_STALL_OUTPUT_BUFFER:
            return SHARC_STREAM_STATE_STALL_ON_OUTPUT_BUFFER;

        default:
            return SHARC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }
}

SHARC_FORCE_INLINE SHARC_STREAM_STATE sharc_stream_compress_finish(sharc_stream * stream) {
    SHARC_STREAM_STATE streamState = sharc_stream_check_conformity(stream);
    if(streamState)
        return streamState;

    SHARC_ENCODE_STATE returnState = sharc_encode_finish(&stream->in, &stream->out, &stream->internal_state.internal_state);
    stream->in_total_read = stream->internal_state.internal_state.totalRead;
    stream->out_total_written = stream->internal_state.internal_state.totalWritten;

    switch (returnState) {
        case SHARC_ENCODE_STATE_FINISHED:
            return SHARC_STREAM_STATE_FINISHED;

        case SHARC_ENCODE_STATE_STALL_OUTPUT_BUFFER:
            return SHARC_STREAM_STATE_STALL_ON_OUTPUT_BUFFER;

        default:
            return SHARC_STREAM_STATE_ERROR_INVALID_INTERNAL_STATE;
    }
}

SHARC_FORCE_INLINE SHARC_STREAM_STATE sharc_stream_decompress_init(sharc_stream *);
SHARC_FORCE_INLINE SHARC_STREAM_STATE sharc_stream_decompress_continue(sharc_stream *);
SHARC_FORCE_INLINE SHARC_STREAM_STATE sharc_stream_decompress_finish(sharc_stream *);