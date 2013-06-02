/*
 * Copyright (c) 2013, Guillaume Voirin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Centaurean nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Centaurean BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Sharc
 * www.centaurean.com
 *
 * 01/06/13 17:31
 */

#include "hash_cipher.h"

FORCE_INLINE void writeSignature(/*const bool bit*/) {
    signature |= 1 << /*(31 - */state/*)*/;
}

FORCE_INLINE bool flush() {
    if((outPosition + 4 + 128) > outSize)
        return FALSE;
    *(unsigned int*)(outBuffer + outPosition) = signature;
    outPosition += 4;
#pragma unroll(32)
    for(byte b = 0; b < state; b ++) {
        unsigned int chunk = chunks[b];
        switch((signature >> /*(31 - */b/*)*/) & 0x1) {
            case 0:
                *(unsigned int*)(outBuffer + outPosition) = chunk;
                outPosition += 4;
                break;
            case 1:
                *(unsigned short*)(outBuffer + outPosition) = chunk;
                outPosition += 2;
                break;
        }
    }
    inPosition += (state << 2);
    return TRUE;
}

FORCE_INLINE void reset() {
    state = 0;
    signature = 0;
}

FORCE_INLINE void resetDictionary() {
    for(unsigned int i = 0; i < (1 << HASH_BITS); i ++)
        //*(unsigned int*)&dictionary[i] &= 0xFFFFFF;
        //dictionary[i].exists = 0;
        *(unsigned int*)&dictionary[i] = 0;
}

FORCE_INLINE bool checkState() {
    switch(state) {
        case 32:
            if(!flush())
                return FALSE;
            reset();
            break;
    }
    return TRUE;
}

FORCE_INLINE void computeHash(unsigned int* hash, const unsigned int value) {
    *hash = HASH_OFFSET_BASIS;
    *hash ^= value;
    *hash *= HASH_PRIME;
    *hash = /*(*hash >> (32 - HASH_BITS));*/(*hash >> (32 - HASH_BITS)) ^ (*hash & 0xFFFF);
}

FORCE_INLINE bool updateEntry(ENTRY* entry, const unsigned int chunk, const unsigned int index) {
	*(unsigned int*)entry = (index & 0xFFFFFF) | MAX_BUFFER_REFERENCES;
	//writeSignature(FALSE);
    chunks[state++] = chunk;
    return checkState();
}