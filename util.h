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
int query_two_byte_encoding();

#endif
