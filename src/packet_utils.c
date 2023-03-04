#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "packet_interface.h"
#include "utils.h"



int min(int a, int b){
    return a > b ? b: a;
}
int max(int a, int b){
    return a > b ? a: b;
}

void print_data(pkt_t* pkt){
    fprintf(stderr," ================= \n"); 
	fprintf(stderr, "type=%u, tr=%u, window=%u, length=%u, seqnum=%u, timestamp=%u, crc1=%u, crc2=%u, payload=%s \n", pkt_get_type(pkt),pkt_get_tr(pkt),pkt_get_window(pkt),pkt_get_length(pkt),pkt_get_seqnum(pkt),pkt_get_timestamp(pkt),pkt_get_crc1(pkt),pkt_get_crc2(pkt), pkt_get_payload(pkt));
    fprintf(stderr," ================= \n"); 
}

void print_ack(pkt_t* pkt){
    fprintf(stderr," ================= \n"); 
	fprintf(stderr, "type=%u, window=%u, seqnum=%u, timestamp=%u, crc1=%u \n", pkt_get_type(pkt),pkt_get_window(pkt),pkt_get_seqnum(pkt),pkt_get_timestamp(pkt),pkt_get_crc1(pkt));
    fprintf(stderr," ================= \n"); 
}

void print_buffer(pkt_t** buffer, int buffer_size){
    fprintf(stderr,"buffer: \n");
    for(int i=0; i< buffer_size; i++){
        if(buffer[i] == NULL){
            fprintf(stderr, "\t%d: packet NULL, \n", i);
        }
        else{
            fprintf(stderr, "\t%d: packet %d, \n", i, pkt_get_seqnum(buffer[i]) );
        }
    }
    printf("\n");
}

int create_data_pkt(uint8_t tr, uint8_t window, uint16_t length, uint8_t seqnum, uint32_t timestamp, char* payload, pkt_t* packet){
    
    int rslt = pkt_set_type(packet, PTYPE_DATA);
    if( rslt != PKT_OK){
        return -1;
    }
    rslt = pkt_set_tr(packet, tr);
    if( rslt != PKT_OK){
        return -1;
    }
    rslt = pkt_set_window(packet, window);
    if( rslt != PKT_OK){
        return -1;
    }
    rslt = pkt_set_length(packet, length);
    if( rslt != PKT_OK){
        return -1;
    }
    rslt = pkt_set_seqnum(packet, seqnum);
    if( rslt != PKT_OK){
        return -1;
    }
    rslt = pkt_set_timestamp(packet, timestamp);     
    if( rslt != PKT_OK){
        return -1;
    }
    rslt = pkt_set_payload(packet, payload, length);
    if( rslt != PKT_OK){
        return -1;
    }
    return 1;
}

int create_ack_pkt(uint8_t window, uint8_t seqnum, uint32_t timestamp, pkt_t* packet){
    int rslt = pkt_set_type(packet, PTYPE_ACK);
    if( rslt != PKT_OK){
        error(rslt, "type");
        return -1;
    }
    rslt = pkt_set_window(packet, window);
    if( rslt != PKT_OK){
        error(rslt, "window");
        return -1;
    }
    rslt = pkt_set_length(packet,0);
    if( rslt != PKT_OK){
        error(rslt, "length");
        return -1;
    }
    rslt = pkt_set_seqnum(packet, seqnum);
    if( rslt != PKT_OK){
        error(rslt, "seqnum");
        return -1;
    }
    rslt = pkt_set_timestamp(packet, timestamp);     
    if( rslt != PKT_OK){
        error(rslt, "timestamp");
        return -1;
    }
    return 11;
}

int is_in_window( int seqnum, int low_window, int high_window){
    if(low_window <= high_window){
        return seqnum >= low_window && seqnum <= high_window;
    }else{ // low_window > high_window
        return (low_window <= seqnum && seqnum < MAX_SEQ_NUM)  || ( 0 <= seqnum && seqnum <= high_window); 
    }
}

int send_pkt(int sock, struct sockaddr_in6 addr, pkt_t* packet){

    size_t len = predict_header_length(packet) + 4 + pkt_get_length(packet) +  4;
    char* buf = (char*) malloc(len);
    if(buf == NULL){
        error(-1, "Cannot allocate memory for buffer to encode packet");
        return -1;
    }
    int rslt = pkt_encode(packet, buf, &len);
    if( rslt != PKT_OK){
        error(-1, "Cannot encode packet");
        return -1;
    }
    
    int status = sendto(sock, (void*) buf, len , 0, (struct sockaddr*) &addr, sizeof(addr));
    if (status == -1) {
        error(-1, "Cannot send packet");
        pkt_del(packet);
        close(sock);
		return -1;
    }
    return 1;
}

int send_hello(int sock, struct sockaddr_in6 addr){
    char message[] = "Hello, World !";
    int status = sendto(sock, message, strlen(message), 0, (struct sockaddr*) &addr, sizeof(addr));
    if (status == -1) {
        // error sending message
        error(-1, "Cannot send message");
        close(sock);
		return -1;
    }
    return 1;
}

int receive_pkt(int sock, pkt_t* packet){
    // allocate a buffer of MAX_MESSAGE_SIZE bytes
    char buf[MAX_MESSAGE_SIZE]; 
    // read socket
    ssize_t len = recv(sock, buf, MAX_MESSAGE_SIZE, 0);
    //fprintf(stderr,"received %ld bytes\n", len);

    // convert into pkt_t
    int rslt = pkt_decode(buf, len, packet);
    if( rslt != PKT_OK){
        return EXIT_FAILURE;
    }

    return 1;
}

int receive(int sock){
    // allocate a buffer of MAX_MESSAGE_SIZE bytes
    char buffer[MAX_MESSAGE_SIZE]; 
    ssize_t n_received = recv(sock, buffer, MAX_MESSAGE_SIZE, 0);
    if (n_received == -1) {
        error(n_received,"Error received message");
        return -1;
    }

    // let's print what we received !
    //printf("received %ld bytes:\n", n_received);
    for (int i = 0 ; i < n_received ; i++) {
        //printf("0x%hhx ('%c') ", buffer[i], buffer[i]);
        printf("%c", buffer[i]);
        
    }
    printf("\n");
    return n_received;
}

int receive_from(int sock){
    // allocate a buffer of MAX_MESSAGE_SIZE bytes
    char buffer[MAX_MESSAGE_SIZE]; 
    
    // source address of the message
    struct sockaddr_storage src_addr;
    socklen_t addrlen = sizeof(struct sockaddr_storage);

    ssize_t n_received = recvfrom(sock, buffer, MAX_MESSAGE_SIZE, 0, (struct sockaddr*) &src_addr, &addrlen); //ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    if(n_received == -1){
        error(n_received, "Error received message");
        return -1;
    }

    // let's print what we received !
    printf("received %ld bytes:\n", n_received);
    for (int i = 0 ; i < n_received ; i++) {
        //printf("0x%hhx ('%c') ", buffer[i], buffer[i]);
        printf("%c", buffer[i]);
    }
    printf("\n");
    
    // let's now print the address of the peer
    uint8_t *peer_addr_bytes = (uint8_t *) &src_addr;
    printf("the socket address of the peer is (%d bytes):\n", addrlen);
    for (uint i = 0 ; i < addrlen ; i++) {
        printf("0x%hhx ", peer_addr_bytes[i]);
    }
    printf("\n");
    return n_received;
}



