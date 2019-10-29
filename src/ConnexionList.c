#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include "packet_interface.h"
#include "ConnexionList.h"
#include "read_write_loop.h"

typedef struct c_node{
	connexion_t* c_connexion;
	struct c_node* next;
}c_node;


typedef struct connexion
{
	struct sockaddr_in6 client_addr;
	int fd_to_write; // File descriptor pour ecrire dans le fichier correspondant
	int closed; // Par defaut les connexions ne sont pas terminee =0
	uint8_t tailleWindow; // taille par défaut de la fenetre du sender[i]
	int windowMin ;
	int windowMax;
	node_t** head; //tete de la liste chainee associee à la connexion[i]
	
}connexion;



c_node** createConnexionList()
{
	c_node** head =(c_node**) malloc(sizeof(c_node*));
	if(head==NULL)
	{
		return NULL;
	}
	return head;
}

c_node* newConnexionNode(struct sockaddr_in6 client_addr, int numeroFichier, char* formatSortie)
{
	c_node* newNode = (c_node*) malloc(sizeof(c_node));
	if(newNode==NULL)
	{
		return NULL;
	}

	newNode->c_connexion = (connexion*) malloc(sizeof(struct connexion));
	if(newNode->c_connexion == NULL)
	{
		return NULL;
	}

	newNode->c_connexion->client_addr = client_addr;
	newNode->c_connexion->closed = 0;
	newNode->c_connexion->tailleWindow = 1;
	newNode->c_connexion->windowMin = 0;
	newNode->c_connexion->windowMax = 0;
	newNode->c_connexion->head = createList(); //attention ceci crée une liste pour le buffer de cette connexion spécifique 
	
	char buffer[strlen(formatSortie)];
	int cx = snprintf(buffer,strlen(formatSortie), formatSortie,numeroFichier);
	if(cx <0)
	{
		fprintf(stderr," snprintf() a plante \n");
		return NULL;
	}


	FILE* f = fopen(buffer,"w+"); //read,Write et create
	if(f == NULL)
	{
		fprintf(stderr, "Erreur dans l'ouverture du fichier \n");
		return NULL;
	}
	int fichier = fileno(f);
	
	if(fichier == -1) // cas où @fileno() a planté
	{
		fprintf(stderr, "Erreur dans fileno() \n");
		return NULL;
	}
	newNode->c_connexion->fd_to_write = fichier;


	return newNode;
}


/**
* Retourne - un pointeur vers la structure connexion si @client_addr est deja dans la liste chainee
	   - NULL sinon
*/
connexion* contains(c_node** head, struct sockaddr_in6 client_addr)
{
	if(head==NULL)
	{
		return NULL;
	}
	

	if((*head)==NULL)//liste est vide
	{
		return NULL;
	}

	c_node* runner = *head;
	while(runner != NULL)
	{
		if(memcmp( (const void*) &(runner->c_connexion->client_addr), (const void *) &client_addr, sizeof(struct sockaddr_in6)) == 0)//si le client_addr du runner == client_addr en parametre
		{
			return runner->c_connexion;
		}
		runner=runner->next;
	}

	return NULL;
}


/**
* Ajoute un nouveau noeud contenant une connexion ayant un client_addr = client_addr
* Retourne 1 si l'ajout dans la liste de connexion s'est effectuée avec succès
	   -1 sinon
*/
connexion* add(c_node** head, struct sockaddr_in6 client_addr,int numeroFichier, char* formatSortie)
{
	c_node* nouveauNode = newConnexionNode(client_addr,numeroFichier,  formatSortie);
	if(nouveauNode == NULL)
	{
		return NULL;
	}
	
	nouveauNode->next = *head;
	*head = nouveauNode;
	return nouveauNode->c_connexion;
}


/**
* Retire le noeud ayant la connexion @connexionToRemove de la liste chainée 
* return 0 si il est retiré sans problème
*        -1 sinon
*/
int removeConnexion(c_node** head, connexion* connexionToRemove)
{
	if(head==NULL)
	{
		return -1;
	}
	
	if(*head == NULL)//liste vide
	{
		return -1;
	}

	if((*head)->next == NULL)//un seul element dans la liste 
	{
		if(memcmp((const void*) (*head)->c_connexion, (const void*) connexionToRemove, sizeof(struct connexion))==0)
		{
			close((*head)->c_connexion->fd_to_write);
			freeLinkedList((*head)->c_connexion->head);

			free((*head)->c_connexion);
			free(*head);
			*head=NULL;
			fprintf(stderr, "connexion removed successfully\n");
			return 0;
		}
		return -1; // Le seul element qui est actuellement dans la liste n'est pas celui qu'il faut retirer
	}

	c_node* previous = *head;
	c_node* runner = (*head)->next;

	if(memcmp((const void*) previous->c_connexion, (const void*) connexionToRemove, sizeof(struct connexion))==0)//retirer le premier 
	{
		close(previous->c_connexion->fd_to_write);
		freeLinkedList(previous->c_connexion->head);
		
		free(previous->c_connexion);
		free(previous);
		*head = runner;
		fprintf(stderr, "connexion removed successfully\n");
		return 0;
	}
	
	while(runner != NULL)
	{
		if(memcmp((const void*) runner->c_connexion, (const void*) connexionToRemove, sizeof(struct connexion))==0)
		{
			close(runner->c_connexion->fd_to_write);
			freeLinkedList(runner->c_connexion->head);
			
			previous->next = runner->next;
			
			free(runner->c_connexion);
			free(runner);
			fprintf(stderr, "connexion removed successfully\n");
			return 0;
		}
		previous = previous->next;
		runner = runner->next;
	}		
	return 0;
}

