#ifndef __READ_WRITE_LOOP_H_
#define __READ_WRITE_LOOP_H_

#include <stdio.h> // pour fprintf()
#include <sys/types.h> // pour connect() 
#include <sys/socket.h> // pour getaddrinfo()
#include <netdb.h> // pour getaddrinfo()
#include <string.h>  // Pour memset()
#include <poll.h> // pour pollfd
#include <unistd.h> // pour read(), write() et close()
#include <errno.h> // pour le detail des erreurs
#include "LinkedList.h"
#include "ConnexionList.h"


typedef struct c_node c_node_t;

typedef struct connexion connexion_t;

/**
  * return true si c'est dans l'intervalle de la fenetre et false sinon
  *
  *
*/
int isInWindow(int seqnum, int windowMin, int windowMax);

/**
  *augmente la borne de la window tout en la remettant a zero si elle depasse la limite 
  *de 255
  *
*/
void augmenteBorne(int* borne);


/** La fonction read_write_loop() permet de lire le contenu du socket et l'ecrire dans un fichier, tout en permettant de renvoyer des accuses sur ce meme socket.
 * Si A et B tapent en même temps, la lecture et l'écriture est traitée simultanément à l'aide de l'appel système: poll()
 * Loop reading a socket and writing into a file,
 * while writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: dès qu'un paquet DATA avec le champ length à 0 et dont le numéro de séquence correspond au dernier numéro d'acquittement envoyé par le destinataire.
 */

void read_write_loop(int sfd, c_node_t** head, char* formatSortie, int nbreConnexionMax);


/** ------------------ struct addrinfo ---------------------
{
    * int              ai_family; the desired address family for the returned addresses. 
    * int              ai_socktype; the preferred socket type
    * int              ai_protocol; the protocol for the returned socket addresses. Specifying 0 for any protocol

     Other fields must contain either 0 or a null pointer, as appropriate.
    * int              ai_flags;
    * size_t           ai_addrlen;
    * struct sockaddr *ai_addr;
    * char            *ai_canonname;
    * struct addrinfo *ai_next;

};*/ 

/* ------------------ struct sockaddr_in6 ---------------------
{
               sa_family_t     sin6_family;    AF_INET6 
               in_port_t       sin6_port;      port number
               uint32_t        sin6_flowinfo;  IPv6 flow information 
               struct in6_addr sin6_addr;      IPv6 address 
               uint32_t        sin6_scope_id;  Scope ID (new in 2.4)
};*/


/* ------------------ struct pollfd ---------------------------
{
	int   fd;          file descriptor
	short events;      requested events
	short revents;     returned events 
};*/




#endif
