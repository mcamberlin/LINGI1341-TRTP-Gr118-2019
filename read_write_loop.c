#include "socket.h" 
#include "packet_interface.h"

#include <stdio.h> // pour fprintf()
#include <sys/types.h> // pour connect() 
#include <sys/socket.h> // pour getaddrinfo()
#include <netdb.h> // pour getaddrinfo()
#include <string.h>  // Pour memset()
#include <poll.h> // pour pollfd
#include <unistd.h> // pour read(), write() et close()
#include <errno.h> // pour le detail des erreurs


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
  *augmente la borne de la window tout en la remettant a zero si elle depasse la limite 
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
int windowMin =0;
int windowMax = 0;
node_t** head;

/** La fonction read_write_loop() permet de lire le contenu du socket et l'ecrire dans un fichier, tout en permettant de renvoyer des accuses sur ce meme socket.
 * Si A et B tapent en même temps, la lecture et l'écriture est traitée simultanément à l'aide de l'appel système: poll()
 * Loop reading a socket and writing into a file,
 * while writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: dès qu'un paquet DATA avec le champ length à 0 et dont le numéro de séquence correspond au dernier numéro d'acquittement envoyé par le destinataire.
 */
//void read_write_loop(int[] fd_to_listen, int[] fd_to_write) //METTRE ICI LE TABLEAU DE CONNEXION
void read_write_loop(connexion[] tabConnexion, int nbreConnexion)
{
	const int MAXSIZE = MAX_PAYLOAD_SIZE + 12 + 4; //   EN-TETE + CRC1 + payload + CRC2 
	char buffer_socket[MAXSIZE]; //buffer de la lecture du socket 

	for(int i=0; i<nbreConnexion; i++)
	{
		

	while(1) 
	{
		nfds_t nfds = 1;
		struct pollfd filedescriptors[(int) nfds];
		//for loop
		filedescriptors[0].fd = sfd;         // Surveiller le socket
		filedescriptors[0].events = POLLIN; // When there is data to read on socket
		
		int timeout = -1; //poll() shall wait for an event to occur 
		
		int p = poll(filedescriptors,nfds, timeout); 
		// int poll(struct pollfd fds[], nfds_t nfds, int timeout);
		if(p == -1)
		{
			fprintf(stderr, "Erreur dans poll read_write_loop(): %s \n", strerror(errno));
			return;
		}
		
		//for loop
		if(filedescriptors[0].revents & POLLIN) // There is data to read on the socket
		{
			//1. Lire le socket
            		memset(buffer_socket,0,MAXSIZE); // Remettre le buffer à 0 avant d'écrire dedans
			ssize_t r_socket = read(sfd, buffer_socket, MAXSIZE); //ssize_t read(int fd, void *buf, size_t count)
			if(r_socket == -1)
			{
				fprintf(stderr, "Erreur lecture socket : %s \n", strerror(errno));
				return;
			}
			
			// 2. TRANSFORMER le buffer en un paquet avec decode
			pkt_t* pkt_recu = pkt_new();
			
			pkt_status_code code = pkt_decode((char*) buffer_in, r_socket, pkt_recu );

			if(code != PKT_OK) // Si le paquet n'est pas conforme
			{
				if(code == E_TYPE) // Erreur liée au champ Type
				{
					fprintf(stderr,"Erreur DECODE : /* Erreur liee au champs Type */, code = %d \n", code);
				}
				if(code == E_TR)
				{
					fprintf(stderr,"Erreur DECODE : /* Erreur liee au champ TR */, code = %d \n", code);
					//envoie d'un NACK
					pkt_t* pkt_nack = pkt_new();
					pkt_set_type(pkt_nack, PTYPE_ACK);
					pkt_set_seqnum(pkt_nack, pkt_get_seqnum(pkt_recu));
					char* buf = malloc(sizeof(pkt_t)); //Quelle est la taille d'un NACK ???????????????????
					if(buf==NULL)
					{
						fprintf(stderr, "Erreur malloc nack\n");
						return;
					}
					pkt_status_code status = pkt_encode(pkt_nack, buf, sizeof(pkt_t));
					//J'AI BESOIN DE L'ADRESSE POUR ENVOYER OU EST ELLE ?
					//int sent = sendto(fd_to_listen[i], buf, sizeof(buf), 0, 
					//ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
				}
				if(code == E_LENGTH)
				{
					fprintf(stderr,"Erreur DECODE : /* Erreur liee au champs Length  */, code = %d \n", code);

				}
				if(code == E_CRC)
				{
					fprintf(stderr,"Erreur DECODE : /* CRC invalide */, code = %d \n", code);
				}
				if(code == E_WINDOW)
				{
					fprintf(stderr,"Erreur DECODE : /* Erreur liee au champs Window */, code = %d \n", code);
				}
				if(code == E_SEQNUM)
				{
					fprintf(stderr,"Erreur DECODE : /* Numero de sequence invalide */, code = %d \n", code);
				}
				if(code == E_NOMEM)
				{
					fprintf(stderr,"Erreur DECODE : /* Pas assez de memoire */, code = %d \n", code);
				}
				if(code == E_NOHEADER)
				{
					fprintf(stderr, "Erreur DECODE : /* Le paquet n'a pas de header (trop court) */, code = %d \n", code);    			
				}
				if(code == E_UNCONSISTENT)
				{
					fprintf(stderr, "Erreur DECODE : /* Le paquet est incoherent */, code = %d \n", code);    
				}
				
			}

			// 3. Reagir differemment selon le type recu
			if(code == PKT_OK) //PKT_OK = 0,     
			{
				fprintf(stderr, "SUCCES DECODE : /* Le paquet a ete traite avec succes */ \n"); 
				int dernierAck = windowMin-1;
				if(windowMin==0)
				{
					dernierAck = 255;
				}
				if(pkt_get_type(pkt_recu) == PTYPE_DATA && pkt_get_length(pkt_recu)==0 && pkt_get_seqnum(pkt_recu) == dernierAck)
				{
					//fin de la transmission si PTYPEDATA && length== && seqnum == dernier seqnum envoyé
					conexion[i].closed == 1;
					
				}				
				
				if(pkt_get_type(pkt_recu) == PTYPE_DATA) //permet d'ignorer les autres types
				{
					int windowSender = pkt_get_window(pkt_recu);
					if(windowSender != tailleWindow) //Si la fenetre actuelle est differente de celle recue dans le pkt (fenetre dynamique)
					{
						tailleWindow = windowSender;
						windowMax = windowMin + tailleWindow - 1; 
						
						
						if(windowMax > 255) // il faut encore prendre le cas ou la fenetre recommence a zero
						{
							windowMax = windowMax-255;
						}
					}

					//printList(head);

					uint8_t seqnum = pkt_get_seqnum(pkt_recu);
					if(isInWindow(seqnum, windowMin, windowMax)) //si seqnum est dans la fenetre courante de reception 
					{
						if(seqnum==windowMin) //si le numero de seq correspond a la limite min de la fenetre, on ecrit ce paquet et tout ceux qui suivent dans une liste chainee pour les trier par seqnum 
						{
							// FAIRE LES OPEN AVANT !!!! 
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
								printf("début boucle while\n");
								printList(head);
								printf("message = %s\n", pkt_get_payload(pktSuivant));

								while(pktSuivant != NULL)
								{
									w = write(fichier, pkt_get_payload(pktSuivant), pkt_get_length(pktSuivant));
									if(w==-1)
									{
										fprintf(stderr, "Erreur dans l'ecriture dans le fichier \n"); 
									}
									pkt_del(pktSuivant);
									seqnum_suivant ++;
									pktSuivant = isInList(head, seqnum_suivant);
									if(pktSuivant!=NULL)
									{
										augmenteBorne(&windowMin);
										augmenteBorne(&windowMax);
									}
								}
							}
							
						}
						else //le seqnum appartient a la fenetre mais plus grand que la borne inferieur -> on met dans la liste chainee
						{
							printf("met dans la liste chainee\n");
							int err = insert(head, pkt_recu, pkt_get_seqnum(pkt_recu));
							if(err == -1)
							{
								fprintf(stderr, "Erreur dans l'insertion dans la liste chainee  \n");
							}
							printList(head);
						}
					}

				}
				//envoie d'un ack
				//sendto : besoin de l'adresse de l'expediteur 
			}
		}
	}
}


