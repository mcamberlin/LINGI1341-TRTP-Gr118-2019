#ifndef __LINKEDLIST_H_
#define __LINKEDLIST_H_


#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include "packet_interface.h"

typedef struct node node_t;

node_t** createList();

node_t* newNode(int indice);

int insert(node_t** head, pkt_t* newpkt, int newIndice);


/**
* return le pkt contenu dans le noeud et retire le noeud de la chaine
*/
pkt_t* isInList(node_t** head, int indice);

void printList(node_t** head);

int freeLinkedList(node_t** head);


#endif
