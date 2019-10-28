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


	// Creation d'une liste chainée qui contiendra l'ensemble des structures @connexion
	c_node ** c_head = createConnexionList();


	// 1. Resolve the hostname 
	struct sockaddr_in6 addr;	
	const char *err = real_address(hostname,&addr);
	if (err)
	{
		fprintf(stderr, "Could not resolve hostname %s: %s\n", hostname, err);
		return EXIT_FAILURE;
	}

	// 2. Creation d'un socket pour toutes les connexions sans le connecter pour ecouter de partout
	int sfd = create_socket(&addr, port,NULL,-1);
	//int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port)
	if(sfd == -1)
	{
		fprintf(stderr,"Erreur createsocket() \n");	
	}	
	

	fprintf(stderr,"-----------DEBUT READ_WRITE_LOOP ----------------- \n");
	read_write_loop(sfd, c_head , nbreConnexion);
	//void read_write_loop(int sfd, c_node ** head, int nbreConnexion)


	return EXIT_SUCCESS;
}
