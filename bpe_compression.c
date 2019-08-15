/****************************************/
/* Byte Pair Encoding Related Functions */
/****************************************/


/************/
/* Includes */
/************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "bpe_compression.h"


/***********/
/* Defines */
/***********/
#define DELIMITER      "`"
#define NUM_CH_CODES   0xF0  /* 240 (0-239) */

/* Usage Struct for New/Existing Codes */
typedef struct chCodeRecord chCodeRecord;
struct chCodeRecord{
    unsigned int inuse;
    unsigned int encoded;
    unsigned char encvalue[5];   // 2 byte BPE Encoded Value
    unsigned char utf8value[5];  // UTF8 Value
};


/* Globals */
static chCodeRecord ch_codes[NUM_CH_CODES];



/***********************/
/* Function Prototypes */
/***********************/

/* Loading Existing BPE Data */
int loadUtf8MappingForBPETable(char* bpeUtf8MappingTable);
int loadBPETable(char* bpeTableName, char* bpeUtf8MappingTable);
int utf8_to_bpe(char* utf8_in, unsigned char* rval);
int bpe_to_utf8(unsigned char bpe_in, unsigned char* utf8_out);
int _8bit_binary_to_utf8Text(unsigned char* bdata, unsigned int binSizeBytes, char** pText);


/* Compression/Decompression */
void decompressBPE(unsigned char* dst, unsigned char* src, unsigned int* nBytes);
void compressBPE(unsigned char* src, unsigned int* nBytes);




/*******************************************************************/
/* loadUtf8MappingForBPETable                                      */
/* Loads in the UTF-8 data associated with single character        */
/* encodings used by the BPE Table.                                */
/* Format for each line should be "index,utf-8_value"              */
/* Lines should end in line feed, tabs and spaces may be used for  */
/* separation.                                                     */
/* Values for Enter and Space are hard-coded.                      */
/*******************************************************************/
int loadUtf8MappingForBPETable(char* bpeUtf8MappingTable){

    static char linebuf[300];
    int index, numBytes, x;
    FILE* infile = NULL;
    char* ptr_line = NULL;
    memset(linebuf, 0, 300);

    /* Open the input file */
    infile = fopen(bpeUtf8MappingTable,"r");
    if(infile == NULL){
		printf("Error opening %s for reading.\n", bpeUtf8MappingTable);
        return -1;
    }

    /* Read until the end is detected */
    while(!feof(infile)){

        /* Get the index location */
        /* Skip lines that dont start with numbers */
        /* Assume they are comments, '#' is supposed to be a comment marker */
        fgets(linebuf, 299, infile);
        if (linebuf[0] == '#')
            continue;
        if (sscanf(linebuf, "%d", &index) != 1){
            fgets(linebuf, 299, infile);
            continue;
        }
        ptr_line = linebuf;

        /* Increment past numerical digits */
        while ((*ptr_line >= '0') && (*ptr_line <= '9'))
            ptr_line++;

        /* Index Bounds Check */
        if(index >= NUM_CH_CODES){
			break; /* All values are encoded past this point */
        }

        /* Dont bother to copy UTF-8 Value if entry is not being used */
        /* Or if it is being used but is a BPE encoding instead of a character */
        if((ch_codes[index].inuse == 0) || 
           ((ch_codes[index].inuse == 1) && (ch_codes[index].encoded == 1)) )
            continue;

        /* Init the array for the index */
        memset(ch_codes[index].utf8value,0,5);

        /* Read until a comma is found */
        while (*ptr_line != ','){
            ptr_line++;
        }
        ptr_line++;

        /* Read until not a space or tab */
        while(1){
            if ((*ptr_line == ' ') || (*ptr_line == '\t')){
                ptr_line++;
                continue;
            }
            break;
        }

        /* Check for end of line */
        if ((*ptr_line == '\r') || (*ptr_line == '\n'))
            *ptr_line = '\0';

        /* Have the first byte of the utf8 character, get # bytes */
        numBytes = numBytesInUtf8Char((unsigned char)*ptr_line);
        ch_codes[index].utf8value[0] = *ptr_line;

        /* Read the rest of the utf8 character */
        for(x=1; x < numBytes; x++){
            ptr_line++;
            ch_codes[index].utf8value[x] = *ptr_line;
        }
    }

    /* Fill in Fixed Entries */
    ch_codes[10].utf8value[0] = 0x0A; //Enter
	ch_codes[10].utf8value[1] = 0x00; //Enter
    ch_codes[32].utf8value[0] = 0x20; //Space
    ch_codes[32].utf8value[1] = 0x00; //Space

    fclose(infile);

    return 0;
}




/********************************************************/
/* Opens a file containing the Byte Pair Table Encoding */
/* File Format:  256 2-byte Entries (Binary).           */
/*                  0xFFFF <-- Not Used                 */
/*                  0xFFXX <-- 1 Byte Encoded           */
/*                  0xXXXX <-- 2 Byte BPE Encoding      */
/* Returns 0 on success, -1 on failure.                 */
/********************************************************/
int loadBPETable(char* bpeTableName, char* bpeUtf8MappingTable){

    FILE* tablefile = NULL;
    int x;

    /* Open BPE Encodings Table File */
    tablefile = fopen(bpeTableName, "rb");
    if (tablefile == NULL){
        printf("Error opening bpe table input file for encoding table\n");
        return -1;
    }

    for (x = 0; x < NUM_CH_CODES; x++){
        unsigned char byte1 = 0xFF;
        unsigned char byte2 = 0xFF;

        /* Init associated UTF8 and Encoding Values */
        memset(ch_codes[x].utf8value,0,5);
        memset(ch_codes[x].encvalue,0,5);

        /* Read 2 Bytes */
        if( fread(&byte1,1,1,tablefile) != 1){
            printf("Error reading BPE Byte #1, index = %d",x);
            fclose(tablefile);
            return -1;
        }
        if( fread(&byte2,1,1,tablefile) != 1){
            printf("Error reading BPE Byte #2, index = %d",x);
            fclose(tablefile);
            return -1;
        }

        /* Assign if the entry is being used or not */
        if((byte1 == 0xFF) && (byte2 == 0xFF)){
            ch_codes[x].inuse = 0;
        }
        else{
            ch_codes[x].inuse = 1;

            /* Assign Table Bytes that were Read */
            ch_codes[x].encvalue[0] = byte1;
            ch_codes[x].encvalue[1] = byte2;

            /* Flag if the value is a BPE encoding or just a 1-byte value */
            if(byte1 == 0xFF)
                ch_codes[x].encoded = 0;
            else
                ch_codes[x].encoded = 1;
        }


    }
    fclose(tablefile);

    /* Fill in UTF-8 Encodings */
	if (loadUtf8MappingForBPETable(bpeUtf8MappingTable) < 0){
        printf("Error loading UTF-8 Codes for BPE Entries\n");
        return -1;
    }

    return 0;
}




/***************************************************************/
/* utf8_to_bpe - Convert a UTF8 encoded character to its 8-bit */
/*               BPE equivalent.  Returns 0 on success, -1 on  */
/*               error.                                        */
/***************************************************************/
int utf8_to_bpe(char* utf8_in, unsigned char* rval){

    int x;

    for(x = 0; x < NUM_CH_CODES; x++){
        if((ch_codes[x].inuse == 1) && (ch_codes[x].encoded == 0)){
            /* Check to see if encoding for this value should take place */
            if(strcmp((char*)ch_codes[x].utf8value, (char*)utf8_in) == 0){
                *rval = ch_codes[x].encvalue[1];
                return 0;
            }
        }
    }

    /* Not Found */
    return -1;
}




/***************************************************************/
/* bpe_to_utf8 - Convert a BPE character to its UTF8           */
/*               equivalent.  Returns 0 on success, -1 on      */
/*               error.                                        */
/***************************************************************/
int bpe_to_utf8(unsigned char bpe_in, unsigned char* utf8_out){

    int x;

    for(x = 0; x < NUM_CH_CODES; x++){
        if((ch_codes[x].inuse == 1) && (ch_codes[x].encoded == 0)){
            /* Check to see if encoding for this value should take place */
            if(ch_codes[x].encvalue[1] == bpe_in){
                strcpy((char*)utf8_out, (char*)ch_codes[x].utf8value);
                return 0;
            }
        }
    }

    /* Not Found */
    utf8_out[0] = '\0';
    return -1;
}




/**************************************************************/
/* utf8Text_to_8bit_binary                                    */
/* Converts UTF-8 Text to 8-bit binary based on input table.  */
/* Performs no compression, just substitution.                */
/* Returns 0 on success, -1 on error.                         */
/**************************************************************/
int utf8Text_to_8bit_binary(char* pText, unsigned int* binSizeBytes){

    char* ptrInput, * ptrOutput;
    *binSizeBytes = 0;
    ptrInput = ptrOutput = pText;

    /* Parse through the input text, converting each UTF8 Code to an 8-bit Value */
    /* Abort if an 8-bit mapping is found to not exist.  This is an error.       */
    while(*ptrInput != '\0'){
        int numBytes;
		char tmpUtf8[5];
		memset(tmpUtf8, 0, 5);
        numBytes = numBytesInUtf8Char((unsigned char)*ptrInput);
		strncpy(tmpUtf8, ptrInput, numBytes);
        if(utf8_to_bpe(tmpUtf8, (unsigned char*)ptrOutput) != 0){
            printf("Error, failed to look up 8-bit code for UTF8 Code\n");
            return -1;
        }
        ptrInput += numBytes;
        ptrOutput++;
        *binSizeBytes = *binSizeBytes + 1;
    }

    return 0;
}




/**************************************************************/
/* _8bit_binary_to_utf8Text                                   */
/* Converts 8-bit binary based on input table to UTF-8 Text.  */
/* Performs no decompression, just substitution.              */
/* Returns 0 on success, -1 on error.                         */
/**************************************************************/
int _8bit_binary_to_utf8Text(unsigned char* bdata, unsigned int binSizeBytes, char** pText){

	int x;
    unsigned char* ptrOutput;
    unsigned char* pTemp = NULL;
    *pText = NULL;

    /* Allocate worst-case space - 4 bytes for each character plus terminator */
    pTemp = (unsigned char*)malloc(binSizeBytes*4 +1);
    if(pTemp == NULL){
        printf("Error allocating scratch space in _8bit_binary_to_utf8Text\n");
        return -1;
    }
    ptrOutput = pTemp;

    /* Parse through the input text, converting each UTF8 Code to an 8-bit Value */
    /* Abort if an 8-bit mapping is found to not exist.  This is an error.       */
    x = 0;
    while(binSizeBytes > 0){
        int numBytes;

        /* Retrieve utf-8 character */
        if(bpe_to_utf8(bdata[x], ptrOutput) != 0){
            printf("Error, failed to look up 8-bit code for UTF8 Code\n");
            free(pTemp);
            return -1;
        }

        /* Move the output pointer */
        numBytes = numBytesInUtf8Char(*ptrOutput);
        ptrOutput += numBytes;

        /* Adjust index/size left */
        x++;
        binSizeBytes--;
    }

    /* Copy Data from Scratch Space */
    *pText = (char*)malloc(strlen((char*)pTemp) + 1);
    if(pText == NULL){
        printf("Error allocating space in _8bit_binary_to_utf8Text\n");
        return -1;
    }
    strcpy(*pText,(char*)pTemp);
    free(pTemp);

    return 0;
}




/********************************************************************/
/* compressBPE - Re-encodes an 8-bit encoded string using byte pair */
/*               encoding.                                          */
/* src - Source input.  Compression is performed in-place.          */
/* nBytes - Pass in number of bytes in src by reference.  When the  */
/*          function has concluded execution, this number will be   */
/*          updated to reflect the comrpessed size in bytes.        */
/********************************************************************/
void compressBPE(unsigned char* src, unsigned int* nBytes){

    int x,y;
    unsigned char newCode;
    unsigned char newSeq[2];
    unsigned int srch_offset = 0;

    for(x = 0; x < NUM_CH_CODES; x++){

        /* Check to see if encoding for this value should take place */
        if((ch_codes[x].inuse == 1) && (ch_codes[x].encoded == 1)){
            newSeq[0] = ch_codes[x].encvalue[0];
            newSeq[1] = ch_codes[x].encvalue[1];
            newCode = x;
        }
        else{
            /* Skip */
            continue;
        }

        /* Perform Compression */
        srch_offset = 0;
        while (srch_offset < ((*nBytes) - 1)){
            if (memcmp(newSeq, &src[srch_offset], 2) == 0){
                src[srch_offset] = newCode;
                for (y = (int)(srch_offset + 1); y < (int)((*nBytes) - 1); y++){
                    src[y] = src[y + 1];
                }
                (*nBytes)--;
                src[(*nBytes)] = 0xFF;
                srch_offset = 0;
				x = 0;  /* Keep iterating until nothing can be further compressed */
			}
            else{
                srch_offset++;
            }
        }
    }

    return;
}




/******************************************************************/
/* decompressBPE - Decompresses a string using byte pair encoding */
/*                                                                */
/* dst - ptr to preallocated destination.                         */
/* src - ptr to source buffer.  end of src buffer signified by an */
/*       intput value >= 0xF0                                     */
/* nbytes - Number of bytes written to dst.  Passed by reference. */
/******************************************************************/
void decompressBPE(unsigned char* dst, unsigned char* src, unsigned int* nBytes){

    unsigned int x;

    /* Base Case */
    if (*src >= 0xF0)
        return;

    /* Loop */
    while (*src < 0xF0){
        int byte1, byte2;

        /* Read Next Byte Value from table */
        byte1 = ch_codes[*src].encvalue[0];
        byte2 = ch_codes[*src].encvalue[1];

        /* One character case */
        if (!ch_codes[*src].encoded){
			unsigned int numutf8bytes = numBytesInUtf8Char(ch_codes[*src].utf8value[0]);
			for (x = 0; x < numutf8bytes; x++){
                *dst = ch_codes[*src].utf8value[x];
                dst++;
                (*nBytes)++;
            }
        }
        else{
            /* Two Byte Encoded Case */
            unsigned int nEncBytes1 = 0;
            unsigned int nEncBytes2 = 0;
            unsigned char encodedVal1[10];
            unsigned char encodedVal2[10];

            memset(encodedVal1, 0xFF, 10);
            memset(encodedVal2, 0xFF, 10);

            if (!ch_codes[byte1].encoded){
				unsigned int numutf8bytes = numBytesInUtf8Char(ch_codes[byte1].utf8value[0]);
				for (x = 0; x < numutf8bytes; x++){
                    encodedVal1[x] = ch_codes[byte1].utf8value[x];
                }
            }
            else{
                encodedVal1[0] = ch_codes[byte1].encvalue[0];
                encodedVal1[1] = ch_codes[byte1].encvalue[1];
            }

            if (!ch_codes[byte2].encoded){
				unsigned numutf8bytes = numBytesInUtf8Char(ch_codes[byte2].utf8value[0]);
				for (x = 0; x < numutf8bytes; x++){
                    encodedVal2[x] = ch_codes[byte2].utf8value[x];
                }
            }
            else{
                encodedVal2[0] = ch_codes[byte2].encvalue[0];
                encodedVal2[1] = ch_codes[byte2].encvalue[1];
            }

            /* Recurse */
            decompressBPE(dst, &encodedVal1[0], &nEncBytes1);
            dst += nEncBytes1;
            *nBytes += nEncBytes1;
            decompressBPE(dst, &encodedVal2[0], &nEncBytes2);
            dst += nEncBytes2;
            *nBytes += nEncBytes2;
        }

        src++;
    }

    return;
}
