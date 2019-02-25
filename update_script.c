/*****************************************************************************/
/* parse.c : Code to parse lunar SSS and maintain script linkage             */
/*****************************************************************************/
#pragma warning(disable:4996)

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

/* Defines */


/* Globals */



/* Function Prototypes */
int updateScript(FILE* inFile, FILE* upFile, FILE* outFile);


/*****************************************************************************/
/* Function: updateScript                                                    */
/* Purpose: Reads a metadata version of the script and applies updates that  */
/*          are listed in an update file.  The modified version of the       */
/*          script is written out to another file.                           */
/* Inputs:  Pointers to input/update/output files.                           */
/* Outputs: 0 on Pass, -1 on Fail.                                           */
/*****************************************************************************/
int updateScript(FILE* inFile, FILE* upFile, FILE* outFile){

    return 0;
}
