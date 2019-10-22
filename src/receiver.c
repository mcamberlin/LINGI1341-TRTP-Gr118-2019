#include <stdlib.h> // pour EXIT_X
#include <stdio.h>  // fprintf
#include <unistd.h> // getopt

#include "packet_interface.h"
#include "socket.h"
#include "read_write_loop.h"
#include "LinkedList.h"



/*--------------------------MAIN------------------------------------*/


int nbreConnexion = 100; //Valeur par défaut du nombre de connexions
char* formatSortie = "fichier_%02d.txt"; //format par défaut choisi par nous
int port = 0;
char* hostname = NULL;

/** La fonction main est la fonction principale de notre programme:
	@pre - argc = nombre d'arguments passés lors de l'appel de l'exécutable
		- argv = tableau de pointeurs reprenant les arguments donnés par l'exécutable
	@post - exécute les différentes instructions du code
		- retourn EXIT_FAILURE si une erreur s'est produite et affiche sur stderr un détail de l'erreur
*/
int main(int argc, char* argv[]) 
{
	fprintf(stderr,"\n \t\t\t Interprétation des commandes \n");
	int opt;
	int index = 1; 
	int isNumber;

	if(argc <2)
	{	
		fprintf(stderr, "Usage:\n -n Nombre N de connexions que le recever doit pouvoir traiter de facon concurrente\n -o type de format pour le formattage des fichiers de sortie\n hostname \n portnumber (server)\n");
		return EXIT_FAILURE;	
	}
	
	
	while (index<argc) // Tant que toutes les options n'ont pas été vérifées
	{
		opt = getopt(argc, argv, "o:m:");
		switch (opt) 
		{
			case 'o': // format des fichiers de sortie spécifié
				formatSortie = optarg;
				fprintf(stderr,"-o spécifié : %s\n",formatSortie);
				index+=2;
				break;
			case 'm':
				nbreConnexion = atoi(optarg);
				fprintf(stderr,"-m spécifié : %d\n",nbreConnexion);
				index+=2;
				break;
	
			default: 
				isNumber = 1;
				for(int i = 0; *(argv[index] + i) !='\0';i++)
				{
					if(*(argv[index] + i) == ':')
					{
						isNumber = 0;
					}
				}
				
				if(isNumber == 1 && !( atoi( argv[index]) == 0) ) 
				{
					port = atoi(argv[index]);
					fprintf(stderr,"port : %d\n",port);
					index+=1;
					break;
				}
				else
				{
					hostname = (char*) malloc(sizeof(argv[index]));
					if(hostname == NULL)
					{
						fprintf(stderr,"Erreur allocation de mémoire pour @hostname dans interprétation des commandes");
						return EXIT_FAILURE;
					}

					hostname = argv[index];
					fprintf(stderr,"hostname : %s \n",hostname);
					index +=1;
					break;
				}				
		}
	}
	fprintf(stderr,"\t\t\t Fin de l'interprétation des commandes \n\n");



	// Resolve the hostname 
	struct sockaddr_in6 addr[nbreConnexion];
	
	// Creation d'un tableau de @nbreConnexion de structure @connexion
	connexion tabConnexion[nbreConnexion];
	
	
	// Get a socket
	for(int i=0; i<nbreConnexion;i++) //creation d'un socket par connexion
	{		
		const char *err = real_address(hostname, &(addr[i]));
		if (err) {
			fprintf(stderr, "Could not resolve hostname %s: %s\n", hostname, err);
			return EXIT_FAILURE;
		}


		int sfd = create_socket(&(addr[i]), port,NULL,-1); // Bound
		//int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port)
		tabConnexion[i].sfd = sfd;
		tabConnexion[i].closed=0; // Par defaut les connexions ne sont pas terminee =0

		tabConnexion[i].tailleWindow = 1; // taille par défaut de la fenetre du sender[i]
		tabConnexion[i].windowMin =0;
		tabConnexion[i].windowMax = 0;
		tabConnexion[i].head = createList();

		fprintf(stderr, " En attente de la connexion du client %d ...\n", i+1);
		int w = wait_for_client(sfd);
		if (sfd > 0 && w < 0) 
		{ 
			fprintf(stderr,	"Could not connect the %dth socket after the first message.\n", i);
			tabConnexion[i].closed = 1;
			return EXIT_FAILURE;
		}

		if (sfd < 0) {
			fprintf(stderr, "Failed to create the socket!\n");
			tabConnexion[i].closed = 1;
			return EXIT_FAILURE;
		}
		
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
	}
	
	
	read_write_loop(tabConnexion, nbreConnexion);
	//void read_write_loop(connexion[] tabConnexion, int nbreConnexion)

	//fermer les fd des sockets
	for(int i=0; i<nbreConnexion;i++)
	{
		close(tabConnexion[i].sfd);
	}

	//fermer les fd des fd_to_write
	for(int i=0; i<nbreConnexion;i++)
	{
		close(tabConnexion[i].fd_to_write);
	}
	
	free(hostname); //liberer l'espace alloue pour hostname

	fprintf(stderr,"---------------------------- \n");
	return EXIT_SUCCESS;
}
