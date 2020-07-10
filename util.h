#ifndef UTIL_H
#define UTIL_H

/* Defines */
#define TEXT_DECODE_TWO_BYTES_PER_CHAR 0
#define TEXT_DECODE_ONE_BYTE_PER_CHAR  1
#define TEXT_DECODE_UTF8               2
#define TEXT_DECODE_UTF8_ENG           3
#define TEXT_DECODE_PSX_ENG	           4
#define TEXT_DECODE_TWO_BYTES_ASCII    5

/* Function Prototypes */
void setSSSEncode();
int getSSSEncode();
void swap16(void* pWd);
void swap32(void* pWd);
int numBytesInUtf8Char(unsigned char val);
int loadUTF8Table(char* fname);
int getUTF8character(int index, char* utf8Value);
int getUTF8code_Byte(char* utf8Value, unsigned char* utf8Code);
int getUTF8code_Short(char* utf8Value, unsigned short* utf8Code);
void setBinOutputMode(int mode);
int getBinOutputMode();
void setBinMaxSize(unsigned int maxSize);
unsigned int getBinMaxSize();
void setMetaScriptInputMode(int mode);
int getMetaScriptInputMode();
void setTableOutputMode(int mode);
int getTableOutputMode();

/**************************/
/* Meta Script Read Fctns */
/**************************/
int read_ID(unsigned char* pInput);
int readLW(unsigned char* pInput, unsigned int* data);
int readSW(unsigned char* pInput, unsigned short* datas);
int readBYTE(unsigned char* pInput, unsigned char* datab);

/* Decode Related Functions */
int getTextDecodeMethod();
void setTextDecodeMethod(int method);
int checkSSSItemHack();

#endif
