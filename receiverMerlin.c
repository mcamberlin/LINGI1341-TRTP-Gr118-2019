/*
	Fichier receiverMerlin.c réprésente le code 

	Auteurs:
		- CAMBERLIN Merlin
		- PISVIN Arthur
	Version: 04-10-19 - Implémentation du format des fichiers

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
#include <inttypes.h> //pour prinft ulong6


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

/** La fonction getBitN() retourne le bit à la position @pos en comptant à partir de la gauche.
 * exemple : 0010 0000 la position du bit 1 est 2.
 * @pre 0<= pos < 64
 * @post 
 */
uint64_t getBitN(uint64_t entete, int position)
{
	uint64_t tmp = entete<<position;
	tmp = tmp>>(64-position-1);
	return tmp;
} 

void testGetBitN()
{
	uint64_t test0 =  0b0000000000000000000000000000000000000000000000000000000000000000;
	uint64_t value0 = getBitN(test0,0);
	
	uint64_t test1 = 0b0100000000000000000000000000000000000000000000000000000000000000;
	uint64_t value1 = getBitN(test1,1);

	uint64_t test2 = 0b0010000000000000000000000000000000000000000000000000000000000000;
	uint64_t value2 = getBitN(test2,2);
	
	uint64_t test3 =  0b0000000000000000000000000000000000000000000000000000000000000001;
	printf("test3 = %" PRIu64 "\n",test3);
	uint64_t value3 = getBitN(test3,3);
	

	printf("\t value0 = %" PRIu64 ", excepted = 0 \n",value0);
	printf("\t value1 = %" PRIu64 ", excepted = 1 \n",value1);
	printf("\t value2 = %" PRIu64 ", excepted = 1 \n",value2);
	printf("\t value3 = %" PRIu64 ", excepted = 1 \n",value3);
	
	
}


PACKET* setPACKET(uint64_t entete, char* donnees)
{
	printf("setPACKET() has started \n");


	PACKET* paquet = createPtrPACKET();
	if(paquet == NULL){printf("ERROR in setPACKET in createPtrPACKET\n");}

	int deplacementBit = 64;

	//1. Set type	
	deplacementBit = deplacementBit-2;
	uint8_t type = entete>>deplacementBit;
	
	if(type == 1)//0b01
	{	paquet->type = 1;}
	else if(type == 2) //0b10
	{	paquet->type = 2;}
	else if(type == 3) //0b11
	{	
		paquet->type = 3;
	}
	else
	{	
		fprintf(stderr, "ERROR in setPACKET in 1.set type - le type de packet est inconnu\n");
		paquet->type = 0;
	}

	//2. Set tr
	deplacementBit = deplacementBit-1;
	uint8_t tr = entete>>deplacementBit;
	printf("TR = %d \n",tr);
	if(tr %2 == 1)
	{
		paquet->tr = 1;
	}
	else 
	{
		paquet->tr = 0;
	}
	
	return paquet;
}

void* testSetPACKET()
{
	printf("testSetPACKET() has started \n");
	
	uint64_t entete0 = (uint64_t) 0b0000000000000000000000000000000000000000000000000000000000000000;
	uint64_t entete1 = (uint64_t) 0b0100000000000000000000000000000000000000000000000000000000000000;
	uint64_t entete2 = (uint64_t) 0b1010000000000000000000000000000000000000000000000000000000000000;
	uint64_t entete3 = (uint64_t) 0b1110000000000000000000000000000000000000000000000000000000000000;
	char* donnees = NULL;

	PACKET* paquet0 = setPACKET(entete0,donnees);
	printf("\t paquet0->type = %d, excepted = 0 \n \t paquet0->tr = %d, excepted = 0 \n",paquet0->type,paquet0->tr);

	
	PACKET* paquet1 = setPACKET(entete1,donnees);
	printf("\t paquet1->type = %d, excepted = 1 \n \t paquet1->tr = %d, excepted = 0 \n",paquet1->type,paquet1->tr);
	
	PACKET* paquet2 = setPACKET(entete2,donnees);
	printf("\t paquet2->type = %d, excepted = 2 \n \t paquet2->tr = %d, excepted = 1 \n",paquet2->type,paquet2->tr);		

	PACKET* paquet3 = setPACKET(entete3,donnees);
	printf("\t paquet3->type = %d, excepted = 3 \n \t paquet2->tr = %d, excepted = 1 \n",paquet3->type,paquet3->tr);

	printf("testSetPACKET() has finished \n");
	
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
	testGetBitN();
	//testSetPACKET();	
	printf("main() has finished \n");
}
