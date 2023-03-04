#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "socket.h"
#include "utils.h"
#include "packet_interface.h"
#include "packet_utils.h"

#define MAX_SEQ_NUM 256

// Command parameters
char* format = "%d"; // default file format
int connections = 1; // default value 
char* hostname;
int port;

// Global variables
int iout = 0; //number of output file

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

int selective_repeat_receiver(int sock){
    
    int window_size = 3;
    int free = window_size;
    int window_start = 0;
    int window_end = window_size-1;
    int last_ack = -1; // last in-sequence data packet received
    pkt_t* buffer[window_size];
    for(int i =0; i< window_size; i++){
        buffer[i] = NULL;
    }

    pkt_t* data_packet;
    pkt_t* ack_packet;
    int rslt;
    FILE* fp  = NULL;
    bool end = false;
    bool first_time = true;
    while(!end){
        
        // Retrieve address of emettor
        struct sockaddr_in6 src_addr;
        socklen_t addrlen = sizeof(struct sockaddr_in6);
        ssize_t rec = recvfrom(sock, NULL, 0, MSG_PEEK,(struct sockaddr *) &src_addr, &addrlen);
        if(rec == -1){
            error(rec, "Cannot retrieve address of emettor: %s \n");
            return -1;
        }

        data_packet = pkt_new();
        rslt = receive_pkt(sock, data_packet);
        if(rslt ==-1){
            error(rslt, "Cannot receive packet");
            return -1;
        }
        print_data(data_packet);

        int seqnum = pkt_get_seqnum(data_packet);

        fprintf(stderr, "Low window: %d\n", window_start);
        fprintf(stderr, "High window: %d\n", window_end);
        fprintf(stderr, "Last ack: %d\n", last_ack);
        

        // if packet is inside the receiving window
        if( is_in_window(seqnum, window_start, window_end) && pkt_get_type(data_packet) == PTYPE_DATA){
            
            // if packet received is the next expected
            if(seqnum == ( (last_ack +1)%MAX_SEQ_NUM) ){

                // Open a file on first packet received
                if( first_time ){ 
                    fprintf(stderr, "CREATE NEW FILE \n");         
                    rslt = create_file(format, &iout, &fp);
                    if(rslt !=1){
                        error(rslt, "Cannot create file");
                        return -1;
                    }   
                    first_time = false;
                }

                // process packet
                fprintf(stderr, "PACKET %d PROCESSING \n", seqnum);
                fwrite(pkt_get_payload(data_packet), sizeof(char), pkt_get_length(data_packet), fp);
                fprintf(stderr, "PACKET %d PROCESSED \n", seqnum);
                
                last_ack = (last_ack + 1)%MAX_SEQ_NUM; 
                // slide window
                window_start = (window_start + 1)%MAX_SEQ_NUM; 
                window_end = (window_end +1)%MAX_SEQ_NUM;
                
                // last packet received => end connection
                if(pkt_get_length(data_packet) == 0){
                    fprintf(stderr, "LAST PACKET %d RECEIVED \n", seqnum);
                    end = true;
                }
            } 

            // if packet received is NOT the next expected
            else{
                // buffer packet
                fprintf(stderr, "PACKET %d BUFFERED \n", seqnum);
                buffer[seqnum%window_size] = data_packet; 
                free--;       
            }

            // check buffer for in-order packets
            // remove all consecutive packets starting at lastack (if any) from the receive buffer
            for(int i = 0; i< window_size; i++){
                if(buffer[i] != NULL && pkt_get_seqnum(buffer[i]) == last_ack +1){
                    
                    // process buffered packet
                    fprintf(stderr, "BUFFERED PACKET %d PROCESSING \n", pkt_get_seqnum(buffer[i]));
                    fwrite(pkt_get_payload(data_packet), sizeof(char), pkt_get_length(data_packet), fp);
                    fprintf(stderr, "BUFFERED PACKET %d PROCESSED \n", pkt_get_seqnum(buffer[i]));
                    // delete packet
                    pkt_del(buffer[i]);
                    buffer[i] = NULL;
                    // update variables
                    free++;
                    last_ack = (last_ack + 1)%MAX_SEQ_NUM;
                    // slide window
                    window_start = (window_start + 1)%MAX_SEQ_NUM;
                    window_end = (window_end +1)%MAX_SEQ_NUM;                        
                    
                    if(pkt_get_length(buffer[i]) == 0){
                        // last packet received
                        fprintf(stderr, "LAST PACKET %d RECEIVED \n", pkt_get_seqnum(buffer[i]));                        
                        end = true;
                    }
                }
            }

            print_buffer( (pkt_t**)&buffer, window_size);

            // acknowledge the last in-sequence received packet
            ack_packet = pkt_new();
            rslt = create_ack_pkt( free, last_ack, pkt_get_timestamp(data_packet), ack_packet);
            if(rslt == -1){
                error(rslt, "Cannot create acknowledgment");
                return -1;
            } 
            
            rslt = send_pkt(sock, src_addr, ack_packet);
            if(rslt == -1){
                error(rslt, "Cannot send acknowledgment");
                return -1;
            }
            print_ack(ack_packet);
            pkt_del(ack_packet);
            pkt_del(data_packet); //?
        }
        // if packet is outside the receiving window
        else{ 
            // discard the packet
            fprintf(stderr, "PACKET %d DISCARDED \n", seqnum);
            pkt_del(data_packet);
        }        
    }
    fclose(fp);
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

    rslt = selective_repeat_receiver(sfd);
    if(rslt == -1){
        error(rslt, "Selective repeat receiver");
        free(hostname);
        close(sfd);
        return EXIT_FAILURE;
    }else{
        free(hostname);
        close(sfd);
        return EXIT_SUCCESS;
    }
}

/* ======================================================
    // selective-repeat receiver
    // int receiving_window_size = ;
    int buffer[5]; // receiving buffer 
    int last_ack = 0; // last in-sequence packet that is has received

    // if packet is inside the receiving window
    if(){

        // place the packet inside the buffer 

        // remove all consecutive packets starting at lastack (if any) from the receive buffer

        // update lastack and receiving window

        // acknowledge the last packet received in sequence

    }

    // if packet outside the receiving window
    else{ 
        
        // discard the packet

        // acknowledge the last in-sequence packet received

    }

*/