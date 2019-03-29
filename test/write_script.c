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
#include "snode_list.h"

/* Defines */


/* Globals */
static unsigned char* pOutput = NULL;
static unsigned int offset = 0x00;
static unsigned char* obuf = NULL;

/* Binary Output Unique Globals */
static unsigned int max_size_bytes;
static unsigned int output_endian_type;
static unsigned int G_table_mode;
static unsigned int max_boutput_size_bytes = 0;


/* Function Prototypes */
int writeBinScript(FILE* outFile);
int writeScript(FILE* outFile);
int dumpScript(FILE* outFile);

/* Write Fctns */
static int writeLW(unsigned int data);
static int writeSW(unsigned short data);
static int writeBYTE(unsigned char data);
static char* formatVal(unsigned int value);
char* ctrlCodeLkup(unsigned short ctrlCode);




/*****************************************************************************/
/* Function: writeLW                                                         */
/* Purpose: Writes a long to a simulated file in memory.  Updates file ptr.  */
/* Returns 0 on success, -1 on error.                                        */
/*****************************************************************************/
static int writeLW(unsigned int data){

    unsigned int* pData = (unsigned int*)pOutput;

    /* Verify write can take place */
    if( (offset + 4) > (max_size_bytes-1)){
        printf("Error, LW Write would exceed MAX filesize.\n");
        return -1;
    }

    if (output_endian_type == LITTLE_ENDIAN){
        *pData = data;
    }
    else{
        unsigned char* pSrc = (unsigned char*)&data;
        unsigned char* pDst = (unsigned char*)pOutput;

        pDst[0] = pSrc[3];
        pDst[1] = pSrc[2];
        pDst[2] = pSrc[1];
        pDst[3] = pSrc[0];
    }

    /* Advance Output Pointer */
    pOutput += 4;
    offset += 4;

    /* Update size of data to write to the binary output file */
    if(offset > max_boutput_size_bytes)
        max_boutput_size_bytes = offset;

    return 0;
}




/*****************************************************************************/
/* Function: writeSW                                                         */
/* Purpose: Writes a short to a simulated file in memory.  Updates file ptr. */
/* Returns 0 on success, -1 on error.                                        */
/*****************************************************************************/
static int writeSW(unsigned short data){

    unsigned short* pData = (unsigned short*)pOutput;

    /* Verify write can take place */
    if( (offset + 2) > (max_size_bytes-1)){
        printf("Error, SW Write would exceed MAX filesize.\n");
        return -1;
    }

    if (output_endian_type == LITTLE_ENDIAN){
        *pData = data;
    }
    else{
        unsigned char* pSrc = (unsigned char*)&data;
        unsigned char* pDst = (unsigned char*)pOutput;

        pDst[0] = pSrc[1];
        pDst[1] = pSrc[0];
    }

    /* Advance Output Pointer */
    pOutput += 2;
    offset += 2;

    /* Update size of data to write to the binary output file */
    if (offset > max_boutput_size_bytes)
        max_boutput_size_bytes = offset;

    return 0;
}




/*****************************************************************************/
/* Function: writeBYTE                                                       */
/* Purpose: Writes a byte to a simulated file in memory.  Updates file ptr.  */
/* Returns 0 on success, -1 on error.                                        */
/*****************************************************************************/
static int writeBYTE(unsigned char data){

    /* Verify write can take place */
    if( (offset + 1) > (max_size_bytes-1)){
        printf("Error, Byte Write would exceed MAX filesize.\n");
        return -1;
    }

    *pOutput = data;

    /* Advance Output Pointer */
    pOutput++;
    offset++;

    /* Update size of data to write to the binary output file */
    if (offset > max_boutput_size_bytes)
        max_boutput_size_bytes = offset;

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

    int x;
    scriptNode* pNode = NULL;
    max_boutput_size_bytes = 0;

    /* Get Max Filesize for Binary Output */
    max_size_bytes = getBinMaxSize();

    /* Get Binary File Output Mode */
    output_endian_type = getBinOutputMode();

    /* Check text table encoding for later output */
    G_table_mode = getTableOutputMode();

    /* Allocate memory for output */
    /* File will be kept in memory until completed */
    obuf = (unsigned char*)malloc(max_size_bytes);
    if (obuf == NULL){
        printf("Error allocating memory for output buffer.\n");
        return -1;
    }
    memset(obuf, DEFAULT_FILL, max_size_bytes);
    pOutput = obuf;
    offset = 0x00;

    /* Get a Pointer to the Head of the linked list */
    pNode = getHeadPtr();

    /******************************************************************/
    /* Loop until the binary output corresponding to all list entries */
    /* has been output to memory.  Then write to disk.                */
    /******************************************************************/
    while(pNode != NULL){

        switch(pNode->nodeType){

            /********/
            /* goto */
            /********/
            case NODE_GOTO:
            {
                /* Update output pointer */
                offset = (unsigned int)pNode->byteOffset;
                pOutput = obuf + offset;
                pNode->fileOffset = offset;  //Book keeping
            }
            break;


            /********/
            /* fill */
            /********/
            case NODE_FILL_SPACE:
            {
                unsigned int numBytes;
                numBytes = pNode->unit_count*pNode->unit_size;

                /* Check that operation can be completed */
                if ((numBytes + offset) > max_size_bytes){
                    printf("Error, fill extending beyond max file size.\n");
                    return -1;
                }

                pNode->fileOffset = offset;  //Book keeping

                /* Copy the fill data */
                switch (pNode->unit_size){
                    case 1:
                    {
                        unsigned char fill_byte;
                        pNode->fillVal &= 0xFF;
                        fill_byte = (unsigned char)pNode->fillVal;
                        for (x = 0; x < (int)pNode->unit_count; x++){
                            writeBYTE(fill_byte);
                        }
                        break;
                    }
                    case 2:
                    {
                        unsigned short fill_short;
                        pNode->fillVal &= 0xFFFF;
                        fill_short = (unsigned short)pNode->fillVal;
                        for (x = 0; x < (int)pNode->unit_count; x++){
                            writeSW(fill_short);
                        }
                        break;
                    }
                    case 4:
                    {
                        for (x = 0; x < (int)pNode->unit_count; x++){
                            writeLW(pNode->fillVal);
                        }
                        break;
                    }
                    default:
                        printf("Invalid unit size.\n");
                        return -1;
                    }
            }
            break;


            /***********/
            /* pointer */
            /***********/
            case NODE_POINTER:
            {
                /* Move file pointer to where the write is to take place */
                offset = pNode->byteOffset;
                pOutput = obuf + offset;
                pNode->fileOffset = offset;  //Book keeping

                /* Check that operation can be completed */
                if ((pNode->ptrSize + offset) > max_size_bytes){
                    printf("Error, pointer extending beyond max file size.\n");
                    return -1;
                }

                /* If value is selected: */
                /* Write out the pointer value and update the ptr */
                if (pNode->ptrValueFlag){
                    switch (pNode->ptrSize){
                        case 2:  /* Short Ptr */
                        {
                            writeSW((unsigned short)(pNode->ptrValue & 0xFFFF));
                            break;
                        }
                        case 4: /* Long Ptr */
                        {
                            writeLW(pNode->ptrValue);
                            break;
                        }
                        default:
                        {
                            printf("Invalid pointer size.\n");
                            return -1;
                        }
                    }
                }
                else{
                    /* ID is selected, hold off on pointer writing for later */
                    /* For now just */
                    pOutput += pNode->ptrSize;
                    offset += pNode->ptrSize;
                    break;
                }
            }
            break;


            /*****************************************************/
            /* execute-subroutine                                */
            /* Output Subroutine Code and Parameters to the file */
            /*****************************************************/
            case NODE_EXE_SUB:
            {
                int numBytes = 0;

                /* Check that operation can be completed */
                numBytes += 2; /* At least need to output subroutine code */
                for (x = 0; x < (int)pNode->num_parameters; x++){
                    switch (pNode->subParams[x].type){
                        case BYTE_PARAM:
                            numBytes++;
                            break;
                        case SHORT_PARAM:
                            numBytes+=2;
                            break;
                        case LONG_PARAM:
                            numBytes+=4;
                            break;
                        case ALIGN_2_PARAM:
                            numBytes++; /* overestimate for simplicity */
                            break;
                        case ALIGN_4_PARAM:
                            numBytes+=3; /* overestimate for simplicity */
                            break;
                        default:
                            printf("Error, bad subroutine parameter detected.\n");
                            return -1;
                    }
                }
                if ((numBytes + offset) > max_size_bytes){
                    printf("Error, subroutine would extend beyond max file size.\n");
                    return -1;
                }

                pNode->fileOffset = offset;  //Book keeping

                /* Write Subroutine Code */
                writeSW(pNode->subroutine_code);
    
                /* Write Parameters */
                for (x = 0; x < (int)pNode->num_parameters; x++){
                    switch (pNode->subParams[x].type){
                        case BYTE_PARAM:
                            writeBYTE(pNode->subParams[x].value & 0xFF);
                            break;
                        case SHORT_PARAM:
                            writeSW(pNode->subParams[x].value & 0xFFFF);
                            break;
                        case LONG_PARAM:
                            writeLW(pNode->subParams[x].value);
                            break;
                        case ALIGN_2_PARAM:
                            if ((offset & 0x1) != 0x0){
                                writeBYTE(pNode->alignfillVal);
                            }
                            break;
                        case ALIGN_4_PARAM:
                            while ((offset & 0x3) != 0x0){
                                writeBYTE(pNode->alignfillVal);
                            }
                            break;
                        default:
                            printf("Error, writing subroutine parameters, can't get here.\n");
                            return -1;
                    }
                }
            }
            break;


            /****************/
            /* run-commands */
            /****************/
            case NODE_RUN_CMDS:
            {
                runParamType* rpNode = pNode->runParams;
                int numBytes = 0;

                /* Check that operation can be completed */
                numBytes += 2; /* At least need to output subroutine code */
                while (rpNode != NULL) {

                    switch (rpNode->type){

                    case ALIGN_2_PARAM:
                        numBytes++; /* overestimate for simplicity */
                        break;
                    case ALIGN_4_PARAM:
                        numBytes+=3; /* overestimate for simplicity */
                        break;
                    case SHOW_PORTRAIT_LEFT:
					case SHOW_PORTRAIT_RIGHT:
					case TIME_DELAY:
                        numBytes+=2;
                        break;
                    case PRINT_LINE:
                    {
                        int numBytes2, numBytes3;
                        unsigned char* pText = rpNode->str;
                        numBytes2 = numBytes3 = 0;
                        while (*pText != '\0'){

                            /* Read in a utf8 character */
                            numBytes3 = numBytesInUtf8Char((unsigned char)*pText);

                            /* Look up associated code */
                            if ((numBytes3 == 1) && (*pText == ' ')){
                                numBytes2 += 2; /* Space */
                            }
                            else{
                                if (G_table_mode == ONE_BYTE_ENC){
                                    numBytes2++;
                                }
                                else if (G_table_mode == TWO_BYTE_ENC){
                                    numBytes2 += 2;
                                }
                                else{   //Straight UTF-8 Encoding
                                    numBytes2 += numBytes3;
                                }
                            }
                            pText += numBytes3;
                        }
                        numBytes += numBytes2;
                    }
                    break;
                    case CTRL_CODE:
                        numBytes+=2;
                        break;
                    default:
                        printf("Error, bad run cmd parameter detected.\n");
                        return -1;
                    }

                    rpNode = rpNode->pNext;
                }
                if ((numBytes + offset) > max_size_bytes){
                    printf("Error, subroutine 0002 would extend beyond max file size.\n");
                    return -1;
                }

                pNode->fileOffset = offset;  //Book keeping

                /* This is all part of subroutine code 0x0002 */
                writeSW(0x0002);

                rpNode = pNode->runParams;
                while (rpNode != NULL) {

                    switch (rpNode->type){

                        /***********/
                        /* align-2 */
                        /***********/
                        case ALIGN_2_PARAM:
                            if ((offset & 0x1) != 0x0){
                                writeBYTE((unsigned char)rpNode->value);
                            }
                            break;


                        /***********/
                        /* align-4 */
                        /***********/
                        case ALIGN_4_PARAM:
                            while ((offset & 0x3) != 0x0){
                                writeBYTE((unsigned char)rpNode->value);
                            }
                            break;


                        /**********************/
                        /* show-portrait-left */
                        /**********************/
                        case SHOW_PORTRAIT_LEFT:
                        {
                            unsigned short portraitCode;
                            portraitCode = 0xFA00 | ((unsigned short)rpNode->value);
                            writeSW(portraitCode);
                        }
                        break;
                    
						/***********************/
						/* show-portrait-right */
						/***********************/
						case SHOW_PORTRAIT_RIGHT:
						{
							unsigned short portraitCode;
							portraitCode = 0xFB00 | ((unsigned short)rpNode->value);
							writeSW(portraitCode);
						}
						break;

						/**************/
						/* time-delay */
						/**************/
						case TIME_DELAY:
						{
							unsigned short timedelay;
							timedelay = 0xF800 | ((unsigned short)rpNode->value);
							writeSW(timedelay);
						}
							break;

                        /**************/
                        /* print-line */
                        /**************/
                        case PRINT_LINE:
                        {
                            unsigned char* pText = rpNode->str;
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
                                    writeSW(0xF90A); /* Space */
                                }
                                else{
                                    if (G_table_mode == ONE_BYTE_ENC){
                                        if (getUTF8code_Byte(tmp, &code) < 0){
                                            printf("Error looking up 1-byte code corresponding with UTF-8 character\n");
                                            return -1;
                                        }

                                        /* Write the code to the output file */
                                        writeBYTE(code);
                                    }

                                    else if (G_table_mode == TWO_BYTE_ENC){
                                        if (getUTF8code_Short(tmp, &scode) < 0){
                                            printf("Error looking up 2-byte code corresponding with UTF-8 character\n");
                                            return -1;
                                        }

                                        /* Write the code to the output file */
                                        writeSW(scode);
                                    }

                                    else{   //Straight UTF-8 Encoding
                                        int z;

                                        /* Write the data to the output file */
                                        for (z = 0; z < numBytes; z++){
                                            writeBYTE(tmp[z]);
                                        }
                                    }
                                }
                                pText += numBytes;
                            }
                        }
                        break;

                        /****************/
                        /* control-code */
                        /****************/
                        case CTRL_CODE:
                            writeSW((unsigned short)rpNode->value);
                            break;


                        default:
                            printf("Error, bad run cmd parameter detected.\n");
                            return -1;
                    }

                    rpNode = rpNode->pNext;
                }
            }
            break;


			/***********/
			/* options */
			/***********/
            case NODE_OPTIONS:
			{
				runParamType* rpNode = NULL;
				int numBytes = 0;

				/* Check that operation can be completed */
				numBytes += 6; /* At least need to output subroutine code & 2 param */
				
				for (x = 0; x < 2; x++){
					if (x == 0)
						rpNode = pNode->runParams;
					else
						rpNode = pNode->runParams2;
					while (rpNode != NULL) {

						switch (rpNode->type){

						case ALIGN_2_PARAM:
							numBytes++; /* overestimate for simplicity */
							break;
						case ALIGN_4_PARAM:
							numBytes += 3; /* overestimate for simplicity */
							break;
						case PRINT_LINE:
						{
							int numBytes2, numBytes3;
							unsigned char* pText = rpNode->str;
							numBytes2 = numBytes3 = 0;
							while (*pText != '\0'){

								/* Read in a utf8 character */
								numBytes3 = numBytesInUtf8Char((unsigned char)*pText);

								/* Look up associated code */
								if ((numBytes3 == 1) && (*pText == ' ')){
									numBytes2 += 2; /* Space */
								}
								else{
									if (G_table_mode == ONE_BYTE_ENC){
										numBytes2++;
									}
									else if (G_table_mode == TWO_BYTE_ENC){
										numBytes2 += 2;
									}
									else{   //Straight UTF-8 Encoding
										numBytes2 += numBytes3;
									}
								}
								pText += numBytes3;
							}
							numBytes += numBytes2;
						}
							break;
						case CTRL_CODE:
							numBytes += 2;
							break;
						default:
							printf("Error, bad run cmd parameter detected.\n");
							return -1;
						}

						rpNode = rpNode->pNext;
					}
				}
				if ((numBytes + offset) > max_size_bytes){
					printf("Error, subroutine 0007 would extend beyond max file size.\n");
					return -1;
				}

				pNode->fileOffset = offset;  //Book keeping

				/* This is all part of subroutine code 0x0007 */
				writeSW(0x0007);
				writeSW(pNode->subParams[0].value);
				writeSW(pNode->subParams[1].value);

				/*****************************/
				/* Loop Through Both Options */
				/*****************************/
				for (x = 0; x < 2; x++){
					if (x == 0)
						rpNode = pNode->runParams;
					else
						rpNode = pNode->runParams2;

					while (rpNode != NULL) {

						switch (rpNode->type){

							/***********/
							/* align-2 */
							/***********/
							case ALIGN_2_PARAM:
								if ((offset & 0x1) != 0x0){
									writeBYTE((unsigned char)rpNode->value);
								}
							break;


							/***********/
							/* align-4 */
							/***********/
							case ALIGN_4_PARAM:
								while ((offset & 0x3) != 0x0){
									writeBYTE((unsigned char)rpNode->value);
								}
							break;


							/**************/
							/* print-line */
							/**************/
							case PRINT_LINE:
							{
								unsigned char* pText = rpNode->str;
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
										writeSW(0xF90A); /* Space */
									}
									else{
										if (G_table_mode == ONE_BYTE_ENC){
											if (getUTF8code_Byte(tmp, &code) < 0){
												printf("Error looking up 1-byte code corresponding with UTF-8 character\n");
												return -1;
											}

											/* Write the code to the output file */
											writeBYTE(code);
										}

										else if (G_table_mode == TWO_BYTE_ENC){
											if (getUTF8code_Short(tmp, &scode) < 0){
												printf("Error looking up 2-byte code corresponding with UTF-8 character\n");
												return -1;
											}

											/* Write the code to the output file */
											writeSW(scode);
										}

										else{   //Straight UTF-8 Encoding
											int z;

											/* Write the data to the output file */
											for (z = 0; z < numBytes; z++){
												writeBYTE(tmp[z]);
											}
										}
									}
									pText += numBytes;
								}
							}
							break;

							/****************/
							/* control-code */
							/****************/
							case CTRL_CODE:
								writeSW((unsigned short)rpNode->value);
								break;


							default:
								printf("Error, bad run cmd parameter detected.\n");
								return -1;
						}

						rpNode = rpNode->pNext;
					}
				}
			}	
			break;



            default:
            {
                printf("ERROR, unrecognized node.  HALTING output.\n");
                return -1;
            }
            break;
        }
        pNode = pNode->pNext;
    }


    /**************************************************************/
    /* Handle Skipped Pointer's that were tied to Node ID offsets */
    /**************************************************************/

    /* Get a Pointer to the Head of the linked list */
    pNode = getHeadPtr();
    while (pNode != NULL){

        scriptNode *ptrTarget;
        unsigned int ptrOffset;

        if ((pNode->nodeType == NODE_POINTER) && (pNode->ptrValueFlag == 0)){

            /* Move file pointer to where the write is to take place */
            offset = pNode->byteOffset;
            pOutput = obuf + offset;

            /* Check that operation can be completed */
            if ((pNode->ptrSize + offset) > max_size_bytes){
                printf("Error, pointer extending beyond max file size.\n");
                return -1;
            }

            /* Retrieve the starting offset of the node in question */
            ptrTarget = getListItemByID(pNode->ptrID);
            if (ptrTarget == NULL){
                printf("Error locating pointer target.\n");
                return -1;
            }
            ptrOffset = ptrTarget->fileOffset;

            switch (pNode->ptrSize){
                case 2:  /* Short Ptr */
                {
                    ptrOffset /= 2;
                    writeSW((unsigned short)(ptrOffset & 0xFFFF));
                    break;
                }
                case 4: /* Long Ptr */
                {
                    ptrOffset /= 4;
                    writeLW(ptrOffset);
                    break;
                }
                default:
                {
                    printf("Invalid pointer size.\n");
                    return -1;
                }
            }
        }

        pNode = pNode->pNext;
    }

    /**************************************/
    /* Output the binary data to the file */
    /**************************************/
    fwrite(obuf, 1, max_boutput_size_bytes, outFile);
    if (obuf != NULL)
        free(obuf);

    return 0;
}




/******************************************************/
/* formatVal - formats the given number as Hex or Dec */
/******************************************************/
static char* formatVal(unsigned int value){
    
    static char scratch[64];
    int mode = getMetaScriptInputMode();

    memset(scratch, 0, 64);
    if (mode == RADIX_HEX){
        sprintf(scratch, "%X",value);
    }
    else{
        sprintf(scratch, "%d", value);
    }

    return scratch;
}




/*****************************************************************************/
/* Function: writeScript                                                     */
/* Purpose: Reads from a linked list data structure in memory to create a    */
/*          human readable metadata version of the script.                   */
/* Inputs:  Pointer to output file.                                          */
/* Outputs: 0 on Pass, -1 on Fail.                                           */
/*****************************************************************************/
int writeScript(FILE* outFile){
    
    int x;
    scriptNode* pNode = NULL;

    /* Output Header */
    fprintf(outFile, "(start\r\n");
    if( getBinOutputMode() == BIG_ENDIAN)
        fprintf(outFile, "    (endian=big)\r\n");
    else
        fprintf(outFile, "    (endian=little)\r\n");
    if (getMetaScriptInputMode() == RADIX_HEX)
        fprintf(outFile, "    (radix=hex)\r\n");
    else
        fprintf(outFile, "    (radix=dec)\r\n");
    fprintf(outFile, "    (max_size_bytes=%s)\r\n", formatVal(getBinMaxSize()));
    fprintf(outFile, ")\r\n");

    /* Get a Pointer to the Head of the linked list */
    pNode = getHeadPtr();

    /******************************************************************/
    /* Loop until the binary output corresponding to all list entries */
    /* has been output to memory.  Then write to disk.                */
    /******************************************************************/
    while (pNode != NULL){

        switch (pNode->nodeType){

            /********/
            /* goto */
            /********/
            case NODE_GOTO:
            {
                fprintf(outFile, "(goto id=%u\r\n", pNode->id);
                fprintf(outFile, "    (location %s)\r\n", formatVal(pNode->byteOffset));
                fprintf(outFile, ")\r\n");
            }
            break;


            /********/
            /* fill */
            /********/
            case NODE_FILL_SPACE:
            {
                fprintf(outFile, "(fill-space id=%u\r\n", pNode->id);
                fprintf(outFile, "    (unit-size %s)\r\n", formatVal(pNode->unit_size));
                fprintf(outFile, "    (fill-value %s)\r\n", formatVal(pNode->fillVal));
                fprintf(outFile, "    (unit-count %s)\r\n", formatVal(pNode->unit_count));
                fprintf(outFile, ")\r\n");
            }
            break;


            /***********/
            /* pointer */
            /***********/
            case NODE_POINTER:
            {
                fprintf(outFile, "(pointer id=%u\r\n", pNode->id);
                fprintf(outFile, "    (byteoffset %s)\r\n", formatVal(pNode->byteOffset));
                fprintf(outFile, "    (size %s)\r\n", formatVal(pNode->ptrSize));
                if (pNode->ptrValueFlag)
                    fprintf(outFile, "    (value %s)\r\n", formatVal(pNode->ptrValue));
                else
                    fprintf(outFile, "    (id-link %s)\r\n", formatVal(pNode->ptrID));
                fprintf(outFile, ")\r\n");
            }
            break;


            /*****************************************************/
            /* execute-subroutine                                */
            /*****************************************************/
            case NODE_EXE_SUB:
            {
                fprintf(outFile, "(execute-subroutine id=%u\r\n", pNode->id);
                fprintf(outFile, "    (subroutine %s)\r\n", formatVal(pNode->subroutine_code));
                fprintf(outFile, "    (num-parameters %s)\r\n", formatVal(pNode->num_parameters));
                if (pNode->num_parameters > 0){
                    fprintf(outFile, "    (align-fill-byteval %s)\r\n", formatVal(pNode->alignfillVal));
                    fprintf(outFile, "    (parameter-types ");
                    for (x = 0; x < (int)pNode->num_parameters; x++){
                        switch (pNode->subParams[x].type){
                            case BYTE_PARAM:
                                fprintf(outFile, "1 ");
                                break;
                            case SHORT_PARAM:
                                fprintf(outFile, "2 ");
                                break;
                            case LONG_PARAM:
                                fprintf(outFile, "4 ");
                                break;
                            case ALIGN_2_PARAM:
                                fprintf(outFile, "align-2 ");
                                break;
                            case ALIGN_4_PARAM:
                                fprintf(outFile, "align-4 ");
                                break;
                            default:
                                printf("Error, bad subroutine parameter detected.\n");
                                return -1;
                        }
                    }
                    fprintf(outFile, ")\r\n");
                    
                    fprintf(outFile, "    (parameter-values ");
                    for (x = 0; x < (int)pNode->num_parameters; x++){
                        if ((pNode->subParams[x].type == ALIGN_2_PARAM) || (pNode->subParams[x].type == ALIGN_4_PARAM))
                            continue;
                        fprintf(outFile, "%s ", formatVal(pNode->subParams[x].value));
                    }
                    fprintf(outFile, ")\r\n");
                }
                fprintf(outFile, ")\r\n");
            }
            break;


            /****************/
            /* run-commands */
            /****************/
            case NODE_RUN_CMDS:
            {
                runParamType* rpNode = pNode->runParams;

                fprintf(outFile, "(run-commands id=%u\r\n", pNode->id);

                while (rpNode != NULL) {

                    switch (rpNode->type){

                        case ALIGN_2_PARAM:
                            fprintf(outFile, "    (align-2 %s)\r\n", formatVal(rpNode->value));
                            break;
                        case ALIGN_4_PARAM:
                            fprintf(outFile, "    (align-4 %s)\r\n", formatVal(rpNode->value));
                            break;
                        case SHOW_PORTRAIT_LEFT:
                            fprintf(outFile, "    (show-portrait-left %s)\r\n", formatVal(rpNode->value & 0xFF));
                            break;
						case SHOW_PORTRAIT_RIGHT:
							fprintf(outFile, "    (show-portrait-right %s)\r\n", formatVal(rpNode->value & 0xFF));
							break;
						case TIME_DELAY:
							fprintf(outFile, "    (time-delay %s)\r\n", formatVal(rpNode->value & 0xFF));
							break;
                        case PRINT_LINE:
                            fprintf(outFile, "    (print-line \"%s\")\r\n", rpNode->str);
                            break;
                        case CTRL_CODE:
                            fprintf(outFile, "    (control-code %s)\r\n", formatVal(rpNode->value));
                            break;
                        default:
                            printf("Error, bad run cmd parameter detected.\n");
                            return -1;
                    }

                    rpNode = rpNode->pNext;
                }
                fprintf(outFile, "    (commands-end)\r\n");
                fprintf(outFile, ")\r\n");
            }
            break;


			/***********/
			/* Options */
			/***********/
			case NODE_OPTIONS:
			{
				runParamType* rpNode = NULL;

				fprintf(outFile, "(options id=%u\r\n", pNode->id);

				/* Print the 2 required parameters */
				fprintf(outFile, "    (jmpparam %s)\r\n", formatVal(pNode->subParams[0].value));
				fprintf(outFile, "    (param2 %s)\r\n", formatVal(pNode->subParams[1].value));

				/* Fixed at 2 options */
				for (x = 0; x < 2; x++){
					if (x == 0){
						rpNode = pNode->runParams;
						fprintf(outFile, "    (opt1)\r\n");
					}
					else{
						rpNode = pNode->runParams2;
						fprintf(outFile, "    (opt2)\r\n");
					}
					while (rpNode != NULL) {

						switch (rpNode->type){

						case ALIGN_2_PARAM:
							fprintf(outFile, "    (align-2 %s)\r\n", formatVal(rpNode->value));
							break;
						case ALIGN_4_PARAM:
							fprintf(outFile, "    (align-4 %s)\r\n", formatVal(rpNode->value));
							break;
						case PRINT_LINE:
							fprintf(outFile, "    (print-line \"%s\")\r\n", rpNode->str);
							break;
						case CTRL_CODE:
							fprintf(outFile, "    (control-code %s)\r\n", formatVal(rpNode->value));
							break;
						default:
							printf("Error, bad run cmd parameter detected.\n");
							return -1;
						}

						rpNode = rpNode->pNext;
					}
					fprintf(outFile, "    (opt-end)\r\n");
				}
				fprintf(outFile, ")\r\n");
			}
			break;


            default:
            {
                printf("ERROR, unrecognized node.  HALTING output.\n");
                return -1;
            }
            break;
        }
        pNode = pNode->pNext;
    }

    /* End Footer */
    fprintf(outFile, "(end)\r\n");

    return 0;
}




/*****************************************************************************/
/* Function: dumpScriptText                                                  */
/* Purpose: Reads from a linked list data structure in memory to create a    */
/*          CSV tab delimited version of the script.                         */
/* Inputs:  Pointer to output file.                                          */
/* Outputs: 0 on Pass, -1 on Fail.                                           */
/*****************************************************************************/
int dumpScript(FILE* outFile){

	int x;
	scriptNode* pNode = NULL;

	/* Output Header */
	fprintf(outFile, "Parent Ptr\tNode ID\tType\tJP Text\tCtrl Codes\r\n");

	/* Get a Pointer to the Head of the linked list */
	pNode = getHeadPtr();

	/******************************************************************/
	/* Loop until the binary output corresponding to all list entries */
	/* has been output to memory.  Then write to disk.                */
	/******************************************************************/
	while (pNode != NULL){

		switch (pNode->nodeType){

			/********/
			/* goto */
			/********/
			case NODE_GOTO:
			{
				;
			}
			break;


			/********/
			/* fill */
			/********/
			case NODE_FILL_SPACE:
			{
				;
			}
			break;


			/***********/
			/* pointer */
			/***********/
			case NODE_POINTER:
			{
				;
			}
			break;


			/*****************************************************/
			/* execute-subroutine                                */
			/*****************************************************/
			case NODE_EXE_SUB:
			{
				int x;

				if (pNode->pointerID != INVALID_PTR_ID)
					fprintf(outFile, "%u", pNode->pointerID);
				fprintf(outFile, "\t%u", pNode->id);

				switch (pNode->subroutine_code){

					/* CHRSET */
					/* CHRREST */
					case 0x000E:
					case 0x000F:
					{
						int numparam = pNode->num_parameters;

						if (pNode->subroutine_code == 0x000E)
							fprintf(outFile, "\tCHRSET ");
						else
							fprintf(outFile, "\tCHRREST ");

						for (x = 0; x < numparam; x++){
							int val1, val2;
							if (x > 0)
								fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							if (x == (int)(pNode->num_parameters - 1))
								fprintf(outFile, "%d", val1);
							else
								fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}


					/* MONSET -  1,100,200*/
					case 0x0013:
					{
						fprintf(outFile, "\tMONSET ");
						for (x = 0; x < (int)(pNode->num_parameters); x++){
							int val1, val2;
							fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							if( x > 0)
								fprintf(outFile, ":");
							if (x == (int)(pNode->num_parameters - 1))
								;
							else
								fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}


#if 0
					/* TBD B -  7 */
					case 0x0014:
					{
						fprintf(outFile, "\tTBD ",
							for (x = 0; x < (int)(pNode->num_parameters); x++){
							int val1, val2;
							fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							if (x == (int)(pNode->num_parameters - 1))
								fprintf(outFile, "%d", val1);
							else
								fprintf(outFile, "%d,%d", val1, val2);
							}
						fprintf(outFile, "\r\n");
						break;
					}

#endif
#if 1
					/* OPENSET -  1,7,8,20*/
					case 0x0018:
					{
						fprintf(outFile, "\tOPENSET ");
						for (x = 0; x < (int)(pNode->num_parameters); x++){
							int val1, val2;
							if(x > 0)
 								fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							if (x == (int)(pNode->num_parameters - 1))
								fprintf(outFile, "%d", val1);
							else
								fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}

#endif

					/* OPENRESET */
					case 0x0019:
					{
						fprintf(outFile, "\tOPENRESET %d\r\n", (pNode->subParams[0].value & 0xFF00) >> 8);
						break;
					}

#if 1
					/* BOXSET -  1,2,4,11*/
					case 0x001A:
					{
						fprintf(outFile, "\tBOXSET ");
						for (x = 0; x < (int)(pNode->num_parameters-1); x++){
							int val1, val2;
							if(x > 0)
								fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}

#endif

#if 0
						/* TBD3 0, 2,33,7,9,45,51*/
					case 0x001C:
					{
						fprintf(outFile, "\tTBD %d:%d,%d\r\n",
							pNode->subParams[0].value, (pNode->subParams[1].value & 0xFF00) >> 8, pNode->subParams[1].value & 0xFF);
						break;
					}
#endif						

					/* TAKEMONEY */
					case 0x001D:
					{
						fprintf(outFile, "\tTAKEMONEY %d\r\n", 
							pNode->subParams[1].value);
						break;
					}

					/* CHKMONEY */
					case 0x001E:
					{
						fprintf(outFile, "\tCHKMONEY %d:%d\r\n", 
							pNode->subParams[0].value, pNode->subParams[1].value);
						break;
					}

					/* SETITEM */
					case 0x001F:
					{
						fprintf(outFile, "\tSETITEM %d,%d\r\n", (pNode->subParams[0].value & 0xFF00) >> 8, pNode->subParams[0].value & 0xFF);
						break;
					}

#if 0
					/* TBD2 0,3,4,5,7,8,13,14,15,17,25,27,42,44,45,50,200*/
					case 0x0020:
					{
						fprintf(outFile, "\tTBD %d:%d,%d\r\n",
							pNode->subParams[0].value, (pNode->subParams[1].value & 0xFF00) >> 8, pNode->subParams[1].value & 0xFF);
						break;
					}
#endif

					/* CHKITEM */
					case 0x0021:
					{
						fprintf(outFile, "\tCHKITEM %d:%d,%d\r\n", 
							pNode->subParams[0].value, (pNode->subParams[1].value & 0xFF00) >> 8, pNode->subParams[1].value & 0xFF);
						break;
					}

#if 0
						/* TBD3 0, 44, 200*/
					case 0x0022:
					{
						fprintf(outFile, "\tTBD %d:%d,%d\r\n",
							pNode->subParams[0].value, (pNode->subParams[1].value & 0xFF00) >> 8, pNode->subParams[1].value & 0xFF);
						break;
					}
#endif

					/* WITHIN */
					case 0x0023:
					{
						fprintf(outFile, "\tWITHIN %d,%d\r\n", (pNode->subParams[0].value & 0xFF00) >> 8, pNode->subParams[0].value & 0xFF);
						break;
					}


#if 1
						/* WITHOUT - 0,1,2,4,5,6,7*/
					case 0x0024:
					{
						fprintf(outFile, "\tWITHOUT %d,%d\r\n",
							pNode->subParams[0].value & 0xFF,(pNode->subParams[0].value & 0xFF00) >> 8);
						break;
					}
#endif


					/* KILL */
					case 0x0025:
					{
						fprintf(outFile, "\tKILL ");
						int numparam = pNode->num_parameters;
						for (x = 0; x < numparam; x++){
							int val1, val2;
							if (x > 0)
								fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							if (x == (int)(pNode->num_parameters - 1))
								fprintf(outFile, "%d", val1);
							else
								fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}

					/* WARP */
					case 0x0027:
					{
						fprintf(outFile, "\tWARP %d,%d\r\n", (pNode->subParams[0].value & 0xFF00) >> 8, pNode->subParams[0].value & 0xFF);
						break;
					}

					/* OPEN */
					case 0x0028:
					{
						fprintf(outFile, "\tOPEN %d\r\n", (pNode->subParams[0].value & 0xFF00) >> 8);
						break;
					}

					/* FADEOUT */
					case 0x0029:
					{
						fprintf(outFile, "\tFADEOUT %d\r\n", (pNode->subParams[0].value & 0xFF00) >> 8);
						break;
					}

					/* FADEIN */
					case 0x002A:
					{
						fprintf(outFile, "\tFADEIN %d\r\n", (pNode->subParams[0].value & 0xFF00) >> 8);
						break;
					}


#if 0
						/* TBD3 0, 4,5,6,8,9,11*/
					case 0x002B:
					{
						fprintf(outFile, "\tTBD %d:%d,%d\r\n",
							pNode->subParams[0].value, (pNode->subParams[1].value & 0xFF00) >> 8, pNode->subParams[1].value & 0xFF);
						break;
					}
#endif


					/* SHAKE */
					case 0x002C:
					{
						fprintf(outFile, "\tSHAKE %d",
							(pNode->subParams[0].value & 0xFF00) >> 8);
						for (x = 1; x < (int)(pNode->num_parameters); x++){
							int val1, val2;
							fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							if (x == (int)(pNode->num_parameters-1))
								fprintf(outFile, "%d", val1);
							else
								fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}

					/* EVENT */
					case 0x002D:
					{
						fprintf(outFile, "\tEVENT ");
						int numparam = pNode->num_parameters;
						for (x = 0; x < numparam; x++){
							int val1, val2;
							if (x > 0)
								fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							if (x == (int)(pNode->num_parameters - 1))
								fprintf(outFile, "%d", val1);
							else
								fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}

#if 1
					/* SHOP 0,2,3,5  */
					case 0x002E:
					{
						fprintf(outFile, "\tSHOP %d\r\n",
							pNode->subParams[0].value >> 8);
						break;
					}
#endif

					/* FIGURE */
					case 0x002F:
					{
						fprintf(outFile, "\tFIGURE\r\n");
						break;
					}

					/* PLAY */
					case 0x0030:
					{
						fprintf(outFile, "\tPLAY %d\r\n", (pNode->subParams[0].value & 0xFF00) >> 8);
						break;
					}

					/* SE */
					case 0x0031:
					{
						fprintf(outFile, "\tSE:%d:\r\n", pNode->subParams[0].value);
						break;
					}

					/* MVSTART */
					case 0x0032:
					{
						fprintf(outFile, "\tMVSTART\r\n");
						break;
					}

					/* MVEND */
					case 0x0033:
					{
						fprintf(outFile, "\tMVEND\r\n");
						break;
					}

					/* MVSET */
					case 0x0034:
					{
						fprintf(outFile, "\tMVSET %d:%d", (pNode->subParams[0].value & 0xFF00) >> 8, pNode->subParams[0].value & 0xFF);
						if (((pNode->subParams[0].value & 0xFF00) >> 8) >= 0xB){
							for (x = 1; x < (int)(pNode->num_parameters); x++){
								int val1, val2;
								fprintf(outFile, ",");
								val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
								val2 = pNode->subParams[x].value & 0xFF;
								if (x == (int)(pNode->num_parameters - 1))
									fprintf(outFile, "%d", val1);
								else
									fprintf(outFile, "%d,%d", val1, val2);
							}
						}
						fprintf(outFile, "\r\n");
						break;
					}

					/* MVKILL */
					case 0x0035:
					{
						fprintf(outFile, "\tMVKILL\r\n", (pNode->subParams[0].value & 0xFF00) >> 8);
						break;
					}

#if 0
					/* TBD B -  200*/
					case 0x0036:
					{
						fprintf(outFile, "\tTBD ");
						for (x = 0; x < (int)(pNode->num_parameters); x++){
							int val1, val2;
							fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							if (x == (int)(pNode->num_parameters - 1))
								fprintf(outFile, "%d", val1);
							else
								fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}

#endif

					/* MVDATA */
					case 0x0038:
					{
						fprintf(outFile, "\tMVDATA ");
						for (x = 0; x < (int)(pNode->num_parameters); x++){
							int val1, val2;
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}

					/* MVPARTYON */
					case 0x0039:
					{
						fprintf(outFile, "\tMVPARTYON\r\n");
						break;
					}

					/* MVPARTYOFF */
					case 0x003A:
					{
						fprintf(outFile, "\tMVPARTYOFF\r\n");
						break;
					}

#if 0
					/* TBD - 8,9,14,17,34,39,50 */
					case 0x003B:
					{
						fprintf(outFile, "\tTBD\r\n");
						break;
					}
#endif

					/* MVSTOPOFF */
					case 0x003C:
					{
						fprintf(outFile, "\tMVSTOPOFF\r\n");
						break;
					}

					/* MVSYNC */
					case 0x003D:
					{
						fprintf(outFile, "\tMVSYNC\r\n");
						break;
					}

					/* FACELOAD */
					case 0x003E:
					{
						fprintf(outFile, "\tFACELOAD ");
						int numparam = pNode->num_parameters;
						for (x = 0; x < numparam; x++){
							int val1, val2;
							if (x > 0)
								fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							if (x == (int)(pNode->num_parameters - 1))
								fprintf(outFile, "%d", val1);
							else
								fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}

#if 1
						/* MVSCRON - 1,2,4 */
					case 0x003F:
					{
						fprintf(outFile, "\tMVSCRON\r\n");
						break;
					}
#endif

					/* MVSCROFF */
					case 0x0040:
					{
						fprintf(outFile, "\tMVSCROFF\r\n");
						break;
					}


					/* CHRON */
					case 0x0041:
					{
						fprintf(outFile, "\tCHRON %d\r\n", (pNode->subParams[0].value & 0xFF00) >> 8);
						break;
					}


#if 1
					/* CHANGE 0,1,29,39,42,200*/
					case 0x0043:
					{
						fprintf(outFile, "\tCHANGE %d\r\n",
							pNode->subParams[0].value >> 8);
						break;
					}
#endif

#if 0
					/* TBD2 40,42*/
					case 0x0044:
					{
						fprintf(outFile, "\tTBD %d:%d,%d\r\n",
							pNode->subParams[0].value, (pNode->subParams[1].value & 0xFF00) >> 8, pNode->subParams[1].value & 0xFF);
						break;
					}
#endif

					/* WIPEIN */
					case 0x0045:
					{
						fprintf(outFile, "\tWIPEIN %d\r\n", (pNode->subParams[0].value & 0xFF00) >> 8);
						break;
					}

					/* OFFSET */
					case 0x0046:
					{
						fprintf(outFile, "\tOFFSET ");
						int numparam = pNode->num_parameters;
						for (x = 0; x < numparam; x++){
							int val1, val2;
							if (x > 0)
								fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							if (x == (int)(pNode->num_parameters - 1))
								fprintf(outFile, "%d", val1);
							else
								fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}

					/* COLMIX */
					case 0x0047:
					{
						fprintf(outFile, "\tCOLMIX ");
						int numparam = pNode->num_parameters;
						for (x = 0; x < numparam; x++){
							int val1, val2;
							if (x > 0)
								fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}

#if 0
					/* TBD2 0,4,18,25,28,41,42,44,51,200*/
					case 0x0048:
					{
						fprintf(outFile, "\tTBD %d:%d,%d\r\n",
							pNode->subParams[0].value, (pNode->subParams[1].value & 0xFF00) >> 8, pNode->subParams[1].value & 0xFF);
						break;
					}
#endif

#if 0
					/* TBD2 25,45,200 */
					case 0x0049:
					{
						fprintf(outFile, "\tTBD %d:%d,%d\r\n",
							pNode->subParams[0].value, (pNode->subParams[1].value & 0xFF00) >> 8, pNode->subParams[1].value & 0xFF);
						break;
					}
#endif

#if 0
					/* TBD - 20,22,28,30,34,50 */
					case 0x004C:
					{
						fprintf(outFile, "\tTBD\r\n");
						break;
					}

					/* TBD - 20,22,28,30,34,50 */
					case 0x004D:
					{
						fprintf(outFile, "\tTBD\r\n");
						break;
					}
#endif

#if 0
					/* TBD B -  16,18,20*/
					case 0x004E:
					{
						fprintf(outFile, "\tTBD ",
						for (x = 0; x < (int)(pNode->num_parameters); x++){
							int val1, val2;
							fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							if (x == (int)(pNode->num_parameters - 1))
								fprintf(outFile, "%d", val1);
							else
								fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}

#endif

#if 0
					/* TBD B -  16,18,20*/
					case 0x004F:
					{
						fprintf(outFile, "\tTBD ",
						for (x = 0; x < (int)(pNode->num_parameters); x++){
							int val1, val2;
							fprintf(outFile, ",");
							val1 = (pNode->subParams[x].value & 0xFF00) >> 8;
							val2 = pNode->subParams[x].value & 0xFF;
							if (x == (int)(pNode->num_parameters - 1))
								fprintf(outFile, "%d", val1);
							else
								fprintf(outFile, "%d,%d", val1, val2);
						}
						fprintf(outFile, "\r\n");
						break;
					}

#endif

#if 0
					/* TBD2  16,18,20 */
					case 0x0050:
					{
						fprintf(outFile, "\tTBD %d,%d\r\n",
							pNode->subParams[0].value, pNode->subParams[1].value);
						break;
					}
#endif

#if 0
					/* TBD 16, 20, 2, 29, 30, 39, 41, 42*/
					case 0x0051:
					{
						fprintf(outFile, "\tTBD\r\n");
						break;
					}
#endif
					/* VOICELOAD */
					case 0x0052:
					{
						fprintf(outFile, "\tVOICELOAD %d\r\n", (pNode->subParams[0].value & 0xFF00) >> 8);
						break;
					}

					/* VOICE */
					case 0x0053:
					{
						fprintf(outFile, "\tVOICE\r\n");
						break;
					}

					/* VOICESYNC */
					case 0x0054:
					{
						fprintf(outFile, "\tVOICESYNC\r\n");
						break;
					}

					/* FEEDSET */
					case 0x0055:
					{
						fprintf(outFile, "\tFEEDSET\r\n");
						break;
					}

#if 0
						/* TBD2  12 */
					case 0x0056:
					{
						fprintf(outFile, "\tTBD %d,%d\r\n",
							pNode->subParams[0].value, pNode->subParams[1].value);
						break;
					}
#endif

#if 0
						/* TBD2  4,5,9,10,11,12,28,34,47 */
					case 0x0057:
					{
						fprintf(outFile, "\tTBD %d,%d\r\n",
							pNode->subParams[0].value, pNode->subParams[1].value);
						break;
					}
#endif

#if 0
					/* TBD - 28*/
					case 0x0058:
					{
						fprintf(outFile, "\tTBD\r\n");
						break;
					}
#endif

#if 0
					/* TBD2  14,17,18,25,30 */
					case 0x0059:
					{
						fprintf(outFile, "\tTBD %d,%d\r\n",
							pNode->subParams[0].value, pNode->subParams[1].value);
						break;
					}
#endif

					/* VOLUME */
					case 0x005A:
					{
						fprintf(outFile, "\tVOLUME %d,%d\r\n", 
							(pNode->subParams[0].value & 0xFF00) >> 8, (pNode->subParams[0].value & 0xFF));
						break;
					}

#if 0
					/* TBD - 58, 200 */
					case 0x005B:
					{
						fprintf(outFile, "\tTBD\r\n");
						break;
					}
#endif
#if 0
						/* TBD3 200*/
					case 0x005C:
					{
						fprintf(outFile, "\tTBD %d:%d,%d\r\n",
							pNode->subParams[0].value, (pNode->subParams[1].value & 0xFF00) >> 8, pNode->subParams[1].value & 0xFF);
						break;
					}
#endif
#if 0
						/* TBD3 200*/
					case 0x005D:
					{
						fprintf(outFile, "\tTBD %d:%d,%d\r\n",
							pNode->subParams[0].value, (pNode->subParams[1].value & 0xFF00) >> 8, pNode->subParams[1].value & 0xFF);
						break;
					}
#endif

					/* TBD - iOS - 42 */
					case 0x005E:
					{
						fprintf(outFile, "\tiOS Subroutine 5E\r\n");
						break;
					}

#if 1
					/* TBD2 - iOS  13,17 */
					case 0x005F:
					{
						fprintf(outFile, "\tiOS Subroutine 5F\r\n");
						break;
					}
#endif


					/* TBD2 iOS 14, 17, 18, 25, 30 */
					case 0x0060:
					{
						fprintf(outFile, "\tiOS Subroutine 60\r\n");
						break;
					}





					/* RETURNS */
					case 0x0005:
					case 0x0026:
					{
						if (pNode->subroutine_code == 0x0005)
							fprintf(outFile, "\tRETURN\r\n");
						else{
							fprintf(outFile, "\tEXIT %d,%d,%d\r\n",
								pNode->subParams[1].value >> 8, pNode->subParams[1].value & 0xFF,
								pNode->subParams[0].value);
						}
						break;
					}

					/* UNCONDITIONAL JUMP */
					case 0x0003:
					case 0x0004:
					{
						if (pNode->subroutine_code == 0x0003)
							fprintf(outFile, "\tJUMP ");
						else
							fprintf(outFile, "\tCALL ");
						fprintf(outFile, "%d\r\n",pNode->subParams[0].value);
						break;
					}

					/* BIT ROUTINES */
					case 0x0008:
					case 0x0009:
					case 0x000A:
					{
						if (pNode->subroutine_code == 0x0009)
							fprintf(outFile, "\tSET ");
						else if (pNode->subroutine_code == 0x000A)
							fprintf(outFile, "\tRESET ");
						else
							fprintf(outFile, "\tRND ");

						for (x = 0; x < (int)(pNode->num_parameters-1); x++){
							if (x >= 1)
								fprintf(outFile, ",");
							fprintf(outFile, "%d", pNode->subParams[x].value);
						}
						fprintf(outFile, "\r\n");
						break;
					}

					/* CONDITIONAL JUMP */
					case 0x000B: /* AND */
					case 0x000C: /* NOT */
					case 0x000D: /* OR  */
					case 0x0010: /* CHRAND */
					case 0x0011: /* ?? - 21 */
					{
						if (pNode->subroutine_code == 0x000B)
							fprintf(outFile, "\tAND %d:", pNode->subParams[0].value);
						else if (pNode->subroutine_code == 0x000C)
							fprintf(outFile, "\tNOT %d:", pNode->subParams[0].value);
						else if (pNode->subroutine_code == 0x000D)
							fprintf(outFile, "\tOR %d:", pNode->subParams[0].value);
						else if (pNode->subroutine_code == 0x0010)
							fprintf(outFile, "\tCHRAND %d:", pNode->subParams[0].value);
						else
							fprintf(outFile, "\tUNKNOWN_%X %d:", pNode->subroutine_code, pNode->subParams[0].value);

						for (x = 1; x < (int)(pNode->num_parameters-1); x++){
							if (x >= 2)
								fprintf(outFile, ",");
							fprintf(outFile, "%d", pNode->subParams[x].value);
						}
						fprintf(outFile, "\r\n");
						break;
					}

					/* MONNOT */
					case 0x0016:
					{
						fprintf(outFile, "\tMONNOT %d:%d,%d\r\n",
							pNode->subParams[0].value, (pNode->subParams[1].value & 0xFF00) >> 8, pNode->subParams[1].value & 0xFF);
						break;
					}

					case 0x0042: /* JUMPPACK */
					{
						fprintf(outFile, "\tJUMPPACK %d:%d", (pNode->subParams[1].value>>8) & 0xFF,pNode->subParams[0].value);
						fprintf(outFile, "\r\n");
						break;
					}


					default:
						; /* Do Nothing */
				}
			}
			break;


			/****************/
			/* run-commands */
			/****************/
			case NODE_RUN_CMDS:
			{
				int ctrlMode = 0;
				runParamType* rpNode = pNode->runParams;

				if (pNode->pointerID != INVALID_PTR_ID)
					fprintf(outFile, "%u", pNode->pointerID);

				fprintf(outFile, "\t%u", pNode->id);

				fprintf(outFile, "\tTALK", pNode->id);

				while (rpNode != NULL) {

					switch (rpNode->type){

					case ALIGN_2_PARAM:
						break;
					case ALIGN_4_PARAM:
						break;
					case SHOW_PORTRAIT_LEFT:
					case SHOW_PORTRAIT_RIGHT:
						if (ctrlMode == 1){
							ctrlMode = 0;
							fprintf(outFile, "\r\n\t\t");
						}
						if (rpNode->type == SHOW_PORTRAIT_LEFT)
							fprintf(outFile, "\t>%d", rpNode->value);
						else 
							fprintf(outFile, "\t<%d", rpNode->value);
						ctrlMode = 1;
						break;
					case PRINT_LINE:
						if (ctrlMode == 1){
							ctrlMode = 0;
							fprintf(outFile, "\r\n\t\t");
						}

						fprintf(outFile, "\t%s\t", rpNode->str);
						ctrlMode = 1;
						break;
					case TIME_DELAY:
					case CTRL_CODE:
						if (ctrlMode == 0){
							ctrlMode = 1;
							fprintf(outFile, "\t\t");
						}
						else{
							fprintf(outFile, "(control-code %s%s) ", formatVal(rpNode->value), ctrlCodeLkup((unsigned short)rpNode->value));
						}
						break;
					default:
						printf("Error, bad run cmd parameter detected.\n");
						return -1;
					}
					rpNode = rpNode->pNext;
				}
				fprintf(outFile, "\r\n");
			}

			break;


			/***********/
			/* Options */
			/***********/
			case NODE_OPTIONS:
			{
				int ctrlMode = 0;
				runParamType* rpNode = NULL;

				if (pNode->pointerID != INVALID_PTR_ID)
					fprintf(outFile, "%u", pNode->pointerID);

				fprintf(outFile, "\t%u", pNode->id);

				/* Fixed at 2 options */
				for (x = 0; x < 2; x++){
					if (x == 0){
						rpNode = pNode->runParams;
						fprintf(outFile, "\tOpt1");
					}
					else{
						rpNode = pNode->runParams2;
						fprintf(outFile, "\r\n\t\tOpt2 : %d",pNode->subParams[0].value);
					}
					ctrlMode = 0;
					while (rpNode != NULL) {

						switch (rpNode->type){

						case ALIGN_2_PARAM:
							break;
						case ALIGN_4_PARAM:
							break;
						case PRINT_LINE:

							if (ctrlMode == 1){
								ctrlMode = 0;
								fprintf(outFile, "\r\n\t\t");
							}

							fprintf(outFile, "\t%s\t", rpNode->str);
							ctrlMode = 1;
							break;
						case CTRL_CODE:

							if (ctrlMode == 0){
								ctrlMode = 1;
								fprintf(outFile, "\t\t");
							}
							else{
								fprintf(outFile, "(control-code %s%s) ", formatVal(rpNode->value), ctrlCodeLkup((unsigned short)rpNode->value));
							}

							break;
						default:
							printf("Error, bad run cmd parameter detected.\n");
							return -1;
						}

						rpNode = rpNode->pNext;
					}
				}
				fprintf(outFile, "\r\n");
			}

			break;


		default:
		{
			printf("ERROR, unrecognized node.  HALTING output.\n");
			return -1;
		}
			break;
		}
		pNode = pNode->pNext;
	}

	return 0;
}


/*
FF00 <WaitForButton>
FF01 \n<AutoAdvance>\n
FF02 <Newline>\n
FF03 \n<NewTextBox>\n
FF04 \n<EndText>\n
*/
char* ctrlCodeLkup(unsigned short ctrlCode){

	static char ttxt[300];

	switch (ctrlCode){
		case 0xFF00:
			strcpy(ttxt," <WaitForButton>");
			break;
		case 0xFF01:
			strcpy(ttxt, " <ResetTextBox>");
			break;
		case 0xFF02:
			strcpy(ttxt, " <Newline>");
			break;
		case 0xFF03:
			strcpy(ttxt, " <CloseTextBox>");
			break;
		case 0xFFFF:
			strcpy(ttxt, " <EndText>");
			break;
		default:
			strcpy(ttxt, "\0");
			break;
	}

	/* Space Check */
	if ((ctrlCode & 0xFF00) == 0xF900){
		int spacelen;
		spacelen = ctrlCode & 0x00FF;
		sprintf(ttxt, " <Space-%d>", spacelen);
	}

	/* Delay Check */
	if ((ctrlCode & 0xFF00) == 0xF800){
		int delaylen;
		delaylen = ctrlCode & 0x00FF;
		sprintf(ttxt, " <Delay-%d>", delaylen);
	}

	return ttxt;
}



#if 0
/*****************************************************************************/
/* Function: dumpScriptText                                                  */
/* Purpose: Reads from a linked list data structure in memory to create a    */
/*          CSV tab delimited version of the script.                         */
/* Inputs:  Pointer to output file.                                          */
/* Outputs: 0 on Pass, -1 on Fail.                                           */
/*****************************************************************************/
int dumpScript(FILE* outFile){

	int x;
	scriptNode* pNode = NULL;

	/* Output Header */
	fprintf(outFile, "Node ID\tType\tJP Text\r\n");

	/* Get a Pointer to the Head of the linked list */
	pNode = getHeadPtr();

	/******************************************************************/
	/* Loop until the binary output corresponding to all list entries */
	/* has been output to memory.  Then write to disk.                */
	/******************************************************************/
	while (pNode != NULL){

		switch (pNode->nodeType){

			/********/
			/* goto */
			/********/
		case NODE_GOTO:
		{
			;
		}
			break;


			/********/
			/* fill */
			/********/
		case NODE_FILL_SPACE:
		{
			;
		}
			break;


			/***********/
			/* pointer */
			/***********/
		case NODE_POINTER:
		{
			;
		}
			break;


			/*****************************************************/
			/* execute-subroutine                                */
			/*****************************************************/
		case NODE_EXE_SUB:
		{
			;
		}
			break;


			/****************/
			/* run-commands */
			/****************/
		case NODE_RUN_CMDS:
		{
			int count = 0;
			runParamType* rpNode = pNode->runParams;

			fprintf(outFile, "%u", pNode->id);

			fprintf(outFile, "\tDialog", pNode->id);

			while (rpNode != NULL) {

				switch (rpNode->type){

				case ALIGN_2_PARAM:
					break;
				case ALIGN_4_PARAM:
					break;
				case SHOW_PORTRAIT_LEFT:
				case SHOW_PORTRAIT_RIGHT:
					if (count == 0){
						count++;
					}
					else{
						fprintf(outFile, "\t");
					}
					if (rpNode->type == SHOW_PORTRAIT_LEFT)
						fprintf(outFile, "\t (portrait-left %s)\r\n", formatVal(rpNode->value));
					else
						fprintf(outFile, "\t (portrait-right %s)\r\n", formatVal(rpNode->value));
					break;
				case TIME_DELAY:
					if (count == 0){
						count++;
					}
					else{
						fprintf(outFile, "\t");
					}
					fprintf(outFile, "\t (delay-time %s)\r\n", formatVal(rpNode->value));
					break;
				case PRINT_LINE:
					if (count == 0){
						count++;
					}
					else{
						fprintf(outFile, "\t");
					}
					fprintf(outFile, "\t%s\r\n", rpNode->str);
					break;
				case CTRL_CODE:
					if (count == 0){
						count++;
					}
					else{
						fprintf(outFile, "\t");
					}

					fprintf(outFile, "\t (control-code %s%s)\r\n", formatVal(rpNode->value), ctrlCodeLkup((unsigned short)rpNode->value));
					break;
				default:
					printf("Error, bad run cmd parameter detected.\n");
					return -1;
				}

				rpNode = rpNode->pNext;
			}
		}
			break;


			/***********/
			/* Options */
			/***********/
		case NODE_OPTIONS:
		{
			int count = 0;
			runParamType* rpNode = NULL;

			fprintf(outFile, "%u", pNode->id);


			/* Fixed at 2 options */
			for (x = 0; x < 2; x++){
				if (x == 0){
					rpNode = pNode->runParams;
					fprintf(outFile, "\tOpt1");
				}
				else{
					rpNode = pNode->runParams2;
					fprintf(outFile, "\tOpt2");
				}
				count = 0;
				while (rpNode != NULL) {

					switch (rpNode->type){

					case ALIGN_2_PARAM:
						break;
					case ALIGN_4_PARAM:
						break;
					case PRINT_LINE:
						if (count == 0){
							count++;
						}
						else{
							fprintf(outFile, "\t");
						}

						fprintf(outFile, "\t%s\r\n", rpNode->str);
						break;
					case CTRL_CODE:
						if (count == 0){
							count++;
						}
						else{
							fprintf(outFile, "\t");
						}

						fprintf(outFile, "\t (control-code %s%s)\r\n", formatVal(rpNode->value), ctrlCodeLkup((unsigned short)rpNode->value));
						break;
					default:
						printf("Error, bad run cmd parameter detected.\n");
						return -1;
					}

					rpNode = rpNode->pNext;
				}
			}
		}
			break;


		default:
		{
			printf("ERROR, unrecognized node.  HALTING output.\n");
			return -1;
		}
			break;
		}
		pNode = pNode->pNext;
	}

	return 0;
}
#endif