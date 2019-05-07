/*****************************************************************************/
/* parse_script.c : Code to parse metadata script for lunar SSS              */
/*****************************************************************************/
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

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
static int radix_type = RADIX_HEX;

/* Function Prototypes */
int encodeScript(FILE* infile, FILE* outfile);
int decode_goto(int id);
int decode_fill(int id);
int decode_pointer(int id);
int decode_exesub(int id);
int decode_runcmds(int id);
int decode_options(int id);




/*****************************************************************************/
/* Function: encodeScript                                                    */
/* Purpose: Parses a metadata text version of the script, and puts it into   */
/*          a linked list data structure in memory.                          */
/* Inputs:  Pointers to input/output files.                                  */
/* Outputs: 0 on Pass, -1 on Fail.                                           */
/*****************************************************************************/
int encodeScript(FILE* infile, FILE* outfile){

    int rval, output_endian_type;
    unsigned int fsize, max_size_bytes;
    unsigned char* pBuffer = NULL;

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
    pInput = (unsigned char*)strtok((char *)pBuffer, "() \t=\r\n");
    if ((pInput == NULL) || (strcmp((char *)pInput, "start") != 0)) {
        printf("Error, start not found\n");
        return -1;
    }

    /*********/
    /* Start */
    /*********/

    /* start - endian (big or little) */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((char *)pInput, "endian") != 0) {
        printf("Error, start endian not found\n");
        return -1;
    }

    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((char *)pInput, "big") == 0) {
        output_endian_type = LUNAR_BIG_ENDIAN;
        printf("Setting Big Endian.\n");
    }
    else if (strcmp((char *)pInput, "little") == 0){
        output_endian_type = LUNAR_LITTLE_ENDIAN;
        printf("Setting Little Endian.\n");
    }
    else{
        printf("Invalid Endian.\n");
        return -1;
    }
    setBinOutputMode(output_endian_type);

    /* start - radix (hex or decimal) */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((char *)pInput, "radix") != 0) {
        printf("Error, start radix not found\n");
        return -1;
    }
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((char *)pInput, "hex") == 0) {
        radix_type = RADIX_HEX;
        printf("Setting Radix to HEX.\n");
    }
    else if(strcmp((char *)pInput, "dec") == 0) {
        radix_type = RADIX_DEC;
        printf("Setting Radix to DEC.\n");
    }
    else{
        printf("Unknown Radix.\n");
        return -1;
    }
    setMetaScriptInputMode(radix_type);

    /* start - max_size_bytes */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((char *)pInput, "max_size_bytes") != 0) {
        printf("Error, max_size_bytes not found\n");
        return -1;
    }
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (readLW(pInput, &max_size_bytes) < 0){
        printf("Error invalid max_size_bytes\n");
        return -1;
    }

    /* Default to sega saturn limitation if not set */
    if (max_size_bytes == 0)
        max_size_bytes = 0x10000;
    setBinMaxSize(max_size_bytes);


    /************************************************************/
    /* Parse the rest of the file until EOF or "end" is located */
    /************************************************************/
    rval = 0;
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    while ((pInput != NULL) && rval == 0){
        int id;

        /* goto */
        if (strcmp((char *)pInput, "goto") == 0){
            id = read_ID(pInput);
            rval = decode_goto(id);
        }

        /* fill-space */
        else if (strcmp((char *)pInput, "fill-space") == 0){
            id = read_ID(pInput);
            rval = decode_fill(id);
        }

        /* pointer */
        else if (strcmp((char *)pInput, "pointer") == 0){
            id = read_ID(pInput);
            rval = decode_pointer(id);
        }

        /* execute-subroutine */
        else if (strcmp((char *)pInput, "execute-subroutine") == 0){
            id = read_ID(pInput);
            rval = decode_exesub(id);
        }

        /* run-commands */
        else if (strcmp((char *)pInput, "run-commands") == 0){
            id = read_ID(pInput);
            rval = decode_runcmds(id);
        }

        /* options */
        else if (strcmp((char *)pInput, "options") == 0){
            id = read_ID(pInput);
            rval = decode_options(id);
        }

        /* end */
        else if (strcmp((char *)pInput, "end") == 0){
            printf("Detected END\n");
            break;
        }
        else{
            printf("Invalid Command Detected.\n");
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


/******************************/
/* goto                       */
/******************************/
int decode_goto(int id){

    unsigned int offset;
    scriptNode* newNode = NULL;

    /* read location */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((char *)pInput, "location") != 0) {
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
    createScriptNode(&newNode);
    newNode->nodeType = NODE_GOTO;
    newNode->id = id;
    newNode->byteOffset = offset;

    /* Add the script node to the list */
    addNode(newNode, METHOD_NORMAL,0);
    free(newNode);

    return 0;
}


/******************************/
/* fill                       */
/******************************/
int decode_fill(int id){

    scriptNode* newNode;
    unsigned int unitSize = 0;
    unsigned int unitCount = 0;
    unsigned int fillValue = 0;

    /* read unit-size */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((char *)pInput, "unit-size") != 0) {
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
    if (strcmp((char *)pInput, "fill-value") != 0) {
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
    if (strcmp((char *)pInput, "unit-count") != 0) {
        printf("Error, unit-count expected\n");
        return -1;
    }
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (readLW(pInput, &unitCount) < 0){
        printf("Error invalid unit count\n");
        return -1;
    }

    /* Create a script node */
    createScriptNode(&newNode);
    newNode->nodeType = NODE_FILL_SPACE;
    newNode->id = id;
    newNode->unit_size = unitSize;
    newNode->fillVal = fillValue;
    newNode->unit_count = unitCount;

    /* Add the script node to the list */
    addNode(newNode, METHOD_NORMAL,0);
    free(newNode);

    return 0;
}


/******************************/
/* pointer                    */
/******************************/
int decode_pointer(int id){

    scriptNode* newNode;
    unsigned int byteOffset;
    unsigned int dataSize;
    unsigned int value;
    unsigned int id_link;
    unsigned int value_selected = 0;

    /* read byteoffset */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((char *)pInput, "byteoffset") != 0) {
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
    if (strcmp((char *)pInput, "size") != 0) {
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
    if (strcmp((char *)pInput, "value") == 0) {
        pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
        if (readLW(pInput, &value) < 0){
            printf("Error invalid value\n");
            return -1;
        }
        value_selected = 1;
    }
    else if (strcmp((char *)pInput, "id-link") == 0) {
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
    createScriptNode(&newNode);
    newNode->nodeType = NODE_POINTER;
    newNode->id = id;
    newNode->byteOffset = byteOffset;
    newNode->ptrSize = dataSize;
    newNode->ptrValueFlag = value_selected;
    if (value_selected){
        newNode->ptrValue = value;
    }
    else{
        newNode->ptrID = id_link;
    }

    /* Add the script node to the list */
    addNode(newNode, METHOD_NORMAL,0);
    free(newNode);

    return 0;
}


/******************************/
/* execute-subroutine         */
/******************************/
int decode_exesub(int id){
    
    scriptNode* newNode;
    int x;
    unsigned short subrtn_code;
    unsigned int numparam;
    unsigned char fillVal;
    paramType* params = NULL;

    /* Read subroutine value */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((char *)pInput, "subroutine") != 0) {
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
    if (strcmp((char *)pInput, "num-parameters") != 0) {
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
        if (strcmp((char *)pInput, "align-fill-byteval") != 0) {
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
        if (strcmp((char *)pInput, "parameter-types") != 0) {
            printf("Error, parameter-types expected\n");
            return -1;
        }
        for (x = 0; x < (int)numparam; x++){
            pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
            if (strcmp((char *)pInput, "1") == 0)
                params[x].type = BYTE_PARAM;
            else if (strcmp((char *)pInput, "2") == 0)
                params[x].type = SHORT_PARAM;
            else if (strcmp((char *)pInput, "4") == 0)
                params[x].type = LONG_PARAM;
            else if (strcmp((char *)pInput, "align-2") == 0)
                params[x].type = ALIGN_2_PARAM;
            else if (strcmp((char *)pInput, "align-4") == 0)
                params[x].type = ALIGN_4_PARAM;
            else{
                printf("Error invalid parameter type read\n");
                return -1;
            }
        }

        /* Parameter Values */
        pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
        if (strcmp((char *)pInput, "parameter-values") != 0) {
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
    createScriptNode(&newNode);
    newNode->nodeType = NODE_EXE_SUB;
    newNode->id = id;
    newNode->subroutine_code = subrtn_code;
    newNode->num_parameters = numparam;
    if (numparam > 0){
        newNode->alignfillVal = fillVal;
        newNode->subParams = params;
    }
    else{
        newNode->subParams = NULL;
    }
    newNode->runParams = NULL;

    /* Add the script node to the list */
    addNode(newNode, METHOD_NORMAL,0);
    free(newNode);

    return 0;
}


/******************************/
/* run-commands               */
/******************************/
int decode_runcmds(int id){

    scriptNode* newNode = NULL;
    runParamType* rpNode = NULL;
    runParamType* rpHead = NULL;
    runParamType* pPrev = NULL;
    int len = 0;

    /* read series of commands until the end of them is reached */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    while (strcmp((char *)pInput, "commands-end") != 0) {

        /* Create a runcmds parameter */
        rpNode = (runParamType*)malloc(sizeof(runParamType));
        if(rpNode == NULL){
            printf("Error allocing space for run parameter struct.\n");
            return -1;
        }
        rpNode->str = NULL;
        rpNode->pNext = NULL;

        /* Book keeping */
        if(rpHead == NULL)
            rpHead = rpNode;
        if(pPrev != NULL){
            pPrev->pNext = rpNode;
        }

        /* print-line */
        if (strcmp((char *)pInput, "print-line") == 0){
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
            rpNode->str = malloc(len+1);
            memset(rpNode->str,0,len+1);
            memcpy(rpNode->str,pText,len);
        }
        
        /* show-portrait-left */
        else if (strcmp((char *)pInput, "show-portrait-left") == 0){
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
        else if (strcmp((char *)pInput, "show-portrait-right") == 0){
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
        else if (strcmp((char *)pInput, "time-delay") == 0){
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
        else if (strcmp((char *)pInput, "control-code") == 0){
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
        else if (strcmp((char *)pInput, "align-2") == 0){
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
        else if (strcmp((char *)pInput, "align-4") == 0){
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
            printf("Error unknown command %s detected in run-commands\n",pInput);
            return -1;
        }

        pPrev = rpNode;

        /* Read next token */
        pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    }

    /* Create a script node */
    createScriptNode(&newNode);
    newNode->nodeType = NODE_RUN_CMDS;
    newNode->id = id;
    newNode->subroutine_code = 0x0002;
    newNode->runParams = rpHead;
    newNode->subParams = NULL;

    /* Add the script node to the list */
    addNode(newNode, METHOD_NORMAL,0);
    free(newNode);

    return 0;
}




/*************************/
/* options               */
/*************************/
int decode_options(int id){

    int x;
    scriptNode* node = NULL;
    unsigned short jmpParam, param2;
    paramType* params = NULL;
    runParamType* rpNode;
    runParamType* rpHead = NULL;
    runParamType* rpHead1 = NULL;
    runParamType* pPrev = NULL;
    int len = 0;

    /* Create a script node */
    createScriptNode(&node);

    /***********************************************/
    /* Read in the two fixed subroutine parameters */
    /***********************************************/

    /* JMP Offset */
    pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
    if (strcmp((char *)pInput, "jmpparam") != 0) {
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
    if (strcmp((char *)pInput, "param2") != 0) {
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
            if (strcmp((char *)pInput, "opt1") != 0) {
                printf("Error, opt1 expected\n");
                return -1;
            }
        }
        else{
            rpHead1 = rpHead;
            rpHead = NULL;
            pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
            if (strcmp((char *)pInput, "opt2") != 0) {
                printf("Error, opt2 expected\n");
                return -1;
            }
        }
        pPrev = NULL;
        pInput = (unsigned char*)strtok(NULL, "()\t = \r\n");
        while (strcmp((char *)pInput, "opt-end") != 0) {

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
            if (strcmp((char *)pInput, "print-line") == 0){
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

            /* control-code */
            else if (strcmp((char *)pInput, "control-code") == 0){
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
            else if (strcmp((char *)pInput, "align-2") == 0){
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
            else if (strcmp((char *)pInput, "align-4") == 0){
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

    /* Add the script node to the list */
    addNode(node, METHOD_NORMAL, 0);
    free(node);

    return 0;
}
