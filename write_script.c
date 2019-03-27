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
				switch (pNode->subroutine_code){

					/* RETURNS */
					case 0x0005:
					case 0x0026:
					{
						if (pNode->pointerID != INVALID_PTR_ID)
							fprintf(outFile, "%u", pNode->pointerID);

						fprintf(outFile, "\t%u", pNode->id);

						if (pNode->subroutine_code == 0x0005)
							fprintf(outFile, "\tRETURN\r\n");
						else{
							int x;
							fprintf(outFile, "\tEXIT ");

							for (x = 0; x < (int)(pNode->num_parameters); x++){
								if (x >= 1)
									fprintf(outFile, ",");
								fprintf(outFile, "%d", pNode->subParams[x].value);
							}
							fprintf(outFile, "\r\n");
						}
						break;
					}

					/* UNCONDITIONAL JUMP */
					case 0x0003:
					case 0x0004:
					{
						if (pNode->pointerID != INVALID_PTR_ID)
							fprintf(outFile, "%u", pNode->pointerID);

						fprintf(outFile, "\t%u", pNode->id);

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
						if (pNode->pointerID != INVALID_PTR_ID)
							fprintf(outFile, "%u", pNode->pointerID);

						fprintf(outFile, "\t%u", pNode->id);

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
					case 0x0010: /* ?? */
					case 0x0011: /* ?? */
					case 0x0016: /* ?? */
					{
						int x;

						if (pNode->pointerID != INVALID_PTR_ID)
							fprintf(outFile, "%u", pNode->pointerID);

						fprintf(outFile, "\t%u", pNode->id);

						if (pNode->subroutine_code == 0x000B)
							fprintf(outFile, "\tAND %d:", pNode->subParams[0].value);
						else if (pNode->subroutine_code == 0x000C)
							fprintf(outFile, "\tNOT %d:", pNode->subParams[0].value);
						else if (pNode->subroutine_code == 0x000D)
							fprintf(outFile, "\tOR %d:", pNode->subParams[0].value);
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

					case 0x0042: /* JUMPPACK */
					{
						if (pNode->pointerID != INVALID_PTR_ID)
							fprintf(outFile, "%u", pNode->pointerID);

						fprintf(outFile, "\t%u", pNode->id);

						fprintf(outFile, "\tJUMPPACK %d:%d", (pNode->subParams[1].value>>8) & 0xFF,pNode->subParams[0].value);
						fprintf(outFile, "\r\n");
						break;
					}

					case 0x001D:
					{
						if (pNode->pointerID != INVALID_PTR_ID)
							fprintf(outFile, "%u", pNode->pointerID);

						fprintf(outFile, "\t%u", pNode->id);
						fprintf(outFile, "\tTAKEMONEY ");
						fprintf(outFile, "%d\r\n", pNode->subParams[1].value);
						break;
					}

					case 0x001E:
					{
						if (pNode->pointerID != INVALID_PTR_ID)
							fprintf(outFile, "%u", pNode->pointerID);

						fprintf(outFile, "\t%u", pNode->id);
						fprintf(outFile, "\tCHKMONEY ");
						fprintf(outFile, "%d:%d\r\n", pNode->subParams[0].value, pNode->subParams[1].value);
						break;
					}

					case 0x0021:
					{
						if (pNode->pointerID != INVALID_PTR_ID)
							fprintf(outFile, "%u", pNode->pointerID);

						fprintf(outFile, "\t%u", pNode->id);
						fprintf(outFile, "\tCHKITEM ");
						fprintf(outFile, "%d:%d,%d\r\n", pNode->subParams[0].value, (pNode->subParams[1].value&0xFF00)>>8, pNode->subParams[1].value&0xFF);
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