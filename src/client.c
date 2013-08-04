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
 * 03/07/13 14:55
 */

#include "client.h"

SHARC_FORCE_INLINE FILE* sharc_checkOpenFile(const char* fileName, const char* options, const sharc_bool checkOverwrite) {
    if(checkOverwrite && access(fileName, F_OK) != -1) {
        printf("File %s already exists. Do you want to overwrite it (y/N) ? ", fileName);
        switch(getchar()) {
            case 'y':
                break;
            default:
                exit(0);
        }
    }
    FILE* file = fopen(fileName, options);
    if(file == NULL) {
        printf("Unable to open file : %s\n", fileName);
        exit(0);
    }
    return file;
}

SHARC_FORCE_INLINE void sharc_version() {
    printf("Centaurean Sharc %i.%i.%i\n", SHARC_MAJOR_VERSION, SHARC_MINOR_VERSION, SHARC_REVISION);
    printf("Built for %s (%s endian system, %li bits) using GCC %d.%d.%d, %s %s\n", SHARC_PLATFORM_STRING, SHARC_ENDIAN_STRING, 8 * sizeof(void*), __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__, __DATE__, __TIME__);
}

SHARC_FORCE_INLINE void sharc_usage() {
    sharc_version();
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
    printf("  -i, --stdin                       Read from stdin\n");
    printf("  -o, --stdout                      Write to stdout\n");
    printf("  -v, --version                     Display version information\n");
    printf("  -h, --help                        Display this help\n");
    exit(0);
}

SHARC_FORCE_INLINE void sharc_clientCompress(SHARC_CLIENT_IO* in, SHARC_CLIENT_IO* out, const sharc_byte attemptMode, const uint32_t blockSize, const sharc_bool prompting) {
    struct stat64 attributes;
    char outFileName[strlen(in->name) + 6];
    
    if(in->type == SHARC_TYPE_FILE) {
        if(out->type == SHARC_TYPE_FILE) {
            sprintf(outFileName, "%s.sharc", in->name);
            
            out->name = outFileName;
        }
        
        in->stream = sharc_checkOpenFile(in->name, "rb", SHARC_FALSE);
        
        stat64(in->name, &attributes);
    } else {
        if(out->type == SHARC_TYPE_FILE)
            out->name = SHARC_STDIN_COMPRESSED;
        
        in->stream = stdin;
        in->name = SHARC_STDIN;
    }
    
    if(out->type == SHARC_TYPE_FILE)
        out->stream = sharc_checkOpenFile(out->name, "wb", prompting);
    else {
        out->stream = stdout;
        out->name = SHARC_STDOUT;
    }
    
    SHARC_BYTE_BUFFER read = sharc_createByteBuffer(sharc_readBuffer, 0, blockSize);
    SHARC_BYTE_BUFFER inter = sharc_createByteBuffer(sharc_interBuffer, 0, blockSize);
    SHARC_BYTE_BUFFER write = sharc_createByteBuffer(sharc_writeBuffer, 0, blockSize);
    
    SHARC_CHRONO chrono;
    sharc_chronoStart(&chrono);
    sharc_compress(in->stream, out->stream, &read, &inter, &write, attemptMode, blockSize, attributes);
    sharc_chronoStop(&chrono);
    
    if(out->type == SHARC_TYPE_FILE) {
        const double elapsed = sharc_chronoElapsed(&chrono);
        
        uint64_t totalWritten = ftello(out->stream);
        fclose(out->stream);
        
        if(in->type == SHARC_TYPE_FILE) {
            uint64_t totalRead = ftello(in->stream);
            fclose(in->stream);
        
            double ratio = (100.0 * totalWritten) / totalRead;
            double speed = (1.0 * totalRead) / (elapsed * 1024.0 * 1024.0);
            printf("Compressed %s to %s, %llu bytes in, %llu bytes out, ", in->name, out->name, totalRead, totalWritten);
            printf("Ratio out / in = %.1lf%%, Time = %.3lf s, Speed = %.0lf MB/s\n", ratio, elapsed, speed);
        } else
            printf("Compressed %s to %s, %llu bytes written.\n", in->name, out->name, totalWritten);
    }
}

SHARC_FORCE_INLINE void sharc_clientDecompress(SHARC_CLIENT_IO* in, SHARC_CLIENT_IO* out, const sharc_bool prompting) {
    const unsigned long originalFileNameLength = strlen(in->name) - 6;
    char outFileName[originalFileNameLength];
    
    if(in->type == SHARC_TYPE_FILE) {
        if(out->type == SHARC_TYPE_FILE) {
            outFileName[0] = '\0';
            strncat(outFileName, in->name, originalFileNameLength);
            
            out->name = outFileName;
        }
    
        in->stream = sharc_checkOpenFile(in->name, "rb", SHARC_FALSE);
    } else {
        if(out->type == SHARC_TYPE_FILE)
            out->name = SHARC_STDIN;
        
        in->stream = stdin;
        in->name = SHARC_STDIN;
    }
    
    if(out->type == SHARC_TYPE_FILE)
        out->stream = sharc_checkOpenFile(out->name, "wb", prompting);
    else {
        out->stream = stdout;
        out->name = SHARC_STDOUT;
    }
    
    SHARC_BYTE_BUFFER read = sharc_createByteBuffer(sharc_readBuffer, 0, 0);
    SHARC_BYTE_BUFFER inter = sharc_createByteBuffer(sharc_interBuffer, 0, 0);
    SHARC_BYTE_BUFFER write = sharc_createByteBuffer(sharc_writeBuffer, 0, 0);
    
    SHARC_CHRONO chrono;
    sharc_chronoStart(&chrono);
    SHARC_FILE_HEADER fileHeader = sharc_decompress(in->stream, out->stream, &read, &inter, &write);
    sharc_chronoStop(&chrono);
    
    if(out->type == SHARC_TYPE_FILE) {
        const sharc_bool notFromStdin = strcmp(out->name, SHARC_STDIN);
        const double elapsed = sharc_chronoElapsed(&chrono);
        
        uint64_t totalWritten = ftello(out->stream);
        fclose(out->stream);
        
        if(notFromStdin)
            sharc_restoreFileAttributes(fileHeader, out->name);
        
        if(in->type == SHARC_TYPE_FILE) {
            uint64_t totalRead = ftello(in->stream);
            fclose(in->stream);
        
            if(notFromStdin)
                if(totalWritten != fileHeader.originalFileSize)
                    sharc_error("Input file is corrupt !");
        
            double speed = (1.0 * totalWritten) / (elapsed * 1024.0 * 1024.0);
            printf("Decompressed %s to %s, %llu bytes in, %llu bytes out, ", in->name, out->name, totalRead, totalWritten);
            printf("Time = %.3lf s, Speed = %.0lf MB/s\n", elapsed, speed);
        } else
            printf("Decompressed %s to %s, %llu bytes written.\n", in->name, out->name, totalWritten);
    }
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stderr), _O_BINARY);
#endif

    if(argc <= 1)
        sharc_usage();
    
    sharc_byte action = SHARC_ACTION_COMPRESS;
	sharc_byte mode = SHARC_MODE_SINGLE_PASS;
    sharc_byte prompting = SHARC_PROMPTING;
    SHARC_CLIENT_IO in;
    in.type = SHARC_TYPE_FILE;
    SHARC_CLIENT_IO out;
    out.type = SHARC_TYPE_FILE;
    
    size_t argLength;
    for(int i = 1; i < argc; i ++) {
        switch(argv[i][0]) {
            case '-':
                argLength = strlen(argv[i]);
                if(argLength < 2)
                    sharc_usage();
                switch(argv[i][1]) {
                    case 'c':
                        if(argLength == 2) {
                            mode = SHARC_MODE_SINGLE_PASS;
                            break;
                        }
                        if(argLength != 3)
                            sharc_usage();
                        mode = argv[i][2] - '0';
                        break;
                    case 'd':
                        action = SHARC_ACTION_DECOMPRESS;
                        break;
                    case 'n':
                        prompting = SHARC_NO_PROMPTING;
                        break;
                    case 'i':
                        in.type = SHARC_TYPE_STREAM;
                        break;
                    case 'o':
                        out.type = SHARC_TYPE_STREAM;
                        break;
                    case 'v':
                        sharc_version();
                        exit(0);
                        break;
                    case 'h':
                        sharc_usage();
                        break;
					case '-':
						if(argLength < 3)
							sharc_usage();
						switch(argv[i][2]) {
							case 'c':
                                if(argLength == 10)
                                    break;
								if(argLength != 12)
									sharc_usage();
								mode = argv[i][11] - '0';
								break;
							case 'd':
                                if(argLength != 12)
                                    sharc_usage();
								action = SHARC_ACTION_DECOMPRESS;
								break;
                            case 'n':
                                if(argLength != 11)
                                    sharc_usage();
                                prompting = SHARC_NO_PROMPTING;
                                break;
                            case 's':
                                if(argLength == 7)
                                    in.type = SHARC_TYPE_STREAM;
                                else if(argLength == 8)
                                    out.type = SHARC_TYPE_STREAM;
                                else
                                    sharc_usage();
                                break;
                            case 'v':
                                if(argLength != 9)
                                    sharc_usage();
                                sharc_version();
                                exit(0);
                                break;
                            case 'h':
                                sharc_usage();
                                break;
							default:
								sharc_usage();
						}
						break;
                }
                break;
            default:
                in.name = argv[i];
                switch(action) {
                    case SHARC_ACTION_DECOMPRESS:
                        sharc_clientDecompress(&in, &out, prompting);
                        break;
                    default:
                        sharc_clientCompress(&in, &out, mode, SHARC_PREFERRED_BUFFER_SIZE, prompting);
                        break;
                }
                break;
        }
    }
    
    if(in.type == SHARC_TYPE_STREAM) {
        switch(action) {
            case SHARC_ACTION_DECOMPRESS:
                sharc_clientDecompress(&in, &out, prompting);
                break;
            default:
                sharc_clientCompress(&in, &out, mode, SHARC_PREFERRED_BUFFER_SIZE, prompting);
                break;
        }
    }
}


