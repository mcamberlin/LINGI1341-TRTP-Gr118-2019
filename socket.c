#include <stdio.h> // pour fread() et fwrite()
#include <sys/types.h> // pour recvfrom(), pour connect() et les 3 prochains includes pour getaddrinfo()
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>  // Pour memset()
#include <poll.h> // pour pollfd
#include <unistd.h> // pour read() et pour close()
#include <errno.h>

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


/* Erreur INGINIOUS
[Client]source_addr est NULL
[Client]Failed to create the socket!
[Server]dest_addr est NULL
[Server]Failed to create the socket!
The process crashed! (There is still data to send on stdin, but the program stopped...)
*/

/** La fonction real_address() permet de convertir une chaîne de caractères représentant soit un nom de domaine soit une adresse IPv6, en une structure  struct @sockaddr_in6 utilisable par l'OS 
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 *          (const char* means the caller cannot modify or free the return value,
 *           so do not use malloc!)
 */
// Inspiré de la solution trouvée sur : https://github.com/ddo/c-iplookup/blob/master/main.c
const char* real_address(const char *address, struct sockaddr_in6 *rval)
{
    // 1. Récupérer les informations concernant l'addresse
    
    struct addrinfo hints;
    memset(&hint, 0, sizeof hint);
    hints.ai_family = AF_INET6;     // AF_INET6 indique que getaddrinfo() ne devrait retourner que des adresses IPv6.
    hints.ai_socktype = SOCK_DGRAM; // Type de socket pour les protocoles UDP 
    hints.ai_flags=AI_CANONNAME;   
    /* Utile ?
    hints.ai_protocol = 0;          
    hints.ai_addrlen = 0;
    hints.ai_addr = NULL;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;
    */

    struct addrinfo *res;
    char *service = NULL;

    int test = getaddrinfo(address, service, &hints, &res);
    //int getaddrinfo (const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
    if (test != 0) // Si ca plante 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(test));
        return gai_strerror(test);
    } 
 
    // !!! getaddrinfo() écrit dans res une liste de socket address structures returned respectant les critères de hints
    // Avant de réécrire dans rval, il faut donc récupérer une structure de res


    struct sockaddr_in6* adresseIPv6 = (struct sockaddr_in6*) res->ai_addr; 
    
    // 2. Copier le résultat de getaddrinfo() dans rval 
    
    memcpy(rval,adresseIPv6,sizeof(struct sockaddr_in6));
    // void *memcpy(void *dest, const void *src, size_t n);
    
    // 3. Libérer res
    freeaddrinfo(res);

    return NULL;
}

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

	// 0. Verifier les arguments
	if(source_addr != NULL)
	{ 
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
		};*/
		
		source_addr->sin6_port = htons(src_port);
		dest_addr->sin6_port = htons(dst_port);
		
		
		// 1. Create a IPv6 socket supporting datagrams

		int fd_src = socket(AF_INET6, SOCK_DGRAM, 0); 
		//int socket(int domain, int type, int protocol); 
		// AF_INET6 pour l'adresse en IPv6 et  SOCK_DGRAM pour UDP et 0 pour protocole par default. 
		
		if (fd_src == -1) 
		{  
			fprintf(stderr, "socket: %s , errno %d\n", strerror(fd_src), fd_src);
			return -1;
		}

		// 2. Lier le socket avec la source using the bind() system call
		size_t len_addr = sizeof(struct  sockaddr_in6);                      
		int lien = bind(fd_src,(struct sockaddr*) source_addr, len_addr);
		if(lien == -1)
		{
		    fprintf(stderr,"Erreur dans bind() \n");
		    return -1;
		}   
		
		// 3.Connect the socket to the address of the destination using the connect() system call
		int connect_src = connect(fd_src,(struct sockaddr*) dest_addr, len_addr);
		if(connect_src == -1)
		{
		    fprintf(stderr,"Erreur dans connect() \n");
		    return -1;
		}
		return fd_src;    
	}
	else
	{
		fprintf(stderr, "source_addr est NULL \n");
		return -1;
	}
}



/** La fonction read_write_loop() permet de lire le contenu de l'entrée standard et de l'envoyer sur un socket, tout en permettant d'afficher sur la sortie standard ce qui est lu sur ce même socket.  

Si A et B tapent en même temps, les lectures et écritures sont multiplexées sur le même file descriptor (le socket) afin de ne pas bloquer le processus indéfiniment (par ex. en attendant que A finisse de taper avant d'afficher le texte de B), il est nécessaire de traiter la lecture et l'écriture simultanément à l'aide des appels systèmes : select ou poll

NB : fread() et fwrite() font des lectures/écritures via un buffer c-à-d non immédiates!

La taille maximale des segments envoyés ou reçus sera de 1024 bytes.

/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_loop(int sfd)
{
	const int MAXSIZE = 1024;
		
	struct pollfd pfd;
        pfd.fd = sfd; 
        pfd.events = POLLIN; 

	struct pollfd filedescriptors[1];
	filedescriptors[0] = pfd;

	nfds_t nfds = 1;
	int timeout = 5000; //poll() shall wait at least 'timeout' [ms] for an event to occur on any of the selected file descriptors.
	// 500ms ont ete choisis comme dans l'exemple de la man page
	
	int p = poll(filedescriptors,nfds, timeout); // int poll(struct pollfd fds[], nfds_t nfds, int timeout);
	if(p == -1)
	{
		fprintf(stderr, "An error occur: %s \n", strerror(errno));
		return;
	}
	else if(p == 0)
	{
		fprintf(stderr, "The call timed out and no file descriptors have been selected \n");
		return;
	}
	else
	{
		// Boucle pour lire un socket et imprimer sur stdout, tout en lisant stdin et en écrivant sur le socket
		char buffer_socket[MAXSIZE];
		char buffer_stdin[MAXSIZE];
		while(1)
		{

			// 1. Lire le contenu de l'entrée standard
			
			int r_stdin = read(0, buffer_stdin, MAXSIZE); //ssize_t read(int fd, void *buf, size_t count)
			// le fd = 0 correspond à stdin
			if(r_stdin == -1)
			{
				fprintf(stderr, "An error occured \n");
				return;
			}
			if(r_stdin > MAXSIZE)
			{
				fprintf(stderr, "La taille de la lecture est supérieure à la taille du buffer (%d bytes) \n",MAXSIZE);
				return;
			}
			if(r_stdin == 0)
			{
				fprintf(stderr, "Fin de la lecture de l'entrée standard \n");
				return;
			}

			// 2. L'écrire dans le socket
			int w_socket = write(sfd, buffer_stdin, MAXSIZE); //ssize_t write(int fd, const void *buf, size_t count);  	
			if(w_socket == -1)
			{
				fprintf(stderr,"An error occured while writing on the socket \n");
				return;
			}
		
			//3. Lire le socket
			ssize_t r_socket = read(sfd, buffer_socket, MAXSIZE); //ssize_t read(int fd, void *buf, size_t count)
			if(r_socket == -1)
			{
				fprintf(stderr, "An error occured \n");
				return;
			}
			if(r_socket > MAXSIZE)
			{
				fprintf(stderr, "La taille de la lecture est supérieure à la taille du buffer (%d bytes) \n",MAXSIZE);
				return;
			}
			if(r_socket == 0)
			{
				fprintf(stderr, "Fin de la lecture atteinte \n");
				return;
			}
			
			// 4. Afficher sur la sortie standard ce qui a été lu sur le socket
			fprintf(stdout,"%s",buffer_socket);	

		}
	}		
}
 
/* Lorsque un client veut parler à un serveur, il spécifie l'adresse et port du serveur, et choisit un port aléatoire pour lui. Le serveur par contre ne connait pas à priori l'adresse du client qui se connectera.

Ecrire une fonction qui interceptera le premier message reçu par le serveur, afin de connaître l'adresse du client et de pouvoir connecter le socket du serveur au client.

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
	    fprintf(stderr, "An error occur in wait_for_client with recfrom: %s \n", strerror(errno));
	    return -1;
	}

	// 2. Connecter le socket du serveur au client via connect()
	int connexion = connect(sfd, (struct sockaddr *) &client_addr,addrlen);
	// int connect(int socket, const struct sockaddr *address, socklen_t address_len);
	
	if(connexion == -1) // Si ca plante
	{
		fprintf(stderr, "An error occur in wait_for_client with connect: %s \n", strerror(errno));
		return -1;  
	}
	
	return 0;
}




int main()
{
	// 1. test implémentation real_address()
	struct sockaddr_in6 rslt;
	const char * monAdresse ="2a02:a03f:3afd:1600:73ba:ddbe:a4b1:9e3a";
	real_address(monAdresse, &rslt); 
	//const char * real_address(const char *address, struct sockaddr_in6 *rval)

	
/*struct sockaddr_in6 
{
	sa_family_t     sin6_family;    AF_INET6 
	in_port_t       sin6_port;      port number 
	uint32_t        sin6_flowinfo;  IPv6 flow information 
	struct in6_addr sin6_addr;      IPv6 address 
	uint32_t        sin6_scope_id;  Scope ID (new in 2.4)
};
*/

	printf("%s",rslt.sin6_addr->s6_addr);
}



