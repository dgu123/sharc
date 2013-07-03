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
 * 01/06/13 13:08
 */

#include "sharc.h"

FORCE_INLINE void compress(const char* inFileName, const byte attemptMode, const uint32_t blockSize, const bool prompting) {
    char outFileName[strlen(inFileName) + 6];
    
    outFileName[0] = '\0';
    strcat(outFileName, inFileName);
    strcat(outFileName, ".sharc");
    
    FILE* inFile = checkOpenFile(inFileName, "rb", FALSE);
    FILE* outFile = checkOpenFile(outFileName, "wb+", prompting);
    
    ENCODING_RESULT result;
    CHRONO chrono;
    chronoStart(&chrono);
    
	struct stat attributes;
    stat(inFileName, &attributes);
    
    FILE_HEADER fileHeader = createFileHeader(blockSize, attributes);
    fwrite(&fileHeader, sizeof(FILE_HEADER), 1, outFile);
    
    BYTE_BUFFER in = createByteBuffer(readBuffer[1]/*[nThread]*/, 0, blockSize);
    BYTE_BUFFER inter = createByteBuffer(interBuffer[1], 0, blockSize);
    BYTE_BUFFER out = createByteBuffer(writeBuffer[1]/*[nThread]*/, 0, blockSize);
    
    while((in.size = (uint32_t)fread(in.pointer, sizeof(byte), blockSize, inFile)) > 0) {
        result = sharcEncode(&in, &inter, &out, attemptMode);
        
        BLOCK_HEADER blockHeader = createBlockHeader(result.reachableMode, result.out->position);
        fwrite(&blockHeader, sizeof(BLOCK_HEADER), 1, outFile);
        fwrite(result.out->pointer, sizeof(byte), result.out->position, outFile);
        
        rewindByteBuffer(&in);
        rewindByteBuffer(&out);
    }
    chronoStop(&chrono);
    const double elapsed = chronoElapsed(&chrono);
    
	uint64_t totalRead = ftell(inFile);
    uint64_t totalWritten = ftell(outFile);
    
    fclose(inFile);
    fclose(outFile);

    double ratio = (1.0 * totalWritten) / totalRead;
    double speed = (1.0 * totalRead) / (elapsed * 1024.0 * 1024.0);
    printf("Compressed file %s, %lli bytes in, %lli bytes out, ", inFileName, totalRead, totalWritten);
    printf("Ratio out / in = %g, Time = %.3lf s, Speed = %f MB/s\n", ratio, elapsed, speed);
} 

FORCE_INLINE void decompress(const char* inFileName, const bool prompting) {
    unsigned long originalFileNameLength = strlen(inFileName) - 6;
    char outFileName[originalFileNameLength];

    outFileName[0] = '\0';
    strncat(outFileName, inFileName, originalFileNameLength);
    
    FILE* inFile = checkOpenFile(inFileName, "rb", FALSE);
    FILE* outFile = checkOpenFile(outFileName, "wb+", prompting);
    
    CHRONO chrono;
    chronoStart(&chrono);
    
    FILE_HEADER fileHeader = readFileHeader(inFile);
    BYTE_BUFFER in = createByteBuffer(readBuffer[1]/*[nThread]*/, 0, 0);
    BYTE_BUFFER inter = createByteBuffer(interBuffer[1], 0, 0);
    BYTE_BUFFER out = createByteBuffer(writeBuffer[1]/*[nThread]*/, 0, fileHeader.bufferSize);
    
    BLOCK_HEADER blockHeader;
    while(readBlockHeaderFromFile(&blockHeader, inFile) > 0) {
        in.size = (uint32_t)fread(in.pointer, sizeof(byte), blockHeader.nextBlock, inFile);
        switch(blockHeader.mode) {
            case MODE_COPY:
                fwrite(in.pointer, sizeof(byte), in.size, outFile);
                break;
            default:
                if(sharcDecode(&in, &inter, &out, blockHeader.mode) ^ 0x1)
                    error("Unable to decompress !");
                fwrite(out.pointer, sizeof(byte), out.position, outFile);
                break;
        }
        
        rewindByteBuffer(&in);
        rewindByteBuffer(&out);
    }
    chronoStop(&chrono);
    const double elapsed = chronoElapsed(&chrono);
    
    uint64_t totalRead = ftell(inFile);
    uint64_t totalWritten = ftell(outFile);
    
    fclose(inFile);
    fclose(outFile);
    
    if(totalWritten != fileHeader.originalFileSize)
        error("Input file is corrupt !");
    
    restoreFileAttributes(fileHeader, outFileName);
    
    double speed = (1.0 * totalWritten) / (elapsed * 1024.0 * 1024.0);
    printf("Decompressed file %s, %lli bytes in, %lli bytes out, ", inFileName, totalRead, totalWritten);
    printf("Time = %.3lf s, Speed = %f MB/s\n", elapsed, speed);
}

FORCE_INLINE void version() {
    printf("Centaurean Sharc %i.%i.%i\n", MAJOR_VERSION, MINOR_VERSION, REVISION);
}

FORCE_INLINE void usage() {
    version();
    printf("Copyright (C) 2013 Guillaume Voirin\n");
    printf("Usage : sharc [OPTIONS]... [FILES]...\n")   ;
    printf("Superfast archiving of files.\n\n");
    printf("Available options :\n");
    printf("  -c[LEVEL], --compress[=LEVEL]     Compress files using LEVEL if specified (default)\n");
	printf("                                    LEVEL can have the following values :\n");
	printf("                                    0 = Fastest compression algorithm (default)\n");
	printf("                                    1 = Better compression (dual pass), slightly slower\n");
	printf("  -d, --decompress                  Decompress files\n");
    printf("  -n, --no-prompt                   Overwrite without prompting\n");
    printf("  -v, --version                     Display version information\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    if(argc <= 1)
		usage();

    byte action = ACTION_COMPRESS;
	byte mode = MODE_SINGLE_PASS;
    byte prompting = PROMPTING;
    
    size_t argLength;
    for(int i = 1; i < argc; i ++) {
        switch(argv[i][0]) {
            case '-':
                argLength = strlen(argv[i]);
                if(argLength < 2)
                    usage();
                switch(argv[i][1]) {
                    case 'c':
                        if(argLength == 2) {
                            mode = MODE_SINGLE_PASS;
                            break;
                        }
                        if(argLength != 3)
                            usage();
                        mode = argv[i][2] - '0';
                        break;
                    case 'd':
                        action = ACTION_DECOMPRESS;
                        break;
                    case 'n':
                        prompting = NO_PROMPTING;
                        break;
                    case 'v':
                        version();
                        exit(0);
                        break;
					case '-':
						if(argLength < 3)
							usage();
						switch(argv[i][2]) {
							case 'c':
                                if(argLength == 10)
                                    break;
								if(argLength != 12)
									usage();
								mode = argv[i][11] - '0';
								break;
							case 'd':
                                if(argLength != 12)
                                    usage();
								action = ACTION_DECOMPRESS;
								break;
                            case 'n':
                                if(argLength != 11)
                                    usage();
                                prompting = NO_PROMPTING;
                                break;
                            case 'v':
                                if(argLength != 9)
                                    usage();
                                version();
                                exit(0);
                                break;
							default:
								usage();
						}
						break;
                }
                break;
            default:
                switch(action) {
                    case ACTION_DECOMPRESS:
                        decompress(argv[i], prompting);
                        break;
                    default:
                        compress(argv[i], mode, PREFERRED_BUFFER_SIZE, prompting);
                        break;
                }
                break;
        }
    }
}
