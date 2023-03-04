#include "socket.h" 
#include "utils.h"

int resolve(const char* hostname, int port, struct sockaddr_in6* resolved){

    // Address attributes to retrieve
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6; // for IPv6
    hints.ai_socktype = SOCK_DGRAM; // for UDP
    hints.ai_flags = AI_CANONNAME; // to support Fully Qualified Domaine Name (FQDN)

	// Get address info
	struct addrinfo* results;
	int status = getaddrinfo(hostname, NULL, &hints, &results);
	if( status != 0) {
		fprintf(stderr,"Error resolving hostname %s\nReturned: %d\nErrno message: %s\n", hostname, status, gai_strerror(status));
		return -1;
	}
	
	fprintf(stderr, "Resolved %s to:\n", hostname);	
	for (struct addrinfo *rp = results; rp != NULL; rp = rp->ai_next) {
		char host[NI_MAXHOST];
		status = getnameinfo(rp->ai_addr, rp->ai_addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST);
		if (status != 0) {
			fprintf(stderr,"Error resolving address %s\nReturned: %d\nErrno message: %s\n", hostname, status, gai_strerror(status));
			continue;
		}
		fprintf(stderr, "- %s\n", host);
	}	
	
	// store first resolved address
    struct sockaddr_in6* address = (struct sockaddr_in6*) results->ai_addr;  
	memcpy(resolved, address, sizeof(struct sockaddr_in6));

	// Set port number of addr
	resolved->sin6_port = htons(port);

	freeaddrinfo(results);
	return 1;
}

int create_socket(){
	int fd = socket(AF_INET6, SOCK_DGRAM, 0); //int socket(int domain, int type, int protocol); 
	if (fd == -1){  
		error(fd, "Error in socket creation");
		return -1;
	}
	return fd;
}

int bind_socket(int fd, struct sockaddr_in6* addr){
	// Assign receiver address to the socket
	int rslt = bind(fd,(struct sockaddr*) addr, (socklen_t) sizeof(struct sockaddr_in6)); //int bind(int socket, const struct sockaddr *address, socklen_t address_len);
	if(rslt == -1){
		error(rslt,"Error in socket binding");
		return -1;
	}   
	return fd;    
}

