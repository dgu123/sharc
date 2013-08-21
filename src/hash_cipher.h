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
 * 01/06/13 17:27
 */

#ifndef SHARC_HASH_CIPHER_H
#define SHARC_HASH_CIPHER_H

#include "byte_buffer.h"
#include "dictionary.h"

#include <stdio.h>

void sharc_writeSignature(uint64_t*, const sharc_byte*);
void sharc_flush(SHARC_BYTE_BUFFER*, SHARC_BYTE_BUFFER*, const uint64_t*, const sharc_byte*, const uint32_t*);
sharc_bool sharc_reset(SHARC_BYTE_BUFFER*, SHARC_BYTE_BUFFER*, uint64_t*, sharc_byte*, uint32_t*);
sharc_bool sharc_checkState(SHARC_BYTE_BUFFER*, SHARC_BYTE_BUFFER*, uint64_t*, sharc_byte*, uint32_t*);
void sharc_computeHash(uint32_t*, const uint32_t, const uint32_t);
sharc_bool sharc_updateEntry(SHARC_BYTE_BUFFER*, SHARC_BYTE_BUFFER*, SHARC_ENTRY*, const uint32_t, const uint32_t, uint64_t*, sharc_byte*, uint32_t*);
sharc_bool sharc_kernelEncode(SHARC_BYTE_BUFFER*, SHARC_BYTE_BUFFER*, const uint32_t, const uint32_t, const uint32_t*, const uint32_t, SHARC_ENTRY*, uint32_t*, uint64_t*, sharc_byte*, uint32_t*);
void sharc_kernelDecode(SHARC_BYTE_BUFFER*, SHARC_BYTE_BUFFER*, SHARC_ENTRY*, const uint32_t, const sharc_bool);

sharc_bool sharc_hashEncode(SHARC_BYTE_BUFFER*, SHARC_BYTE_BUFFER*, const uint32_t, SHARC_ENTRY*);
void sharc_byteCopy(SHARC_BYTE_BUFFER*, SHARC_BYTE_BUFFER*, const uint32_t);
sharc_bool sharc_hashDecode(SHARC_BYTE_BUFFER*, SHARC_BYTE_BUFFER*, const uint32_t, SHARC_ENTRY*);

#endif