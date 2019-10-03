/*
	Fichier receiverMerlin.c réprésente le code 

	Auteurs:
		- CAMBERLIN Merlin
		- PISVIN Arthur
	Version: 03-10-19 - Implémentation du format des fichiers

	Commandes à indiquer dans le shell:
		- cd ~/Documents/LINGI 1341
		- gcc receiverMerlin.c -Wall -Werror
		- ./receiver 

	Commandes git:
		- git status
		- git add monfichier
		- git commit -m "blablabla" monfichier
		- git commit -i -m "blablabla" monfichier // en cas de fusion de conflits
		- git push

	Remarques:
		- Par convention, les variables sont en français, les types de données et structures sont en anglais.
		- Ne pas oublier de faire les docstring à chaque fois
		- A chaque malloc, ne pas oublier en cas d'erreur
		- A chaque appel, tester la valeur de retour pour s'assurer du bon fonctionnement
		- Ajuster la version et le commentaire avec
		- fprintf(stderr, "Erreur malloc cas où argument -o spécifié %d\n", errno);
*/

// Includes
#include <stdio.h> 
#include <stdint.h> // pour les uint_xx
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h> 


/** PACKET est une structure de données représentant un paquet.
*/
typedef struct PACKET
{
	uint8_t type:2;
	uint8_t tr:1;
	uint8_t window:5;
	uint8_t l:1;
	uint16_t length :15;
	uint8_t seqnum;
	uint32_t timestamp;
	uint32_t crc1;
	char* payload ; // utiliser un pointeur est plus intéressant au point de vue des performances et de la consommation de la mémoire
	uint32_t crc2;
}PACKET;



/** La fonction createPtrPACKET() crée un pointeur vers un PACKET
@pre - 
@post - NULL si une erreur dans le malloc
	PACKET* sinon
*/
PACKET* createPtrPACKET() // devrait prendre en argument les 4224 bits 
{
	PACKET* ptrPaquet = (PACKET*) malloc(sizeof(struct PACKET*));
	if(ptrPaquet == NULL){return NULL;}

	return ptrPaquet;
}

/** La fonction freePACKET() libère la mémoire allouée dans le paquet pointé par "paquet".
@pre - PACKET* ptr = pointeur vers le paquet à libérer
@post - - -1 si une erreur s'est produite
	- 1 sinon
*/
int freePACKET(PACKET* paquet)
{
	if(paquet == NULL) {return -1;}
	else
	{
		free(paquet->payload); 
		paquet->payload = NULL; //Pour être sur que le pointeur est libéré.
		return 1;
	}
}


/*--------------------------MAIN------------------------------------*/

/** La fonction main est la fonction principale de notre programme:
	@pre - argc = nombre d'arguments passés lors de l'appel de l'exécutable
		- argv = tableau de pointeurs reprenant les arguments donnés par l'exécutable
	@post - exécute les différentes instructions du code
		- retourn EXIT_FAILURE si une erreur s'est produite et affiche sur la sortie stderr un détail de l'erreur
*/
int main(int argc, char *argv[]) 
{
	printf("main() has started \n");
	
	
	printf("main() has finished \n");
}
