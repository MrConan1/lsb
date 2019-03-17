/******************************************************************************/
/* main.c - Main execution file for lunarScriptBuilder                        */
/******************************************************************************/

/***********************************************************************/
/* lunarScriptBuilder (lsb.exe) Usage                                  */
/* ==================================                                  */
/* lsb.exe decode InputFname OutputFname ienc [sss]                    */
/* lsb.exe encode InputFname OutputFname [sss]                         */
/* lsb.exe update InputFname OutputFname UpdateFname                   */
/*                                                                     */
/* Note: Expects table file to be within same directory as exe.        */
/*       Table file should be named font_table.exe                     */
/***********************************************************************/
#pragma warning(disable:4996)

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "parse_binary.h"
#include "parse_script.h"
#include "update_script.h"
#include "snode_list.h"
#include "write_script.h"


#define VER_MAJ    1
#define VER_MIN    01

/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n");
    printf("Usage:\n=========\n");
	printf("lsb.exe decode InputFname OutputFname ienc [sss]\n");
	printf("lsb.exe encode InputFname OutputFname [sss]\n");
    printf("lsb.exe update InputFname OutputFname UpdateFname\n");
	printf("Use Decode to take a binary TEXTxxx.DAT file and convert to metadata format.\n");
	printf("Use Encode to take a script in metadata format and convert to binary.\n");
    printf("Use Update to create modified version of a script in metadata format.\n");
	printf("    ienc = 0 for 2-Byte encoded text\n");
	printf("    ienc = 1 for 1-Byte encoded text hack\n");
	printf("    ienc = 2 for utf8 (iOS) encoded text\n");
	printf("    sss will interpret SSSC JP table as the SSS JP table.\n");
	printf("    Table file must be for SSSC, named \"font_table.txt\", and be located in the exe directory.\n");
	printf("\n\n");
    return;
}


/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

	FILE *inFile, *upFile, *outFile, *csvOutFile;
    static char inFileName[300];
    static char upFileName[300];
    static char outFileName[300];
	static char csvOutFileName[300];
    int rval, ienc;

    printf("Lunar Script Builder v%d.%02d\n", VER_MAJ, VER_MIN);

    /**************************/
    /* Check input parameters */
    /**************************/

    /* Check for valid # of args */
    if (argc < 4){
        printUsage();
        return -1;
    }

	//Handle Generic Input Parameters
	memset(inFileName, 0, 300);
	memset(outFileName, 0, 300);
	strcpy(inFileName, argv[2]);
	strcpy(outFileName, argv[3]);
	strcpy(csvOutFileName, argv[3]);
	strcat(csvOutFileName, "_dump.csv");

	/***********************************/
	/* Check & Decode Input Parameters */
	/***********************************/
	if ((strcmp(argv[1], "decode") == 0)){
		/* Check decode parameters */
		ienc = atoi(argv[4]);
		if((argc == 6) && (strcmp(argv[5], "sss") == 0))
			setSSSEncode();
		if (argc > 6){
			printUsage();
			return -1;
		}
	}
    else if ((strcmp(argv[1], "encode") == 0)){
		/* Check encode parameters */
		if ((argc == 5) && (strcmp(argv[4], "sss") == 0))
			setSSSEncode();
		if (argc > 5){
			printUsage();
			return -1;
		}
    }
    else if ((strcmp(argv[1], "update") == 0)){
		/* Check update parameters */
		if (argc != 5){
			printUsage();
			return -1;
		}
		memset(upFileName, 0, 300);
		strcpy(upFileName, argv[4]);
    }
    else{
        /* Invalid Mode */
        printUsage();
        return -1;
    }

    
    /********************************************/
    /* Load in the Table File for Decoding Text */
    /********************************************/
    if (loadUTF8Table("font_table.txt") < 0){
        printf("Error loading UTF8 Table for Text Decoding.\n");
        return -1;
    }


    /*******************************/
    /* Open the input/output files */
    /*******************************/
    inFile = outFile = NULL;
    inFile = fopen(inFileName, "rb");
    if (inFile == NULL){
        printf("Error occurred while opening input script %s for reading\n", inFile);
        return -1;
    }
    outFile = fopen(outFileName, "wb");
    if (outFile == NULL){
        printf("Error occurred while opening output file %s for writing\n", outFile);
        fclose(inFile);
        return -1;
    }


    /**************************************************************************/
    /* Parse the Input Script File (Req'd for Encoding to Binary or Updating) */
    /**************************************************************************/
    printf("Parsing input file.\n");
    
    /* Init Linked List for storing node data */
    initNodeList();

	if ((strcmp(argv[1], "encode") == 0) ||
		(strcmp(argv[1], "update") == 0))
	{
		rval = encodeScript(inFile, outFile);
	}
	else if (strcmp(argv[1], "decode") == 0){
		rval = decodeBinaryScript(inFile, outFile);
	}
	fclose(inFile);
	if (rval == 0){
		printf("Input File Parsed Successfully.\n");
	}
	else{
		printf("Input File Parsing FAILED. Aborting further operations.\n");
		fclose(outFile);
		destroyNodeList();
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

        printf("UPDATE Mode Entered.\n");

        /* Open the update file */
        upFile = NULL;
        upFile = fopen(upFileName, "rb");
        if (upFile == NULL){
            printf("Error occurred while opening update file %s for reading\n", upFile);
            fclose(outFile);
            destroyNodeList();
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
	else if ((strcmp(argv[1], "decode") == 0)){
		
		printf("DECODE Mode Entered.\n");

		/* Write out the data as a Script file */
		rval = writeScript(outFile);
		if (rval == 0){
			printf("Input Script File Updated Successfully.\n");
		}
		else{
			printf("Input Script File Updating FAILED.\n");
		}

		/* CSV File Output */
		csvOutFile = fopen(csvOutFileName, "wb");
		if (csvOutFile == NULL){
			printf("Error occurred while opening CSV output file %s for writing\n", csvOutFile);
			return -1;
		}
		else{
			rval = dumpScript(csvOutFile);
			if (rval == 0){
				printf("Script File CSV Dump Created.\n");
			}
			else{
				printf("Script File CSV Dump FAILED.\n");
			}
			fclose(csvOutFile);
		}
	}
    else{
        printUsage();
        fclose(outFile);
        destroyNodeList();
        return -1;
    }

    /* Close files */
    fclose(outFile);

    /* Release Resources */
    destroyNodeList();

    return 0;
}
