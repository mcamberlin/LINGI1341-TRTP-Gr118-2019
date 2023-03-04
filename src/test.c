//gcc -o test utils.c socket.c packet_implem.c packet_utils.c test.c -lz -Wall -Werror && ./test :: 12345
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "packet_interface.h"
#include "packet_utils.h"
#include "socket.h"
#include "utils.h"

int test_data_pkt(pkt_t* packet, char* payload, uint8_t seqnum){
    if(pkt_get_type(packet) != PTYPE_DATA){
        error(-1, "type");
        return -1;
    }else if(pkt_get_tr(packet) != 0){
        error(-1, "tr");
        return -1;
    }else if(pkt_get_window(packet) != 3){
        error(-1, "window");
        return -1;
    }else if(pkt_get_length(packet) != strlen(payload) +1){
        error(-1, "length");
        return -1;
    }else if(pkt_get_seqnum(packet) != seqnum){
        error(-1, "seqnum");
        return -1;
    }else if(pkt_get_timestamp(packet) != 3735928559){
        error(-1, "timestamp");
        return -1;
    }else if(strcmp(pkt_get_payload(packet), payload) != 0){
        error(-1, "payload");
        return -1;
    }
    return 1;
}

int test_ack_pkt(pkt_t* packet, uint8_t window, uint8_t seqnum, uint8_t timestamp){
    if(pkt_get_type(packet) != PTYPE_ACK){
        error(-1, "type");
        return -1;
    }else if(pkt_get_tr(packet) != 0){
        error(-1, "tr");
        return -1;
    }else if(pkt_get_window(packet) != window){
        error(-1, "window");
        return -1;
    }else if(pkt_get_length(packet) != 0){
        error(-1, "length");
        return -1;
    }else if(pkt_get_seqnum(packet) != seqnum){
        error(-1, "seqnum");
        return -1;
    }else if(pkt_get_timestamp(packet) != timestamp){
        error(-1, "timestamp");
        return -1;
    }
    return 1;
}

int encode_decode_data_test(){
    char payload[] = "Hello, World !";
    pkt_t* src_packet = pkt_new();
    create_data_pkt(0, 3, strlen(payload)+1, 0, 3735928559, payload, src_packet);
    
    size_t len = predict_header_length(src_packet) + 4 + pkt_get_length(src_packet) +  4;
    char* buf = (char*) malloc(len);
    if(buf == NULL){
        error(-1, "malloc");
        return -1;
    }
    int rslt = pkt_encode(src_packet, buf, &len);
    if( rslt != PKT_OK){
        return -1;
    }

    pkt_t* dest_packet = pkt_new();
    rslt = pkt_decode(buf, len, dest_packet);
    if( rslt != PKT_OK){
        return -1;
    }

    rslt = test_data_pkt(dest_packet, payload, 0);
    if(rslt != 1){
        return -1;
    } else {
        return 1;
    }
}

int encode_decode_ack_test(){
    pkt_t* src_packet = pkt_new();
    int rslt = create_ack_pkt(3, 0, 0, src_packet);
    if(rslt == -1){
        error(rslt, "create");
        return -1;
    }

    size_t len = 11;
    char* buf = (char*) malloc(len);
    if(buf == NULL){
        error(-1, "malloc");
        return -1;
    }
    rslt = pkt_encode(src_packet, buf, &len);
    if( rslt != PKT_OK){
        error(-1, "encode");
        return -1;
    }

    pkt_t* dest_packet = pkt_new();
    rslt = pkt_decode(buf, len, dest_packet);
    if( rslt != PKT_OK){
        error(-1, "decode");
        return -1;
    }

    rslt = test_ack_pkt(dest_packet, 3, 0, 0);
    if(rslt != 1){
        return -1;
    } else {
        return 1;
    }
}

void window_test(int fd, struct sockaddr_in6 addr){
    char payload[] = "Hello World !";
    for(int i=0; i < 258; i++){
        pkt_t* packet = pkt_new();
        create_data_pkt(0, 3, strlen(payload)+1, i, 0, payload, packet);
        send_pkt(fd, addr, packet);
    }
}

/**
 * Not in-sequence sending with window = 3
*/
void unorder_test(int fd, struct sockaddr_in6 addr){
    char payload[] = "Hello  World!";

    pkt_t* packet = pkt_new();
    create_data_pkt(0, 3, strlen(payload)+1, 2, 0, payload, packet);
    send_pkt(fd, addr, packet);

    packet = pkt_new();
    create_data_pkt(0, 3, strlen(payload)+1, 1, 0, payload, packet);
    send_pkt(fd, addr, packet);

    packet = pkt_new();
    create_data_pkt(0, 3, strlen(payload)+1, 3, 0, payload, packet);
    send_pkt(fd, addr, packet);

    packet = pkt_new();
    create_data_pkt( 0, 3, strlen(payload)+1, 0, 0, payload, packet);
    send_pkt(fd, addr, packet);

    packet = pkt_new();
    create_data_pkt(0, 3, strlen(payload)+1, 3, 0, payload, packet);
    send_pkt(fd, addr, packet);

}



int ack_test(int fd, struct sockaddr_in6 addr){

    pkt_t* ack_packet = pkt_new();
    create_ack_pkt(3, 0, 0, ack_packet);
    send_pkt(fd, addr, ack_packet);
    print_ack(ack_packet);
    
    
    sleep(3);

    
    ack_packet = pkt_new();
    create_ack_pkt(1, 2, 2, ack_packet);
    send_pkt(fd, addr, ack_packet);
    sleep(3);
    
    ack_packet = pkt_new();
    create_ack_pkt(0, 2, 2, ack_packet);
    send_pkt(fd, addr, ack_packet);
    sleep(3);

    ack_packet = pkt_new();
    create_ack_pkt(3, 3, 3, ack_packet);
    send_pkt(fd, addr, ack_packet);
    sleep(3);

    ack_packet = pkt_new();
    create_ack_pkt(1, 6, 6, ack_packet);
    send_pkt(fd, addr, ack_packet);
    sleep(3);
   
    return 1;
}

// =====================================================================
// Global variables
char* format = "%d"; // default file format
int connections = 1; // default value 
char* hostname;
int port;

int parse_commands(int argc, char *argv[]){

    // code to handle option arguments
    int c;
    while ((c = getopt(argc, argv, "o:m:")) != -1) {
        switch (c) {
            case 'o':
                format = optarg;
                fprintf(stderr, "-o with value '%s'\n", optarg);
                break;
            case 'm':
                if( (connections = atoi(optarg)) == 0) {
                    error(connections, "Error converting connections number to an integer");
                    return -1;
                }
                fprintf(stderr, "-c with value '%s'\n", optarg);
                break;
            case '?':
                // code to handle invalid options
                break;
            default:
                // code to handle other options
                break;
        }
    }

    // check if correct number of remaining args
    int index = optind;
    if( (argc - index) != 2){
        fprintf(stderr,"Wrong number of args \n");
        return -1;
    }

    // code to handle non-option arguments
    // hostname
    hostname = (char*) malloc(sizeof(argv[index]));
    if(hostname == NULL){
        error(-1,"Cannot allocate memory for hostname");
        return -1;
    }
    strcpy(hostname, argv[index]);
    fprintf(stderr, "hostname: '%s'\n", hostname);
    index++;

    // port
    if( (port = atoi(argv[index])) == 0 ){
        error(port, "Error converting port number to an integer");
        free(hostname);
        return -1;
    }
    fprintf(stderr, "port: '%d'\n", port); 
    return 1;
}

int main(int argc, char *argv[]){
    int rslt = parse_commands(argc, argv);
	if( rslt != 1){
        error(rslt, "Cannot parse command-line options");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "%d\n", encode_decode_data_test());
    return 1;
    /*
    // Resolve hostname 
	struct sockaddr_in6 addr;	
	rslt = resolve(hostname, port, &addr);
	if (rslt != 1){
		error(rslt, "Cannot resolve hostname");
        free(hostname);
		return EXIT_FAILURE;
	}

    // Create a socket
	int sfd = create_socket(); 
	if(sfd == -1){
        error(sfd, "Cannot create socket");
        free(hostname);
		return EXIT_FAILURE;	
	}

    // Bind socket
    rslt = bind_socket(sfd, &addr);
    if(rslt == -1){
        error(rslt, "Cannot bind socket");
        free(hostname);
        close(sfd);
		return EXIT_FAILURE;	
    }

    // Retrieve address of emettor
    struct sockaddr_in6 src_addr;
    socklen_t addrlen = sizeof(struct sockaddr_in6);
    ssize_t rec = recvfrom(sfd, NULL, 0, MSG_PEEK,(struct sockaddr *) &src_addr, &addrlen);
    if(rec == -1){
        error(rec, "Cannot retrieve address of emettor: %s \n");
        return -1;
    }

    rslt = ack_test(sfd, src_addr);
    if(rslt == -1){
        return EXIT_FAILURE;
    }else{
        return EXIT_SUCCESS;
    }
    */
}
