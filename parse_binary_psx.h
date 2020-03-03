/*****************************************************************************/
/* parse_binary_psx.h : Code to parse lunar SSS and maintain script linkage  */
/*                      PSX English Edition - Created using Supper's notes,  */
/*                      comparing with Saturn version, and taking some       */
/*                      guesses.  (I didn't feel like staring at PSX ASM).   */
/*****************************************************************************/
#ifndef PARSE_BINARY_PSX_H
#define PARSE_BINARY_PSX_H

#include <stdio.h>

int decodeBinaryScript_PSX(FILE* inFile, FILE* outFile);


#endif