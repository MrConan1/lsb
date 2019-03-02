/**********************************************************************/
/* hit_map.c - Functions to keep track of read file locations.        */
/**********************************************************************/
#pragma warning(disable:4996)

/************/
/* Includes */
/************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "script_node_types.h"
#include "snode_list.h"


/***********************/
/* Function Prototypes */
/***********************/
int initNodeList();
int destroyNodeList();
scriptNode* getHeadPtr();
int createScriptNode(scriptNode** node);
int addNode(scriptNode* node, int method, int target_id);
int removeNode(int id);
int overwriteNode(int id, scriptNode* node);
scriptNode* getListItemByID(unsigned int id);


/* Globals */
static scriptNode *pHead = NULL;    /* List Ptr  */


/*******************************************************************/
/* initNodeList                                                    */
/* Creates an empty list to hold the script node information.      */
/*******************************************************************/
int initNodeList()
{
    if(pHead != NULL)
		destroyNodeList();
    pHead = NULL;

    return 0;
}




/*******************************************************************/
/* getHeadPtr - Returns the list's head pointer.                   */
/*******************************************************************/
scriptNode* getHeadPtr(){
	return pHead;
}




/*******************************************************************/
/* destroyNodeList                                                 */
/* Destroys all items in the list.                                 */
/*******************************************************************/
int destroyNodeList()
{
    scriptNode *pCurrent = NULL;
	scriptNode *pItem = pHead;

	while (pItem != NULL){
		pCurrent = pItem;
		pItem = pItem->pNext;
        free(pCurrent);
    }

    return 0;
}


/*******************************************************************/
/* createScriptNode                                                */
/* Creates and initializes a new script node.                      */
/* Returns 0 on success, -1 on failure.                            */
/*******************************************************************/
int createScriptNode(scriptNode** node){

    scriptNode* pNode;
    *node = (scriptNode*)malloc(sizeof(scriptNode));
    pNode = *node;
    if(pNode == NULL){
        printf("Error creating new script node\n");
        return -1;
    }
    memset(pNode,0,sizeof(scriptNode));
	pNode->pNext = pNode->pPrev = NULL;
	pNode->subParams = NULL;
	pNode->runParams = NULL;
    return 0;
}


/*******************************************************************/
/* addNode                                                         */
/* Inserts an element in the list.                                 */
/* Returns 0 on success, -1 on failure.                            */
/*******************************************************************/
int addNode(scriptNode* node, int method, int target_id){

	scriptNode * newItem, *pCurrent, *pNext, *pPrev;

    /* Create a new item */
	newItem = (scriptNode*)malloc(sizeof(scriptNode));
    if(newItem == NULL){
        printf("Error allocing memory for new item in addNode.\n");
        return -1;
    }
	memcpy(newItem, node, sizeof(scriptNode));
    newItem->pNext = NULL;

    /* Head is Empty */
    if(pHead == NULL){
        pHead = newItem;
        return 0;
    }

    /* Head != NULL, Insert at Head ? */
	if ((method == METHOD_INSERT_BEFORE) && (pHead->id == target_id)){
		newItem->pNext = pHead;
		pHead = newItem;
		return 0;
	}

    /* First item is not empty and insertion will not take place at head */
	pPrev = NULL;
	pCurrent = pHead;
    while(pCurrent != NULL){
        pNext = pCurrent->pNext;

        /* Check to insert */
		if ((method == METHOD_INSERT_BEFORE) && (pCurrent->id == target_id)){
			newItem->pNext = pCurrent;
			if (pPrev != NULL)
   			    pPrev->pNext = newItem;
			return 0;
		}
		else if ((method == METHOD_INSERT_AFTER) && (pCurrent->id == target_id)){
			newItem->pNext = pCurrent->pNext;
			pCurrent->pNext = newItem;
			return 0;
		}
		else if ((method == METHOD_NORMAL) && (pNext == NULL)){
			pCurrent->pNext = newItem;
			return 0;
		}

		pPrev = pCurrent;
        pCurrent = pCurrent->pNext;
    }

	printf("Error, insertion failed.\n");
    return -1;
}


/*******************************************************************/
/* removeNode                                                      */
/* Removes an element in the list.                                 */
/* Returns 0 on success, -1 on failure.                            */
/*******************************************************************/
int removeNode(int id){

	scriptNode *pCurrent, *pNext, *pPrev;

	pPrev = NULL;
	pCurrent = pHead;
	while (pCurrent != NULL){
		pNext = pCurrent->pNext;

		/* Check to remove */
		if (pCurrent->id == id){
			if (pPrev != NULL)
				pPrev->pNext = pNext;
			if (pHead == pCurrent){ //Update Head
				pHead = pCurrent->pNext;
			}
			free(pCurrent);
			return 0;
		}

		pPrev = pCurrent;
		pCurrent = pCurrent->pNext;
	}

	printf("Error, node removal failed.\n");
	return -1;
}


/*******************************************************************/
/* overwriteNode                                                   */
/* Overwrites an element in the list.                              */
/* Returns 0 on success, -1 on failure.                            */
/*******************************************************************/
int overwriteNode(int id, scriptNode* node){

	scriptNode *pCurrent;

	pCurrent = pHead;
	while (pCurrent != NULL){

		/* Check to overwrite data */
		if (pCurrent->id == id){
			node->pNext = pCurrent->pNext;
			memcpy(pCurrent, node, sizeof(scriptNode));
			return 0;
		}

		pCurrent = pCurrent->pNext;
	}

	printf("Error, node overwrite failed.\n");
	return -1;
}




/*******************************************************************/
/* getListItemByID                                                 */
/* Returns a pointer to the scriptNode with the given ID.          */
/* NULL is returned on failure.                                    */
/*******************************************************************/
scriptNode* getListItemByID(unsigned int id){

	scriptNode *pCurrent = pHead;
	while (pCurrent != NULL){
		/* Check for the id */
		if (pCurrent->id == id){
			return pCurrent;
		}
		pCurrent = pCurrent->pNext;
	}

	printf("Error, node not found.\n");
	return NULL;
}
