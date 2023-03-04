#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zlib.h>

#include "socket.h"
#include "utils.h"
#include "packet_utils.h"


#define RETRANSMISSION_TIMEOUT 5
#define RETRANSMISSION_LIMIT 3

// Global variables
char* filename;
char* hostname;
int port;
char payload[] = "Hello World !";

int parse_commands(int argc, char *argv[]){

    // code to handle option arguments
    int c;
    while ((c = getopt(argc, argv, "f:")) != -1) {
        switch (c) {
            case 'f':
                fprintf(stderr, "-f with value '%s'\n", optarg);
                filename = (char*) malloc(sizeof(optarg));
                if(filename == NULL){
                    error(-1, "Cannot allocate memory for filename");
                    return -1;
                }
                strcpy(filename, optarg);
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
        free(filename);
        return -1;
    }

    // code to handle non-option arguments
    // hostname
    hostname = (char*) malloc(sizeof(argv[index]));
    if(hostname == NULL){
        error(-1,"Cannot allocate memory for hostname");
        free(filename);
        return -1;
    }
    strcpy(hostname, argv[index]);
    fprintf(stderr, "hostname: '%s'\n", hostname);
    index++;

    // port
    if( (port = atoi(argv[index])) == 0 ){
        error(port, "Error converting port number to an integer");
        free(filename);
        free(hostname);
        return -1;
    }
    fprintf(stderr, "port: '%d'\n", port); 
    
    return 1;
}

/**
 * Code to handle retransmission timeout

void retransmission_timeout_handler(int sig) {
    // retransmit unacknowledged packets

    for (int i = base; i < base + WINDOW_SIZE; i++) {
        if (sent[i] && !acked[i]) {
            send_packet(i);
        }
    }
}
*/
int selective_repeat_sender(int sock, struct sockaddr_in6 addr){
    
    // set retransmission timeout handler
    //signal(SIGALRM, retransmission_timeout_handler);

    int window_size = 1; // by default the sending window is equal to 1
    int window_start = 0;
    int window_end = window_size-1;
    int free = window_size; // nb of available place in the buffer
    int seqnum = 0;

    pkt_t* buffer[MAX_WINDOW_SIZE];
    for(int i =0; i< MAX_WINDOW_SIZE; i++){
        buffer[i] = NULL;
    }

    pkt_t* data_packet;
    pkt_t* ack_packet;
    int rslt;
    int k =0; // TODO: remove
    bool end = false;
    while(k<10 && !end){
        
        fprintf(stderr, "--- Sending phase ---\n");
        for(int i=0; i< window_size && free >0; i++){
            if(buffer[i] == NULL){
                // create packet
                data_packet = pkt_new();
                //int rslt = create_data_pkt(0, window_size, strlen(payload)+1, seqnum, (uint32_t) time(NULL), payload, data_packet);
                int rslt = create_data_pkt(0, window_size, strlen(payload)+1, seqnum, k, payload, data_packet);
                k++;
                if(rslt == -1){
                    error(rslt, "Cannot create data packet");
                    free(buffer);
                    pkt_del(data_packet);
                    return -1;
                }
                
                // store the packet in the buffer
                buffer[i] = data_packet; 
                free--;
                
                // send packet
                rslt = send_pkt(sock, addr, data_packet); fprintf(stderr, "PACKET %d SENT \n", seqnum); print_data(data_packet);
                if(rslt == -1){
                    error(rslt, "Cannot send packet");
                    free(buffer);
                    pkt_del(data_packet);
                    return -1;
                } 
                seqnum++;
            }
        }
        
        print_buffer((pkt_t**) buffer, window_size);
        /*
        fprintf(stderr, "--- Timer phase ---\n");
        // set retransmission timer for unacknowledged packets
        for (int i = base; i < next_seq_num; i++) {
            if (!acked[i]) {
                alarm(RETRANSMISSION_TIMEOUT);
                break;
            }
        }
        */

        fprintf(stderr, "--- Acknowledgment phase ---\n");
        ack_packet = pkt_new();
        rslt = receive_pkt(sock, ack_packet);
        if(rslt ==-1){
            error(rslt, "Cannot receive acknowledgment");
            free(buffer);
            pkt_del(data_packet);
            pkt_del(ack_packet);
            return -1;
        }
        print_ack(ack_packet);

        uint8_t ack_seqnum = pkt_get_seqnum(ack_packet);
        uint32_t ack_timestamp = pkt_get_timestamp(ack_packet);
        uint8_t new_window_size = pkt_get_window(ack_packet);
        pkt_del(ack_packet);

        rslt = resize_buffer(&window_size, &window_end, &buffer, new_window_size);
        if(rslt != 1){
            error(-1, "Cannot reallocate buffer");
            free(buffer);
            pkt_del(data_packet);
            return -1;
        }

        // if ack is inside the window
        if(is_in_window(ack_seqnum, window_start, window_end)){
            
            for(int i = 0; i < window_size; i++){

                if(buffer[i] != NULL && pkt_get_timestamp(buffer[i]) <= ack_timestamp){
                    fprintf(stderr, "PACKET %d REMOVED FROM BUFFER \n", pkt_get_seqnum(buffer[i]));
                    pkt_del(buffer[i]);
                    buffer[i] = NULL;
                    free++;
                }
            }
            

            // slide window
            window_start = (ack_seqnum + 1)%MAX_SEQ_NUM; fprintf(stderr, "Window start: %d\n", window_start);
            window_end = (window_start + free_receiver)%MAX_WINDOW_SIZE; fprintf(stderr, "Window end: %d\n", window_end);

            /*           
            // stop retransmission timer for acknowledged packets
            for (int i = base; i < next_seq_num; i++) {
                if (acked[i]) {
                    alarm(0);
                    break;
                }
            }
            */
        } 
        // Discard duplicated ack
        else{
            fprintf(stderr, "ACKNOWLEDGMENT %d DISCARDED \n", ack_seqnum);
            pkt_del(ack_packet);
        }
    }
    free(buffer);
    return 1;
}


int main (int argc, char *argv[])  {
    
    int rslt = parse_commands(argc, argv);
	if( rslt != 1){
        error(rslt, "Cannot parse command-line options");
        return EXIT_FAILURE;
    }

    // Resolve hostname 
	struct sockaddr_in6 addr;	
	rslt = resolve(hostname, port, &addr);
	if (rslt != 1){
		error(rslt, "Cannot resolve hostname");
        free(filename);
        free(hostname);
		return EXIT_FAILURE;
	}
    
    // Create a socket
	int fd = create_socket(); 
	if(fd == -1){
        error(fd, "Cannot create socket");
        free(filename);
        free(hostname);
		return EXIT_FAILURE;	
	}
    
    rslt = selective_repeat_sender(fd, addr);
    if(rslt == -1){
        error(rslt, "");
        free(filename);
        free(hostname);
        return EXIT_FAILURE;
    }
    
    free(filename);
    free(hostname);
    return EXIT_SUCCESS;
}



/* ===========================================================================
    // selective-repeat sender
    //int sending_window_size = 2**n/2; // where n is the number of bits on which sequence numbers are encoded
    
    int buffer[5]; // sending buffer

    // While buffer not full
    while(){
        
        // send frame
        
        // start timer
    
    }

    // if receive an acknowledgement
    if(){

        // stop corresponding timer

        // check last_ack received

        // remove from buffer all consecutive frames acknowledged until last_ack

        // update sending window

    }

    // TODO: count number of times a timer expires and stop if a threshold is reached
    // if a timer expires
    if(){

        // retransmit the frame

        // restart timer

    }
*/
