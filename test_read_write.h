#ifndef __TEST_READ_WRITE_H_
#define __TEST_READ_WRITE_H_

#include "packet_interface.h"
#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>  		// pour utiliser read(), close()
#include <sys/types.h> 		// pour utiliser open()
#include <sys/stat.h> 		// pour utiliser open()
#include <fcntl.h>   		// pour utiliser open()
#include "LinkedList.h"

node_t** head;

extern node_t** head;

int isInWindow(int seqnum, int windowMin, int windowMax);

void augmenteBorne(int* borne);

void test(pkt_t* pkt_recu);

#endif
