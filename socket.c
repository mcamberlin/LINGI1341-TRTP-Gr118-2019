#include <stdio.h>
#include <sys/types.h> // 3 prochains includes pour getaddrinfo
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>  // Pour memset()

/** La fonction real_address() permet de convertir une chaîne de caractères représentant soit un nom de dommaine soit une adresse IPv6, en une structure  struct @sockaddr_in6 utilisable par l'OS 
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: 1 si l'operation reussi,
           -1 si une erreur se produit, un message decrivant l'erreur est alors affiche sur la sortie standard d'erreur.
 */
const char * real_address(const char *address, struct sockaddr_in6 *rval)
{
    char *service;
    struct addrinfo hints;
    struct addrinfo *res;
    int test;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    // void *memset(void *s, int c, size_t n);
/* Contenu de la structure de données addrinfo :
struct addrinfo {
    * int              ai_family;
This field specifies the desired address family for the returned addresses. Valid values for this field include AF_INET and AF_INET6. The value AF_UNSPEC indicates that getaddrinfo() should return socket addresses for any address family (either IPv4 or IPv6, for example) that can be used with node and service.
    * int              ai_socktype; 
This field specifies the preferred socket type, for example SOCK_STREAM or SOCK_DGRAM. Specifying 0 in this field indicates that socket addresses of any type can be returned by getaddrinfo().
    * int              ai_protocol;
This field specifies the protocol for the returned socket addresses. Specifying 0 in this field indicates that socket addresses with any protocol can be returned by getaddrinfo().

All the other fields in the structure pointed to by hints must contain either 0 or a null pointer, as appropriate.
    * int              ai_flags;
    * size_t           ai_addrlen;
    * struct sockaddr *ai_addr;
    * char            *ai_canonname;
    * struct addrinfo *ai_next;

};
*/ 
    hints.ai_family = AF_INET6;     // La valeur AF_INET6 indique que getaddrinfo() ne devrait retourner que des adresses de socket de la famille IPv6.
    hints.ai_socktype = SOCK_DGRAM; // Type de socket pour les protocoles UDP 
    hints.ai_protocol = 0;          
    hints.ai_addrlen = 0;
    hints.ai_addr = NULL;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;

    test = getaddrinfo(address, service, &hints, &res);
    //int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
    if (test != 0) // Si ca plante 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(test));
        return gai_strerror(test);
    } 
    return NULL;
}



#include <sys/socket.h>

/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket( struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port)
{

	printf(stderr,"===CREATION SOCKET===\n");

	// 0. Verifier les arguments
	if(source_addr == NULL)
	{ 
		fprintf(stderr, "source_addr est NULL \n");
		return -1;
	}
	if(src_port <= 0)
	{
		fprintf(stderr, "src_port est zero ou negatif \n");
		return -1;
	}
	if(dest_addr == NULL)
	{
		fprintf(stderr, "dest_addr est NULL \n");
		return -1;
	}
	if(dst_port <= 0)
	{
		fprintf(stderr, "dst_port est zero ou negatif \n");
		return -1;
	}
    
/*struct sockaddr_in6 
{
	sa_family_t     sin6_family;    AF_INET6 
	in_port_t       sin6_port;      port number 
	uint32_t        sin6_flowinfo;  IPv6 flow information 
	struct in6_addr sin6_addr;      IPv6 address 
	uint32_t        sin6_scope_id;  Scope ID (new in 2.4)
};
*/
	
	source_addr->sin6_port = htons(src_port);
	dest_addr->sin6_port = htons(dst_port);
	
        
	// 1. Create a IPv6 socket supporting datagrams

	int fd_src = socket(AF_INET6, SOCK_DGRAM, 0); //int socket(int domain, int type, int protocol); 
	// AF_INET6 pour l'adresse en IPv6 et  SOCK_DGRAM pour UDP et 0 pour protocole par default. 
	
	if (fd_src == -1) 
	{  
		fprintf(stderr, "socket: %s , errno %d\n", strerror(sock), sock);
		return -1;
	}

	// 2. Lier le socket avec la source using the bind() system call
	
	size_t len_addr = sizeof(struct sockaddr_in6);                      
	int lien = bind(fd_src,(struct sockaddr*) source_addr, len_addr)
        if(lien == -1)
        {
            fprintf(stderr,"Erreur dans bind() \n");
            return -1;
        }   
        
	// 3.Connect the socket to the address of the destination using the connect() system call
	int connect_src = connect(fd_src,(struct sockaddr*) dest_addr, len_addr)
	if(connect_src == -1)
        {
            fprintf(stderr,"Erreur dans connect() \n");
            return -1;
        }
     
	close(sock);                                            // release the resources used by the socket
	return 0;    
}


/* ---------------------------------------------------- */

/* EXAMPLE getaddrinfo()
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUF_SIZE 500

int
main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    char buf[BUF_SIZE];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(struct addrinfo)); 
    hints.ai_family = AF_UNSPEC;    / Allow IPv4 or IPv6 /
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket /
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address /
    hints.ai_protocol = 0;          /* Any protocol /
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(NULL, argv[1], &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully bind(2).
       If socket(2) (or bind(2)) fails, we (close the socket
       and) try the next address. /

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;                  /* Success /

        close(sfd);
    }

    if (rp == NULL) {               /* No address succeeded /
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           /* No longer needed /

    /* Read datagrams and echo them back to sender /

    for (;;) {
        peer_addr_len = sizeof(struct sockaddr_storage);
        nread = recvfrom(sfd, buf, BUF_SIZE, 0,
                (struct sockaddr *) &peer_addr, &peer_addr_len);
        if (nread == -1)
            continue;               /* Ignore failed request /

        char host[NI_MAXHOST], service[NI_MAXSERV];

        s = getnameinfo((struct sockaddr *) &peer_addr,
                        peer_addr_len, host, NI_MAXHOST,
                        service, NI_MAXSERV, NI_NUMERICSERV);
       if (s == 0)
            printf("Received %ld bytes from %s:%s\n",
                    (long) nread, host, service);
        else
            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

        if (sendto(sfd, buf, nread, 0,
                    (struct sockaddr *) &peer_addr,
                    peer_addr_len) != nread)
            fprintf(stderr, "Error sending response\n");
    }
}
*/
