#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include "packet_interface.h"
#include "ConnexionList.h"


/**
* Retourne - un pointeur vers la structure connexion si @client_addr est deja dans la liste chainee
	   - NULL sinon
*/
connexion* contains(struct sockaddr_in6 client_addr);


/**
* Retourne 1 si l'ajout dans la liste de connexion s'est effectuée avec succès
	   -1 sinon
*/
int add(struct sockaddr_in6 client_addr);

/*
	char buffer[strlen(formatSortie)];
	int cx = snprintf(buffer,strlen(formatSortie),formatSortie,i);
	if(cx <0)
	{
		fprintf(stderr," snprintf() a plante \n");
		return EXIT_FAILURE;
	}


	FILE* f = fopen(buffer,"w+"); //read,Write et create
	if(f == NULL)
	{
		fprintf(stderr, "Erreur dans l'ouverture du fichier \n");
		return EXIT_FAILURE;
	}
	int fichier = fileno(f);
	//int fichier = fopen(formatSortie, O_CREAT | O_WRONLY | O_APPEND, S_IRWXU); //si le fichier n'existe pas, il le crée
	if(fichier == -1) // cas où @fopen() a planté
	{
		fprintf(stderr, "Erreur dans fileno() \n");
		return EXIT_FAILURE;
	}
	tabConnexion[i].fd_to_write = fichier;
*/

