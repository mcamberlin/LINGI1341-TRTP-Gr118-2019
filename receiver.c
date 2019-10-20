#include <stdlib.h> /* EXIT_X */
#include <stdio.h> /* fprintf */
#include <unistd.h> /* getopt */

#include "packet_interface.h"
#include "socket.h"
#include "read_write_loop.h"

/*--------------------------MAIN------------------------------------*/



int nbreConnexion = 1;
char* formatSortie;
int port = 0;
char* hostname = NULL;

/** La fonction main est la fonction principale de notre programme:
	@pre - argc = nombre d'arguments passés lors de l'appel de l'exécutable
		- argv = tableau de pointeurs reprenant les arguments donnés par l'exécutable
	@post - exécute les différentes instructions du code
		- retourn EXIT_FAILURE si une erreur s'est produite et affiche sur stderr un détail de l'erreur
*/
int main(int argc, char *argv[]) 
{
	printf("\n \t\t\t Interprétation des commandes \n");
	int opt;
	int index = 1; 
	if(argc <2)
	{	
		fprintf(stderr, "Usage:\n -n      Nombre N de connexions que le recever doit pouvoir traiter de facon concurrente \n -o      type de format pour le formattage des fichiers de sortie\n hostname \n portnumber\n");

		return EXIT_FAILURE;	
	}
	fprintf(stderr,"\t\t\t Fin de l'interprétation des commandes \n\n");


	/* Resolve the hostname */
	struct sockaddr_in6 addr;
	const char *err = real_address(hostname, &addr);
	if (err) {
		fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
		return EXIT_FAILURE;
	}


/*
	 Get a socket 
	int sfd;
	if () {
		sfd = create_socket(NULL, -1, &addr, port);  Connected 
	} 
	else {
		sfd = create_socket(&addr, port, NULL, -1); Bound 
		if (sfd > 0 && wait_for_client(sfd) < 0) {  Connected 
			fprintf(stderr,
					"Could not connect the socket after the first message.\n");
			close(sfd);
			return EXIT_FAILURE;
		}
	}
	if (sfd < 0) {
		fprintf(stderr, "Failed to create the socket!\n");
		return EXIT_FAILURE;
	}
	 Process I/O 
	read_write_loop(sfd);


	close(sfd);
*/

	return EXIT_SUCCESS;
}
