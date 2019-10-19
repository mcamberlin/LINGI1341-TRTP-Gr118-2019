#ifndef __LINKEDLIST_H_
#define __LINKEDLIST_H_


#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include "packet_interface.h"

typedef struct node node_t;

int createList();

node_t newNode(int indice);

int insert(pkt_t* newpkt, int newIndice);


#endif
