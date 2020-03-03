#ifndef PARSE_BINARY_H
#define PARSE_BINARY_H

#include <stdio.h>
#include "script_node_types.h"

int decodeBinaryScript(FILE* inFile, FILE* outFile);
runParamType* getRunParam(int textMode, char* pdata);

#endif