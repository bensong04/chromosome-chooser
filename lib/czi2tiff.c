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
                 uint64_t check = *(uint64_t *)(cur_byte) >> 2*8; // looking at the first 6 bytes of the header only
                 if (check == zisraw) { // this means the byte starts a header
                    uint64_t spec_check = *(uint64_t *)(buffer + offset + 8); // check the next 8 bytes (after 8 bytes of the header)
                    if (spec_check >> 2*8 != bblock) { // non-data segment 
                        // skip to next segment
                        offset += 32; // skip over header
                        switch ((spec_check >> 6*8)) {
                            case 0x0000000000004c45 : // 'le' -> ZISRAWFI[LE]
                                offset += 512; // skip to next segment
                                break;
                            case 0x0000000000005441 : // 'ta' -> ZISRAWME[TA]DATA OR ZISRAWAT[TA]CH
                                offset += (256 + read_value_from_four_byte_buff(buffer + offset)); 
                                break;
                                // size of xml AND attachment is immediately after header and is 4 bytes
                                // there is also a fixed 256-byte segment
                            case 0x0000000000005245 : // 're' -> ZISRAWDI[RE]CTORY
                                offset += (128 + read_value_from_four_byte_buff(buffer + offset)*128);
                                break;
                                // size of directory is represented by number of entries
                                // number of entries is immediately after header and is 4 bytes
                                // each entry is 128 bytes
                                // there is also a fixed 128-byte segment
                            case 0x0000000000005444 : // 'td' -> ZISRAWAT[TD]IR
                                offset += (128 + read_value_from_four_byte_buff(buffer + offset)*128);
                                break;
                                // exact same as regular directory
                                // size of directory is represented by number of entries
                                // number of entries is immediately after header and is 4 bytes
                                // each entry is 128 bytes
                                // there is also a fixed 128-byte segment
                            default :
                                break;
                        }
                        // start reading from new offset
                        continue;
                    }
                    else { // the byte starts the header of a data segment
                        offset += 32; // skip the header entirely
                        // refactor; we're adding 32 bytes to the offset in either case
                        // we don't need to call continue, because we're now going to call code that
                        // analyze data bytes.
                    }
                 }
            }
            
            // code for if the byte is just data


            offset++; // offset should be incremented after each iteration of the loop
        }
        fileptr += stepsize; // increase offset of file pointer
        free(buffer);
    }
    fclose(fileptr); // close file
}

/*
code graveyard
old case code for 'ta' in the 9th and 10th bytes in the SID.
removed because the code for skipping over a zisrawattach and a zisrawmetadata are exactly the same

// shift four bytes to check 'tada' vs 'tach'
if(spec_check >> 4*8 == 0x0000000054414348) { // 'tach' -> ZISRAWAT[TACH]
    offset += (256 + read_value_from_four_byte_buff(buffer + offset));
    break;
}
offset += (256 + read_value_from_four_byte_buff(buffer + offset)); 
break;
// size of xml is immediately after header and is 4 bytes
// there is also a fixed 256-byte segment

*/