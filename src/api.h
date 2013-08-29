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
 * 27/08/13 15:01
 */

#ifndef SHARC_API_H
#define SHARC_API_H

#include "globals.h"

#define SHARC_YES    1
#define SHARC_NO     0

/*
 * If you intend to use SHARC as a library, replace SHARC_NO by SHARC_YES.
 * client.c and client.h content will be masked : there will not be any main() function compiled.
 */
#define SHARC_USE_AS_LIBRARY    SHARC_NO

typedef enum {
    SHARC_API_COMPRESSION_MODE_FASTEST,
    SHARC_API_COMPRESSION_MODE_DUAL_PASS
} SHARC_API_COMPRESSION_MODE;

typedef enum {
    SHARC_API_OUTPUT_TYPE_WITH_HEADER,
    SHARC_API_OUTPUT_TYPE_WITHOUT_HEADER
} SHARC_API_OUTPUT_TYPE;

/*
    SHARC byte buffer utilities

    void sharc_byte_buffer_encapsulate(sharc_byte_buffer *, sharc_byte*, uint32_t, uint32_t);
    void sharc_byte_buffer_rewind(sharc_byte_buffer *);
*/

/*
    SHARC stream API functions :

    SHARC_STREAM_STATE sharc_stream_compressInit(sharc_stream *, SHARC_API_COMPRESSION_MODE, SHARC_API_OUTPUT_TYPE);
    SHARC_STREAM_STATE sharc_stream_decompressInit(sharc_stream *);
    SHARC_STREAM_STATE sharc_stream_compress(sharc_stream *);
    SHARC_STREAM_STATE sharc_stream_decompress(sharc_stream *);
    SHARC_STREAM_STATE sharc_stream_compressEnd(sharc_stream *);
    SHARC_STREAM_STATE sharc_stream_decompressEnd(sharc_stream *);
*/

/*
    SHARC buffers API functions :

    uint64_t sharc_api_buffers_max_compressed_total_length(uint64_t);
    uint64_t sharc_api_buffers_max_compressed_length_without_header(uint64_t);
    uint32_t sharc_api_buffers_compress(uint8_t*, uint64_t, uint8_t*, uint64_t*, uint32_t);
    uint32_t sharc_api_buffers_decompress(uint8_t*, uint64_t, uint8_t*, uint64_t*);
*/

#endif