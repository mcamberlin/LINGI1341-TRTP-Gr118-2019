#ifndef __SOCKET_H_
#define __SOCKET_H_

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <stdlib.h>

/** Resolve the hostname
 * @hostname: 
 * @port: port number
 * @resolved: pointer to a structure containing the IPv6 address
 * @return: 1 on success, -1 in case of failure
 */
int resolve(const char* hostname, int port, struct sockaddr_in6 *resolved);

/** Create an IPv6 socket supporting datagrams
 * @return: the file descriptor corresponding to the created socket on success, -1 in case of failure
 */
int create_socket();

/** Bind the socket to a specific network address. 
 * This associates the socket with a specific IP address and port number,
 * allowing it to listen for incoming connections on that address and port.
 * @fd: file descriptor representing the socket to bind
 * @source_addr: source address that should be bound to this socket
 * @return: 1 on success, -1 in case of failure
 */
int bind_socket(int fd, struct sockaddr_in6* source_addr);

#endif
