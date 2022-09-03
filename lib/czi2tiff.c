#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

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
        // malloc failed for some reason
    }
    fread(buffer, filelen, 1, fileptr); // read in the entire file
    fclose(fileptr); // close the file

    /* allocate new memory for byte array in tiff format */

    /* loop through czi segments */
    size_t offset = 0; // read from first byte
    uint8_t *header;
    uint8_t *data;

    int endRead = 0;
    uint64_t zisraw = 0x005A4953524157; // checks for ZISRAW in segment ID

    while (1) {
        header = buffer + offset;
        data = buffer + 16; // header is 16 bytes long

        uint64_t check = *header;
        check = check >> 2; // longs are 8 bytes long but we're looking at the first 6 bytes only
        if (check != zisraw) {
            // data is corrupt; all headers should begin with ZISRAW
            // throw an exception
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

