#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include "test.h"

typedef struct C_node{
	int c_connexion;
	struct C_node* next;
}C_node;


C_node** create()
{
	C_node** head = (C_node**) malloc(sizeof(struct C_node*));
	if(head==NULL)
	{
		return NULL;
	}
	return head;
}


