/*****************************************************************************/
/* write_script.c : Outputs data from a script in either binary format       */
/*                  (writeBinScript) or the metadata human readable format.  */
/*                  (writeScript)                                            */
/*****************************************************************************/
#pragma warning(disable:4996)

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "script_node_types.h"

/* Defines */


/* Globals */
static unsigned char* pOutput = NULL;
static unsigned int output_endian_type = BIG_ENDIAN;
static unsigned int max_size_bytes;
static unsigned int offset = 0x00;
static unsigned char* obuf = NULL;


/* Function Prototypes */
int writeBinScript(scriptNode* pHead, FILE* outFile);
int writeScript(scriptNode* pHead, FILE* outFile);

static int writeLW(unsigned char* ptr, unsigned int data);
static int writeSW(unsigned char* ptr, unsigned short data);
static int writeBYTE(unsigned char* ptr, unsigned char data);



static int writeLW(unsigned char* ptr, unsigned int data){

	unsigned int* pData = (unsigned int*)ptr;
	if (output_endian_type == LITTLE_ENDIAN){
		*pData = data;
	}
	else{
		unsigned char* pSrc = (unsigned char*)&data;
		unsigned char* pDst = (unsigned char*)ptr;

		pDst[0] = pSrc[3];
		pDst[1] = pSrc[2];
		pDst[2] = pSrc[1];
		pDst[3] = pSrc[0];
	}

	return 0;
}

static int writeSW(unsigned char* ptr, unsigned short data){

	unsigned short* pData = (unsigned short*)ptr;
	if (output_endian_type == LITTLE_ENDIAN){
		*pData = data;
	}
	else{
		unsigned char* pSrc = (unsigned char*)&data;
		unsigned char* pDst = (unsigned char*)ptr;

		pDst[0] = pSrc[1];
		pDst[1] = pSrc[0];
	}

	return 0;
}

static int writeBYTE(unsigned char* ptr, unsigned char data){

	*ptr = data;
	return 0;
}


/*****************************************************************************/
/* Function: writeBinScript                                                  */
/* Purpose: Reads from a linked list data structure in memory to create the  */
/*          binary SSS/SSSC compatible TEXTxxx.DAT file.                     */
/* Inputs:  Pointers to input/update/output files.                           */
/* Outputs: 0 on Pass, -1 on Fail.                                           */
/*****************************************************************************/
int writeBinScript(FILE* outFile){

	output_endian_type = getBinOutputMode();
#if 0





	/* Check text table encoding for later */
	G_table_mode = getTableOutputMode();


	obuf = (unsigned char*)malloc(max_size_bytes);
	if (obuf == NULL){
		printf("Error allocating memory for output buffer.\n");
		return -1;
	}
	memset(obuf, DEFAULT_FILL, max_size_bytes);
	pOutput = obuf;
	offset = 0x00;


	/*goto*/

	/* Update output pointer */
	pOutput = obuf + offset;

	/*fill*/
	/* Check that operation can be completed */
	numBytes = unitCount*unitSize;
	if (numBytes + offset > max_size_bytes){
		printf("Error, fill extending beyond max file size.\n");
		return -1;
	}

	/* Copy the fill data */
	switch (unitSize){
	case 1:
	{
		unsigned char fill_byte;
		fillValue &= 0xFF;
		fill_byte = (unsigned char)fillValue;
		for (x = 0; x < (int)unitCount; x++){
			writeBYTE(pOutput, fill_byte);
			pOutput += 1;
			offset += 1;
		}
		break;
	}
	case 2:
	{
		unsigned short fill_short;
		fillValue &= 0xFFFF;
		fill_short = (unsigned short)fillValue;
		for (x = 0; x < (int)unitCount; x++){
			writeSW(pOutput, fill_short);
			pOutput += 2;
			offset += 2;
		}
	}
	case 4:
	{
		for (x = 0; x < (int)unitCount; x++){
			writeLW(pOutput, fillValue);
			pOutput += 4;
			offset += 4;
		}
		break;
	}
	default:
		printf("Invalid unit size.\n");
		return -1;
	}


	/* pointer */
	/* Move file pointer to where the write is to take place */
	offset = byteOffset;
	pOutput = obuf + offset;

	/* Check that operation can be completed */
	if (dataSize + offset > max_size_bytes){
		printf("Error, pointer extending beyond max file size.\n");
		return -1;
	}

	/* If value is selected: */
	/* Write out the pointer value and update the ptr */
	if (value_selected){
		switch (dataSize){
		case 2:  /* Short Ptr */
		{
			writeSW(pOutput, (unsigned short)(value & 0xFFFF));
			break;
		}
		case 4: /* Long Ptr */
		{
			writeLW(pOutput, (value));
			break;
		}
		default:
			printf("Invalid pointer size.\n");
			return -1;
		}
	}
	else{
		/* ID is selected, queue pointer writing for later */

		/* TBD */
		;
	}

	/* Update File Pointer */
	offset += dataSize;
	pOutput += dataSize;


	/* execute-subroutine */

	/*****************************************************/
	/* Output Subroutine Code and Parameters to the file */
	/*****************************************************/

	/* Write Subroutine Code */
	writeSW(pOutput, subrtn_code);
	offset += 2;
	pOutput += 2;

	/* Write Parameters */
	for (x = 0; x < (int)numparam; x++){
		switch (params[x].type){

		case BYTE_PARAM:
			writeBYTE(pOutput, params[x].value & 0xFF);
			offset += 1;
			pOutput += 1;
			break;
		case SHORT_PARAM:
			writeSW(pOutput, params[x].value & 0xFFFF);
			offset += 2;
			pOutput += 2;
			break;
		case LONG_PARAM:
			writeLW(pOutput, params[x].value);
			offset += 4;
			pOutput += 4;
			break;
		case ALIGN_2_PARAM:
			if ((offset & 0x1) != 0x0){
				writeBYTE(pOutput, fillVal);
				offset += 1;
				pOutput += 1;
			}
			break;

		case ALIGN_4_PARAM:
			while ((offset & 0x3) != 0x0){
				writeBYTE(pOutput, fillVal);
				offset += 1;
				pOutput += 1;
			}
			break;

		default:
			printf("Error, writing subroutine parameters, can't get here.\n");
			return -1;
		}
	}


	/* run-commands */

	/* This is all part of subroutine code 0x0002 */
	writeSW(pOutput, 0x0002); /* Space */
	pOutput += 2;
	offset += 2;

	print - line

		while (*pText != '\0'){
		int numBytes;
		unsigned char code;
		unsigned short scode;
		char tmp[5];

		/* Read in a utf8 character */
		numBytes = numBytesInUtf8Char((unsigned char)*pText);
		memset(tmp, 0, 5);
		memcpy(tmp, pText, numBytes);

		/* Look up associated code */
		if ((numBytes == 1) && (*pText == ' ')){
			writeSW(pOutput, 0xF90A); /* Space */
			pOutput += 2;
			offset += 2;
		}
		else{
			if (G_table_mode == ONE_BYTE_ENC){
				if (getUTF8code_Byte(tmp, &code) < 0){
					printf("Error lookup up code corresponding with UTF-8 character\n");
					return -1;
				}

				/* Write the code to the output file */
				writeBYTE(pOutput, code);
				pOutput += 1;
				offset += 1;
			}
			else if (G_table_mode == TWO_BYTE_ENC){
				if (getUTF8code_Short(tmp, &scode) < 0){
					printf("Error lookup up code corresponding with UTF-8 character\n");
					return -1;
				}

				/* Write the code to the output file */
				writeSW(pOutput, scode);
				pOutput += 2;
				offset += 2;
			}
			else{   //Straight UTF-8 Encoding

				/* Write the data to the output file */
				for (z = 0; z < numBytes; z++){
					writeBYTE(pOutput, tmp[z]);
					pOutput += 1;
					offset += 1;
				}
			}
		}
		pText += numBytes;
		}


	show - portrait
		*pOutput = 0xFF;
	offset += 1;
	pOutput += 1;
	writeBYTE(pOutput, portraitCode);
	offset += 1;
	pOutput += 1;


	control - code			writeSW(pOutput, ctrlCode);
	offset += 2;
	pOutput += 2;

	align - 2
		if ((offset & 0x1) != 0x0){
		writeBYTE(pOutput, fillVal);
		offset += 1;
		pOutput += 1;
		}
	align - 4
		while ((offset & 0x3) != 0x0){
		writeBYTE(pOutput, fillVal);
		offset += 1;
		pOutput += 1;
		}

	fwrite(obuf, 1, max_size_bytes, outfile);
	if (obuf != NULL)
		free(obuf);


#endif
    return 0;
}




/*****************************************************************************/
/* Function: writeScript                                                     */
/* Purpose: Reads from a linked list data structure in memory to create a    */
/*          human readable metadata version of the script.                   */
/* Inputs:  Pointers to input/update/output files.                           */
/* Outputs: 0 on Pass, -1 on Fail.                                           */
/*****************************************************************************/
int writeScript(FILE* outFile){

	
    
	return 0;
}
