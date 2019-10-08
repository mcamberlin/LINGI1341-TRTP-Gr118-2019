#include <stdio.h>
#include <sys/socket.h>


int recv_and_handle_message(const struct sockaddr *src_addr, socklen_t addrlen) {
    // TODO: Create a IPv6 socket supporting datagrams
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if(sock==-1)
    {
        return -1;
    }
    
    // TODO: Bind it to the source
    int bound = bind(sock, src_addr, addrlen);
    if(bound==-1)
    {
        return -1;
    }
    
    // TODO: Receive a message through the socket
    void* buf = calloc(256,4);
    if(buf==NULL)
    {
        return -1;
    }
    
    struct sockaddr addr;
    socklen_t len = sizeof(addr);
    
    int received = recvfrom(sock, buf, 1024, 0, &addr, &len);
    if(received==-1)
    {
        return -1;
    }
    
    // TODO: Perform the computation
    int sum=0;
    for(int i=0; i< (int) (1024/sizeof(int)); i++)
    {
        int* pt = (int*) (buf+i*4);
        sum += *pt;
    }
    
    // TODO: Send back the result
    int sent = sendto(sock, &sum, sizeof(int), 0, &addr, len);
    if(sent==-1)
    {
        return -1;
    }
    
    free(buf);
    
    return 0;
}
