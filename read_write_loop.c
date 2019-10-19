#include "packet_interface.h"
#include <stdio.h> 
#include <stdlib.h> // pour calloc, malloc, free
#include <string.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <stdint.h>


/**
 * @pre - Buffer = pointeur vers un buffer representant le 
 *
 *
*/
#include <stdio.h> // pour fread() et fwrite()
#include <sys/types.h> // pour recvfrom(), pour connect() et les 3 prochains includes pour getaddrinfo()
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>  // Pour memset()
#include <poll.h> // pour pollfd
#include <unistd.h> // pour read() et pour close()
#include <errno.h>
pkt_status_code read_write_loop(int sfd)
{
	const int MAXSIZE = 1024;
	char buffer_socket[MAXSIZE];
	char buffer_stdin[MAXSIZE];
	
	// Boucle pour lire un socket et imprimer sur stdout, tout en lisant stdin et en écrivant sur le socket
	while(1)
	{
		struct pollfd pfd;
		pfd.fd = sfd; 
		pfd.events = POLLIN | POLLOUT; // Data other than high-priority data may be read without blocking.

		struct pollfd filedescriptors[1];
		filedescriptors[0] = pfd;

		nfds_t nfds = 1;
		int timeout = -1; //poll() shall wait for an event to occur 
		
		int p = poll(filedescriptors,nfds, timeout); 
		// int poll(struct pollfd fds[], nfds_t nfds, int timeout);
		if(p == -1)
		{
			//fprintf(stderr, "An error occur: %s \n", strerror(errno));
			return;
		}

		if(pfd.revents == POLLIN) // There is data to read
		{
			//1. Lire le socket
            memset(buffer_socket,0,MAXSIZE);
			ssize_t r_socket = read(sfd, buffer_socket, MAXSIZE); //ssize_t read(int fd, void *buf, size_t count)
			if(r_socket == -1 || r_socket > MAXSIZE)
			{
				//fprintf(stderr, "Erreur lecture socket : %s \n", strerror(errno));
				return;
			}
			if(r_socket == 0)
			{
				//fprintf(stderr, "Fin de la lecture du socket atteinte \n");
				return;
			}
			// 2. TRANSFORMER le buffer en un paquet avec decode
			pkt_t* pkt_recu = pkt_new();
			
			pkt_status_code = pkt_decode();
			






	
		}
		if(pfd.revents == POLLOUT) // Writing is now possible
		{
			// 1. Remettre les buffers à 0 avant d'écrire dedans:
			memset(buffer_stdin,0,MAXSIZE);
			// 2. Lire le contenu de l'entrée standard	
			int r_stdin = read(0, buffer_stdin, MAXSIZE); 
			//ssize_t read(int fd, void *buf, size_t count)
			// le fd = 0 correspond à stdin
			if(r_stdin == -1 || r_stdin > MAXSIZE)
			{
				//fprintf(stderr, "Erreur lecture entrée standard : %s \n",strerror(errno));
				return;
			}
			if(r_stdin == 0)
			{
				//fprintf(stderr, "Fin de la lecture de l'entrée standard \n");
				return;
			}

			// 2. L'écrire dans le socket
			int w_socket = write(sfd, buffer_stdin, MAXSIZE); //ssize_t write(int fd, const void *buf, size_t count);  	
			if(w_socket == -1)
			{
				//fprintf(stderr,"An error occured while writing on the socket \n");
				return;
			}
        }
}

