/*****************************************************************************/
/* psx_string_decode.c : Code to decode WD's Text in Lunar's Script files.   */
/*                      PSX English Edition - Created using Supper's notes,  */
/*                      comparing with Saturn version, and taking some       */
/*                      guesses.  (I didn't feel like staring at PSX ASM).   */
/*****************************************************************************/
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Defines */
#define psxBufferSize (10*1024) //Should be Overkill

/* Function Prototypes */
int loadPSXStringTable(char* inFname);
int releasePSXStringTable();
int getPSXComprStr(int compressionIndex, char* target, int* tlen);
int convertPSXText(char* strIn, char** strOut, int len, int* lenOut);


static unsigned char tmpPsxBuf[psxBufferSize];
static int G_NumPSXTableEntries = 0;
static char** pPSXTableEntries = NULL;


/******************************************************************************/
/* loadPSXStringTable - Loads in a table of NULL-terminated lookup strings.   */
/* Input - Name of file to load.                                              */
/* Returns 0 on success, -1 on failure.                                       */
/******************************************************************************/
int loadPSXStringTable(char* inFname){

	FILE* infile;
	int entryIndex;
	int x = 0;
	infile = fopen(inFname, "rb");
	if (infile == NULL){
		printf("Error opening %s\n", inFname);
		return -1;
	}

	/* Part 1, count # entries */
	while (1){
		fread(tmpPsxBuf, 1, 1, infile);
		if (feof(infile))
			break;

		if (*tmpPsxBuf == 0x00)
			G_NumPSXTableEntries++;
	}

	/* Part 2, Allocate Space and add entries */
	fseek(infile, 0, SEEK_SET);
	entryIndex = 0;

	pPSXTableEntries = (char**)malloc(sizeof(char*)*G_NumPSXTableEntries);
	if (*pPSXTableEntries == NULL){
		printf("Error allocating memory for table entries.\n");
		return -1;
	}

	while (1){
		x = 0;
		fread(&tmpPsxBuf[x], 1, 1, infile);
		if (feof(infile))
			break;

		while (tmpPsxBuf[x] != 0x00){
			x++;
			fread(&tmpPsxBuf[x], 1, 1, infile);
		}
		pPSXTableEntries[entryIndex] = (char*)malloc(x+1);
		if (pPSXTableEntries[entryIndex] == NULL){
			printf("Error allocating memory for table entry.\n");
			return -1;
		}
		memset(pPSXTableEntries[entryIndex], 0,x+1);
		memcpy(pPSXTableEntries[entryIndex], tmpPsxBuf, x);
		entryIndex++;
	}
	fclose(infile);

	return 0;
}


/******************************************************************************/
/* releasePSXStringTable - Removes resources used by a loaded string table.   */
/* Returns 0 on success, -1 on failure.                                       */
/******************************************************************************/
int releasePSXStringTable(){

	int x;
	for (x = 0; x < G_NumPSXTableEntries; x++){
		if (pPSXTableEntries[x] != NULL){
			free(pPSXTableEntries[x]);
			pPSXTableEntries[x] = NULL;
		}
	}
	if (pPSXTableEntries != NULL){
		free(pPSXTableEntries);
	}
	pPSXTableEntries = NULL;
	G_NumPSXTableEntries = 0;

	return 0;
}


/******************************************************************************/
/* getPSXComprStr - Looks up a compression string by an index value and       */
/*                  copies the string and length of the string.               */
/* Input - Lookup index.                                                      */
/* Returns 0 on success, -1 on failure.                                       */
/******************************************************************************/
int getPSXComprStr(int compressionIndex, char* target, int* tlen){

	if (compressionIndex >= G_NumPSXTableEntries){
		printf("Invalid index in getPSXComprStr\n");
		return -1;
	}
	else{
		strcpy(target, pPSXTableEntries[compressionIndex]);
		*tlen = strlen(pPSXTableEntries[compressionIndex]);
	}
	return 0;
}


/******************************************************************************/
/* convertPSXText - Decompresses a compressed string.                         */
/*                  I took a lot of guessing here.  Seems to work fine for all*/
/*                  files except maybe File51?  iOS has 3 text statements     */
/*                  there that print as "{10}" whereas this outputs "ta{10}I "*/
/*                  Maybe thats normal, not a big deal if it isn't.           */
/* Input - Compressed String.                                                 */
/* Returns 0 on success, -1 on failure.                                       */
/******************************************************************************/
int convertPSXText(char* strIn, char** strOut, int len, int* lenOut){

	unsigned char* ptarget, *optr;
	unsigned char input, inputNext;
	int compressionIndex, x, stroutIndex;
	int offset = 0;
	int out_offset = 0;
	int enableCtrlCodes = 1;
	*lenOut = 0;
	ptarget = tmpPsxBuf;
	memset(ptarget, 0, psxBufferSize);

	/* Decode the message */
	while (offset < len){

		int tlen = 0;
		input = (unsigned char)strIn[offset];
		inputNext = (unsigned char)strIn[offset+1];

		if (input == 0x0){
			offset++;  //maybe this re-enables control codes?
            enableCtrlCodes = 1;
			continue;
		}
		else if (input == 0xFF){
			printf("End of input text stream detected.\n");
			ptarget[out_offset++] = 0xFF;
			ptarget[out_offset++] = 0xFF;
			offset++;
			break;
		}



		if (enableCtrlCodes){

			if (input == 0xE){
				enableCtrlCodes = 0;
				offset++;
				continue;
			}
			if ((input == 0x1) || (input == 0x2) || (input == 0x3)){
				ptarget[out_offset++] = 0xFF;
				ptarget[out_offset++] = input;
				offset++;
				continue;
			}
			if (input == 0x4){
				ptarget[out_offset++] = 0xFF;
				ptarget[out_offset++] = 0x00;
				offset++;
				continue;
			}

			if (((input == 0xF9) || (input == 0xF8) || (input == 0xFA) || (input == 0xFB) ||
				(input == 0xFC) || (input == 0xF6) || (input == 0xF1))){

				//Copy Ctrl Code
				ptarget[out_offset++] = input;
				ptarget[out_offset++] = inputNext;
				offset += 2;
				continue;
			}

			if (input == 0xFF){
				printf("End of input text stream detected.\n");
				ptarget[out_offset++] = 0xFF;
				ptarget[out_offset++] = 0xFF;
				offset++;
				break;
			}

			//If you get here, assume not a control code, enter Text Mode
			//printf("Unknown value 0x%X encountered. '%c'\n",input, input+0x1F);
		}

		enableCtrlCodes = 0;
		if (input < 0x5C){

			if (input == 0xB){
				ptarget[out_offset++] = 0xFF;
				ptarget[out_offset++] = 0x00;
				ptarget[out_offset++] = 0xFF;
				ptarget[out_offset++] = 0x01;
				offset++;
				continue;
			}
			else if (input == 0x21){
				ptarget[out_offset++] = 0xFF;
				ptarget[out_offset++] = 0x02;
				offset++;
				continue;
			}
			else if (input == 0x6){
				ptarget[out_offset++] = 0xFF;
				ptarget[out_offset++] = 0x00;
				offset++;
				continue;
			}
			else if (input == 0x5){
				ptarget[out_offset++] = 0xFF;
				ptarget[out_offset++] = 0x00;
				ptarget[out_offset++] = 0xFF;
				ptarget[out_offset++] = 0x03;
				ptarget[out_offset++] = 0xFF;
				ptarget[out_offset++] = 0xFF;
				offset++;
				break;
			}
			else{
				//Uncompressed ASCII Literal
				ptarget[out_offset] = input + (unsigned char)0x1F;
				out_offset++;
			}
		}
		else if (input == 0xFD){
			//Compression Extended Lkup Version 1
			offset++;
			input = (unsigned char)strIn[offset];
			compressionIndex = (unsigned int)input + 0x00A1;
			getPSXComprStr(compressionIndex, (char*)&ptarget[out_offset], &tlen);
			out_offset += tlen;
		}
		else if (input == 0xFE){
			//Compression Extended Lkup Version 2
			offset++;
			input = (unsigned char)strIn[offset];
			compressionIndex = (unsigned int)input + 0x019E;
			getPSXComprStr(compressionIndex, (char*)&ptarget[out_offset], &tlen);
			out_offset += tlen;
		}
		else{
			//Std Lookup
			compressionIndex = input - 0x005C;
			getPSXComprStr(compressionIndex, (char*)&ptarget[out_offset], &tlen);
			out_offset += tlen;
		}

		offset++;

		/* End of text */
		if (ptarget[out_offset - 1] == '$')
			break;
	}


	/* Copy out actual decompressed string */
	*strOut = (char*)malloc(2*(out_offset + 1));
	if (*strOut == NULL){
		printf("Error allocating memory for string buffer.\n");
		return -1;
	}

	optr = (unsigned char*)(*strOut);
	stroutIndex = 0;
	memset(optr, 0, 2 * (out_offset + 1));
	for (x = 0; x < out_offset; x++){

		if (ptarget[x] >= 0xF0){  //ctrl code
			optr[stroutIndex++] = ptarget[x++];
			optr[stroutIndex++] = ptarget[x];
		}
		else{
			optr[stroutIndex++] = ptarget[x];
		}
	}
	*lenOut = stroutIndex;

	return offset;
}
