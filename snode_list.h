/**********************************************************************/
/* snode_list.h - Linked list to store script meta data information.  */
/**********************************************************************/
#pragma warning(disable:4996)
#ifndef SNODE_LIST_H
#define SNODE_LIST_H
#include "script_node_types.h"

/* Defines */
#define METHOD_NORMAL        0 //Add at tail
#define METHOD_INSERT_BEFORE 1 //Add prior to a specified id
#define METHOD_INSERT_AFTER  2 //Add after a specified id

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



#endif
