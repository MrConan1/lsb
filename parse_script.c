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
#define BIG_ENDIAN    0
#define LITTLE_ENDIAN 1
#define RADIX_DEC     0
#define RADIX_HEX     1
#define DEFAULT_FILL  0x00

/* Globals */
static int output_endian_type = BIG_ENDIAN;
static int radix_type = RADIX_HEX;
static unsigned char* pInput = NULL;
static unsigned char* pOutput = NULL;
static unsigned int fsize, max_size_bytes;
static unsigned int offset = 0x00;
static unsigned char* obuf = NULL;
static int G_byte_mode = 1;         /* Enable short text when 0, byte when 1 */

/* Function Prototypes */
int encodeScript(FILE* infile, FILE* outfile);
int decode_goto();
int decode_fill();
int decode_pointer();
int decode_exesub();
int decode_runcmds();

/* Read/Write Fctns */
static int readLW(unsigned int* data);
static int readSW(unsigned short* datas);
static int readBYTE(unsigned char* datab);
static int writeLW(unsigned char* ptr, unsigned int data);
static int writeSW(unsigned char* ptr, unsigned short data);
static int writeBYTE(unsigned char* ptr, unsigned char data);

static int readLW(unsigned int* data){
	if (radix_type == RADIX_HEX){
		if (sscanf(pInput,"%X", data) != 1){
			printf("Error reading long\n");
			return -1;
		}
	}
	else{
		if (sscanf(pInput,"%u", data) != 1){
			printf("Error reading long\n");
			return -1;
		}
	}

	return 0;
}

static int readSW(unsigned short* datas){

	unsigned int local;

	if (radix_type == RADIX_HEX){
		if (sscanf(pInput, "%X", &local) != 1){
			printf("Error reading short\n");
			return -1;
		}
		*datas = local & 0xFFFF;
	}
	else{
		if (sscanf(pInput, "%hu", datas) != 1){
			printf("Error reading short\n");
			return -1;
		}
	}

	return 0;
}

static int readBYTE(unsigned char* datab){

	unsigned int local;

	if (radix_type == RADIX_HEX){
		if (sscanf(pInput, "%X", &local) != 1){
			printf("Error reading byte\n");
			return -1;
		}
		*datab = local & 0xFF;
	}
	else{
		if (sscanf(pInput, "%c", &datab) != 1){
			printf("Error reading byte\n");
			return -1;
		}
	}

	return 0;
}


static int writeLW(unsigned char* ptr, unsigned int data){

	unsigned int* pData = (unsigned int*)ptr;
	if (output_endian_type == LITTLE_ENDIAN){
		*pData = data;
	}
	else{
		unsigned char* pSrc = (unsigned char*)data;
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
		unsigned char* pSrc = (unsigned char*)data;
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
/* Function: encodeScript                                                    */
/* Purpose: Parses a metadata text version of the script, and from that      */
/*          creates a binary script file.                                    */
/* Inputs:  Pointers to input/output files.                                  */
/* Outputs: 0 on Pass, -1 on Fail.                                           */
/*****************************************************************************/
int encodeScript(FILE* infile, FILE* outfile){

	int rval;
	unsigned char* pBuffer = NULL;

	/* Check text encoding two see if 2 byte encoding is required */
	if (query_two_byte_encoding())
		G_byte_mode = 0;

	/* Determine Input File Size */
	if (fseek(infile, 0, SEEK_END) != 0){
		printf("Error seeking in input file.\n");
		return -1;
	}
	fsize = ftell(infile);
	if (fseek(infile, 0, SEEK_SET) != 0){
		printf("Error seeking in input file.\n");
		return -1;
	}

	/* Read the entire file into memory */
	pBuffer = (unsigned char*)malloc(fsize);
	if (pBuffer == NULL){
		printf("Error allocating to put input file in memory.\n");
		return -1;
	}
	if (fread(pBuffer, 1, fsize, infile) != fsize){
		printf("Error, reading file into memory\n");
	}

	/****************************************************/
	/* Parse the input file to create the binary output */
	/****************************************************/
	pInput = strtok(pBuffer, "( \t=\r\n");
	if ((pInput == NULL) || (strcmp(pInput, "start") != 0)) {
		printf("Error, start not found\n");
		return -1;
	}

	/*********/
	/* Start */
	/*********/

	/* start - endian (big or little) */
	pInput = strtok(NULL, "(\t = \r\n");
	if (strcmp(pInput, "endian") != 0) {
		printf("Error, start endian not found\n");
		return -1;
	}
	pInput = strtok(NULL, "(\t = \r\n");
	if (strcmp(pInput, "big") == 0) {
		output_endian_type = BIG_ENDIAN;
		printf("Setting Big Endian.\n");
	}
	else if (strcmp(pInput, "little") == 0){
		output_endian_type = LITTLE_ENDIAN;
		printf("Setting Little Endian.\n");
	}
	else{
		printf("Invalid Endian.\n");
		return -1;
	}

	/* start - radix (hex or decimal) */
	pInput = strtok(NULL, "(\t = \r\n");
	if (strcmp(pInput, "radix") != 0) {
		printf("Error, start radix not found\n");
		return -1;
	}
	pInput = strtok(NULL, "(\t = \r\n");
	if (strcmp(pInput, "hex") == 0) {
		radix_type = RADIX_HEX;
		printf("Setting Radix to HEX.\n");
	}
	else if(strcmp(pInput, "dec") == 0) {
		radix_type = RADIX_DEC;
		printf("Setting Radix to DEC.\n");
	}
	else{
		printf("Unknown Radix.\n");
		return -1;
	}

	/* start - max_size_bytes */
	pInput = strtok(NULL, "(\t = \r\n");
	if (strcmp(pInput, "radix") != 0) {
		printf("Error, start radix not found\n");
		return -1;
	}
	pInput = strtok(NULL, "(\t = \r\n");
	if (readLW(&max_size_bytes) < 0){
		printf("Error invalid max_size_bytes\n");
		return -1;
	}
	obuf = (unsigned char*)malloc(max_size_bytes);
	if (obuf == NULL){
		printf("Error allocating memory for output buffer.\n");
		return -1;
	}
	memset(obuf, DEFAULT_FILL, max_size_bytes);
	pOutput = obuf;
	offset = 0x00;

	/************************************************************/
	/* Parse the rest of the file until EOF or "end" is located */
	/************************************************************/
	rval = 0;
	pInput = strtok(NULL, "(\t = \r\n");
	while ((pInput != NULL) && rval == 0){

		/* goto */
		if (strcmp(pInput, "goto") == 0){
			rval = decode_goto();
		}

		/* fill-space */
		else if (strcmp(pInput, "fill-space") == 0){
			rval = decode_fill();
		}

		/* pointer */
		else if (strcmp(pInput, "pointer") == 0){
			rval = decode_pointer();
		}

		/* execute-subroutine */
		else if (strcmp(pInput, "execute-subroutine") == 0){
			rval = decode_exesub();
		}

		/* run-commands */
		else if (strcmp(pInput, "run-commands") == 0){
			rval = decode_runcmds();
		}

		/* end */
		else if (strcmp(pInput, "end") == 0){
			printf("Detected END\n");
			break;
		}
		else{
			printf("Invalid Command Detected.\n");
			return -1;
		}
	}

	return 0;
}



int decode_goto(){

	/* read offset */
	pInput = strtok(NULL, "(\t = \r\n");
	if (readLW(&offset) < 0){
		printf("Error invalid goto offset\n");
		return -1;
	}

	/* Update output pointer */
	pOutput = obuf + offset;

	return 0;
}


int decode_fill(){

	int numBytes, x;
	unsigned int unitSize = 0;
	unsigned int unitCount = 0;
	unsigned int fillValue = 0;

	/* read unit-size */
	pInput = strtok(NULL, "(\t = \r\n");
	if (strcmp(pInput, "unit-size") != 0) {
		printf("Error, unit-size expected\n");
		return -1;
	}
	pInput = strtok(NULL, "(\t = \r\n");
	if (readLW(&unitSize) < 0){
		printf("Error invalid goto offset\n");
		return -1;
	}

	/* read fill-value */
	pInput = strtok(NULL, "(\t = \r\n");
	if (strcmp(pInput, "fill-size") != 0) {
		printf("Error, fill-size expected\n");
		return -1;
	}
	pInput = strtok(NULL, "(\t = \r\n");
	if (readLW(&fillValue) < 0){
		printf("Error invalid unit size\n");
		return -1;
	}

	/* read unit-count */
	pInput = strtok(NULL, "(\t = \r\n");
	if (strcmp(pInput, "unit-count") != 0) {
		printf("Error, unit-count expected\n");
		return -1;
	}
	pInput = strtok(NULL, "(\t = \r\n");
	if (readLW(&unitCount) < 0){
		printf("Error invalid unit count\n");
		return -1;
	}

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
		unsigned char* pOut_b = pOutput;
		fillValue &= 0xFF;
		fill_byte = (unsigned char)fillValue;
		for (x = 0; x < (int)unitCount; x++)
			pOut_b[x] = fill_byte;
		break;
	}
	case 2:
	{
		unsigned short fill_short;
		unsigned short* pOut_s = (unsigned short*)pOutput;
		fillValue &= 0xFFFF;
		fill_short = (unsigned short)fillValue;
		for (x = 0; x < (int)unitCount; x++)
			pOut_s[x] = fill_short;
		break;
	}
	case 4:
	{
		unsigned int* pOut_l = (unsigned int*)pOutput;
		for (x = 0; x < (int)unitCount; x++)
			pOut_l[x] = fillValue;
		break;
	}
	default:
		printf("Invalid unit size.\n");
		return -1;
	}

	/* Update output pointer and offset */
	offset += numBytes;
	pOutput = obuf + offset;

	return 0;
}


int decode_pointer(){

	unsigned int byteOffset;
	unsigned int dataSize;
	unsigned int value;
	unsigned int id;
	unsigned int value_selected = 0;

	/* read byteoffset */
	pInput = strtok(NULL, "(\t = \r\n");
	if (strcmp(pInput, "byteoffset") != 0) {
		printf("Error, byteoffset expected\n");
		return -1;
	}
	pInput = strtok(NULL, "(\t = \r\n");
	if (readLW(&byteOffset) < 0){
		printf("Error invalid pointer byte offset\n");
		return -1;
	}

	/* read size of pointer */
	pInput = strtok(NULL, "(\t = \r\n");
	if (strcmp(pInput, "size") != 0) {
		printf("Error, size expected\n");
		return -1;
	}
	pInput = strtok(NULL, "(\t = \r\n");
	if (readLW(&dataSize) < 0){
		printf("Error invalid pointer data size\n");
		return -1;
	}

	/* read value or id to point to */
	pInput = strtok(NULL, "(\t = \r\n");
	if (strcmp(pInput, "value") == 0) {
		pInput = strtok(NULL, "(\t = \r\n");
		if (readLW(&value) < 0){
			printf("Error invalid value\n");
			return -1;
		}
		value_selected = 1;
	}
	else if (strcmp(pInput, "id") == 0) {
		pInput = strtok(NULL, "(\t = \r\n");
		if (readLW(&id) < 0){
			printf("Error invalid id\n");
			return -1;
		}
		value_selected = 0;
	}
	else{
		printf("Error, value or id expected\n");
		return -1;
	}

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
			unsigned short* pOut_s = (unsigned short*)pOutput;
			*pOut_s = value & 0xFFFF;
			break;
		}
		case 4: /* Long Ptr */
		{
			unsigned int* pOut_l = (unsigned int*)pOutput;
			*pOut_l = value;
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
	pOutput = obuf + offset;

	return 0;
}


/* Parameter Type */
typedef struct paramType paramType;
typedef struct paramType{
	unsigned int type;
	unsigned int value;
};
#define BYTE_PARAM    0
#define SHORT_PARAM   1
#define LONG_PARAM    2
#define ALIGN_2_PARAM 3
#define ALIGN_4_PARAM 4

int decode_exesub(){
	
	int x;
	unsigned short subrtn_code;
	unsigned int numparam;
	unsigned char fillVal;
	paramType* params = NULL;

	/* Read subroutine value */
	pInput = strtok(NULL, "(\t = \r\n");
	if (readSW(&subrtn_code) < 0){
		printf("Error invalid subroutine code\n");
		return -1;
	}

	/* read num-parameters */
	pInput = strtok(NULL, "(\t = \r\n");
	if (strcmp(pInput, "num-parameters") != 0) {
		printf("Error, num-parameters expected\n");
		return -1;
	}
	pInput = strtok(NULL, "(\t = \r\n");
	if (readLW(&numparam) < 0){
		printf("Error invalid number of parameters\n");
		return -1;
	}

	if (numparam > 0){

		/* read align-fill-byteval */
		pInput = strtok(NULL, "(\t = \r\n");
		if (strcmp(pInput, "align-fill-byteval") != 0) {
			printf("Error, align-fill-byteval expected\n");
			return -1;
		}
		pInput = strtok(NULL, "(\t = \r\n");
		if (readBYTE(&fillVal) < 0){
			printf("Error invalid alignment byte fill value\n");
			return -1;
		}

		/* Allocate memory for parameters */
		params = (paramType*)malloc(numparam * sizeof(paramType));
		if (params == NULL){
			printf("Error allocing memory for parameters\n");
			return -1;
		}

		/**********************/
		/* read in parameters */
		/**********************/

		/* Parameter Type Information */
		pInput = strtok(NULL, "(\t = \r\n");
		if (strcmp(pInput, "parameter-types") != 0) {
			printf("Error, parameter-types expected\n");
			return -1;
		}
		for (x = 0; x < (int)numparam; x++){
			pInput = strtok(NULL, "(\t = \r\n");
			if (strcmp(pInput, "1") == 0)
				params[x].type = BYTE_PARAM;
			else if (strcmp(pInput, "2") == 0)
				params[x].type = SHORT_PARAM;
			else if (strcmp(pInput, "4") == 0)
				params[x].type = LONG_PARAM;
			else if (strcmp(pInput, "align-2") == 0)
				params[x].type = ALIGN_2_PARAM;
			else if (strcmp(pInput, "align-4") == 0)
				params[x].type = ALIGN_4_PARAM;
			else{
				printf("Error invalid parameter type read\n");
				return -1;
			}
		}

		/* Parameter Values */
		pInput = strtok(NULL, "(\t = \r\n");
		if (strcmp(pInput, "parameter-values") != 0) {
			printf("Error, parameter-values expected\n");
			return -1;
		}
		for (x = 0; x < (int)numparam; x++){
			pInput = strtok(NULL, "(\t = \r\n");
			if (readLW(&params[x].value) < 0){
				printf("Error invalid parameter value\n");
				return -1;
			}
		}
	}


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

	return 0;
}

#if 0
(run-commands
	(show-portrait 0x00)
	(print-line "")
	(align-2 0x00)
	(control-code 0xFFFF)
	(align-4 0xFF)
	(commands-end)
)
#endif
int decode_runcmds(){

	/* This is all part of subroutine code 0x0002 */
	writeSW(pOutput, 0x0002); /* Space */
	pOutput += 2;
	offset += 2;

	/* read series of commands until the end of them is reached */
	pInput = strtok(NULL, "(\t = \r\n");
	while (strcmp(pInput, "commands-end") != 0) {

		/* print-line */
		if (strcmp(pInput, "print-line") == 0){
			unsigned char* pText;

			/* Get Text String (UTF-8) */
			pInput = strtok(NULL, "\"");
			if (pInput == NULL){
				printf("Error, bad text input.\n");
				return -1;
			}
			pText = pInput;
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
				if((numBytes == 1) && (*pText == ' ')){
					writeSW(pOutput, 0xF90A); /* Space */
					pOutput += 2;
					offset += 2;
				}
				else{
					if (G_byte_mode){
						if (getUTF8code_Byte(tmp, &code) < 0){
							printf("Error lookup up code corresponding with UTF-8 character\n");
							return -1;
						}

						/* Write the code to the output file */
						writeBYTE(pOutput, code); /* Space */
						pOutput += 1;
						offset += 1;
					}
					else{
						if (getUTF8code_Short(tmp, &scode) < 0){
							printf("Error lookup up code corresponding with UTF-8 character\n");
							return -1;
						}

						/* Write the code to the output file */
						writeSW(pOutput, scode); /* Space */
						pOutput += 2;
						offset += 2;
					}
				}
				pText += numBytes;
			}
		}
		
		/* show-portrait */
		else if (strcmp(pInput, "show-portrait") == 0){
			unsigned char portraitCode;
			pInput = strtok(NULL, "(\t = \r\n");
			if (readBYTE(&portraitCode) < 0){
				printf("Error invalid portrait code.\n");
				return -1;
			}
			*pOutput = 0xFF;
			offset += 1;
			pOutput += 1;
			writeBYTE(pOutput, portraitCode);
			offset += 1;
			pOutput += 1;
		}

		/* control-code */
		else if (strcmp(pInput, "control-code") == 0){
			unsigned short ctrlCode;
			pInput = strtok(NULL, "(\t = \r\n");
			if (readSW(&ctrlCode) < 0){
				printf("Error invalid control code.\n");
				return -1;
			}
			writeSW(pOutput, ctrlCode);
			offset += 2;
			pOutput += 2;
		}

		/* align-2 */
		else if (strcmp(pInput, "align-2") == 0){
			if ((offset & 0x1) != 0x0){
				writeBYTE(pOutput, 0xFF);
				offset += 1;
				pOutput += 1;
			}
		}
		
		/* align-4 */
		else if (strcmp(pInput, "align-4") == 0){
			while ((offset & 0x3) != 0x0){
				writeBYTE(pOutput, 0xFF);
				offset += 1;
				pOutput += 1;
			}
		}

		/* Unknown */
		else{
			printf("Error unknown command %s detected in run-commands\n",pInput);
			return -1;
		}

	}

	return 0;
}
