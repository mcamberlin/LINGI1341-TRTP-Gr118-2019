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

typedef struct connexion
{
	int sfd; // File descriptor du socket ouvert pour la connexion
	int fd_to_write; // File descriptor pour ecrire dans le fichier correspondant
	int closed = 0; // Par defaut les connexions ne sont pas terminee

	uint8_t tailleWindow = 1; // taille par défaut de la fenetre du sender[i]
	int windowMin =0;
	int windowMax = 0;
	
	node_t** head; //tete de la liste chainee associee à la connexion[i]
	
}connexion;

node_t** head;

extern node_t** head;

int isInWindow(int seqnum, int windowMin, int windowMax);

void augmenteBorne(int* borne);

void test(pkt_t* pkt_recu);

#endif
