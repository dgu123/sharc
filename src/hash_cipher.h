/*
 * Copyright (c) 2013, Guillaume Voirin
 * All rights reserved.
 *
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in licenses/commercial.txt.
 *
 * Centaurean SHARC
 * www.centaurean.com/sharc
 *
 * 01/06/13 17:27
 */

#ifndef HASH_CIPHER_H
#define HASH_CIPHER_H

#include "cipher.h"

#include <stdio.h>

#define HASH_BITS                   16
#define HASH_OFFSET_BASIS           2166115717	//14695981039346656037//
#define HASH_PRIME                  16777619	//1099511628211//

#define MAX_BUFFER_REFERENCES       (1 << 24) // 3 bytes, = ENTRY offset size
#define PREFERRED_BUFFER_SIZE       MAX_BUFFER_REFERENCES >> 2 // Has to be < to MAX_BUFFER_REFERENCES << 2
#define MAX_BUFFER_SIZE             PREFERRED_BUFFER_SIZE

#pragma pack(push)
#pragma pack(4)
typedef struct {
	byte offset[3];
    byte exists;
} ENTRY;
#pragma pack(pop)

ENTRY dictionary[1 << HASH_BITS]/*[MAX_PARALLELISM]*/;
uint64_t signature/*[MAX_PARALLELISM]*/;
byte state/*[MAX_PARALLELISM]*/;
uint32_t chunks[64]/*[MAX_PARALLELISM]*/;
uint32_t hash/*[MAX_PARALLELISM]*/;

void writeSignature(const byte);
bool flush(BYTE_BUFFER*, BYTE_BUFFER*, const byte);
void reset(const byte);
void resetDictionary(const byte);
bool checkState(BYTE_BUFFER*, BYTE_BUFFER*, const byte);
void computeHash(uint32_t*, const uint32_t, const uint32_t);
bool updateEntry(BYTE_BUFFER*, BYTE_BUFFER*, ENTRY*, const uint32_t, const uint32_t, const byte);
bool kernel(BYTE_BUFFER*, BYTE_BUFFER*, const uint32_t, const uint32_t, const uint32_t*, const uint32_t, const byte);

bool hashEncode(BYTE_BUFFER*, BYTE_BUFFER*, const uint32_t, const byte);
//bool hashDecode(byte*, uint32_t, const uint32_t);

#endif