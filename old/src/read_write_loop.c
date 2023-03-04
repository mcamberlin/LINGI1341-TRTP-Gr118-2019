#include "socket.h" 
#include "packet_interface.h"
#include "LinkedList.h"
#include "ConnexionList.h"

#include <stdio.h> // pour fprintf()
#include <sys/types.h> // pour connect() 
#include <sys/socket.h> // pour getaddrinfo()
#include <netdb.h> // pour getaddrinfo()
#include <string.h>  // Pour memset()
#include <poll.h> // pour pollfd
#include <unistd.h> // pour read(), write() et close()
#include <errno.h> // pour le detail des erreurs

size_t tailleAck = 11; //taille d'un ACK et d'un NACK en bytes
int nbrePktBuffer = 0;
int dernierAck=-1; //initialisation 

int numeroFichier = 0;
int nbreConnexionEnCours = 0;

typedef struct connexion
{
	struct sockaddr_in6 client_addr;
	int fd_to_write; // File descriptor pour ecrire dans le fichier correspondant
	int closed; // Par defaut les connexions ne sont pas terminee =0
	uint8_t tailleWindow; // taille par défaut de la fenetre du sender[i]
	int windowMin ;
	int windowMax;
	node_t** head; //tete de la liste chainee associee à la connexion[i]
	
}connexion;

typedef struct c_node{
	connexion_t* c_connexion;
	struct c_node* next;
}c_node;




int printPkt(pkt_t* pkt)
{
	
	if(pkt==NULL)
	{
		return -1;
	}
	fprintf(stderr, "type=%d, tr=%d, window=%d, length=%d, seqnum=%d, timestamp=%d, crc1=%d, payload=%p, crc2=%d\n", pkt_get_type(pkt),pkt_get_tr(pkt),pkt_get_window(pkt),pkt_get_length(pkt),pkt_get_seqnum(pkt),pkt_get_timestamp(pkt),pkt_get_crc1(pkt),pkt_get_payload(pkt),pkt_get_crc2(pkt));
	fprintf(stderr," ================= \n"); 	
	return 0;
}


/**
  * return true si c'est dans l'intervalle de la fenetre et false sinon
  *
  *
*/
int isInWindow(int seqnum, int windowMin, int windowMax)
{
	if(windowMin<=windowMax) //cas ou la borne superieur n'est pas encore revenue a zero
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
	(*borne)=(*borne)+1;
	return;
}


/** La fonction read_write_loop() permet de lire le contenu du socket et l'ecrire dans un fichier, tout en permettant de renvoyer des accuses sur ce meme socket.
 * Si A et B tapent en même temps, la lecture et l'écriture est traitée simultanément à l'aide de l'appel système: poll()
 * Loop reading a socket and writing into a file,
 * while writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: dès qu'un paquet DATA avec le champ length à 0 et dont le numéro de séquence correspond au dernier numéro d'acquittement envoyé par le destinataire.
 */
void read_write_loop(int sfd, c_node_t** c_head, char* formatSortie, int nbreConnexionMax)
{
	const int MAXSIZE = 12 + MAX_PAYLOAD_SIZE + 4; //   EN-TETE + CRC1 + payload + CRC2 
	char buffer_socket[MAXSIZE]; //buffer de la lecture du socket 

	nfds_t nfds = 1;
	struct pollfd filedescriptors[1];

	filedescriptors[0].fd = sfd;                      // Surveiller le socket emetteur
	filedescriptors[0].events = POLLIN; 		  // Surveiller lorsque il y a de la donnee a lire sur le socket
	
	int timeout = 0; //poll() shall wait for an event to occur 


	while(1)
	{
		int p = poll(filedescriptors,nfds, timeout); 
		// int poll(struct pollfd fds[], nfds_t nfds, int timeout);
		if(p == -1)
		{
			fprintf(stderr, "Erreur dans poll read_write_loop(): %s \n", strerror(errno));
			return;
		}
		

		if(filedescriptors[0].revents & POLLIN) // Il y a de la donnee a lire sur le socket et le nombre max
		{
			//0. Récupérer son addresse 
			struct sockaddr_in6 client_addr; // pour s'assurer qu'il s'agisse d'une adresse IPv6
			socklen_t addrlen = sizeof(struct sockaddr_in6);
			

			ssize_t rec = recvfrom(sfd,NULL,0,MSG_PEEK,(struct sockaddr *) &client_addr,&addrlen);
			// ssize_t recvfrom(int socket, void *restrict buffer, size_t length, int flags, struct sockaddr *restrict address, socklen_t *restrict address_len);
			if(rec ==-1) // si recvfrom plante
			{
			    fprintf(stderr, "Erreur avec recfrom(): %s \n", strerror(errno));
			    return;
			}
			

			// 1. Verifier si le client existe deja dans la liste chainee 

			connexion* c_courante = contains(c_head, client_addr); // connexion courante 
			//connexion* contains(c_node** head, struct sockaddr_in6 client_addr)
			

			if( (c_courante == NULL && nbreConnexionEnCours < nbreConnexionMax) || c_courante != NULL) //permet d'ignorer
			{
		
				if(c_courante == NULL && nbreConnexionEnCours < nbreConnexionMax ) // si il n'existe pas et que le nombre de connexionMax n'est pas encore atteint, le creer et l'ajouter dans la liste chainee
				{			
					c_courante = add(c_head, client_addr, numeroFichier, formatSortie);
					if(c_courante==NULL)
					{
						return;
					}

					numeroFichier++; //apres avoir créer le fichier fd_to_write de cette connexion, on incrémente le numero de fichier pour la connexion suivante
					nbreConnexionEnCours = nbreConnexionEnCours + 1;
				}	
					

			
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
				
				pkt_status_code code = pkt_decode((char*) buffer_socket, r_socket, pkt_recu );

				if(code != PKT_OK) // Si le paquet n'est pas conforme
				{
					if(code == E_TYPE) // Erreur liée au champ Type
					{
						fprintf(stderr,"Erreur DECODE : /* Erreur liee au champs Type */, code = %d \n", code);
					}
					if(code == E_TR) // Erreur liée au champ TR implique l'envoi d'un NACK
					{
			
						//envoie d'un NACK
						pkt_t* pkt_nack = pkt_new();
						pkt_set_type(pkt_nack, PTYPE_NACK);
						pkt_set_tr(pkt_nack, 0);
						
						pkt_set_window(pkt_nack, 31-nbrePktBuffer);
						pkt_set_length(pkt_nack, 0);
						pkt_set_seqnum(pkt_nack, pkt_get_seqnum(pkt_recu)); 
						
						pkt_set_timestamp(pkt_nack, pkt_get_timestamp(pkt_recu));
						
						

						char* buf = (char*) malloc(tailleAck);
						if(buf==NULL)
						{
							fprintf(stderr, "Erreur malloc ack dans read_write_loop\n");
							return;
						}
						
						pkt_status_code status = pkt_encode(pkt_nack, buf, &tailleAck);
							
						if(status!=PKT_OK)
						{
							fprintf(stderr, "Erreur dans pkt_encode d'un NACK\n");
						}
						fprintf(stderr," ======= PKT NACK ENVOYE ======= \n\t");
						
						printPkt(pkt_nack); //affiche le NACK renvoye
								
						pkt_del(pkt_nack);
						pkt_del(pkt_recu);
						ssize_t sent = sendto(sfd, buf, tailleAck, 0,(const struct sockaddr *) &client_addr, addrlen );
						//ssize_t sendto(int sockfd, const void* buf, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t addr);
						if(sent == -1)
						{
							fprintf(stderr,"L'envoi du NACK pour la connexion %d a echoue \n",sfd);
						}
						
						free(buf); 
						
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
				if(code == PKT_OK)     
				{
					fprintf(stderr, "numero de sequence du pkt recu : %d, dernier numero de ack : %d\n\n", pkt_get_seqnum(pkt_recu), dernierAck);
					if(pkt_get_type(pkt_recu) == PTYPE_DATA && pkt_get_length(pkt_recu)==0 && pkt_get_seqnum(pkt_recu) == dernierAck)//fin de la transmission si PTYPEDATA && length== && seqnum == dernier seqnum envoyé
					{
						fprintf(stderr," ======= PKT DERNIER PACKET RECU ======= \n");
						printPkt(pkt_recu);
						
						fprintf(stderr, "FIN DE CONNEXION\n\n");
						c_courante->closed = 1;					
						
					}				
					
					if(pkt_get_type(pkt_recu) == PTYPE_DATA && c_courante->closed==0) //permet d'ignorer les autres types et ne pas considerer les connexions fermees
					{
						int windowSender = pkt_get_window(pkt_recu);
						if(windowSender != c_courante->tailleWindow && windowSender!=0) //Si la fenetre actuelle est differente de celle recue dans le pkt (fenetre dynamique) && le cas ou on commence à parler le sender envoie une fenetre = 0
						{
							c_courante->tailleWindow = windowSender;
							c_courante->windowMax = c_courante->windowMin + c_courante->tailleWindow - 1; 
							
							
							if(c_courante->windowMax > 255) // il faut encore prendre le cas ou la fenetre recommence a zero
							{
								c_courante->windowMax = c_courante->windowMax-255;
							}
						}

						//printList(c_courante->head);

						uint8_t seqnum = pkt_get_seqnum(pkt_recu);
						
						if(isInWindow(seqnum, c_courante->windowMin, c_courante->windowMax)==1) //si seqnum est dans la fenetre courante de reception 
						{
							if(seqnum==c_courante->windowMin) //si le numero de seq correspond a la limite min de la fenetre, on ecrit ce paquet et tout ceux qui suivent dans une liste chainee pour les trier par seqnum 
							{
								
								fprintf(stderr," ======= PKT RECU ======= \n"); 
								printPkt(pkt_recu); 

								int w = write(c_courante->fd_to_write, pkt_get_payload(pkt_recu), pkt_get_length(pkt_recu));
								if(w==-1)
								{
									fprintf(stderr, "Erreur dans l'ecriture dans le fichier \n"); 
								}
								

								
								// fprintf(stderr,"1. Valeur borne MIN : %d, Valeur borne MAX : %d, valeur de la window : %d \n",c_courante->windowMin,c_courante->windowMax, pkt_get_window(pkt_recu));
								augmenteBorne(&(c_courante->windowMin));
								augmenteBorne(&(c_courante->windowMax));
								// fprintf(stderr,"2. Valeur borne MIN : %d, Valeur borne MAX : %d, valeur de la window : %d \n",c_courante->windowMin,c_courante->windowMax, pkt_get_window(pkt_recu));
								
								int seqnum_suivant = seqnum+1;

								
								pkt_t* pktSuivant = isInList(c_courante->head, seqnum_suivant);
								if(pktSuivant == NULL)
								{
									fprintf(stderr,"Le numero suivant n'est pas dans la liste, %d\n", seqnum_suivant);
								}
								else
								{

									//printList(c_courante->head);

									while(pktSuivant != NULL)
									{
										w = write(c_courante->fd_to_write, pkt_get_payload(pktSuivant), pkt_get_length(pktSuivant));
										if(w==-1)
										{
											fprintf(stderr, "Erreur dans l'ecriture dans le fichier \n"); 
										}
										pkt_del(pktSuivant);
										seqnum_suivant ++;
										pktSuivant = isInList(c_courante->head, seqnum_suivant);
										if(pktSuivant!=NULL)
										{
											augmenteBorne(&(c_courante->windowMin));
											augmenteBorne(&(c_courante->windowMax));
										}
									}
								}
								
							}
							else //le seqnum appartient a la fenetre mais plus grand que la borne inferieur -> on met dans la liste chainee
							{
								fprintf(stderr,"met dans la liste chainee\n");
								int err = insert(c_courante->head, pkt_recu, pkt_get_seqnum(pkt_recu));
								if(err == -1)
								{
									fprintf(stderr, "Erreur dans l'insertion dans la liste chainee  \n");
								}
								nbrePktBuffer++;
								//printList(c_courante->head);
							}
						}
							
						//envoi d'un ACK
						
						pkt_t* pkt_ack = pkt_new();
						pkt_set_type(pkt_ack, PTYPE_ACK);
						pkt_set_tr(pkt_ack, 0);
						

						fprintf(stderr, "\t Nombre de paquet dans le buffer = %d \n",nbrePktBuffer);
						
						pkt_set_window(pkt_ack, 31-nbrePktBuffer);
						pkt_set_length(pkt_ack, 0);
					
						dernierAck = (pkt_get_seqnum(pkt_recu)+1) % 256;
						fprintf(stderr, "le num de seq envoye est %d\n", dernierAck);
						pkt_set_seqnum(pkt_ack, (pkt_get_seqnum(pkt_recu)+1) % 256); //2⁸ = 256
										
						pkt_set_timestamp(pkt_ack, pkt_get_timestamp(pkt_recu));
						
						//uint32_t crc1R=crc32(0,(const Bytef *) , *len);
						

						char* buf = (char*) malloc(tailleAck);
						if(buf==NULL)
						{
							fprintf(stderr, "Erreur malloc ack dans read_write_loop\n");
							return;
						}
						
						pkt_status_code status = pkt_encode(pkt_ack, buf, &tailleAck);
							
						if(status!=PKT_OK)
						{
							fprintf(stderr, "Erreur dans pkt_encode d'un ACK\n");
						}
						fprintf(stderr," ======= PKT ACCUSE ENVOYE ======= \n");
						printPkt(pkt_ack);

											
						pkt_del(pkt_ack);
						pkt_del(pkt_recu);
						ssize_t sent = sendto(sfd, buf, tailleAck, 0,(const struct sockaddr *) &client_addr, addrlen );
						//ssize_t sendto(int sockfd, const void* buf, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t addr);
						
						if(sent == -1)
						{
							fprintf(stderr,"L'envoi du ACK pour la connexion %d a echoue \n",sfd);
						}
						
						free(buf); 
					}

					if(pkt_get_type(pkt_recu) == PTYPE_DATA && c_courante->closed==1) //envoie le dernier ack pour la fin de connexion
					{
						pkt_t* pkt_ack = pkt_new();
						pkt_set_type(pkt_ack, PTYPE_ACK);
						pkt_set_tr(pkt_ack, 0);
						
						pkt_set_window(pkt_ack, 0);
						pkt_set_length(pkt_ack, 0);
										
						fprintf(stderr, "le num de seq envoye pour la fin de connexion est %d\n", dernierAck);
						pkt_set_seqnum(pkt_ack, dernierAck+1); 
					
						pkt_set_timestamp(pkt_ack, pkt_get_timestamp(pkt_recu));
						
						char* buf = (char*) malloc(tailleAck);
						if(buf==NULL)
						{
							fprintf(stderr, "Erreur malloc ack dans read_write_loop\n");
							return;
						}
						
						pkt_status_code status = pkt_encode(pkt_ack, buf, &tailleAck);
							
						if(status!=PKT_OK)
						{
							fprintf(stderr, "Erreur dans pkt_encode d'un ACK\n");
						}
						fprintf(stderr," ======= PKT ACCUSE ENVOYE ======= \n");
						//printPkt(pkt_ack);
																
						pkt_del(pkt_ack);
						pkt_del(pkt_recu);
						
						
						ssize_t sent = sendto(sfd, buf, tailleAck, 0,(const struct sockaddr *) &client_addr, addrlen );
						//ssize_t sendto(int sockfd, const void* buf, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t addr);
						if(sent == -1)
						{
							fprintf(stderr,"L'envoi du ACK pour la fin de connexion %d a echoue \n",sfd);
						}
						
						free(buf);
						
						nbreConnexionEnCours = nbreConnexionEnCours -1;	

						//removeConnexion ferme fd_to_write, free la liste chainee du buffer, free la connexion et free le noeud de connexionList
						int remove = removeConnexion(c_head, c_courante);
						if(remove == -1)
						{
							fprintf(stderr, "erreur dans removeConnexion\n");
						}
						fprintf(stderr, "ICI \n");
					}
				}
			}
		}
	}
}
