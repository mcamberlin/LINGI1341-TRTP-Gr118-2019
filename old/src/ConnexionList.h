#ifndef __CONNEXIONLIST_H_
#define __CONNEXIONLIST_H_


#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include "packet_interface.h"
#include "read_write_loop.h"


typedef struct c_node c_node_t;

typedef struct connexion connexion_t;


c_node_t** createConnexionList();

c_node_t* newConnexionNode(struct sockaddr_in6 client_addr, int numeroFichier, char* formatSortie);


/**
* Retourne - un pointeur vers la structure connexion si @client_addr est deja dans la liste chainee
	   - NULL sinon
*/
connexion_t* contains(c_node_t** head, struct sockaddr_in6 client_addr);


/**
* Retourne 1 si l'ajout dans la liste de connexion s'est effectuée avec succès
	   -1 sinon
*/
connexion_t* add(c_node_t** head, struct sockaddr_in6 client_addr, int numeroFichier, char* formatSortie);

int removeConnexion(c_node_t** head, connexion_t* connexionToRemove);

int freeConnexionList(c_node_t** head);


#endif
