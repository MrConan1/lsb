/******************************************************************************/
/* main.c - Main execution file for lunarScriptBuilder                        */
/******************************************************************************/

/***********************************************************************/
/* lunarScriptBuilder (lsb.exe) Usage                                  */
/* =================================                                   */
/* lsb.exe encode InputFname OutputFname [sss]                         */
/* lsb.exe update InputFname OutputFname UpdateFname                   */
/*                                                                     */
/* Note: Expects table file to be within same directory as exe.        */
/***********************************************************************/
#pragma warning(disable:4996)

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "parse_script.h"
#include "update_script.h"
#include "snode_list.h"
#include "write_script.h"


#define VER_MAJ    1
#define VER_MIN    00

/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n");
    printf("Usage:\n=========\n");
    printf("lsb.exe encode InputFname OutputFname [sss]\n");
	printf("lsb.exe update InputFname OutputFname UpdateFname\n");
	printf("Use Encode to take a script in metadata format and convert to binary.\n");
	printf("Use Update to create modified version of a script in metadata format.\n");
	printf("\n\n");
    return;
}


/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

    FILE *inFile, *upFile, *outFile;
    static char inFileName[300];
	static char upFileName[300];
    static char outFileName[300];
    int rval;

	printf("Lunar Script Builder v%d.%02d\n", VER_MAJ, VER_MIN);

    /**************************/
    /* Check input parameters */
    /**************************/

	/* Check for valid # of args */
	if ((argc < 4) || (argc > 5)){
		printUsage();
		return -1;
	}

	//Handle Input Parameters
	memset(inFileName, 0, 300);
	memset(outFileName, 0, 300);
	strcpy(inFileName, argv[2]);
	strcpy(outFileName, argv[3]);
	if ((argc == 5) && (strcmp(argv[4], "sss") == 0))
		setSSSEncode();

	/********************************************/
	/* Load in the Table File for Decoding Text */
	/********************************************/
	if (loadUTF8Table("newfont_table.txt") < 0){
		printf("Error loading UTF8 Table for Text Decoding.\n");
		return -1;
	}

	/* Open the input/output files */
	inFile = outFile = NULL;
	inFile = fopen(inFileName, "rb");
	if (inFile == NULL){
		printf("Error occurred while opening input script %s for reading\n", inFile);
		return -1;
	}
	outFile = fopen(outFileName, "w");
	if (outFile == NULL){
		printf("Error occurred while opening output file %s for writing\n", outFile);
		fclose(inFile);
		return -1;
	}

	printf("Parsing input file.\n");

	/* Parse the Input File for Encoding */
	rval = encodeScript(inFile, outFile);
	fclose(inFile);
	if (rval == 0){
		printf("Input File Parsed Successfully.\n");
	}
	else{
		printf("Input File Parsing FAILED. Aborting further operations.\n");
		fclose(outFile);
		return -1;
	}

	/**********************************************/
    /* Parameter Decoding - Encode or Update Mode */
	/**********************************************/
    if( (strcmp(argv[1],"encode") == 0) ){

		printf("ENCODE Mode Entered.\n");

		/* Write out the data as a binary file */
		rval = writeBinScript(outFile);
		if (rval == 0){
			printf("Input File Encoded Successfully.\n");
		}
		else{
			printf("Input File Encoding FAILED.\n");
		}

	}
	else if ((strcmp(argv[1], "update") == 0)){

		memset(upFileName, 0, 300);
		strcpy(upFileName, argv[4]);

		printf("UPDATE Mode Entered.\n");

		/* Open the update file */
		upFile = NULL;
		upFile = fopen(upFileName, "rb");
		if (upFile == NULL){
			printf("Error occurred while opening update file %s for reading\n", upFile);
			return -1;
		}

		/* Parse the Update File for Updating */
		rval = updateScript(upFile);
		fclose(upFile);

		/* Write out the data as a Script file */
		rval = writeScript(outFile);
		if (rval == 0){
			printf("Input Script File Updated Successfully.\n");
		}
		else{
			printf("Input Script File Updating FAILED.\n");
		}
	}
    else{
        printUsage();
        return -1;
    }


    /* Close files */
    fclose(outFile);

    return 0;
}
