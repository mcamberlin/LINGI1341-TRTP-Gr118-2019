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
#include "test_read_write.h"
/**
  *return true si c'est dans l'intervalle de la fenetre et false sinon
  *
  *
*/
int isInWindow(int seqnum, int windowMin, int windowMax)
{
	if(windowMin<windowMax)//cas ou la borne superieur n'est pas encore revenue a zero
	{
		if(windowMin<=seqnum && windowMax>=seqnum)
		{
			return 1;
		}
	}
	else //cas ou la borne max est redescendu a zero
	{
		if((windowMin<=seqnum && seqnum<=255) || (0<=seqnum && seqnum<=windowMax))
		{
			return 1;
		}
	}
	return 0;
}


/**
  *augment la borne de la window tout en la remettant a zero si elle depasse la limite 
  *de 255
  *
*/
void augmenteBorne(int* borne)
{
	if(*borne==255)
	{
		*borne = 0;
		return;
	}
	(*borne)++;
}

uint8_t tailleWindow = 1; // taille par défaut de la fenetre du sender
int windowMin =255;
int windowMax = 255;
node_t** head;

//3.1 PTYPE == DATA
void test(pkt_t* pkt_recu)
{
	
	if(pkt_get_type(pkt_recu) == PTYPE_DATA)
	{
		int windowSender = pkt_get_window(pkt_recu);
		if(windowSender != tailleWindow) //fenetre dynamique
		{
			tailleWindow = windowSender;
			windowMax=windowMin+tailleWindow-1; //il faut encore prende le cas ou la fenetre recommence a zero
			if(windowMax>255)
			{
				windowMax = windowMax-255;
			}
		}
		printf("windowMin = %d\n", windowMin);
		printf("windowMax = %d\n", windowMax);
		printList(head);

		
		uint8_t seqnum = pkt_get_seqnum(pkt_recu);
		if(isInWindow(seqnum, windowMin, windowMax)) //prendre en compte seulement les seqnum dans l'intervalle 
		{
			if(seqnum==windowMin)//si le numero de seq correspond a la limite min de la fenetre, on ecrit ce paquet et tout ceux qui suivent dans la liste chainee 
			{
				int fichier = open("fichierSortie.txt", O_CREAT | O_WRONLY | O_APPEND, S_IRWXU); //si le fichier n'existe pas, il le crée
				if(fichier == -1) // cas où @fopen() a planté
				{
					fprintf(stderr, "Erreur dans l'ouverture du fichier \n");
					return;
				}
				int w = write(fichier, pkt_get_payload(pkt_recu), pkt_get_length(pkt_recu));
				if(w==-1)
				{
					fprintf(stderr, "Erreur dans l'ecriture dans le fichier \n"); 
				}

				printf("le message écrit est : %s\n", pkt_get_payload(pkt_recu));

				pkt_del(pkt_recu);
				augmenteBorne(&windowMin);
				augmenteBorne(&windowMax);
				int seqnum_suivant = seqnum+1;

				printf("avant isinlist\n");
				pkt_t* pktSuivant = isInList(head, seqnum_suivant);
				printf("apres isinlist\n");
				if(pktSuivant == NULL)
				{
					printf("le numero suivant n'est pas dans la liste, %d\n", seqnum_suivant);
				}
				else
				{
					while(pktSuivant != NULL)
					{
						w = write(fichier, pkt_get_payload(pktSuivant), pkt_get_length(pktSuivant));
						if(w==-1)
						{
							fprintf(stderr, "Erreur dans l'ecriture dans le fichier \n"); 
						}
						printf("le message écrit est : %s\n", pkt_get_payload(pktSuivant));
						pkt_del(pktSuivant);
						seqnum_suivant ++;
						pktSuivant = isInList(head, seqnum_suivant);
						augmenteBorne(&windowMin);
						augmenteBorne(&windowMax);
					}
				}
				
			}
			else //le seqnum appartient a la fenetre mais plus grand que la borne inferieur -> on met dans la liste chainee
			{
				int err = insert(head, pkt_recu, pkt_get_seqnum(pkt_recu));
				if(err == -1)
				{
					fprintf(stderr, "Erreur dans l'insertion dans la liste chainee  \n");
				}
				printf("apres la mise dans la liste chainée\n\n");
				printList(head);
			}
			printf("valeurs des bornes à la fin \n\n");
			printf("windowMin = %d\n", windowMin);
			printf("windowMax = %d\n", windowMax);

		}
	}

	
}


int main()
{
/*
	head = createList();

	printf("frame 1 \n\n");

	char msg1[] = "Messsage 1";
	pkt_t* pkt_recu1 = pkt_new();

	pkt_set_type(pkt_recu1, PTYPE_DATA);
	pkt_set_length(pkt_recu1, 11);
	pkt_set_payload(pkt_recu1, msg1, 11);
	pkt_set_seqnum(pkt_recu1, 255);
	pkt_set_window(pkt_recu1, 2);
	test(pkt_recu1);

	printf("frame 2 \n\n");
	char msg0[] = "Message 0";
	pkt_t* pkt_recu0 = pkt_new();

	pkt_set_type(pkt_recu0, PTYPE_DATA);
	pkt_set_length(pkt_recu0, 11);
	pkt_set_payload(pkt_recu0, msg0, 11);
	pkt_set_seqnum(pkt_recu0, 0);
	pkt_set_window(pkt_recu0, 2);
	test(pkt_recu0);

	printf("frame 3 \n\n");
	char msg2[] = "Message 2";
	pkt_t* pkt_recu2 = pkt_new();

	pkt_set_type(pkt_recu2, PTYPE_DATA);
	pkt_set_length(pkt_recu2, 11);
	pkt_set_payload(pkt_recu2, msg2, 11);
	pkt_set_seqnum(pkt_recu2, 1);
	pkt_set_window(pkt_recu2, 2);
	test(pkt_recu2);

	freeLinkedList(head);
*/
}


