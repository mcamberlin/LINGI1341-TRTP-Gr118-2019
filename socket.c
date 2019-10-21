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

/* --------------------------------------------------------------- */

/** La fonction real_address() permet de convertir une chaîne de caractères représentant soit un nom de domaine soit une adresse IPv6, en une structure  struct @sockaddr_in6 utilisable par l'OS 
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 */
// Inspiré de la solution trouvée sur : https://github.com/ddo/c-iplookup/blob/master/main.c
const char* real_address(const char *address, struct sockaddr_in6 *rval)
{
    // 1. Récupérer les informations concernant l'addresse
    
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;     // AF_INET6 indique que getaddrinfo() ne devrait retourner que des adresses IPv6.
    hints.ai_socktype = SOCK_DGRAM; // Type de socket pour les protocoles UDP 
    hints.ai_flags=AI_CANONNAME;   
    
    struct addrinfo *res;
    char *service = NULL;

    int test = getaddrinfo(address, service, &hints, &res);
    //int getaddrinfo (const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
    if (test != 0) // Si ca plante 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(test));
        return gai_strerror(test);
    } 
 
    struct sockaddr_in6* adresseIPv6 = (struct sockaddr_in6*) res->ai_addr; 
    
    // 2. Copier le résultat de getaddrinfo() dans rval 
    
    memcpy(rval,adresseIPv6,sizeof(struct sockaddr_in6));
    // void *memcpy(void *dest, const void *src, size_t n);
    
    // 3. Libérer res
    freeaddrinfo(res);

    return NULL;
}

/* --------------------------------------------------------------- */

/** La fonction create_socket() crée un socket et l'initialise.
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port)
{
	// 1. Create a IPv6 socket supporting datagrams

	int fd = socket(AF_INET6, SOCK_DGRAM, 0); 
	//int socket(int domain, int type, int protocol); 
	// AF_INET6 pour IPv6 et SOCK_DGRAM pour UDP et 0 pour protocole par default ou IPPROTO_UDP.
	if (fd == -1) // si ca plante
	{  
		fprintf(stderr, "Erreur creation de socket dans create_socket(): %s , errno %d\n", strerror(fd), fd);
		return -1;
	}

	if(src_port > 0)
	{
		source_addr->sin6_port = htons(src_port);
	}
	if(dst_port > 0)
	{
		dest_addr->sin6_port = htons(dst_port);
	}	
	
	// 2. Lier le socket avec la source using the bind() system call
	if(source_addr != NULL)
	{                    
		int lien = bind(fd,(struct sockaddr*) source_addr, (socklen_t) sizeof(struct sockaddr_in6));
	    //int bind(int socket, const struct sockaddr *address, socklen_t address_len);
		if(lien == -1)
		{
		    //close(fd); la fermeture des fd se fera tout a la fin de receiver.c
		    fprintf(stderr,"Erreur dans bind() : %s\n", strerror(errno));
		    return -1;
		}   
	}
	
	// 3.Connect the socket to the address of the destination using the connect() system call
	if(dest_addr != NULL)
	{
		int connect_src = connect(fd,(struct sockaddr*) dest_addr, (socklen_t) sizeof(struct sockaddr_in6));
		if(connect_src == -1)
		{
		    //close(fd); la fermeture des fd se fera tout a la fin de receiver.c
		    fprintf(stderr,"Erreur dans connect() \n");
		    return -1;
		}
	}

	return fd;    
}

/* --------------------------------------------------------------- */


/* --------------------------------------------------------------- */
 
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
int wait_for_client(int sfd)
{	
	// 1. Intercepter le premier message reçu par le serveur, afin de connaître l'adresse du client via recvfrom()
	
	struct sockaddr_in6 client_addr; // pour s'assurer qu'il s'agisse d'une adresse IPv6
	socklen_t addrlen = sizeof(struct sockaddr_in6);

	ssize_t rec = recvfrom(sfd,NULL,0,MSG_PEEK,(struct sockaddr *) &client_addr,&addrlen);
	// ssize_t recvfrom(int socket, void *restrict buffer, size_t length, int flags, struct sockaddr *restrict address, socklen_t *restrict address_len);
	
	if(rec ==-1) // si recvfrom plante
	{
	    fprintf(stderr, "Erreur avec recfrom() dans wait_for_client(): %s \n", strerror(errno));
	    return -1;
	}

	// 2. Connecter le socket du serveur au client via connect()
	int connexion = connect(sfd, (struct sockaddr *) &client_addr,addrlen);
	// int connect(int socket, const struct sockaddr *address, socklen_t address_len);
	
	if(connexion == -1) // Si ca plante
	{
		fprintf(stderr, "Erreur avec connect()dans wait_for_client(): %s \n", strerror(errno));
		return -1;  
	}
	
	return 0;
}



