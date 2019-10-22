
#include "socket.h" 
#include "packet_interface.h"
#include "LinkedList.h"

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

typedef struct connexion
{
	int sfd; // File descriptor du socket ouvert pour la connexion
	int fd_to_write; // File descriptor pour ecrire dans le fichier correspondant
	int closed; // Par defaut les connexions ne sont pas terminee =0

	uint8_t tailleWindow; // taille par défaut de la fenetre du sender[i]
	int windowMin ;
	int windowMax;
	
	node_t** head; //tete de la liste chainee associee à la connexion[i]
	
}connexion;


int printPkt(pkt_t* pkt)
{
	if(pkt==NULL)
	{
		return -1;
	}
	
	fprintf(stderr, "type=%d, tr=%d, window=%d, length=%d, seqnum=%d, timestamp=%d, crc1=%d, payload=%p, crc2=%d\n", pkt_get_type(pkt),pkt_get_tr(pkt),pkt_get_window(pkt),pkt_get_length(pkt),pkt_get_seqnum(pkt),pkt_get_timestamp(pkt),pkt_get_crc1(pkt),pkt_get_payload(pkt),pkt_get_crc2(pkt));
	return 0;
}


/**
  * return true si c'est dans l'intervalle de la fenetre et false sinon
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
	(*borne)=(*borne)+1;
}


/** La fonction read_write_loop() permet de lire le contenu du socket et l'ecrire dans un fichier, tout en permettant de renvoyer des accuses sur ce meme socket.
 * Si A et B tapent en même temps, la lecture et l'écriture est traitée simultanément à l'aide de l'appel système: poll()
 * Loop reading a socket and writing into a file,
 * while writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: dès qu'un paquet DATA avec le champ length à 0 et dont le numéro de séquence correspond au dernier numéro d'acquittement envoyé par le destinataire.
 */

void read_write_loop(connexion tabConnexion[], int nbreConnexion)
{
	const int MAXSIZE = MAX_PAYLOAD_SIZE + 12 + 4; //   EN-TETE + CRC1 + payload + CRC2 
	char buffer_socket[MAXSIZE]; //buffer de la lecture du socket 

	nfds_t nfds = nbreConnexion;
	struct pollfd filedescriptors[(int) nfds];
	int nbreConnexionEnCours = nbreConnexion; // nbre de connexion non fermees

	for(int i=0; i<nbreConnexion;i++)
	{
		filedescriptors[i].fd = tabConnexion[i].sfd;         // Surveiller le socket
		filedescriptors[i].events = POLLIN; 		  // Surveiller lorsque il y a de la donnee a lire sur le socket
	}
	
	int timeout = -1; //poll() shall wait for an event to occur 


	while(nbreConnexionEnCours>0) // tant que tous les connexions n'ont pas ete fermees 
	{
		int p = poll(filedescriptors,nfds, timeout); 
		// int poll(struct pollfd fds[], nfds_t nfds, int timeout);
		if(p == -1)
		{
			fprintf(stderr, "Erreur dans poll read_write_loop(): %s \n", strerror(errno));
			return;
		}
		
		for(int i=0; i<nbreConnexion && (tabConnexion[i].closed == 0);i++)
		//Boucle pour parcourir le tableau @filedescriptors et ne pas verifier les connexions deja 
		{

			if(filedescriptors[i].revents & POLLIN) // There is data to read on the socket
			{
				//1. Lire le socket
		    		memset(buffer_socket,0,MAXSIZE); // Remettre le buffer à 0 avant d'écrire dedans
				ssize_t r_socket = read(tabConnexion[i].sfd, buffer_socket, MAXSIZE); //ssize_t read(int fd, void *buf, size_t count)
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
					if(code == E_TR)//envoie d'un NACK
					{
			
						//envoie d'un NACK
						pkt_t* pkt_nack = pkt_new();
						pkt_set_type(pkt_nack, PTYPE_NACK);
						pkt_set_tr(pkt_nack, 0);
						
						pkt_set_window(pkt_nack, (tabConnexion[i].tailleWindow)-nbrePktBuffer);
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
						printPkt(pkt_nack);
						fprintf(stderr," ================= \n"); 

											
						pkt_del(pkt_nack);
						pkt_del(pkt_recu);
						ssize_t sent = send(tabConnexion[i].sfd, buf, tailleAck, 0);
						//ssize_t send(int socket, const void *buffer, size_t length, int flags);
						if(sent == -1)
						{
							fprintf(stderr,"L'envoi du NACK pour la connexion %d a echoue \n",tabConnexion[i].sfd);
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
					fprintf(stderr, "SUCCES DECODE : /* Le paquet a ete traite avec succes */ \n"); 
					
					if(pkt_get_type(pkt_recu) == PTYPE_DATA && pkt_get_length(pkt_recu)==0 && pkt_get_seqnum(pkt_recu) == dernierAck)//fin de la transmission si PTYPEDATA && length== && seqnum == dernier seqnum envoyé
					{
						tabConnexion[i].closed = 1;
						nbreConnexionEnCours = nbreConnexionEnCours -1; //Met à jour le nombre de connexions non fermée
						
					}				
					
					if(pkt_get_type(pkt_recu) == PTYPE_DATA && tabConnexion[i].closed==0) //permet d'ignorer les autres types
					{
						int windowSender = pkt_get_window(pkt_recu);
						if(windowSender != tabConnexion[i].tailleWindow && windowSender!=0) //Si la fenetre actuelle est differente de celle recue dans le pkt (fenetre dynamique) && le cas ou on commence à parler le sender envoie une fenetre = 0
						{
							tabConnexion[i].tailleWindow = windowSender;
							tabConnexion[i].windowMax = tabConnexion[i].windowMin + tabConnexion[i].tailleWindow - 1; 
							
							
							if(tabConnexion[i].windowMax > 255) // il faut encore prendre le cas ou la fenetre recommence a zero
							{
								tabConnexion[i].windowMax = tabConnexion[i].windowMax-255;
							}
						}

						//printList(tabConnexion[i].head);

						uint8_t seqnum = pkt_get_seqnum(pkt_recu);
						
						if(isInWindow(seqnum, tabConnexion[i].windowMin, tabConnexion[i].windowMax)) //si seqnum est dans la fenetre courante de reception 
						{
							if(seqnum==tabConnexion[i].windowMin) //si le numero de seq correspond a la limite min de la fenetre, on ecrit ce paquet et tout ceux qui suivent dans une liste chainee pour les trier par seqnum 
							{
								
								fprintf(stderr," ======= PKT RECU ======= \n\t"); printPkt(pkt_recu); fprintf(stderr," ================= \n"); 

								int w = write(tabConnexion[i].fd_to_write, pkt_get_payload(pkt_recu), pkt_get_length(pkt_recu));
								if(w==-1)
								{
									fprintf(stderr, "Erreur dans l'ecriture dans le fichier \n"); 
								}

								
								fprintf(stderr,"1. Valeur borne MIN : %d, Valeur borne MAX : %d, valeur de la window : %d \n",tabConnexion[i].windowMin,tabConnexion[i].windowMax, pkt_get_window(pkt_recu));
								augmenteBorne(&(tabConnexion[i].windowMin));
								augmenteBorne(&(tabConnexion[i].windowMax));
								fprintf(stderr,"2. Valeur borne MIN : %d, Valeur borne MAX : %d, valeur de la window : %d \n",tabConnexion[i].windowMin,tabConnexion[i].windowMax, pkt_get_window(pkt_recu));

								
								int seqnum_suivant = seqnum+1;

								
								pkt_t* pktSuivant = isInList(tabConnexion[i].head, seqnum_suivant);
								
								if(pktSuivant == NULL)
								{
									printf("le numero suivant n'est pas dans la liste, %d\n", seqnum_suivant);
								}
								else
								{
									printf("début boucle while\n");
									//printList(tabConnexion[i].head);
									printf("message = %s\n", pkt_get_payload(pktSuivant));

									while(pktSuivant != NULL)
									{
										w = write(tabConnexion[i].fd_to_write, pkt_get_payload(pktSuivant), pkt_get_length(pktSuivant));
										if(w==-1)
										{
											fprintf(stderr, "Erreur dans l'ecriture dans le fichier \n"); 
										}
										pkt_del(pktSuivant);
										seqnum_suivant ++;
										pktSuivant = isInList(tabConnexion[i].head, seqnum_suivant);
										if(pktSuivant!=NULL)
										{
											augmenteBorne(&(tabConnexion[i].windowMin));
											augmenteBorne(&(tabConnexion[i].windowMax));
										}
									}
								}
								
							}
							else //le seqnum appartient a la fenetre mais plus grand que la borne inferieur -> on met dans la liste chainee
							{
								printf("met dans la liste chainee\n");
								int err = insert(tabConnexion[i].head, pkt_recu, pkt_get_seqnum(pkt_recu));
								if(err == -1)
								{
									fprintf(stderr, "Erreur dans l'insertion dans la liste chainee  \n");
								}
								nbrePktBuffer++;
								//printList(tabConnexion[i].head);
							}
						}
						//envoie d'un ACK

						dernierAck = (pkt_get_seqnum(pkt_recu)+1) % 256;
						pkt_t* pkt_ack = pkt_new();
						pkt_set_type(pkt_ack, PTYPE_ACK);
						pkt_set_tr(pkt_ack, 0);
						
						pkt_set_window(pkt_ack, (tabConnexion[i].tailleWindow)-nbrePktBuffer);
						pkt_set_length(pkt_ack, 0);
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
						fprintf(stderr," ======= PKT ACCUSE ENVOYE ======= \n\t");
						printPkt(pkt_ack);
						fprintf(stderr," ================= \n"); 

											
						pkt_del(pkt_ack);
						pkt_del(pkt_recu);
						ssize_t sent = send(tabConnexion[i].sfd, buf, tailleAck, 0);
						//ssize_t send(int socket, const void *buffer, size_t length, int flags);
						if(sent == -1)
						{
							fprintf(stderr,"L'envoi du ACK pour la connexion %d a echoue \n",tabConnexion[i].sfd);
						}
						
						free(buf); 
					}
				}
			}
		}
		fprintf(stderr, "nombre de connexion en cours = %d\n", nbreConnexionEnCours); 
	}
}
