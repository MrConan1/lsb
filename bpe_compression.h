/**********************************************************************/
/* bpe_compression.h - Compression Routines.                          */
/**********************************************************************/
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif
#ifndef BPE_COMPRESSION_H
#define BPE_COMPRESSION_H




/***********************/
/* Function Prototypes */
/***********************/

/* Loading Existing BPE Data */
int loadUtf8MappingForBPETable(char* bpeUtf8MappingTable);
int loadBPETable(char* bpeTableName, char* bpeUtf8MappingTable);
int utf8_to_bpe(char* utf8_in, unsigned char* rval);
int bpe_to_utf8(unsigned char bpe_in, unsigned char* utf8_out);
int utf8Text_to_8bit_binary(char* pText, unsigned int* binSizeBytes);
int _8bit_binary_to_utf8Text(unsigned char* bdata, unsigned int binSizeBytes, char** pText);

/* Compression/Decompression */
void decompressBPE(unsigned char* dst, unsigned char* src, unsigned int* nBytes);
void compressBPE(unsigned char* src, unsigned int* nBytes);


#endif
