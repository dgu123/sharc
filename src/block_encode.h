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
 * 29/08/13 12:32
 */

#ifndef SHARC_BLOCK_ENCODE_H
#define SHARC_BLOCK_ENCODE_H

#include "block_footer.h"
#include "block_header.h"
#include "dictionary.h"
#include "hash_encode.h"
#include "header.h"
#include "footer.h"
#include "block_mode_marker.h"

typedef enum {
    SHARC_BLOCK_ENCODE_STATE_READY = 0,
    SHARC_BLOCK_ENCODE_STATE_STALL_ON_OUTPUT_BUFFER,
    SHARC_BLOCK_ENCODE_STATE_STALL_ON_INPUT_BUFFER,
    SHARC_BLOCK_ENCODE_STATE_ERROR
} SHARC_BLOCK_ENCODE_STATE;

typedef enum {
    SHARC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_HEADER,
    SHARC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_MODE_MARKER,
    SHARC_BLOCK_ENCODE_PROCESS_WRITE_BLOCK_FOOTER,
    SHARC_BLOCK_ENCODE_PROCESS_WRITE_LAST_BLOCK_FOOTER,
    SHARC_BLOCK_ENCODE_PROCESS_WRITE_DATA,
    SHARC_BLOCK_ENCODE_PROCESS_FINISHED
} SHARC_BLOCK_ENCODE_PROCESS;

#pragma pack(push)
#pragma pack(4)
typedef struct {
    uint_fast64_t inStart;
    uint_fast64_t outStart;
} sharc_block_encode_current_block_data;

typedef struct {
    sharc_dictionary dictionary;
    uint_fast32_t resetCycle;
    void (*dictionary_reset)(sharc_dictionary *);
} sharc_block_encode_dictionary_data;

typedef struct {
    SHARC_BLOCK_ENCODE_PROCESS process;
    SHARC_BLOCK_MODE targetMode;
    SHARC_BLOCK_MODE currentMode;
    SHARC_BLOCK_TYPE blockType;

    uint_fast64_t totalRead;
    uint_fast64_t totalWritten;

    sharc_hash_encode_state hashEncodeState;
    sharc_block_encode_current_block_data currentBlockData;
    sharc_block_encode_dictionary_data dictionaryData;
} sharc_block_encode_state;
#pragma pack(pop)

SHARC_BLOCK_ENCODE_STATE sharc_block_encode_init(sharc_block_encode_state *, const SHARC_BLOCK_MODE, const SHARC_BLOCK_TYPE, void (*)(sharc_dictionary *));
SHARC_BLOCK_ENCODE_STATE sharc_block_encode_process(sharc_byte_buffer *, sharc_byte_buffer *, sharc_block_encode_state *, const sharc_bool, const uint32_t);
SHARC_BLOCK_ENCODE_STATE sharc_block_encode_finish(sharc_block_encode_state *);

#endif