#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include "packet_interface.h"

typedef struct node{
	pkt_t* pkt;
	int indice;
	struct node* next;
}node;

node** head;

int createList()
{
	head = (node**) malloc(sizeof(node*));
	if(head==NULL)
	{
		return -1;
	}
	return 0;
}

node* nouveauNode(pkt_t pkt, int indice)
{
	node* newNode = (node*) malloc(sizeof(node));
	if(newNode==NULL)
	{
		return NULL;
	}
	newNode->pkt = pkt;
	newNode->indice = indice;
	return newNode;
}

int insert(pkt_t newpkt, int newIndice)
{
	
	if(head==NULL)
	{
		return -1; //non-initialise
	}
	
	node* newNode = nouveauNode(newpkt, newIndice);
	//node* newNode = nouveauNode(newIndice);
	if(newNode==NULL)
	{
		return -1;
	}

	if(*head == NULL) //liste vide
	{
		*head = newNode;
		newNode->next = NULL;
		return 0;
	}
	
	if((*head)->next == NULL) //un seul element
	{
		if((*head)->indice < newNode->indice)
		{
			(*head)->next = newNode;
			newNode->next = NULL;
			return 0;
		}
		else
		{
			newNode->next = *head;
			*head = newNode;
			return 0;
		}
	}

	//plus de un element 

	if(newNode->indice < (*head)->indice)
	{
		newNode->next = *head;
		*head = newNode;
		return 0;
	}

	node* previous = *head; 
	node* ptr = (*head)->next;

	while(ptr->indice < newNode->indice)
	{
		ptr=ptr->next;
		previous= previous->next;
	}

	newNode->next = ptr;
	previous->next = newNode;
	
	return 0;
}


int main()
{
	int a = createList(head);
	if(a==-1)
	{
		printf("createlist\n");
	}

	
	insert(2);
	insert(5);
	insert(0);
	insert(3);
	insert(1);
	insert(1);
	
	node* ptr = *head;
	while(ptr!=NULL)
	{
		printf("%d\n", ptr->indice);
		ptr=ptr->next;
	}

}
