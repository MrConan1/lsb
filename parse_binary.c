/*****************************************************************************/
/* parse_binary.c : Code to parse lunar SSS and maintain script linkage      */
/*****************************************************************************/

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "script_node_types.h"
#include "snode_list.h"
#include "util.h"
#include "bpe_compression.h"

/* Defines */
#define DBUF_SIZE      (128*1024)     /* 128kB */
#define PTR_ARRAY_SIZE (2*1024)       /* 0x800 bytes, or 1024 16-bit LWs */

#define UGLY_ENG_IOS_HACKS

/* Globals */
static char* pdata = NULL;
static char* pdata2 = NULL;
char* pName = NULL;  //Basic Filename

static int G_ID = 1;


/* Function Prototypes */
int decodeBinaryScript(FILE* inFile, FILE* outFile);
int parseCmdSeq(int offset, FILE** ptr_inFile, int singleRunFlag);
int encodeScript(FILE* inFile, FILE* outFile);
runParamType* getRunParam(int textMode, char* pdata);

extern int G_IOS_ENG;


/*****************************************************************************/
/* Function: decodeBinaryScript                                              */
/* Purpose: Parses the script starting at offset 0x0800.                     */
/*          Then considers the top level pointers.                           */
/* Inputs:  Pointers to input/output files.                                  */
/* Outputs: 0 on Pass, -1 on Fail.                                           */
/*****************************************************************************/
int decodeBinaryScript(FILE* inFile, FILE* outFile){

    unsigned short ptrVal;
//	unsigned int ptrID;
    unsigned int iFileSizeBytes;
    int x;
    unsigned short* pIndexPtrs = NULL;
//	scriptNode* pScriptNode = NULL;

    /* Allocate two 128kB buffers, much bigger than the input file */
    if (pdata != NULL){
        free(pdata);
        pdata = NULL;
    }
    pdata = (char*)malloc(DBUF_SIZE);
    if (pdata == NULL){
        printf("Error allocating space for file data buffer.\n");
        return -1;
    }

    if (pdata2 != NULL){
        free(pdata2);
        pdata2 = NULL;
    }
    pdata2 = (char*)malloc(DBUF_SIZE);
    if (pdata2 == NULL){
        printf("Error allocating space for file data buffer 2.\n");
        return -1;
    }

    /* Determine Input File Size */
    if (fseek(inFile, 0, SEEK_END) != 0){
        printf("Error seeking in input file.\n");
        return -1;
    }
    iFileSizeBytes = ftell(inFile);
    if (fseek(inFile, 0, SEEK_SET) != 0){
        printf("Error seeking in input file.\n");
        return -1;
    }


    /*********************************************/
    /* Step 1: Read the script from start to end */
    /*********************************************/
    if (parseCmdSeq(0x0800, &inFile, 0) != 0){
        printf("Error Detected while reading from input file.\n");
        return -1;
    }

    /* Allocate memory for the array */
    pIndexPtrs = (unsigned short*)malloc(PTR_ARRAY_SIZE);
    if (pIndexPtrs == NULL){
        printf("Error allocing memory for Index Ptr Array\n");
        return -1;
    }


    /**********************************************************/
    /* Step 2: Verify each script pointer has a valid target. */
    /**********************************************************/
    fseek(inFile, 0, SEEK_SET);

    for (x = 0; x < 1024; x++){
        unsigned int byteOffset;
        unsigned int currentLocation;
        scriptNode* pNode, *sNode;

        //Read in the pointer value
        if (fread(&ptrVal, 2, 1, inFile) != 1){
            printf("Error Reading Pointer Value\n");
            return -1;
        }
        swap16(&ptrVal);

        //Verify validity
        byteOffset = (unsigned int)ptrVal * 2;
        if ((byteOffset == 0x0000) || (byteOffset >= iFileSizeBytes))
            continue;

        //Verify mapping to an existing script node
        pNode = getListItemByOffset(byteOffset);
        if (pNode == NULL){
            printf("SCRIPT ERROR, POSSIBLE OVERLAP DETECTED.\n");
            currentLocation = ftell(inFile);
            /* Add it anyway - one file should have this issue and this works */
            if (parseCmdSeq(byteOffset, &inFile, 1) != 0){
                printf("Error Detected while reading from input file.\n");
                return -1;
            }
            pNode = getListItemByOffset(byteOffset);
            fseek(inFile, currentLocation, SEEK_SET);
        }

        // Sanity
        if (pNode == NULL){
            printf("Program bug, could not locate item in list corresponding to offset should not get here.\n");
            return -1;
        }

        //Associate Script Node with the Short offset of the Pointer that points to it
        //This is done for book-keeping reasons for the CSV Script Dump
        pNode->pointerID = x;

        //Add pointer as a node
        /* Create a new script node */
        if (createScriptNode(&sNode) < 0){
            printf("Error creating a script node.\n");
            return -1;
        }

        /* Fill in Parameters */
        sNode->id = G_ID++;
        sNode->nodeType = NODE_POINTER;
        sNode->ptrSize = 2;
        sNode->ptrValueFlag = 0;
        sNode->ptrID = pNode->id;
        sNode->byteOffset = x * 2;
        sNode->fileOffset = byteOffset;

        /* Add the node */
        if (addNode(sNode, METHOD_NORMAL, 0) != 0){
            printf("Error occurred adding the script node.\n");
            return -1;
        }
        free(sNode);
    }

#if 0
    /************************************************************************/
    /* Step 3: For linear runs of the script, assign the same Node ID.      */
    /*         Used for CSV Script dump.  Provides some indication of flow. */
    /************************************************************************/
    pScriptNode = getHeadPtr();
    while (pScriptNode != NULL){
        scriptNode* pNext = pScriptNode->pNext;

        /* Update ptrID */
        ptrID = pScriptNode->pointerID;

        /* Check to see if ptrID should be reset due to end of script sequence reached */
        switch (pScriptNode->nodeType){
            case 0x0003:  /* Unconditional JMP */
            case 0x0004:  /* Unconditional JMP */
            case 0x0005:  /* Return */
            case 0x0026:  /* Return & Jump into new scene */
            case 0x0042:  /* Unconditional JMP */
                ptrID = INVALID_PTR_ID;
                break;

            default:
                break; /* Nothing to be done */
        }

        /* If the next node has an invalid PTR ID, and the current            */
        /* Node is not a "Return" node, and the current Node has a valid PTR  */
        /* associated with it, also associate that pointer with the next item */
        /* in the script node list. */
        if (pNext != NULL){
            if ((pNext->ptrID == INVALID_PTR_ID) &&
                (ptrID != INVALID_PTR_ID) ){
                pNext->ptrID = ptrID;
            }
        }

        pScriptNode = pScriptNode->pNext;
    }
#endif
#if 0
case 0x0003: /* 0x0003 - Unconditional Jump, 1 short Arg */
case 0x0004: /* 0x0004 - Unconditional Jump, 1 short Arg */
case 0x0007: /* SELECT */
case 0x0010: /* JUMP (?) */
case 0x0011: /* JUMP (?) */
case 0x0016: /* Conditional JUMP (?) */
case 0x000B: /* Conditional TRUE (AND?) */
case 0x000C: /* Conditional FALSE (NOT?) */
case 0x000D: /* (OR?) - JUMP OR JUST "OR"?*/
case 0x0042: /* Unconditional JUMP (?) */
#endif

    /* Free memory */
    if (pdata != NULL)
        free(pdata);
    if (pdata2 != NULL)
        free(pdata2);


    return 0;
}




/*****************************************************************************/
/* Function: parseCmdSeq                                                     */
/* Purpose: Parses a sequence of script commands into a tree structure.      */
/* Inputs:  Pointer to current node in the script.                           */
/*          Byte offset into file to read from.                              */
/* Outputs: None.                                                            */
/*****************************************************************************/
int parseCmdSeq(int offset, FILE** ptr_inFile, int singleRunFlag){

    scriptNode* sNode;
    paramType* params;
    int rval, z;
    int offsetAddress/*, offsetAddress2*/;     /* File Offset in Bytes */
    unsigned short cmd, wdOffset;
    FILE* inFile;
    unsigned short* pShort = (unsigned short*)pdata;

    /* Update File Pointers */
    inFile = *ptr_inFile;

    /* Go to requested offset */
    if (fseek(inFile, offset, SEEK_SET) != 0){
        printf("Error seeking in input file.\n");
        return -1;
    }

    /* Insert a GOTO Node */
    /* Create a new script node */
    if (createScriptNode(&sNode) < 0){
        printf("Error creating a script node.\n");
        return -1;
    }

    /* Fill in Parameters */
    sNode->id = G_ID++;
    sNode->nodeType = NODE_GOTO;
    sNode->byteOffset = offset;
    sNode->fileOffset = 0xFFFFFFFF; // N/A

    /* Add the node */
    if (addNode(sNode, METHOD_NORMAL, 0) != 0){
        printf("Error occurred adding the script node.\n");
        return -1;
    }
    free(sNode);

    /******************************/
    /* Continue reading until EOF */
    /******************************/
    while (1){

        offset = ftell(inFile);

        /* Read in the Script Command */
        if (fread(&cmd, 2, 1, inFile) != 1){
            if (feof(inFile))
                break;
            printf("Error reading command from input file.\n");
            return -1;
        }
        swap16(&cmd);
#if 0
        printf("CMD = 0x%X  Offset= 0x%X (0x%X short)\n", (unsigned int)cmd, offset, offset / 2);
#endif
        /****************************************************/
        /* Determine what to do based on the Script Command */
        /****************************************************/
        switch (cmd){


            /*==============================*/
            /*= FIXED NUMBERS OF ARGUMENTS =*/
            /*==============================*/


            /******************************/
            /* No arguments with 1 child. */
            /******************************/
            case 0x0039: /*confirmed*/
            case 0x003B: /*confirmed*/
            case 0x002F: /* Restore Life */
            case 0x0032: /* confirmed */
            case 0x003A: /* confirmed */
            case 0x003C:
            case 0x003F: /* confirmed */
            case 0x0040: /* confirmed */
            case 0x004C:
            case 0x004D: /* confirmed */
            case 0x0051: /* confirmed, sometimes crashes during my tests? */
            case 0x0053:
            case 0x0054:
            case 0x0058: /* confirmed */
            case 0x005B: /* confirmed */
    #ifdef UGLY_ENG_IOS_HACKS
            case 0x005E: /* hack for iOS Eng */
            case 0xFF00: /* hack for iOS Eng */
            case 0xFF03: /* hack for iOS Eng */
            case 0xFFFF: /* hack for iOS Eng */
    #endif
            {
                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = 0;
                sNode->fileOffset = offset;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /************************************/
            /* One short argument with 1 child. */
            /************************************/

            case 0x0003: /* 0x0003 - Unconditional Jump, 1 short Arg */
            case 0x0004: /* 0x0004 - Unconditional Jump, 1 short Arg */
            case 0x0020: /* Two byte args to specify an item was obtained.  Text prints using subrtn 0x060B7D30 */
            case 0x0044: /* Really 2 byte args, but basically the same thing */
            case 0x0043: /* Assumption file#1 0x7144  0043 0100 */
            case 0x0028:  /* ASSUMPTION 0028 0800     0028 0900 - mostly traced */
            case 0x0059:  /* Assumption file #1 0x5128 0059 0105*/
            case 0x001F:  /* Display obtained item */
            case 0x0023:  /* partially validated... */
            case 0x0029:  /* two byte arguments */
            case 0x002A:  /* confirmed */
            case 0x002E:  /* Display Shop Menu (Argument is shop type) */
            case 0x0030:
            case 0x0031:
            case 0x0033: /* confirmed */
            case 0x0045: /* Called when Entering/Exiting houses */
            case 0x0047: /* Called when Entering/Exiting houses */
            case 0x0048: /* ASSUMPTION , 1 or no children? */
            case 0x0049: /* confirmed */
            case 0x0052:
            case 0x005A: /* Music Select? */
            case 0x0056:
            case 0x0057: /* two byte arguments */
    #ifdef UGLY_ENG_IOS_HACKS
            case 0x005F: /* hack for iOS Eng */
            case 0x0060: /* hack for iOS Eng */
    #endif
            {
                /* 1 Argument to read */
                fread(&pdata[0], 2, 1, inFile);

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = 1;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                if ((cmd == 0x0003) || (cmd == 0x0004)){
                    sNode->nextPointerID = (unsigned int)params[0].value;
                }

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /************************************/
            /* Two short arguments with 1 child */
            /************************************/
            case 0x0022: /* confirmed, 3 byte args, 1 spare */
            case 0x0021: /* confirmed */
            case 0x002C: /* confirmed */
            case 0x002D: /* confirmed, first byte Arg is video to play */
            case 0x0024: /* confirmed */
            case 0x0046: /*  0046 0106 0100 <-- args are all bytes  */
            case 0x0050: /* actually 3 byte args and 1 spare byte */
            case 0x0042:  /* 0x0042 - Unconditional Jump or End Script Read, 2 short Arg */
            {
                /* 2 Arguments to read */
                fread(&pdata[0], 2, 2, inFile);

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = 2;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                if (cmd == 0x0042){
                    sNode->nextPointerID = (unsigned int)params[0].value;
                }

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /**********************************/
            /* 3 short arguments with 1 child */
            /**********************************/
            case 0x002B: /* (Confirmed, args are: short, byte, byte, byte, byte) */
            {
                /* 3 Arguments to read */
                fread(&pdata[0], 2, 3, inFile);

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = 3;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /**********************************/
            /* 8 short arguments with 1 child */
            /**********************************/
            case 0x005D:  /* only in SSSC, not confirmed */
            case 0x005C: /* (Confirmed, args are: short, then rest bytes) */
            {
                /* 8 Arguments to read */
                fread(&pdata[0], 2, 8, inFile);

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = 8;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /*******************************************************************/
            /* 2 to 3 short words with 1 child                                 */
            /* (Last arg is really a long, which results in padding sometimes) */
            /*******************************************************************/
            case 0x003D:
            {
                unsigned int tmpAddr;
                unsigned int bytesToRead;
//		        int numArg = 0;

                offsetAddress = ftell(inFile); //get current offset
                tmpAddr = (unsigned int)offsetAddress & 0xFFFFFFFC;
                if (offsetAddress == tmpAddr)
                    tmpAddr += 4; /* Add 4 for LW arg.  Ends up being 2 shorts */
                else
                    tmpAddr += 8; /* Add 4 for LW boundary, then add 4 more for LW arg.  Ends up being 3 shorts */
                bytesToRead = tmpAddr - offsetAddress;
//		        numArg = bytesToRead / 2;

                fread(&pdata[0], 1, bytesToRead, inFile);

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = 2;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                params[0].type = ALIGN_4_PARAM;
                params[0].value = 0x00;

                /* Fill in EXE Parameters */
                params[1].type = LONG_PARAM;

                if (bytesToRead == 6){
                    params[1].value = *((unsigned int *)(&pShort[1]));
                    swap32(&params[1].value);
                }
                else{  // 4 bytes
                    params[1].value = *((unsigned int *)(&pShort[0]));
                    swap32(&params[1].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /*******************************************************************/
            /* 3 to 4 short words with 1 child                                 */
            /* (Last arg is really a long, which results in padding sometimes) */
            /*******************************************************************/
            case 0x001C: /* (Traced) Byte arg, align on lw boundary, 1 lw arg  */
            case 0x001E: /* (Traced) Short arg, align on lw boundary, 1 lw arg */
            case 0x001D: /* (Traced) Byte arg, align on lw boundary, 1 lw arg  */
            {
                unsigned int tmpAddr;
                unsigned int bytesToRead;

                offsetAddress = ftell(inFile); //get current offset
                tmpAddr = (unsigned int)(offsetAddress)& 0xFFFFFFFC;
                tmpAddr += 8;
                bytesToRead = tmpAddr - offsetAddress;

                fread(&pdata[0], 1, bytesToRead, inFile);


                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = 3;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                params[0].type = SHORT_PARAM;
                params[0].value = pShort[0];
                swap16(&params[0].value);
                params[1].type = ALIGN_4_PARAM;
                params[1].value = 0x00;
                params[2].type = LONG_PARAM;

                if (bytesToRead == 6){
                    params[2].value = *((unsigned int *)(&pShort[1]));
                    swap32(&params[2].value);
                }
                else{  // == 8
                    params[2].value = *((unsigned int *)(&pShort[2]));
                    swap32(&params[2].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /*================================================================*/
            /*= VARIABLE NUMBERS OF ARGUMENTS, BASED ON FIRST ARGUMENT VALUE =*/
            /*================================================================*/


            /*******************************************************/
            /* 0x0027 - 6 Short Arguments when the first byte of   */
            /*          the first argument is 0x00.  Otherwise 1   */
            /*          short argument.                            */
            /*          Will have only 1 child.                    */
            /*******************************************************/
            case 0x0027:
            {
                int x;
                short val;
                int numArg = 0;
                int totalNumArg = 0;

                /* Determine # of Arguments to read */
                fread(&pdata[numArg * 2], 2, 1, inFile);
                memcpy(&val, &pdata[numArg * 2], sizeof(short));
                swap16(&val);
                if ((val & (short)0xFF00) == (short)0x0000)
                    totalNumArg = 6;
                else
                    totalNumArg = 1;
                numArg++;

                /* Read in additional arguments if required */
                for (x = numArg; x < totalNumArg; x++){
                    fread(&pdata[numArg * 2], 2, 1, inFile);
                    numArg++;
                }


                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                 /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = totalNumArg;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                 sNode->fileOffset = offset;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /***********************************************************/
            /* 0x0034 - 1 parameter if first byte of first arg < 0x0B. */
            /*          7 parameters otherwise                         */
            /*  0034    0B07 0111 3600 0100 0000 FF08 0500             */
            /*  0034    0771                                           */
            /***********************************************************/
            case 0x0034:
            {
                int numargs = 1;

                /* At least 1 Short Argument to read */
                fread(&pdata[0], 2, 1, inFile);

                /* See if 6 additional to read */
                if (pdata[0] >= 0x0B){
                    fread(&pdata[2], 2, 6, inFile);
                    numargs += 6;
                }


                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = numargs;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /*========================================================*/
            /*= VARIABLE NUMBERS OF ARGUMENTS, BASED ON A TERMINATOR =*/
            /*========================================================*/


            /*********************************************************/
            /* 0x0038 - 1 Child                                      */
            /* Byte Terminator of 0xF9 (maintains short wd boundary) */
            /* Has several internal control codes that also dictate  */
            /* Length of argument list                               */
            /*********************************************************/
            /* First byte argument is special, rest are:             **************/
            /*   0xF0 - Incr 2 bytes                                              */
            /*   0xF1 - Incr 2 bytes                                              */
            /*   0xF2 - Incr 2 bytes                                              */
            /*   0xF3 - Incr 5 bytes                                              */
            /*   0xF4 - Incr 5 bytes                                              */
            /*   0xF5 - Incr 2 bytes                                              */
            /*   0xF6 - Incr 2 bytes                                              */
            /*   0xF7 - Incr 5 bytes                                              */
            /*   0xF8 - Incr 4 bytes                                              */
            /*   0xF9 - End of args(incr to next short word offset(1 or 2 bytes)) */
            /**********************************************************************/
            case 0x0038:  /* Byte Terminator of 0xF9 (maintains short wd boundary) */
            {
                int numArg = 0;
                unsigned char argval;

                //First byte argument
                fread(&pdata[numArg], 1, 1, inFile);
                numArg++;

                /* Determine # of Arguments to read */
                while (!feof(inFile)){
                    fread(&pdata[numArg], 1, 1, inFile);
                    memcpy(&argval, &pdata[numArg], 1);
                    numArg++;

                    if ((argval == 0xF0) || (argval == 0xF1) || (argval == 0xF2) || (argval == 0xF5) || (argval == 0xF6)){
                        fread(&pdata[numArg], 1, 1, inFile);
                        numArg++;
                    }
                    else if ((argval == 0xF3) || (argval == 0xF4) || (argval == 0xF7)){
                        fread(&pdata[numArg], 1, 4, inFile);
                        numArg += 4;
                    }
                    else if (argval == 0xF8){
                        fread(&pdata[numArg], 1, 3, inFile);
                        numArg += 3;
                    }
                    else if (argval == 0xF9){
                        //If not aligned, read again
                        if ((ftell(inFile) % 2) != 0){
                            fread(&pdata[numArg], 1, 1, inFile);
                            numArg++;
                        }
                        break;
                    }
                    else{
                        printf("Error parse error in 0x0038, should not get here.\n");
                    }
                }

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Convert to shorts */
                numArg /= 2;

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = numArg;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /**************************************************************/
            /* Variable short arguments that end with a short terminator  */
            /* of 0x0000 and have only 1 child.                           */
            /**************************************************************/

            /*****************************************************************************/
            /* Minimum of 2 short arguments.                                             */
            /* Terminated with 0x0000, 1 child.                                          */
            /*****************************************************************************/
            case 0x000D:
            {
                int index = 0;
                short* val;

                /* N Arguments to read */
                fread(&pdata[index], 2, 1, inFile);  /* Read 1st arg */
                do
                {
                    index += 2;
                    fread(&pdata[index], 2, 1, inFile); /* Read args 2 to N */
                    val = (short*)(&pdata[index]);
                } while (*val != (short)0x0000);
                index += 2;
                index /= 2;


                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = index;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                sNode->nextPointerID = (unsigned int)params[0].value;


                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /**********************************************************************/
            /* Terminated with 0x0000, 1 child.  Terminate if First arg is 0x0000 */
            /**********************************************************************/
            case 0x000A:  /* Clears a bit in the 0x21F8xx region (opposite of 0x0009) */
            case 0x0009:  /* Sets a bit in the 0x21F8xx region (opposite of 0x000A) updated */
            case 0x0008:  /* updated */
            {
                int index = 0;
                short* val;

                /* N Arguments to read */
                fread(&pdata[index], 2, 1, inFile);
                val = (short*)(&pdata[index]);
                while (*val != (short)0x0000){
                    index += 2;
                    fread(&pdata[index], 2, 1, inFile);
                    val = (short*)(&pdata[index]);
                }
                index += 2;
                index /= 2;

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = index;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /*************************************************************************/
            /* xxx byte parameters terminated by first of occurence of 0x00, 1 child */
            /*************************************************************************/
            case 0x0036:  /* confirmed */
            case 0x0013:  /* confirmed, was 0000 , changed to 00 */
            case 0x001A:  /* confirmed */
            case 0x000F:  /* confirmed */
            case 0x0014:  /* confirmed */
            case 0x0018:  /* confirmed */  /* calling 0x60bb1f4 is a dead giveaway for this type */
            case 0x0019:  /* confirmed */
            case 0x0025:  /* confirmed */
            case 0x0035:  /* confirmed */
            case 0x0041:  /* confirmed */
            case 0x004E:  /* confirmed */
            case 0x000E:  /* confirmed */
            case 0x003E:  /* confirmed */
            case 0x0055:  /* confirmed */
            case 0x004F:  /* confirmed 99% sure */
            {
                int numArg = 0;
                unsigned char argval;

                /* Determine # of Arguments to read */
                while (!feof(inFile)){
                    fread(&pdata[numArg], 1, 1, inFile);
                    memcpy(&argval, &pdata[numArg], 1);
                    numArg++;

                    if (argval == 0x00){
                        //If not aligned, read again
                        if ((ftell(inFile) % 2) != 0){
                            fread(&pdata[numArg], 1, 1, inFile);
                            numArg++;
                        }
                        break;
                    }
                }

                numArg /= 2;

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = numArg;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /*=======================================*/
            /*= VARIABLE NUMBERS OF ARGUMENTS, TEXT =*/
            /*=======================================*/


            /************************************/
            /* 0x0002 - Text Drawing subroutine */
            /************************************/
            case 0x0002:
            {
                /* Read until 0xFFFF is reached */
                runParamType* rpHead;
                char prev, cur;
                int index = 0;
                int textMode = getTextDecodeMethod();
                prev = cur = 0;

                while (1){
                    rval = fread(&pdata[index++], 1, 1, inFile);
                    if (rval != 1){
                        printf("Error encountered while reading TEXT portion of script, no termination.\n");
                        break;
                    }
                    cur = pdata[index - 1];

                    //Handle UTF8 Text
                    if ((textMode == TEXT_DECODE_UTF8) && ((unsigned char)cur < 0xF0)){
                        int x;
                        int nbytes = numBytesInUtf8Char(cur);
                        if (nbytes > 1){
                            for (x = 0; x < nbytes - 1; x++){
                                rval = fread(&pdata[index++], 1, 1, inFile);
                                if (rval != 1){
                                    printf("Error encountered while reading TEXT portion of script, no termination.\n");
                                    break;
                                }
                            }
                            //This cant be the termination.  0xFF is 1 byte
                            prev = cur = 0;
                            continue;
                        }
                        else if ((nbytes == 1) && (G_IOS_ENG == 1) && (pdata[index-1] == 0x0A)){
                            //Hack in space for iOS Eng
                            pdata[index - 1] = 0xFF;
                            pdata[index++] = 0x02;
                            nbytes++;
                            //This cant be the termination. Reset.
                            prev = cur = 0;
                        }
                    }
                    else if (textMode == TEXT_DECODE_UTF8){  // >= 0xF0
                        //Read again
                        prev = cur;
                        fread(&pdata[index], 1, 1, inFile);
                        cur = pdata[index];
                        index++;
                    }

                    /* Lunar Text Terminates with short word aligned 0xFFFF */
                    /* Extraction from 1-byte text hack does not require this */
                    if /*(*/ (textMode == TEXT_DECODE_ONE_BYTE_PER_CHAR)/* ||
                                                                        (textMode == TEXT_DECODE_UTF8) )*/{
                        if ((cur == (char)0xFF) && (prev == (char)0xFF))
                            break;
                    }
                    else{
                        if (((ftell(inFile) % 2) == 0) && (cur == (char)0xFF) && (prev == (char)0xFF))
                            break;
                    }
                    prev = cur;
                }

                /* Advance File Pointer to a 16-bit boundary */
                /* Should already be on one unless running a 1-byte text hack */
                if ((ftell(inFile) % 2) != 0){
                    fread(&pdata[index], 1, 1, inFile);
                    //No need to increment index based on my storage format
                }

                /* Create a New Text Script Node */
                /* Pass in data (pdata) and size of data (index) */
                /* For a Text Node, the data count is in bytes, not shorts */

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                rpHead = NULL;
                rpHead = getRunParam(textMode, pdata);

                /* Fill in Remaining Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_RUN_CMDS;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = 0;    /* N/A */
                sNode->alignfillVal = 0xFF;
                sNode->subParams = NULL;
                sNode->runParams = rpHead;
                sNode->fileOffset = offset;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }
#if 0
                FAXX 	Character portrait
                F90A 	Space; the second byte is the size of the space in pixels
                FF00    Button must be pressed to continue text
                FF01 	Text box advances automatically.Any text following this control code will be in the next box
                FF02 	Newline
                FF03 	Start a new text box.If not followed by a portrait control code, the next box will have no portrait
                FFFF 	End of dialogue - no more dialogue boxes are coming
#endif

            /******************************************************************************/
            /* 0x0007 - Select between 2 options.                                         */
            /*          Argument #1:  Offset to use to jump if bottom option is selected. */
            /*                        Otherwise the script sequence continues linearly    */
            /*                        past this portion.                                  */
            /*          Argument #2:  Another parameter, not necessarily 0000             */
            /*          Argument #3:  Text String Ending in FFFF  (Top Option)            */
            /*          Argument #4:  Text String Ending in FFFF  (Bottom Option)         */
            /******************************************************************************/
            case 0x0007:
            {
                runParamType* rpHead1, *rpHead2;
                unsigned short opt2Offset, parameter2;
                char prev, cur;
                int index = 0;
                int index2 = 0;
//              int textSize1 = 0;
//              int textSize2 = 0;
                int textMode = getTextDecodeMethod();

                /* Offset to Opt2 Jump Point */
                fread(&opt2Offset, 2, 1, inFile);
                swap16(&opt2Offset);
//              offsetAddress2 = 2 * opt2Offset;

                /* NULL - well, not really NULL in all cases */
                fread(&parameter2, 2, 1, inFile);
                swap16(&parameter2);
                index = 0;

                /******************************/
                /* Option 1 (Top Option) Text */
                /******************************/

                /* Read until 0xFFFF is reached */
                prev = cur = 0;

                while (1){
                    fread(&pdata[index], 1, 1, inFile);
                    cur = pdata[index];
                    index++;

                    //Handle UTF8 Text
                    if ((textMode == TEXT_DECODE_UTF8) && ((unsigned char)cur < 0xF0)){
                        int x;
                        int nbytes = numBytesInUtf8Char(cur);
                        if (nbytes > 1){
                            for (x = 0; x < nbytes - 1; x++){
                                rval = fread(&pdata[index++], 1, 1, inFile);
                                if (rval != 1){
                                    printf("Error encountered while reading OPT portion of script, no termination.\n");
                                    break;
                                }
                            }
                            //This cant be the termination.  0xFF is 1 byte
                            prev = cur = 0;
                            continue;
                        }
                        else if ((nbytes == 1) && (G_IOS_ENG == 1) && (pdata[index - 1] == 0x0A)){
                            //Hack in space for iOS Eng
                            pdata[index - 1] = 0xFF;
                            pdata[index++] = 0x02;
                            nbytes++;
                            //This cant be the termination. Reset.
                            prev = cur = 0;
                        }
                    }
                    else if (textMode == TEXT_DECODE_UTF8){  // >= 0xF0
                        //Read again
                        prev = cur;
                        fread(&pdata[index], 1, 1, inFile);
                        cur = pdata[index];
                        index++;
                    }

                    /* Lunar Text Terminates with short word aligned 0xFFFF */
                    /* Extraction from 1-byte text hack does not require this */
                    if (textMode == TEXT_DECODE_ONE_BYTE_PER_CHAR){
                        if ((cur == (char)0xFF) && (prev == (char)0xFF))
                            break;
                    }
                    else{
                        if (((ftell(inFile) % 2) == 0) && (cur == (char)0xFF) && (prev == (char)0xFF))
                            break;
                    }

                    prev = cur;
                }

                /* Advance File Pointer to a 16-bit boundary */
                /* Should already be on one unless running a 1-byte text hack */
                if ((ftell(inFile) % 2) != 0){
                    fread(&pdata[index], 1, 1, inFile);
                    //No need to increment index based on my storage format
                }
//              textSize1 = index;


                /*********************************/
                /* Option 2 (Bottom Option) Text */
                /*********************************/

                /* Read until 0xFFFF is reached */
                prev = cur = 0;

                while (1){
                    fread(&pdata2[index2], 1, 1, inFile);
                    cur = pdata2[index2];
                    index2++;

                    //Handle UTF8 Text
                    if ((textMode == TEXT_DECODE_UTF8) && ((unsigned char)cur < 0xF0)){
                        int x;
                        int nbytes = numBytesInUtf8Char(cur);
                        if (nbytes > 1){
                            for (x = 0; x < nbytes - 1; x++){
                                rval = fread(&pdata2[index2++], 1, 1, inFile);
                                if (rval != 1){
                                    printf("Error encountered while reading OPT portion of script, no termination.\n");
                                    break;
                                }
                            }
                            //This cant be the termination.  0xFF is 1 byte
                            prev = cur = 0;
                            continue;
                        }
                    }
                    else if (textMode == TEXT_DECODE_UTF8){  // >= 0xF0
                        //Read again
                        prev = cur;
                        fread(&pdata2[index2], 1, 1, inFile);
                        cur = pdata2[index2];
                        index2++;
                    }

                    /* Lunar Text Terminates with short word aligned 0xFFFF */
                    /* Extraction from 1-byte text hack does not require this */
                    if (textMode == TEXT_DECODE_ONE_BYTE_PER_CHAR){
                        if ((cur == (char)0xFF) && (prev == (char)0xFF))
                            break;
                    }
                    else{
                        if (((ftell(inFile) % 2) == 0) && (cur == (char)0xFF) && (prev == (char)0xFF))
                            break;
                    }

                    prev = cur;
                }

                /* Advance File Pointer to a 16-bit boundary */
                /* Should already be on one unless running a 1-byte text hack */
                if ((ftell(inFile) % 2) != 0){
                    fread(&pdata2[index2], 1, 1, inFile);
                    //No need to increment index2 based on my storage format
                }
                offsetAddress = ftell(inFile);
//              textSize2 = index2;


                /***************************************/
                /* Create a New Two Option Script Node */
                /***************************************/

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Allocate memory for EXE parameters */
                sNode->num_parameters = 2;
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                params[0].type = SHORT_PARAM;
                params[0].value = opt2Offset;
                params[1].type = SHORT_PARAM;
                params[1].value = parameter2;
                sNode->subParams = params;


                /* Convert Text to Run Parameters */
                rpHead1 = rpHead2 = NULL;
                rpHead1 = getRunParam(textMode, pdata);
                rpHead2 = getRunParam(textMode, pdata2);

                /* Fill in Remaining Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_OPTIONS;
                sNode->subroutine_code = cmd;
                sNode->alignfillVal = 0xFF;
                sNode->runParams = rpHead1;
                sNode->runParams2 = rpHead2;
                sNode->fileOffset = offset;

                sNode->nextPointerID = (unsigned int)params[0].value;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);


                break;
            }


            /*============================================*/
            /*= VARIABLE NUMBERS OF ARGUMENTS, JUMP CMDs =*/
            /*============================================*/


            /*****************************************************************/
            /* 0x0016 - Conditional Jump in Script File with Byte parameters */
            /*          that are terminated by 0x00                          */
            /*****************************************************************/
            case 0x0010:  /* confirmed */
            case 0x0016:  /* confirmed */
            {
                int numArg = 0;
                unsigned char argval;

                //Read short jump parameter
                fread(&pdata[numArg], 2, 1, inFile);
                memcpy(&wdOffset, &pdata[numArg], 2);
                swap16(&wdOffset);
                numArg += 2;

                /* Determine # of Arguments to read */
                while (!feof(inFile)){
                    fread(&pdata[numArg], 1, 1, inFile);
                    memcpy(&argval, &pdata[numArg], 1);
                    numArg++;

                    if (argval == 0x00){
                        //If not aligned, read again
                        if ((ftell(inFile) % 2) != 0){
                            fread(&pdata[numArg], 1, 1, inFile);
                            numArg++;
                        }
                        break;
                    }
                }

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = numArg / 2;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                sNode->nextPointerID = (unsigned int)params[0].value;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);


                break;
            }


            /**************************************************************/
            /* 0x0011 - Jump of some sort  (confirmed)                    */
            /*          Args (4 bytes): Short Jump Location, byte, spare  */
            /*          Terminated by occurance of 0x00.                  */
            /**************************************************************/
            case 0x0011:
            {
                unsigned char argval;
                int numArg = 0;

                fread(&pdata[numArg], 2, 1, inFile);
                memcpy(&wdOffset, &pdata[numArg], 2);
                swap16(&wdOffset);
                numArg += 2;


                /* Determine # of Arguments to read */
                while (!feof(inFile)){
                    fread(&pdata[numArg], 1, 1, inFile);
                    memcpy(&argval, &pdata[numArg], 1);
                    numArg++;

                    if (argval == 0x00){
                        //If not aligned, read again
                        if ((ftell(inFile) % 2) != 0){
                            fread(&pdata[numArg], 1, 1, inFile);
                            numArg++;
                        }
                        break;
                    }
                }

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = numArg / 2;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                sNode->nextPointerID = (unsigned int)params[0].value;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /*******************************************************************/
            /* First short arg is JMP, Then search for 0x0000 short terminator */
            /*******************************************************************/

            /**************************************************/
            /* 0x000B - Conditional True Jump in Script File  */
            /* 0x000C - Conditional False Jump in Script File */
            /**************************************************/
            case 0x000B:
            case 0x000C:
            {
                short bitOffset, zeroOffset;
                int numArg = 0;

                fread(&pShort[numArg], 2, 1, inFile);
                memcpy(&wdOffset, &pShort[numArg++], 2);
                swap16(&wdOffset);
                fread(&pShort[numArg], 2, 1, inFile);
                memcpy(&bitOffset, &pShort[numArg++], 2);
                swap16(&bitOffset);
                zeroOffset = bitOffset;

                //Look for 0x0000 Terminator
                while (zeroOffset != 0x0000){
                    fread(&pShort[numArg], 2, 1, inFile);
                    memcpy(&zeroOffset, &pShort[numArg++], 2);
                    swap16(&zeroOffset);
                }

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = numArg;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                sNode->nextPointerID = (unsigned int)params[0].value;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }



            /**************************************************************************/
            /* 0x0026 - Called when a new TEXTxxx.DAT file is loaded in to 0x26000    */
            /*          Will provide the short location of where to start in that     */
            /*          file and do some other stuff.                                 */
            /*         # Args - Either 3 or 8 short arguments.                        */
            /*            Arg1 - Provides short wd location of jump offset.           */
            /*                   (Offset taken care of by parser anyway)              */
            /*            Arg2 - First byte if nonzero, indicates 3 short arg.        */
            /*                   If zero, indicates 8 short arg.                      */
            /*                   Second byte used for some sort of trigger.           */
            /*            Arg3-N - Not traced, not required for script parsing.       */
            /*         No Children.                                                   */
            /**************************************************************************/
            case 0x0026:  /* Called when entering a new scene */
            {
                unsigned short jmploc;
                int numArg;

                /* Minimum 3 Arguments to read */
                fread(&pdata[0], 2, 3, inFile);

                memcpy(&jmploc, &pdata[0], 2);
                swap16(&jmploc);

                /* See if an additional 5 should be read */
                if (pdata[2] == (char)0x00){
                    fread(&pdata[6], 2, 5, inFile);
                    /* Create a new script node, 8 arg */
                    numArg = 8;
                }
                else{
                    /* Create a new script node, 3 arg */
                    numArg = 3;
                }

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = numArg;
                sNode->alignfillVal = 0x00;

                /* Allocate memory for EXE parameters */
                params = (paramType*)malloc(sNode->num_parameters * sizeof(paramType));
                if (params == NULL){
                    printf("Error allocing memory for parameters\n");
                    return -1;
                }

                /* Fill in EXE Parameters */
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);
                }

                sNode->subParams = params;
                sNode->fileOffset = offset;

                sNode->nextPointerID = (unsigned int)params[0].value;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }




            /*************************************************************/
            /* 0x0005 - Subroutine usually called last.                  */
            /* Seems to signify end of the current script sequence read. */
            /*************************************************************/
            case 0x0005:
            {
                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = 0;
                sNode->alignfillVal = 0x00;
                sNode->fileOffset = offset;

                /* Add the node */
                if (addNode(sNode, METHOD_NORMAL, 0) != 0){
                    printf("Error occurred adding the script node.\n");
                    return -1;
                }
                free(sNode);

                break;
            }


            /**************************************/
            /* Default Case - Should NOT Get Here */
            /**************************************/
            default:
            {
                printf("ERROR, Unknown Command 0x%X!\n", cmd);
                return -1;
            }
        }

        /* Run once if this is set */
        if (singleRunFlag)
            break;
    }

    return 0;
}





runParamType* getRunParam(int textMode, char* pdata){

    runParamType* rpHead, *rpNode, *rpCurrent;
    int x;
    unsigned char* tmpText = NULL;
    unsigned int numTextShorts = 0;
    rpHead = rpNode = rpCurrent = NULL;

    /************************************************************************/
    /* Iterate through the data looking to separate text from control codes */
    /************************************************************************/
    switch (textMode)
    {

    case TEXT_DECODE_TWO_BYTES_PER_CHAR:
    {
        unsigned short* ptrS = (unsigned short*)pdata;
        unsigned short* ptrTextStart = NULL;

        while (1){

            swap16(ptrS);

            /* Control Code vs. Text Detection */
            if (*ptrS == 0xF90A){
                /* Part of a Text String (Space) */
                if (numTextShorts == 0)
                    ptrTextStart = ptrS;
                numTextShorts++;
            }
            else if (*ptrS >= 0xF000){  /* Control Codes are >= 0xF000 */

                /* Create a text string run parameter element if one was previously started */
                if (numTextShorts > 0){
                    char tmp[5];

                    /* Create utf8 Text String*/
                    tmpText = (unsigned char*)malloc(5 * (numTextShorts + 1));
                    memset(tmpText, 0, 5 * (numTextShorts + 1));
                    for (x = 0; x < (int)numTextShorts; x++){
                        memset(tmp, 0, 5);
                        if (ptrTextStart[x] == 0xF90A)
                            strcpy(tmp, " ");
                        else
                            getUTF8character(ptrTextStart[x], tmp);
                        strcat((char *)tmpText, tmp);
                    }

                    /* Create a runcmds parameter element */
                    rpNode = (runParamType*)malloc(sizeof(runParamType));
                    if (rpNode == NULL){
                        printf("Error allocing space for run parameter struct.\n");
                        return NULL;
                    }
                    memset(rpNode, 0, sizeof(runParamType));
                    rpNode->pNext = NULL;
                    rpNode->type = PRINT_LINE;
                    rpNode->str = tmpText;

                    /* Add the node to the list */
                    if (rpHead == NULL){
                        rpHead = rpCurrent = rpNode;
                    }
                    else{
                        rpCurrent->pNext = rpNode;
                        rpCurrent = rpNode;
                    }

                    numTextShorts = 0;
                }

                /*********************************/
                /* Create a Control Code element */
                /* OR SHOW_PORTRAIT              */
                /*********************************/

                /* Create a runcmds parameter element */
                rpNode = (runParamType*)malloc(sizeof(runParamType));
                if (rpNode == NULL){
                    printf("Error allocing space for run parameter struct.\n");
                    return NULL;
                }
                memset(rpNode, 0, sizeof(runParamType));
                rpNode->pNext = NULL;
                rpNode->str = NULL;

                if ((*ptrS & 0xFF00) == 0xFA00){
                    rpNode->type = SHOW_PORTRAIT_LEFT;
                    rpNode->value = (*ptrS & 0x00FF);
                }
                else if ((*ptrS & 0xFF00) == 0xFB00){
                    rpNode->type = SHOW_PORTRAIT_RIGHT;
                    rpNode->value = (*ptrS & 0x00FF);
                }
                else if ((*ptrS & 0xFF00) == 0xF800){
                    rpNode->type = TIME_DELAY;
                    rpNode->value = (*ptrS & 0x00FF);
                }
                else{
                    rpNode->type = CTRL_CODE;
                    rpNode->value = *ptrS;
                }

                /* Add the node to the list */
                if (rpHead == NULL){
                    rpHead = rpCurrent = rpNode;
                }
                else{
                    rpCurrent->pNext = rpNode;
                    rpCurrent = rpNode;
                }

                /*****************************/
                /* END OF TEXT BLOCK LOCATED */
                /*****************************/
                if (*ptrS == 0xFFFF){

					/**************************/
					/* Force 2-Byte Alignment */
					/**************************/

					/* Create a runcmds parameter element */
					rpNode = (runParamType*)malloc(sizeof(runParamType));
					if (rpNode == NULL){
						printf("Error allocing space for run parameter struct.\n");
						return NULL;
					}
					memset(rpNode, 0, sizeof(runParamType));
					rpNode->pNext = NULL;
					rpNode->str = NULL;
					rpNode->type = ALIGN_2_PARAM;
					rpNode->value = 0xFF;

					/* Add the node to the list */
					if (rpHead == NULL){
						rpHead = rpCurrent = rpNode;
					}
					else{
						rpCurrent->pNext = rpNode;
						rpCurrent = rpNode;
					}


                    break;
                }
            }
            else{
                /* Part of a Text String */
                if (numTextShorts == 0)
                    ptrTextStart = ptrS;
                numTextShorts++;
            }

            ptrS++;  //Incr Short Ptr
        }
        break;
    }


    case TEXT_DECODE_ONE_BYTE_PER_CHAR:
    {
        int z;
        unsigned char byte_data, byte_data2;
        unsigned short short_data;
        char* ptrText, *ptrStart;

        /* Buffer */
        ptrText = (char*)malloc(1024 * 1024);
        if (ptrText == NULL){
            printf("Error allocing temp memory\n");
            return NULL;
        }
        z = 0;
        ptrStart = NULL;

        while (1){
            byte_data = ((unsigned char)*pdata) & 0xFF;

            if (byte_data < 0xF0){
                /* 1-Byte Text */
                if(ptrStart == NULL)
                    ptrStart = pdata;
                z++;
                pdata++;
            }
            else if (byte_data >= 0xF0){
                /* 2-byte Code or Space */
                pdata++;
                byte_data2 = (((unsigned char)*pdata) & 0xFF);
                short_data = (byte_data << 8) | byte_data2;
				pdata++;

                /*************/
                /* Ctrl Code */
                /*************/

                /**************************************/
                /* Write out any prior text           */
                /* Create a runcmds parameter element */
                /**************************************/
                if (z > 0){
                    unsigned int decmpSize = 0;
                    rpNode = (runParamType*)malloc(sizeof(runParamType));
                    if (rpNode == NULL){
                        printf("Error allocing space for run parameter struct.\n");
                        free(ptrText);
                        return NULL;
                    }
                    memset(rpNode, 0, sizeof(runParamType));  //KANE -- fix
                    rpNode->pNext = NULL;
                    rpNode->type = PRINT_LINE;
                    memset(ptrText, 0, 1024 * 1024);
                    decompressBPE((unsigned char*)ptrText, (unsigned char*)ptrStart, &decmpSize);
                    rpNode->str = malloc(decmpSize + 1);
                    if (rpNode->str == NULL){
                        printf("Error allocing for string.\n");
                        free(ptrText);
                        return NULL;
                    }
                    memset(rpNode->str, 0, decmpSize + 1);
                    strcpy((char *)rpNode->str, (char *)ptrText);

                    /* Add the node to the list */
                    if (rpHead == NULL){
                        rpHead = rpCurrent = rpNode;
                    }
                    else{
                        rpCurrent->pNext = rpNode;
                        rpCurrent = rpNode;
                    }

                    /* Reset start of text section */
                    ptrStart = NULL;
                    z = 0;
                }

                /*********************************/
                /* Create a Control Code element */
                /* OR SHOW_PORTRAIT              */
                /*********************************/

                /* Create a runcmds parameter element */
                rpNode = (runParamType*)malloc(sizeof(runParamType));
                if (rpNode == NULL){
                    printf("Error allocing space for run parameter struct.\n");
                    free(ptrText);
                    return NULL;
                }
                memset(rpNode, 0, sizeof(runParamType));
                rpNode->pNext = NULL;
                rpNode->str = NULL;

                if ((short_data & 0xFF00) == 0xFA00){
                    rpNode->type = SHOW_PORTRAIT_LEFT;
                    rpNode->value = (short_data & 0x00FF);
                }
                else if ((short_data & 0xFF00) == 0xFB00){
                    rpNode->type = SHOW_PORTRAIT_RIGHT;
                    rpNode->value = (short_data & 0x00FF);
                }
                else if ((short_data & 0xFF00) == 0xF800){
                    rpNode->type = TIME_DELAY;
                    rpNode->value = (short_data & 0x00FF);
                }
                else{
                    rpNode->type = CTRL_CODE;
                    rpNode->value = short_data;
                }

                /* Add the node to the list */
                if (rpHead == NULL){
                    rpHead = rpCurrent = rpNode;
                }
                else{
                    rpCurrent->pNext = rpNode;
                    rpCurrent = rpNode;
                }

                /*****************************/
                /* END OF TEXT BLOCK LOCATED */
                /*****************************/
                if (short_data == 0xFFFF){
                    free(ptrText);
					
					/**************************/
					/* Force 2-Byte Alignment */
					/**************************/
					
					/* Create a runcmds parameter element */
					rpNode = (runParamType*)malloc(sizeof(runParamType));
					if (rpNode == NULL){
						printf("Error allocing space for run parameter struct.\n");
						return NULL;
					}
					memset(rpNode, 0, sizeof(runParamType));
					rpNode->pNext = NULL;
					rpNode->str = NULL;
					rpNode->type = ALIGN_2_PARAM;
					rpNode->value = 0xFF;
					
					/* Add the node to the list */
					if (rpHead == NULL){
						rpHead = rpCurrent = rpNode;
					}
					else{
						rpCurrent->pNext = rpNode;
						rpCurrent = rpNode;
					}
					
                    break;
                }
            }
           // pdata++;
        }
        break;
    }

    case TEXT_DECODE_UTF8:
    {
        unsigned char* ptrText = NULL;
        unsigned char utf8data[5];
        unsigned short short_data;
        int numBytes, y;

        /* Buffer */
        ptrText = malloc(1024 * 1024);
        if (ptrText == NULL){
            printf("Error allocing temp memory\n");
        }
        memset(ptrText, 0, 1024 * 1024);

        while (1){
            memset(utf8data, 0, 5);
            short_data = 0;

            /* Grab the first byte of the utf8 character & get # bytes */
            if ((unsigned char)*pdata >= 0xF0)
                numBytes = 1;
            else
                numBytes = numBytesInUtf8Char((unsigned char)*pdata);
            utf8data[0] = (unsigned char)(*pdata);

            /* Read the rest of the utf8 character */
            for (y = 1; y < numBytes; y++){
                pdata++;
                utf8data[y] = (unsigned char)*pdata;
            }

            /* Check for Control Code */
            if ((numBytes == 1) && (utf8data[0] >= 0xF0)){
                pdata++;
                short_data = (utf8data[0] << 8) | (unsigned char)(*pdata);
            }

            if (short_data == 0xF90A){
                strcat((char *)ptrText, " ");
                short_data = 0;
            }
            else if (short_data != 0x0000){

                /**************************************/
                /* Write out any prior text           */
                /* Create a runcmds parameter element */
                /**************************************/
                if (strlen((char *)ptrText) > 0){
                    rpNode = (runParamType*)malloc(sizeof(runParamType));
                    if (rpNode == NULL){
                        printf("Error allocing space for run parameter struct.\n");
                        free(ptrText);
                        return NULL;
                    }
                    memset(rpNode, 0, sizeof(runParamType));
                    rpNode->pNext = NULL;
                    rpNode->type = PRINT_LINE;
                    rpNode->str = malloc(strlen((char *)ptrText) + 1);
                    if (rpNode->str == NULL){
                        printf("Error allocing for string.\n");
                        free(ptrText);
                        return NULL;
                    }
                    memset(rpNode->str, 0, strlen((char *)ptrText) + 1);
                    strcpy((char *)rpNode->str, (char *)ptrText);

                    /* Add the node to the list */
                    if (rpHead == NULL){
                        rpHead = rpCurrent = rpNode;
                    }
                    else{
                        rpCurrent->pNext = rpNode;
                        rpCurrent = rpNode;
                    }
                }

                /*********************************/
                /* Create a Control Code element */
                /* OR SHOW_PORTRAIT              */
                /*********************************/

                /* Create a runcmds parameter element */
                rpNode = (runParamType*)malloc(sizeof(runParamType));
                if (rpNode == NULL){
                    printf("Error allocing space for run parameter struct.\n");
                    free(ptrText);
                    return NULL;
                }
                memset(rpNode, 0, sizeof(runParamType));
                rpNode->pNext = NULL;
                rpNode->str = NULL;

                if ((short_data & 0xFF00) == 0xFA00){
                    rpNode->type = SHOW_PORTRAIT_LEFT;
                    rpNode->value = (short_data & 0x00FF);
                }
                else if ((short_data & 0xFF00) == 0xFB00){
                    rpNode->type = SHOW_PORTRAIT_RIGHT;
                    rpNode->value = (short_data & 0x00FF);
                }
                else if ((short_data & 0xFF00) == 0xF800){
                    rpNode->type = TIME_DELAY;
                    rpNode->value = (short_data & 0x00FF);
                }
                else{
                    rpNode->type = CTRL_CODE;
                    rpNode->value = short_data;
                }

                /* Add the node to the list */
                if (rpHead == NULL){
                    rpHead = rpCurrent = rpNode;
                }
                else{
                    rpCurrent->pNext = rpNode;
                    rpCurrent = rpNode;
                }

                /*****************************/
                /* END OF TEXT BLOCK LOCATED */
                /*****************************/
                if (short_data == 0xFFFF){
                    free(ptrText);

					/**************************/
					/* Force 2-Byte Alignment */
					/**************************/

					/* Create a runcmds parameter element */
					rpNode = (runParamType*)malloc(sizeof(runParamType));
					if (rpNode == NULL){
						printf("Error allocing space for run parameter struct.\n");
						return NULL;
					}
					memset(rpNode, 0, sizeof(runParamType));
					rpNode->pNext = NULL;
					rpNode->str = NULL;
					rpNode->type = ALIGN_2_PARAM;
					rpNode->value = 0xFF;

					/* Add the node to the list */
					if (rpHead == NULL){
						rpHead = rpCurrent = rpNode;
					}
					else{
						rpCurrent->pNext = rpNode;
						rpCurrent = rpNode;
					}

                    break;
                }
                short_data = 0;
                ptrText[0] = '\0';
            }
            else{
                /* Text */
                strcat((char *)ptrText, (char *)utf8data);
            }
            pdata++;
        }
        break;
    }

    default:
    {
        printf("Cant get here.\n");
        break;
    }

    }

    return rpHead;
}
