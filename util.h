#ifndef UTIL_H
#define UTIL_H

/* Function Prototypes */
void setSSSEncode();
void swap16(void* pWd);
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


#endif
