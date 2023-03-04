#ifndef __SOCKET_H_
#define __SOCKET_H_

#include <stdio.h> // pour fprintf()
#include <sys/types.h> // pour connect() 
#include <sys/socket.h> // pour getaddrinfo()
#include <netdb.h> // pour getaddrinfo()
#include <string.h>  // Pour memset()
#include <poll.h> // pour pollfd
#include <unistd.h> // pour read(), write() et close()
#include <errno.h> // pour le detail des erreurs

/** La fonction real_address() permet de convertir une chaîne de caractères représentant soit un nom de domaine soit une adresse IPv6, en une structure  struct @sockaddr_in6 utilisable par l'OS 
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 */
const char* real_address(const char *address, struct sockaddr_in6 *rval);



/** La fonction create_socket() crée un socket et l'initialise.
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port);


/** Lorsque un client veut parler à un serveur, il spécifie l'adresse et port du serveur, et choisit un port aléatoire pour lui. Le serveur par contre ne connait pas à priori l'adresse du client qui se connectera. 
 *
 * La fonction wait_for_client() intercepte le premier message reçu par le serveur, afin de connaître l'adresse du client et de pouvoir connecter le socket du serveur au client.
 * Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd);

/* --------------------------------------------------------- */

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
