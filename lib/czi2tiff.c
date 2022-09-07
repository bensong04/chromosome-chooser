#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

uint8_t CZINOMEM = 0x01;
uint8_t TIFNOMEM = 0x02;
uint8_t CZICORPT = 0x11;

short max_tiffs = 128; // maximum amount of TIFFs generated

long stepsize = 1048576; // stepsize*32 is the size of the buffer in memory (chunk size), in bytes
// since czi files are massive, we allocate small chunks of memory at a time,
// do operations on the czi data, write this data to the output file, release memory 
// then rinse and repeat.

uint32_t read_value_from_four_byte_buff(uint8_t *input) // note that czi is little-endian
{
    uint32_t x;
    x = 0xFF & input[0];
    x |= (0xFF & input[1]) << 8;
    x |= (0xFF & input[2]) << 16;
    x |= (uint32_t)(0xFF & input[3]) << 24;
    return x;
}

uint8_t *czi2tiff(FILE *fp) {
    FILE *fileptr = fopen("myfile.txt", "rb");
    uint8_t *buffer;

    uint8_t *tiff_table = (uint8_t *)malloc(max_tiffs); // each tiff data buffer in memory will have an associated 1 byte pointer
    uint8_t *recognized_z = (uint8_t *)malloc(max_tiffs); // will add to this array each time a new z-layer is encountered

    uint64_t zisraw = 0x00005A4953524157; // checks for ZISRAW in segment ID
    uint64_t bblock = 0x000042424c4f434b; // checks if segment ID ends with BBLOCK (segment ID is ZISRAWSUBBLOCK)

    while(!feof(fileptr)) {
        uint64_t offset = 0; // offset for where we're reading from in the chunk

        // open new chunk
        buffer = (uint8_t *)malloc(stepsize*32); // enough memory for the "chunk" that we're currently parsing.
        fread(buffer, 1, (size_t)(stepsize*32), fileptr); // read stepsize sequences into buffer
        
        // start parsing new chunk
        for( ; offset < stepsize ; ) { // we want to manually control offset incrementing
            uint8_t cur_byte = *(buffer + offset); // get current byte (with appropriate offset)
            if (offset % 32 == 0) {
                 // check if this byte is the start of a segment header
                 uint64_t check = (uint64_t *)(cur_byte) >> 2*8; // looking at the first 6 bytes of the header only
                 if (check == zisraw) { // this means the byte starts a header
                    uint64_t spec_check = (uint64_t *)(buffer + offset + 8) // check the next few bytes
                    if (spec_check >> 2*8 != bblock) { // non-data segment 
                        // skip to next segment
                        switch ((spec_check >> 6*8)) {
                            case 0x0000000000004c45 : // 'le' -> ZISRAWFILE
                                offset += 512; // skip to next segment
                                break;
                            case 0x0000000000005441 : // 'ta' -> ZISRAWMETADATA
                                offset += (256 + 16 + read_value_from_four_byte_buff(buffer + offset + 16)); 
                                // size of xml is 16 bytes past header start and is 4 bytes
                            // TODO: add in cases for other segment types
                            default :
                                break;
                        }
                        offset += 32; // skip to the next segment
                        continue;
                    }
                    else { // the byte starts the header of a data segment
                        offset += 16; // skip the header entirely
                    }
                 }
            }

            offset++; // offset should be incremented after each iteration of the loop
        }
        fileptr += stepsize; // increase offset of file pointer
        free(buffer);
    }
    fclose(fileptr); // close file
}

