#ifndef __UTILS_H_
#define __UTILS_H_
#include "packet_interface.h"

void error(int err, char *msg);

int create_file(char* format, int* iout, FILE** fp);

#endif