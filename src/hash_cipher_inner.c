/*
 * Copyright (c) 2013, Guillaume Voirin (gvoirin@centaurean.com)
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in licenses/commercial.txt.
 *
 * Centaurean SHARC
 * www.centaurean.com/sharc
 *
 * 01/06/13 17:31
 */

#include "hash_cipher_inner.h"

FORCE_INLINE void writeSignature(uint64_t* signature, const byte* state) {
    *signature |= ((uint64_t)1) << *state;
}

FORCE_INLINE void flush(BYTE_BUFFER* in, BYTE_BUFFER* out, const uint64_t* signature, const byte* state, const uint32_t* signaturePointer) {
    *(uint64_t*)(out->pointer + *signaturePointer) = *signature;
    in->position += (*state << 2);
}

FORCE_INLINE bool reset(BYTE_BUFFER* out, uint64_t* signature, byte* state, uint32_t* signaturePointer) {
    *signaturePointer = out->position;
    out->position += 8;
    if((out->position + 256) > out->size)
        return FALSE;
    *state = 0;
    *signature = 0;
    return TRUE;
}

FORCE_INLINE void resetDictionary(ENTRY* dictionary) {
    for(uint32_t i = 0; i < (1 << HASH_BITS); i ++)
        *(uint32_t*)&dictionary[i] = 0;
}

FORCE_INLINE bool checkState(BYTE_BUFFER* in, BYTE_BUFFER* out, uint64_t* signature, byte* state, uint32_t* signaturePointer) {
    switch(*state) {
        case 64:
            flush(in, out, signature, state, signaturePointer);
            if(reset(out, signature, state, signaturePointer) ^ 0x1)
                return FALSE;
            break;
    }
    return TRUE;
}

FORCE_INLINE void computeHash(uint32_t* hash, const uint32_t value, const uint32_t xorMask) {
    *hash = HASH_OFFSET_BASIS;
    *hash ^= (value ^ xorMask);
    *hash *= HASH_PRIME;
    *hash = (*hash >> (32 - HASH_BITS)) ^ (*hash & 0xFFFF);
}

FORCE_INLINE bool updateEntry(BYTE_BUFFER* in, BYTE_BUFFER* out, ENTRY* entry, const uint32_t chunk, const uint32_t index, uint64_t* signature, byte* state, uint32_t* signaturePointer) {
	*(uint32_t*)entry = (index & 0xFFFFFF) | MAX_BUFFER_REFERENCES;
    *(uint32_t*)(out->pointer + out->position) = chunk;
    out->position += 4;
    *state = *state + 1;
    return checkState(in, out, signature, state, signaturePointer);
}

FORCE_INLINE bool kernel(BYTE_BUFFER* in, BYTE_BUFFER* out, const uint32_t chunk, const uint32_t xorMask, const uint32_t* buffer, const uint32_t index, ENTRY* dictionary, uint32_t* hash, uint64_t* signature, byte* state, uint32_t* signaturePointer) {
    computeHash(hash, chunk, xorMask);
    ENTRY* found = &dictionary[*hash];
    if((*(uint32_t*)found) & MAX_BUFFER_REFERENCES) {
        if(chunk ^ buffer[*(uint32_t*)found & 0xFFFFFF]) {
            if(updateEntry(in, out, found, chunk, index, signature, state, signaturePointer) ^ 0x1)
                return FALSE;
        } else {
            writeSignature(signature, state);
            *(uint16_t*)(out->pointer + out->position) = (uint16_t)*hash;
            out->position += 2;
            *state = *state + 1;
            if(checkState(in, out, signature, state, signaturePointer) ^ 0x1)
                return FALSE;
        }
    } else {
        if(updateEntry(in, out, found, chunk, index, signature, state, signaturePointer) ^ 0x1)
            return FALSE;
    }
    return TRUE;
}

FORCE_INLINE bool hashEncode(BYTE_BUFFER* in, BYTE_BUFFER* out, const uint32_t xorMask) {
    ENTRY dictionary[1 << HASH_BITS];
    uint64_t signature;
    uint32_t signaturePointer;
    byte state;
    uint32_t hash;

    reset(out, &signature, &state, &signaturePointer);
    resetDictionary(dictionary);
    
    const uint32_t* intInBuffer = (const uint32_t*)in->pointer;
    const uint32_t intInSize = in->size >> 2;
    
    for(uint32_t i = 0; i < intInSize; i ++)
        if(kernel(in, out, intInBuffer[i], xorMask, intInBuffer, i, dictionary, &hash, &signature, &state, &signaturePointer) ^ 0x1)
            return FALSE;
    
    flush(in, out, &signature, &state, &signaturePointer);
    
    const uint32_t remaining = in->size - in->position;
    for(uint32_t i = 0; i < remaining; i ++) {
        if(out->position < out->size - 1)
            out->pointer[out->position ++] = in->pointer[in->position ++];
        else
            return FALSE;
    }
    
    return TRUE;
}

FORCE_INLINE bool hashDecode(byte* a, uint32_t b, const uint32_t c) {
    /*reset();
     resetDictionary();
     
     uint32_t resultingSize = *(uint32_t*)_inBuffer;
     //if(resultingSize > outSize)
     //    routBuffer = realloc;
     prepareWorkspace(_inBuffer, _inSize, writeBuffer, resultingSize);
     
     
     fread((byte*)finalSize, sizeof(byte), 4, inFile);
     
     uint32_t totalWritten = 0;
     
     while(totalWritten < finalSize) {
     fread((byte*)finalSize, sizeof(byte), 4, inFile);
     uint64_t signature = *(uint64_t*) _inBuffer;
     }
     
     while(ftell(outFile) < limit) {
     uint64_t signature = *(uint64_t*) _inBuffer;
     for (uint32_t i = 0; i < 64; i ++) {
     bool mode = (signature >> i) & 0x1;
     switch(mode) {
     case FALSE:
     fwrite((_inBuffer + inPosition), sizeof(byte), 4, outFile);
     //inPosition
     break;
     }
     }
     }*/
    return TRUE;
}
