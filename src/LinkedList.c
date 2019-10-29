#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include "packet_interface.h"
#include "LinkedList.h"

typedef struct node{
	pkt_t* pkt;
	int indice;
	struct node* next;
}node;


node** createList()
{
	node** head = (node**) malloc(sizeof(node*));
	if(head==NULL)
	{
		return NULL;
	}
	*head = NULL;
	return head;
}

node* nouveauNode(pkt_t* pkt, int indice)
{
	node* newNode = (node*) malloc(sizeof(node));
	if(newNode==NULL)
	{
		return NULL;
	}
	newNode->pkt = pkt;
	newNode->indice = indice;
	newNode->next=NULL;
	return newNode;
}

int insert(node** head, pkt_t* newpkt, int newIndice)
{
	fprintf(stderr, "DEBUT INSERT (linkedlist)\n");
	if(head==NULL)
	{
		return -1; //non-initialise
	}
	
	node* newNode = nouveauNode(newpkt, newIndice);
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

	while(ptr->next!=NULL && ptr->indice < newNode->indice)
	{
		ptr=ptr->next;
		previous= previous->next;
	}

	if(ptr->next!=NULL)
	{
		ptr->next = newNode;
		newNode->next = NULL;
		return 0;
	}

	newNode->next = ptr;
	previous->next = newNode;
	
	return 0;
}


pkt_t* isInList(node_t** head, int indice)
{
	if(head==NULL)
	{
		return NULL;
	}
	

	if((*head)==NULL)//liste est vide
	{
		fprintf(stderr,"liste vide \n");
		return NULL;
	}

	//printList(head);
	fprintf(stderr, "(*head)->indice = %d\n", (*head)->indice);
	if((*head)->indice == indice)
	{
		pkt_t* tmp = (*head)->pkt;
		node* repere = *head;
		*head=(*head)->next;
		free(repere);
		return tmp;
	}
	
	node* previous = *head;
	node* runner = (*head)->next;
	while(runner!=NULL)
	{
		if(runner->indice == indice)
		{
			previous->next = runner->next;
			pkt_t* tmp = runner->pkt;
			free(runner);
			return tmp;
		}
		runner=runner->next;
		previous=previous->next;
	}
	return NULL;
	
}


void printList(node** head)
{
	if(head==NULL)
	{
		fprintf(stderr,"head==NULL\n");
		return;
	}
	printf("debut printList\n");
	node* ptr = *head;
	while(ptr!=NULL)
	{
		fprintf(stderr,"indice = %d - %s\n", ptr->indice, pkt_get_payload(ptr->pkt));
		ptr=ptr->next;
	}

}

int freeLinkedList(node_t** head)
{

	if(head == NULL)
	{
		fprintf(stderr, "**head non spécifié dans freeLinkedList() \n");
		return -1;
	}
	if(*head == NULL) // cas où la liste est vide
	{
		return 0;
	}
	else // cas où la liste n'est pas vide
	{
		node* runner = *head; // noeud courant
		node* previous=runner;	// noeud précédent
		*head=NULL;
		while(runner !=NULL)
		{
			previous = runner;
			runner = runner->next;
			free(previous);			
		}
		return 0;
	}	
}
