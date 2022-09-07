#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

uint8_t CZINOMEM = 0x01;
uint8_t TIFNOMEM = 0x02;
uint8_t CZICORPT = 0x11;

// TODO: handle case for compressed files

uint8_t *czi2tiff(FILE *fp) {
    /* store czi data as byte array in memory */
    FILE *fileptr;
    uint8_t *buffer;
    long filelen;

    fileptr = fopen("myfile.txt", "rb");  // open the file in binary mode
    fseek(fileptr, 0, SEEK_END);          // jump to the end of the file
    filelen = ftell(fileptr);             // get the current byte offset in the file
    rewind(fileptr);                      // jump back to the beginning of the file

    buffer = (uint8_t *)malloc(filelen * sizeof(uint8_t)); // enough memory for the file
    if (buffer == NULL) {
        errno = ENOMEM;
        fclose(fileptr); // close file
        return CZINOMEM;
    }
    fread(buffer, filelen, 1, fileptr); // read in the entire file
    fclose(fileptr); // close the file

    /* 

    allocate new memory for byte array in tiff format. 
    all segments are multipes of 32 bytes, therefore the worst case (most memory needed)
    occurs when all segments are ZISRAWSUBBLOCKS of length 32 bytes.
    therefore, upper bound on number of segments is filesize / 32.

    */

    /* loop through czi segments */

    size_t offset = 0; // read from first byte
    uint8_t *header;
    uint8_t *data;

    int endRead = 0;
    uint64_t zisraw = 0x005A4953524157; // checks for ZISRAW in segment ID
    uint64_t bblock = 0x0042424c4f434b; // checks if segment ID ends with BBLOCK (segment ID is ZISRAWSUBBLOCK)

    while (1) {
        header = buffer + offset;
        data = buffer + 16; // header is 16 bytes long

        uint64_t check = *header;
        check = check >> 2; // uint64_ts are 8 bytes long but we're looking at the first 6 bytes only
        if (check != zisraw) {
            // data is corrupt; all headers should begin with ZISRAW
            // throw an exception
            free(buffer);
            return CZICORPT;
        }

        uint64_t segtype = (*(header + 1)) >> 2; // 2 byte shift to get rid of data picked up from the next section
        if (segtype != bblock) { // skips any segments not marked ZISRAWSUBBLOCK 
            continue;
        }

        long allocSize = *(header + 16);
        offset += (allocSize + 16); // allocated size plus header size
        if (offset > filelen) {
            endRead = 1; // pre-check if this segment is the final segment
        }

        /* translate czi data into tiff data */
        /* write translated data to allocated memory for tiff data */
        
        if (endRead) {
            break;
        }
    }

    free(buffer);
}

