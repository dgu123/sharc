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
 * 01/09/13 19:43
 */

#include "block_footer.h"

SHARC_FORCE_INLINE sharc_bool sharc_block_header_read(sharc_byte_buffer* restrict in, sharc_block_footer* restrict blockHeader) {
    return true;
}


SHARC_FORCE_INLINE uint32_t sharc_block_header_write(sharc_byte_buffer* out, const uint32_t hashsum) {
    return sizeof(sharc_block_footer);
}