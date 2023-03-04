#ifndef __PACKET_UTILS_H_
#define __PACKET_UTILS_H_

#include "packet_interface.h"

int min(int a, int b);

int max(int a, int b);


void print_data(pkt_t* pkt);

void print_ack(pkt_t* pkt);

void print_buffer(pkt_t** buffer, int buffer_size);

int create_data_pkt(uint8_t tr, uint8_t window, uint16_t length, uint8_t seqnum, uint32_t timestamp, char* payload, pkt_t* packet);

int create_ack_pkt(uint8_t window, uint8_t seqnum, uint32_t timestamp, pkt_t* packet);

int is_in_window( int seqnum, int low_window, int high_window);

int send_pkt(int sock, struct sockaddr_in6 addr, pkt_t* packet);

int receive_pkt(int sock, pkt_t* packet);

int receive_pkt_from(int sock, pkt_t* packet, struct sockaddr_in6 src_addr);

// ============== TEST FUNCTIONS ====================
void window_test(int fd, struct sockaddr_in6 addr);

void unorder_test(int fd, struct sockaddr_in6 addr);


#endif