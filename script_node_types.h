#ifndef SCRIPT_NODE_TYPES
#define SCRIPT_NODE_TYPES

/***********/
/* Defines */
/***********/

/* Binary Output Mode */
#define BIG_ENDIAN    0
#define LITTLE_ENDIAN 1

/* Meta Script Read Mode for data values */
#define RADIX_DEC     0
#define RADIX_HEX     1

/* Table output mode */
#define ONE_BYTE_ENC 1
#define TWO_BYTE_ENC 2
#define UTF8_ENC     3

/* Default Fill Byte Value */
#define DEFAULT_FILL  0x00

/* Meta Script Node Types */
#define NODE_GOTO       1
#define NODE_FILL_SPACE 2
#define NODE_POINTER    3
#define NODE_EXE_SUB    4
#define NODE_RUN_CMDS   5
#define NODE_OPTIONS    6

/* Execute Subroutine / Run Cmds Parameter Types */
#define BYTE_PARAM    0
#define SHORT_PARAM   1
#define LONG_PARAM    2
#define ALIGN_2_PARAM 3
#define ALIGN_4_PARAM 4
#define SHOW_PORTRAIT_LEFT 5
#define PRINT_LINE    6
#define CTRL_CODE     7
#define SHOW_PORTRAIT_RIGHT 8
#define TIME_DELAY    9

/* Struct Forward Declarations */
typedef struct scriptNode scriptNode;
typedef struct paramType paramType;
typedef struct runParamType runParamType;

/* Parameter Types */
struct paramType{
	unsigned int type;
	unsigned int value;
};

struct runParamType{
	unsigned int type;
	unsigned int value;
	char* str;
	runParamType* pNext;
};


/* Script Node Datatype */
struct scriptNode
{
    unsigned int id;
    int nodeType;

	//Pointer Values 
	unsigned short byteOffset; // (Goto shares byteOffset)
	unsigned int ptrSize;      // 2 or 4 bytes
	unsigned int ptrValueFlag; // When true, use ptrValue, otherwise ID
	unsigned int ptrValue;  
	unsigned int ptrID;

	//fill-space Parameters
	unsigned int unit_size;
	unsigned int fillVal;
	unsigned int unit_count;

	//Execute-Subroutine Parameters
	unsigned int subroutine_code;
	unsigned int num_parameters;
	unsigned char alignfillVal;
	paramType* subParams;

	//Run-Commands Parameters
	runParamType* runParams;
	runParamType* runParams2;

	//Location when written back out to a binary file
	unsigned int fileOffset;
	
	//Book-keeping for script dumps only
	unsigned int origFileOffset;  // Original file offset of data
	unsigned int numChildNodes;   // 0, 1, or 2
	unsigned int pointerID;       // Pointer ID that brought the script to this node. (FFFFFFFF if unreachable)
	unsigned int nextPointerID;   // Node ID of next non-linear script element (if it exists, FFFFFFFF otherwise)

	//Linked List Pointers
    scriptNode* pNext;
    scriptNode* pPrev;
};



#endif
