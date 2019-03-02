/*****************************************************************************/
/* update_script.c : Reads metadata that dictates updates to an existing     */
/*                   human readable script format.  The updates modify an    */
/*                   already existing linked list of the script in memory    */
/*                   (created by parsing the orignal meta script).           */
/*                   After making the necessary updates, the script is then  */
/*                   rewritten to a file in the same human readable metadata */
/*                   format.                                                 */
/*****************************************************************************/
#pragma warning(disable:4996)

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "script_node_types.h"
#include "snode_list.h"

/* Defines */


/* Globals */
static unsigned char* pInput = NULL;


/* Function Prototypes */
int updateScript(FILE* upFile);
int readNode(scriptNode* node);
int copy_goto(int id, scriptNode* node);
int copy_fill(int id, scriptNode* node);
int copy_pointer(int id, scriptNode* node);
int copy_exesub(int id, scriptNode* node);
int copy_runcmds(int id, scriptNode* node);



/*****************************************************************************/
/* Function: updateScript                                                    */
/* Purpose: Reads a metadata version of the script and applies updates that  */
/*          are listed in an update file.  The modified version of the       */
/*          script is written out to another file.                           */
/* Inputs:  Pointers to input/update/output files.                           */
/* Outputs: 0 on Pass, -1 on Fail.                                           */
/*****************************************************************************/
int updateScript(FILE* upFile){

	int rval;
	unsigned int fsize;
	unsigned char* pBuffer = NULL;

	/* Determine Input File Size */
	if (fseek(upFile, 0, SEEK_END) != 0){
		printf("Error seeking in input file.\n");
		return -1;
	}
	fsize = ftell(upFile);
	if (fseek(upFile, 0, SEEK_SET) != 0){
		printf("Error seeking in input file.\n");
		return -1;
	}

	/* Read the entire file into memory */
	pBuffer = (unsigned char*)malloc(fsize);
	if (pBuffer == NULL){
		printf("Error allocating to put input file in memory.\n");
		return -1;
	}
	rval = fread(pBuffer, 1, fsize, upFile);
	if (rval != fsize){
		printf("Error, reading update file into memory\n");
	}

	/****************************************************/
	/* Parse the input file to create the binary output */
	/****************************************************/
	pInput = strtok(pBuffer, "() \t=\r\n");
	if ((pInput == NULL) || (strcmp(pInput, "start") != 0)) {
		printf("Error, start not found\n");
		return -1;
	}



	/************************************************************/
	/* Parse the rest of the file until EOF or "end" is located */
	/************************************************************/
	rval = 0;
	pInput = strtok(NULL, "()\t = \r\n");
	while ((pInput != NULL) && rval == 0){
		int id;
		scriptNode node;

		/* insert-before-ID */
		if (strcmp(pInput, "insert-before-ID") == 0){
			id = read_ID(pInput);
			readNode(&node);
			rval = addNode(&node, METHOD_INSERT_BEFORE, id);
		}

		/* insert-after-ID */
		else if (strcmp(pInput, "insert-after-ID") == 0){
			id = read_ID(pInput);
			readNode(&node);
			rval = addNode(&node, METHOD_INSERT_AFTER, id);
		}

		/* remove-ID */
		else if (strcmp(pInput, "remove-ID") == 0){
			id = read_ID(pInput);
			rval = removeNode(id);
		}

		/* overwrite-ID */
		else if (strcmp(pInput, "overwrite-ID") == 0){
			id = read_ID(pInput);
			readNode(&node);
			rval = overwriteNode(id, &node);
		}

		/* end */
		else if (strcmp(pInput, "end") == 0){
			printf("Detected END\n");
			break;
		}
		else{
			printf("Invalid Update Command Detected.\n");
			return -1;
		}

		/* Check rval */
		if (rval < 0)
			return rval;

		/* Read Next Token */
		pInput = strtok(NULL, "()\t = \r\n");
	}

	return 0;
}




int readNode(scriptNode* node){

	int rval = 0;
	pInput = strtok(NULL, "()\t = \r\n");
	if ((pInput != NULL) && rval == 0){
		int id;

		/* goto */
		if (strcmp(pInput, "goto") == 0){
			id = read_ID(pInput);
			rval = copy_goto(id, node);
		}

		/* fill-space */
		else if (strcmp(pInput, "fill-space") == 0){
			id = read_ID(pInput);
			rval = copy_fill(id, node);
		}

		/* pointer */
		else if (strcmp(pInput, "pointer") == 0){
			id = read_ID(pInput);
			rval = copy_pointer(id, node);
		}

		/* execute-subroutine */
		else if (strcmp(pInput, "execute-subroutine") == 0){
			id = read_ID(pInput);
			rval = copy_exesub(id, node);
		}

		/* run-commands */
		else if (strcmp(pInput, "run-commands") == 0){
			id = read_ID(pInput);
			rval = copy_runcmds(id, node);
		}

		else{
			printf("Invalid Command Detected.\n");
			return -1;
		}

		/* Check rval */
		if (rval < 0)
			return rval;
	}

	return 0;
}


/******************************/
/* goto                       */
/******************************/
int copy_goto(int id, scriptNode* node){

	unsigned int offset;

	/* read location */
	pInput = strtok(NULL, "()\t = \r\n");
	if (strcmp(pInput, "location") != 0) {
		printf("Error, location expected\n");
		return -1;
	}

	/* read offset */
	pInput = strtok(NULL, "()\t = \r\n");
	if (readLW(pInput, &offset) < 0){
		printf("Error invalid goto offset\n");
		return -1;
	}

	/* Create a script node */
	node->nodeType = NODE_GOTO;
	node->id = id;
	node->byteOffset = offset;

	return 0;
}


/******************************/
/* fill                       */
/******************************/
int copy_fill(int id, scriptNode* node){

	unsigned int unitSize = 0;
	unsigned int unitCount = 0;
	unsigned int fillValue = 0;

	/* read unit-size */
	pInput = strtok(NULL, "()\t = \r\n");
	if (strcmp(pInput, "unit-size") != 0) {
		printf("Error, unit-size expected\n");
		return -1;
	}
	pInput = strtok(NULL, "()\t = \r\n");
	if (readLW(pInput, &unitSize) < 0){
		printf("Error invalid goto offset\n");
		return -1;
	}

	/* read fill-value */
	pInput = strtok(NULL, "()\t = \r\n");
	if (strcmp(pInput, "fill-value") != 0) {
		printf("Error, fill-value expected\n");
		return -1;
	}
	pInput = strtok(NULL, "()\t = \r\n");
	if (readLW(pInput, &fillValue) < 0){
		printf("Error invalid unit size\n");
		return -1;
	}

	/* read unit-count */
	pInput = strtok(NULL, "()\t = \r\n");
	if (strcmp(pInput, "unit-count") != 0) {
		printf("Error, unit-count expected\n");
		return -1;
	}
	pInput = strtok(NULL, "()\t = \r\n");
	if (readLW(pInput, &unitCount) < 0){
		printf("Error invalid unit count\n");
		return -1;
	}

	/* Create a script node */
	node->nodeType = NODE_FILL_SPACE;
	node->id = id;
	node->unit_size = unitSize;
	node->fillVal = fillValue;
	node->unit_count = unitCount;

	return 0;
}


/******************************/
/* pointer                    */
/******************************/
int copy_pointer(int id, scriptNode* node){

	unsigned int byteOffset;
	unsigned int dataSize;
	unsigned int value;
	unsigned int id_link;
	unsigned int value_selected = 0;

	/* read byteoffset */
	pInput = strtok(NULL, "()\t = \r\n");
	if (strcmp(pInput, "byteoffset") != 0) {
		printf("Error, byteoffset expected\n");
		return -1;
	}
	pInput = strtok(NULL, "()\t = \r\n");
	if (readLW(pInput, &byteOffset) < 0){
		printf("Error invalid pointer byte offset\n");
		return -1;
	}

	/* read size of pointer */
	pInput = strtok(NULL, "()\t = \r\n");
	if (strcmp(pInput, "size") != 0) {
		printf("Error, size expected\n");
		return -1;
	}
	pInput = strtok(NULL, "()\t = \r\n");
	if (readLW(pInput, &dataSize) < 0){
		printf("Error invalid pointer data size\n");
		return -1;
	}

	/* read value or id to point to */
	pInput = strtok(NULL, "()\t = \r\n");
	if (strcmp(pInput, "value") == 0) {
		pInput = strtok(NULL, "()\t = \r\n");
		if (readLW(pInput, &value) < 0){
			printf("Error invalid value\n");
			return -1;
		}
		value_selected = 1;
	}
	else if (strcmp(pInput, "id_link") == 0) {
		pInput = strtok(NULL, "()\t = \r\n");
		if (readLW(pInput, &id_link) < 0){
			printf("Error invalid id\n");
			return -1;
		}
		value_selected = 0;
	}
	else{
		printf("Error, value or id expected\n");
		return -1;
	}


	/* Create a script node */
	node->nodeType = NODE_POINTER;
	node->id = id;
	node->byteOffset = byteOffset;
	node->ptrSize = dataSize;
	node->ptrValueFlag = value_selected;
	node->ptrValue = value;
	node->ptrID = id_link;

	return 0;
}


/******************************/
/* execute-subroutine         */
/******************************/
int copy_exesub(int id, scriptNode* node){

	int x;
	unsigned short subrtn_code;
	unsigned int numparam;
	unsigned char fillVal;
	paramType* params = NULL;

	/* Read subroutine value */
	pInput = strtok(NULL, "()\t = \r\n");
	if (strcmp(pInput, "subroutine") != 0) {
		printf("Error, subroutine expected\n");
		return -1;
	}
	pInput = strtok(NULL, "()\t = \r\n");
	if (readSW(pInput, &subrtn_code) < 0){
		printf("Error invalid subroutine code\n");
		return -1;
	}

	/* read num-parameters */
	pInput = strtok(NULL, "()\t = \r\n");
	if (strcmp(pInput, "num-parameters") != 0) {
		printf("Error, num-parameters expected\n");
		return -1;
	}
	pInput = strtok(NULL, "()\t = \r\n");
	if (readLW(pInput, &numparam) < 0){
		printf("Error invalid number of parameters\n");
		return -1;
	}

	if (numparam > 0){

		/* read align-fill-byteval */
		pInput = strtok(NULL, "()\t = \r\n");
		if (strcmp(pInput, "align-fill-byteval") != 0) {
			printf("Error, align-fill-byteval expected\n");
			return -1;
		}
		pInput = strtok(NULL, "()\t = \r\n");
		if (readBYTE(pInput, &fillVal) < 0){
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
		pInput = strtok(NULL, "()\t = \r\n");
		if (strcmp(pInput, "parameter-types") != 0) {
			printf("Error, parameter-types expected\n");
			return -1;
		}
		for (x = 0; x < (int)numparam; x++){
			pInput = strtok(NULL, "()\t = \r\n");
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
		pInput = strtok(NULL, "()\t = \r\n");
		if (strcmp(pInput, "parameter-values") != 0) {
			printf("Error, parameter-values expected\n");
			return -1;
		}
		for (x = 0; x < (int)numparam; x++){
			if ((params[x].type == ALIGN_2_PARAM) || (params[x].type == ALIGN_4_PARAM))
				continue;

			pInput = strtok(NULL, "()\t = \r\n");
			if (readLW(pInput, &params[x].value) < 0){
				printf("Error invalid parameter value\n");
				return -1;
			}
		}
	}

	/* Create a script node */
	node->nodeType = NODE_EXE_SUB;
	node->id = id;
	node->subroutine_code = subrtn_code;
	node->num_parameters = numparam;
	if (numparam > 0){
		node->alignfillVal = fillVal;
		node->subParams = params;
	}

	return 0;
}


/******************************/
/* run-commands               */
/******************************/
int copy_runcmds(int id, scriptNode* node){

	runParamType* rpNode;
	runParamType* rpHead = NULL;
	runParamType* pPrev = NULL;
	int len = 0;

	/* read series of commands until the end of them is reached */
	pInput = strtok(NULL, "()\t = \r\n");
	while (strcmp(pInput, "commands-end") != 0) {

		/* Create a runcmds parameter */
		rpNode = (runParamType*)malloc(sizeof(runParamType));
		if (rpNode == NULL){
			printf("Error allocing space for run parameter struct.\n");
			return -1;
		}
		rpNode->str = NULL;
		rpNode->pNext = NULL;

		/* Book keeping */
		if (rpHead == NULL)
			rpHead = rpNode;
		if (pPrev != NULL){
			pPrev->pNext = rpNode;
		}

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
			len = strlen(pText);

			rpNode->type = PRINT_LINE;
			rpNode->str = (char*)malloc(len + 1);
			memset(rpNode->str, 0, len + 1);
			memcpy(rpNode->str, pText, len);
		}

		/* show-portrait */
		else if (strcmp(pInput, "show-portrait") == 0){
			unsigned char portraitCode;
			pInput = strtok(NULL, "()\t = \r\n");
			if (readBYTE(pInput, &portraitCode) < 0){
				printf("Error invalid portrait code.\n");
				return -1;
			}

			/* Create a runcmds parameter */
			rpNode->type = SHOW_PORTRAIT;
			rpNode->value = portraitCode;
		}

		/* control-code */
		else if (strcmp(pInput, "control-code") == 0){
			unsigned short ctrlCode;
			pInput = strtok(NULL, "()\t = \r\n");
			if (readSW(pInput, &ctrlCode) < 0){
				printf("Error invalid control code.\n");
				return -1;
			}

			/* Create a runcmds parameter */
			rpNode->type = CTRL_CODE;
			rpNode->value = ctrlCode;
		}

		/* align-2 */
		else if (strcmp(pInput, "align-2") == 0){
			unsigned char fillVal;
			pInput = strtok(NULL, "()\t = \r\n");
			if (readBYTE(pInput, &fillVal) < 0){
				printf("Error invalid fill value.\n");
				return -1;
			}

			/* Create a runcmds parameter */
			rpNode->type = ALIGN_2_PARAM;
			rpNode->value = fillVal;
		}

		/* align-4 */
		else if (strcmp(pInput, "align-4") == 0){
			unsigned char fillVal;
			pInput = strtok(NULL, "()\t = \r\n");
			if (readBYTE(pInput, &fillVal) < 0){
				printf("Error invalid fill value.\n");
				return -1;
			}

			/* Create a runcmds parameter */
			rpNode->type = ALIGN_4_PARAM;
			rpNode->value = fillVal;
		}

		/* Unknown */
		else{
			printf("Error unknown command %s detected in run-commands\n", pInput);
			return -1;
		}

		pPrev = rpNode;

		/* Read next token */
		pInput = strtok(NULL, "()\t = \r\n");
	}

	/* Create a script node */
	node->nodeType = NODE_RUN_CMDS;
	node->id = id;
	node->runParams = rpHead;

	return 0;
}
