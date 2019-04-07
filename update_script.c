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
int copy_options(int id, scriptNode* node);



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
    pInput = (unsigned char*)strtok((char*)pBuffer, "() \t=\r\n");
    if ((pInput == NULL) || (strcmp((const char *)pInput, "start") != 0)) {
        printf("Error, start not found\n");
        return -1;
    }



    /************************************************************/
    /* Parse the rest of the file until EOF or "end" is located */
    /************************************************************/
    rval = 0;
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    while ((pInput != NULL) && rval == 0){
        int id;
        scriptNode node;
		node.runParams = node.runParams2 = NULL;
		node.subParams = NULL;
		node.pNext = node.pPrev = NULL;

        /* insert-before-ID */
        if (strcmp((const char *)pInput, "insert-before-ID") == 0){
            id = read_ID(pInput);
            readNode(&node);
            rval = addNode(&node, METHOD_INSERT_BEFORE, id);
        }

        /* insert-after-ID */
        else if (strcmp((const char *)pInput, "insert-after-ID") == 0){
            id = read_ID(pInput);
            readNode(&node);
            rval = addNode(&node, METHOD_INSERT_AFTER, id);
        }

        /* remove-ID */
        else if (strcmp((const char *)pInput, "remove-ID") == 0){
            id = read_ID(pInput);
            rval = removeNode(id);
        }

        /* overwrite-ID */
        else if (strcmp((const char *)pInput, "overwrite-ID") == 0){
            id = read_ID(pInput);
            readNode(&node);
            rval = overwriteNode(id, &node);
        }

        /* end */
        else if (strcmp((const char *)pInput, "end") == 0){
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
        pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    }

    return 0;
}




int readNode(scriptNode* node){

    int rval = 0;
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if ((pInput != NULL) && rval == 0){
        int id;

        /* goto */
        if (strcmp((const char *)pInput, "goto") == 0){
            id = read_ID(pInput);
            rval = copy_goto(id, node);
        }

        /* fill-space */
        else if (strcmp((const char *)pInput, "fill-space") == 0){
            id = read_ID(pInput);
            rval = copy_fill(id, node);
        }

        /* pointer */
        else if (strcmp((const char *)pInput, "pointer") == 0){
            id = read_ID(pInput);
            rval = copy_pointer(id, node);
        }

        /* execute-subroutine */
        else if (strcmp((const char *)pInput, "execute-subroutine") == 0){
            id = read_ID(pInput);
            rval = copy_exesub(id, node);
        }

        /* run-commands */
        else if (strcmp((const char *)pInput, "run-commands") == 0){
            id = read_ID(pInput);
            rval = copy_runcmds(id, node);
        }

		/* options */
		else if (strcmp((const char *)pInput, "options") == 0){
			id = read_ID(pInput);
			rval = copy_options(id, node);
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

    memset(node, 0, sizeof(scriptNode));

    /* read location */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((const char *)pInput, "location") != 0) {
        printf("Error, location expected\n");
        return -1;
    }

    /* read offset */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
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

    memset(node, 0, sizeof(scriptNode));

    /* read unit-size */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((const char *)pInput, "unit-size") != 0) {
        printf("Error, unit-size expected\n");
        return -1;
    }
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (readLW(pInput, &unitSize) < 0){
        printf("Error invalid goto offset\n");
        return -1;
    }

    /* read fill-value */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((const char *)pInput, "fill-value") != 0) {
        printf("Error, fill-value expected\n");
        return -1;
    }
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (readLW(pInput, &fillValue) < 0){
        printf("Error invalid unit size\n");
        return -1;
    }

    /* read unit-count */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((const char *)pInput, "unit-count") != 0) {
        printf("Error, unit-count expected\n");
        return -1;
    }
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
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

    memset(node, 0, sizeof(scriptNode));

    /* read byteoffset */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((const char *)pInput, "byteoffset") != 0) {
        printf("Error, byteoffset expected\n");
        return -1;
    }
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (readLW(pInput, &byteOffset) < 0){
        printf("Error invalid pointer byte offset\n");
        return -1;
    }

    /* read size of pointer */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((const char *)pInput, "size") != 0) {
        printf("Error, size expected\n");
        return -1;
    }
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (readLW(pInput, &dataSize) < 0){
        printf("Error invalid pointer data size\n");
        return -1;
    }

    /* read value or id to point to */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((const char *)pInput, "value") == 0) {
        pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
        if (readLW(pInput, &value) < 0){
            printf("Error invalid value\n");
            return -1;
        }
        value_selected = 1;
    }
    else if (strcmp((const char *)pInput, "id-link") == 0) {
        pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
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
    if (value_selected)
        node->ptrValue = value;
    else
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

    memset(node, 0, sizeof(scriptNode));

    /* Read subroutine value */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((const char *)pInput, "subroutine") != 0) {
        printf("Error, subroutine expected\n");
        return -1;
    }
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (readSW(pInput, &subrtn_code) < 0){
        printf("Error invalid subroutine code\n");
        return -1;
    }

    /* read num-parameters */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((const char *)pInput, "num-parameters") != 0) {
        printf("Error, num-parameters expected\n");
        return -1;
    }
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (readLW(pInput, &numparam) < 0){
        printf("Error invalid number of parameters\n");
        return -1;
    }

    if (numparam > 0){

        /* read align-fill-byteval */
        pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
        if (strcmp((const char *)pInput, "align-fill-byteval") != 0) {
            printf("Error, align-fill-byteval expected\n");
            return -1;
        }
        pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
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
        pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
        if (strcmp((const char *)pInput, "parameter-types") != 0) {
            printf("Error, parameter-types expected\n");
            return -1;
        }
        for (x = 0; x < (int)numparam; x++){
            pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
            if (strcmp((const char *)pInput, "1") == 0)
                params[x].type = BYTE_PARAM;
            else if (strcmp((const char *)pInput, "2") == 0)
                params[x].type = SHORT_PARAM;
            else if (strcmp((const char *)pInput, "4") == 0)
                params[x].type = LONG_PARAM;
            else if (strcmp((const char *)pInput, "align-2") == 0)
                params[x].type = ALIGN_2_PARAM;
            else if (strcmp((const char *)pInput, "align-4") == 0)
                params[x].type = ALIGN_4_PARAM;
            else{
                printf("Error invalid parameter type read\n");
                return -1;
            }
        }

        /* Parameter Values */
        pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
        if (strcmp((const char *)pInput, "parameter-values") != 0) {
            printf("Error, parameter-values expected\n");
            return -1;
        }
        for (x = 0; x < (int)numparam; x++){
            if ((params[x].type == ALIGN_2_PARAM) || (params[x].type == ALIGN_4_PARAM))
                continue;

            pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
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
    node->runParams = NULL;

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

    memset(node, 0, sizeof(scriptNode));

    /* read series of commands until the end of them is reached */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    while (strcmp((const char *)pInput, "commands-end") != 0) {

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
        if (strcmp((const char *)pInput, "print-line") == 0){
            unsigned char* pText;

            /* Get Text String (UTF-8) */
            pInput = (unsigned char*)strtok(NULL, "\"");
            if (pInput == NULL){
                printf("Error, bad text input.\n");
                return -1;
            }
            pText = pInput;
            len = strlen((char *)pText);

            rpNode->type = PRINT_LINE;
            rpNode->str = malloc(len + 1);
            memset(rpNode->str, 0, len + 1);
            memcpy(rpNode->str, pText, len);
        }

		/* show-portrait-left */
		else if (strcmp((const char *)pInput, "show-portrait-left") == 0){
			unsigned char portraitCode;
			pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
			if (readBYTE(pInput, &portraitCode) < 0){
				printf("Error invalid portrait code.\n");
				return -1;
			}

			/* Create a runcmds parameter */
			rpNode->type = SHOW_PORTRAIT_LEFT;
			rpNode->value = portraitCode;
		}

		/* show-portrait-right */
		else if (strcmp((const char *)pInput, "show-portrait-right") == 0){
			unsigned char portraitCode;
			pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
			if (readBYTE(pInput, &portraitCode) < 0){
				printf("Error invalid portrait code.\n");
				return -1;
			}

			/* Create a runcmds parameter */
			rpNode->type = SHOW_PORTRAIT_RIGHT;
			rpNode->value = portraitCode;
		}

		/* time-delay */
		else if (strcmp((const char *)pInput, "time-delay") == 0){
			unsigned char timedelay;
			pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
			if (readBYTE(pInput, &timedelay) < 0){
				printf("Error invalid time delay.\n");
				return -1;
			}

			/* Create a runcmds parameter */
			rpNode->type = TIME_DELAY;
			rpNode->value = timedelay;
		}

        /* control-code */
        else if (strcmp((const char *)pInput, "control-code") == 0){
            unsigned short ctrlCode;
            pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
            if (readSW(pInput, &ctrlCode) < 0){
                printf("Error invalid control code.\n");
                return -1;
            }

            /* Create a runcmds parameter */
            rpNode->type = CTRL_CODE;
            rpNode->value = ctrlCode;
        }

        /* align-2 */
        else if (strcmp((const char *)pInput, "align-2") == 0){
            unsigned char fillVal;
            pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
            if (readBYTE(pInput, &fillVal) < 0){
                printf("Error invalid fill value.\n");
                return -1;
            }

            /* Create a runcmds parameter */
            rpNode->type = ALIGN_2_PARAM;
            rpNode->value = fillVal;
        }

        /* align-4 */
        else if (strcmp((const char *)pInput, "align-4") == 0){
            unsigned char fillVal;
            pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
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
        pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    }

    /* Create a script node */
    node->nodeType = NODE_RUN_CMDS;
    node->id = id;
	node->subroutine_code = 0x0002;
    node->runParams = rpHead;
    node->subParams = NULL;

    return 0;
}




/*************************/
/* options               */
/*************************/
int copy_options(int id, scriptNode* node){

	int x;
	unsigned short jmpParam, param2;
	paramType* params = NULL;
	runParamType* rpNode;
	runParamType* rpHead = NULL;
	runParamType* rpHead1 = NULL;
	runParamType* pPrev = NULL;
	int len = 0;

	memset(node, 0, sizeof(scriptNode));

	/***********************************************/
	/* Read in the two fixed subroutine parameters */
	/***********************************************/

	/* JMP Offset */
	pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
	if (strcmp((const char *)pInput, "jmpparam") != 0) {
		printf("Error, jmpparam expected\n");
		return -1;
	}
	pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
	if (readSW(pInput, &jmpParam) < 0){
		printf("Error invalid subroutine code\n");
		return -1;
	}

	/* 2nd Parameter */
	pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
	if (strcmp((const char *)pInput, "param2") != 0) {
		printf("Error, param2 expected\n");
		return -1;
	}
	pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
	if (readSW(pInput, &param2) < 0){
		printf("Error invalid subroutine code\n");
		return -1;
	}

	/* Allocate Subroutine Parameters and copy */
	params = (paramType*)malloc(2 * sizeof(paramType));
	if (params == NULL){
		printf("Error allocing memory for parameters\n");
		return -1;
	}
	params[0].type = SHORT_PARAM;
	params[0].value = jmpParam;
	params[1].type = SHORT_PARAM;
	params[1].value = param2;

	/* read in the options until the end of them is reached */
	for (x = 0; x < 2; x++){

		if (x == 0){
			pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
			if (strcmp((const char *)pInput, "opt1") != 0) {
				printf("Error, opt1 expected\n");
				return -1;
			}
		}
		else{
			rpHead1 = rpHead;
			rpHead = NULL;
			pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
			if (strcmp((const char *)pInput, "opt2") != 0) {
				printf("Error, opt2 expected\n");
				return -1;
			}
		}
		pPrev = NULL;
		pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
		while (strcmp((const char *)pInput, "opt-end") != 0) {

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
			if (strcmp((const char *)pInput, "print-line") == 0){
				unsigned char* pText;

				/* Get Text String (UTF-8) */
				pInput = (unsigned char*)strtok(NULL, "\"");
				if (pInput == NULL){
					printf("Error, bad text input.\n");
					return -1;
				}
				pText = pInput;
				len = strlen((const char *)pText);

				rpNode->type = PRINT_LINE;
				rpNode->str = malloc(len + 1);
				memset(rpNode->str, 0, len + 1);
				memcpy(rpNode->str, pText, len);
			}

			/* control-code */
			else if (strcmp((const char *)pInput, "control-code") == 0){
				unsigned short ctrlCode;
				pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
				if (readSW(pInput, &ctrlCode) < 0){
					printf("Error invalid control code.\n");
					return -1;
				}

				/* Create a runcmds parameter */
				rpNode->type = CTRL_CODE;
				rpNode->value = ctrlCode;
			}

			/* align-2 */
			else if (strcmp((const char *)pInput, "align-2") == 0){
				unsigned char fillVal;
				pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
				if (readBYTE(pInput, &fillVal) < 0){
					printf("Error invalid fill value.\n");
					return -1;
				}

				/* Create a runcmds parameter */
				rpNode->type = ALIGN_2_PARAM;
				rpNode->value = fillVal;
			}

			/* align-4 */
			else if (strcmp((const char *)pInput, "align-4") == 0){
				unsigned char fillVal;
				pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
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
			pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
		}
	}

	/* Create a script node */
	node->nodeType = NODE_OPTIONS;
	node->id = id;
	node->subroutine_code = 0x0007;
	node->alignfillVal = 0xFF;
	node->num_parameters = 2;
	node->runParams = rpHead1;
	node->runParams2 = rpHead;
	node->subParams = params;

	return 0;
}
