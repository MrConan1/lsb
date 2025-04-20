/*****************************************************************************/
/* parse_binary_reEng.c : Hack of PSX Eng Parse Code                         */
/*****************************************************************************/

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "script_node_types.h"
#include "snode_list.h"
#include "util.h"
#include "bpe_compression.h"
#include "psx_decode.h"
#include "parse_binary.h"


/* Defines */
#define RE_DBUF_SIZE      (128*1024)     /* 128kB */
#define RE_PTR_ARRAY_SIZE (2*1024)       /* 0x800 bytes, or 1024 16-bit LWs */

#define PSX_UGLY_ENG_IOS_HACKS_RE

/* Globals */
static char* pdata = NULL;
static char* pdata2 = NULL;


static int G_ID = 1;


/* Function Prototypes */
int decodeBinaryScript_RE_Eng(FILE* inFile, FILE* outFile);
int parseCmdSeq_RE_Eng(int offset, FILE** ptr_inFile, int singleRunFlag);




/*****************************************************************************/
/* Function: decodeBinaryScript_RE_Eng                                       */
/* Purpose: Parses the script starting at offset 0x0800.                     */
/*          Then considers the top level pointers.                           */
/* Inputs:  Pointers to input/output files.                                  */
/* Outputs: 0 on Pass, -1 on Fail.                                           */
/*****************************************************************************/
int decodeBinaryScript_RE_Eng(FILE* inFile, FILE* outFile){

    unsigned short ptrVal;
    unsigned int iFileSizeBytes;
    int x;
    unsigned short* pIndexPtrs = NULL;

    /* Allocate two 128kB buffers, much bigger than the input file */
    if (pdata != NULL){
        free(pdata);
        pdata = NULL;
    }
    pdata = (char*)malloc(RE_DBUF_SIZE);
    if (pdata == NULL){
        printf("Error allocating space for file data buffer.\n");
        return -1;
    }

    if (pdata2 != NULL){
        free(pdata2);
        pdata2 = NULL;
    }
    pdata2 = (char*)malloc(RE_DBUF_SIZE);
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
    if (parseCmdSeq_RE_Eng(0x0800, &inFile, 0) != 0){
        printf("Error Detected while reading from input file.\n");
        return -1;
    }

    /* Allocate memory for the array */
    pIndexPtrs = (unsigned short*)malloc(RE_PTR_ARRAY_SIZE);
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
        //No word-swap for PSX

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
            if (parseCmdSeq_RE_Eng(byteOffset, &inFile, 1) != 0){
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

    /* Free memory */
    if (pdata != NULL)
        free(pdata);
    if (pdata2 != NULL)
        free(pdata2);


    return 0;
}




/*****************************************************************************/
/* Function: parseCmdSeq_RE_Eng                                              */
/* Purpose: Parses a sequence of script commands into a tree structure.      */
/* Inputs:  Pointer to current node in the script.                           */
/*          Byte offset into file to read from.                              */
/* Outputs: None.                                                            */
/*****************************************************************************/
int parseCmdSeq_RE_Eng(int offset, FILE** ptr_inFile, int singleRunFlag){

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

        //Fix for script bug in File #51
		if (cmd == 0xFF)
			continue;
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
		case 0x0064:  /*REMASTER */
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
    #ifdef PSX_UGLY_ENG_IOS_HACKS_RE
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
case 0x0061: /*REMASTER*/
case 0x0065: /*REMASTER*/
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
    #ifdef PSX_UGLY_ENG_IOS_HACKS_RE
            case 0x005F: /* hack for iOS Eng */
            case 0x0060: /* hack for iOS Eng */
    #endif
            {
                /* 1 Argument to read */
                fread(&pdata[0], 2, 1, inFile);
				
				/* Item Fix for SSS Importing from SSSC */
#define CONV_ITEMS_TO_SSS
#ifdef CONV_ITEMS_TO_SSS
				if(checkSSSItemHack()){
					if( (cmd==0x1F) || (cmd==0x20))
					{
						unsigned char* pItem = (unsigned char*)pdata;
	//					printf("pItem is 0x%X 0x%X\n",pItem[0],pItem[1]);
						if(pItem[0] == 0x90)
							pItem[0] = 0x91;
						else if(pItem[0] == 0x91)
							pItem[0] = 0x92;
						else if(pItem[0] == 0x96)
							pItem[0] = 0x95;
						else if(pItem[0] == 0x97)
							pItem[0] = 0x96;
						else if(pItem[0] == 0x98)
							pItem[0] = 0x97;
						else if(pItem[0] == 0x99)
							pItem[0] = 0x98;
						else if(pItem[0] == 0x9a)
							pItem[0] = 0x98;
						else if(pItem[0] == 0x9b)
							pItem[0] = 0x99;
						else if(pItem[0] == 0xa5)
							pItem[0] = 0x9a;
						else if(pItem[0] == 0xa7)
							pItem[0] = 0x9b;
						else if(pItem[0] == 0xb5)
							pItem[0] = 0xa0;
					}
				}
#endif
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

					if ((cmd == 0x1F) || (cmd == 0x2A) || (cmd == 0x2E) || (cmd == 0x47) || (cmd == 0x45) || 
						(cmd == 0x52) || (cmd == 0x5A) || (cmd == 0x30) || (cmd == 0x29) || (cmd == 0x28)
						|| (cmd == 0x23) || (cmd == 0x20) || (cmd == 0x57) || (cmd == 0x48) 
						|| (cmd == 0x43) || (cmd == 0x44) || (cmd == 0x56) || (cmd == 0x59)
						|| (cmd == 0x5F) || (cmd == 0x60) || (cmd == 0x49))
					{
						swap16(&params[z].value);
					}
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
					if ((z == 0) && ((cmd == 0x24) || (cmd == 0x2D) || (cmd == 0x46) || (cmd == 0x50) || (cmd == 0x22)))
						swap16(&params[z].value);
					else if (z != 0)
	                    swap16(&params[z].value);  //Word-swap for PSX
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
					if (z != 0)
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

					if ((cmd == 0x5C) && ( (z == 2) || (z >= 4) ) )
                        swap16(&params[z].value); //Word-swap for PSX
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
case 0x0062: /* REMASTER: 9 shorts, or is it variable and ends with FFFF + 1 more arg? */
            {
                /* 9 Arguments to read */
                fread(&pdata[0], 2, 9, inFile);

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = 9;
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
                    //swap32(&params[1].value);  //No word-swap for PSX ??  Maybe?
                }
                else{  // 4 bytes
                    params[1].value = *((unsigned int *)(&pShort[0]));
                    //swap32(&params[1].value);  //No word-swap for PSX ??  Maybe?
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

#if 1
			/*******************************************************************/
            /* Variable - last byte is F900 with 1 child                       */
            /* Remaster ONLY.                                                  */
            /*******************************************************************/
            case 0x0063:
            {
				unsigned short tarray[30];
				int found = 0;
				int numarguments = 0;

				/* Variable Arguments to read */
                for (z = 0; z < 30; z++){
					fread(&tarray[z], 2, 1, inFile);
                    swap16(&tarray[z]); //Word-swap, assume LE?

//					printf("TARRAYZ = 0x%X\n",tarray[z]);
					if( (tarray[z] == 0x01F9) || (tarray[z] == 0x00F9) || (tarray[z] == 0xF900) || (tarray[z] == 0x03F9) || (tarray[z] == 0x07F9)) {
						found = 1;
//						printf("FOUND, BREAK\n");
						break;
					}
                }
				if(!found){
					printf("ERROR on 0x0063 CMD.\n");
					exit(-1);
				}
				numarguments = ++z;

                /* Create a new script node */
                if (createScriptNode(&sNode) < 0){
                    printf("Error creating a script node.\n");
                    return -1;
                }

                /* Fill in Parameters */
                sNode->id = G_ID++;
                sNode->nodeType = NODE_EXE_SUB;
                sNode->subroutine_code = cmd;
                sNode->num_parameters = numarguments;
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
                    params[z].value = tarray[z];
//                    swap16(&params[z].value); //Word-swap, assume LE?
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
#endif

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
                //swap16(&params[0].value); //No swap for PSX
                params[1].type = ALIGN_4_PARAM;
                params[1].value = 0x00;
                params[2].type = LONG_PARAM;

                if (bytesToRead == 6){
                    params[2].value = *((unsigned int *)(&pShort[1]));
                    //swap32(&params[2].value);  //No word-swap for PSX ??  Maybe?
                }
                else{  // == 8
                    params[2].value = *((unsigned int *)(&pShort[2]));
                    //swap32(&params[2].value);  //No word-swap for PSX ??  Maybe?
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
                swap16(&val);  //No swap required for PSX
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
	//			swap16(&pdata[0]);  //swap back
                for (z = 0; z < (int)sNode->num_parameters; z++){
                    params[z].type = SHORT_PARAM;
                    params[z].value = pShort[z];
                    swap16(&params[z].value);  //PSX does not need swap
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

//PSX Implements as bytes

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
                    //swap16(&params[z].value);  //PSX does not need
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
                    //swap16(&params[z].value); //Skip for PSX
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
					//if (cmd != 0x1A){
						swap16(&params[z].value);
					//}
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
                static char buf[2100];
				int lout, location, bytesRead;
				char* pOut;
                int nbytes;
                runParamType* rpHead;
                int index = 0;
                int textMode = getTextDecodeMethod();

                location = ftell(inFile);
                memset(buf,0,2100);
                rval = fread(buf, 1, 2048, inFile);
                if (rval <= 0){
                    printf("Error encountered while reading TEXT portion of script, no termination.\n");
                    break;
                }
                nbytes = rval;


//				printf("Parsing Text at 0x%X\n",location);
                if ((bytesRead = convertPSXText(buf, &pOut, nbytes, &lout)) < 0){
                    printf("Conversion Error\n");
                    break;
                }
//				printf("\n%s\n", pOut);


				location += bytesRead;
				fseek(inFile,location, SEEK_SET);

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
                rpHead = getRunParam(textMode, pOut);

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

//FIX
            case 0x0007:
            {
                static char buf[2100];
                int lout,location,bytesRead;
				char* pOut, *pOut2;
                int nbytes;
                runParamType* rpHead1, *rpHead2;
                unsigned short opt2Offset, parameter2;
                int index = 0;
                int textMode = getTextDecodeMethod();
                int storedTextMode = textMode;

                /* Offset to Opt2 Jump Point */
                fread(&opt2Offset, 2, 1, inFile);
                //swap16(&opt2Offset);

                /* NULL - well, not really NULL in all cases */
                fread(&parameter2, 2, 1, inFile);
                swap16(&parameter2);
                index = 0;

                /******************************/
                /* Option 1 (Top Option) Text */
                /******************************/

                location = ftell(inFile);
                memset(buf,0,2100);
                rval = fread(buf, 1, 2048, inFile);
                if (rval <= 0){
                    printf("Error encountered while reading TEXT portion of script, no termination.\n");
                    break;
                }
                nbytes = rval;

				if ((bytesRead = convertPSXText(buf, &pOut, nbytes, &lout)) < 0){
					printf("Conversion Error\n");
					break;
				}
                location += bytesRead;
				fseek(inFile,location, SEEK_SET);

                /* Advance File Pointer to a 16-bit boundary */
                /* Should already be on one unless running a 1-byte text hack */
                if ((ftell(inFile) % 2) != 0){
                    fread(&pdata[index], 1, 1, inFile);
                    //No need to increment index based on my storage format
                }

                /*********************************/
                /* Option 2 (Bottom Option) Text */
                /*********************************/

                location = ftell(inFile);
                memset(buf,0,2100);
                rval = fread(buf, 1, 2048, inFile);
                if (rval <= 0){
                    printf("Error encountered while reading TEXT portion of script, no termination.\n");
                    break;
                }
                nbytes = rval;


				if ((bytesRead = convertPSXText(buf, &pOut2, nbytes, &lout)) < 0){
					printf("Conversion Error\n");
					break;
				}
                location += bytesRead;
				fseek(inFile,location, SEEK_SET);

				/* Advance File Pointer to a 16-bit boundary */
				if ((ftell(inFile) % 2) != 0){
					fread(&pdata[index], 1, 1, inFile);
					//No need to increment index based on my storage format
				}

                offsetAddress = ftell(inFile);

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
                rpHead1 = getRunParam(storedTextMode, pOut);
                rpHead2 = getRunParam(storedTextMode, pOut2);

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
                //swap16(&wdOffset);  //Skip swap for PSX
                numArg += 2;

//Byte ARGs.  Check to see if they need to be swapped
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
					if (z > 0)
                        swap16(&params[z].value);   //PSX swapping
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
                //swap16(&wdOffset);  
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
					if (z > 0)
                        swap16(&params[z].value);  //PSX swap
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
                //swap16(&wdOffset);   //NOT FOR PSX
                fread(&pShort[numArg], 2, 1, inFile);
                memcpy(&bitOffset, &pShort[numArg++], 2);
                //swap16(&bitOffset);  //NOT FOR PSX
                zeroOffset = bitOffset;

                //Look for 0x0000 Terminator
                while (zeroOffset != 0x0000){
                    fread(&pShort[numArg], 2, 1, inFile);
                    memcpy(&zeroOffset, &pShort[numArg++], 2);
                    //swap16(&zeroOffset);    //NOT FOR PSX
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
                    //swap16(&params[z].value);     //NOT FOR PSX
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
				swap16(&pdata[0]);

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
				printf("CMD = 0x%X  Offset= 0x%X (0x%X short)\n", (unsigned int)cmd, offset, offset / 2);
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
