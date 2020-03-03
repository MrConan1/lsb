/*****************************************************************************/
/* psx_string_decode.c : Code to decode WD's Text in Lunar's Script files.   */
/*                      PSX English Edition - Created using Supper's notes,  */
/*                      comparing with Saturn version, and taking some       */
/*                      guesses.  (I didn't feel like staring at PSX ASM).   */
/*****************************************************************************/
#ifndef PSX_DECODE_H
#define PSX_DECODE_H

/* Function Prototypes */
int loadPSXStringTable(char* inFname);
int releasePSXStringTable();
int getPSXComprStr(int compressionIndex, char* target, int* tlen);
int convertPSXText(char* strIn, char** strOut, int len, int* lenOut);





#endif
