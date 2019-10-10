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
    
    memset(&hints, 0, sizeof(struct addrinfo)); // void *memset(void *s, int c, size_t n);

/* Structure de données addrinfo :
struct addrinfo 
{
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
#include <unistd.h> // pour close()
 #include <errno.h>

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
		fprintf(stderr, "socket: %s , errno %d\n", strerror(fd_src), fd_src);
		return -1;
	}

	// 2. Lier le socket avec la source using the bind() system call
	
	size_t len_addr = sizeof(struct sockaddr_in6);                      
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
     
	close(fd_src);                                            // release the resources used by the socket
	return 0;    
}






#include <poll.h> // pour pollfd
#include <stdio.h> // pour fread() et fwrite()
#include <unistd.h> // pour read();


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
        pfd.events = POLLIN; // Jsp lequel mettre

	struct pollfd filedescriptors[1];
	filedescriptors[0] = pfd;

	nfds_t nfds = 1;
	int timeout = 500; //poll() shall wait at least 'timeout' [ms] for an event to occur on any of the selected file descriptors.
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
			//1. Lire le socket
			ssize_t r_socket = read(sfd, buffer_socket, sizeof(char)); //ssize_t read(int fd, void *buf, size_t count)
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
			
			// 2. Afficher sur la sortie standard
			fprintf(stdout,"%s",buffer_socket);	

			// 3. Lire l'entrée standard
			int r_stdin = read(0, buffer_stdin, sizeof(char)); //ssize_t read(int fd, void *buf, size_t count)
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
			}
			
			// 4. Ecrire dans le socket
			int w_socket = write(sfd, buffer_stdin, sizeof(buffer_stdin)); //ssize_t write(int fd, const void *buf, size_t count);  	
			if(w_socket == -1)
			{
				fprintf(stderr,"An error occured while writing on the socket \n");
				return;
			}
			
		}
	}		
}

