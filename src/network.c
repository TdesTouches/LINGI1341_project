#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <unistd.h>

#include "network.h"

#define MAX_SEGMENT_SIZE 1024


const char * real_address(const char *address, struct sockaddr_in6 *rval){
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo; // output of getaddrinfo

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6; // only IPV6
    hints.ai_socktype = SOCK_DGRAM; // tcp
    hints.ai_protocol = IPPROTO_UDP; // udp protocol
    hints.ai_flags = AI_PASSIVE; // fill in my Ip for me

    status = getaddrinfo(address, NULL, &hints, &servinfo);
    if(status != 0){
        const char* error = gai_strerror(status);
        return error;
    }

    if(servinfo->ai_addr != NULL){
        memcpy(rval, servinfo->ai_addr, sizeof(struct sockaddr_in6));
    }

    freeaddrinfo(servinfo);
    return NULL;
}


int create_socket(struct sockaddr_in6 *source_addr,
                 int src_port,
                 struct sockaddr_in6 *dest_addr,
                 int dst_port){

    int sfd = socket(AF_INET6 , SOCK_DGRAM, IPPROTO_UDP); // udp and ipv6
    if(sfd==-1){
        fprintf(stderr, "The socket could not be created\n");
        return -1;
    }

    if(source_addr != NULL && src_port > 0){
        source_addr->sin6_port = htons(src_port);
        source_addr->sin6_family = AF_INET6;
        if(bind(sfd, (struct sockaddr *) source_addr, sizeof(struct sockaddr_in6)) != 0){
            fprintf(stderr, "Binding error : %s\n", strerror(errno));
            return -1;
        }
    }

    if(dest_addr != NULL && dst_port > 0){
        dest_addr->sin6_port = htons(dst_port);
        dest_addr->sin6_family = AF_INET6;
        if(connect(sfd, (struct sockaddr *) dest_addr, sizeof(*dest_addr)) !=0){
            fprintf(stderr, "Connect error : %s\n", strerror(errno));
            return -1;
        }
    }
    return sfd;
}


int wait_for_client(int sfd){
    char in[MAX_SEGMENT_SIZE];
    struct sockaddr_in6 from;
    int fromlen = sizeof(from);

    memset(&from, 0, sizeof(from));
    if(recvfrom(sfd, in, (size_t) MAX_SEGMENT_SIZE, MSG_PEEK, (struct sockaddr*) &from, (socklen_t*) &fromlen) ==-1){
        fprintf(stderr, "Error while recvfrom : %s\n", strerror(errno));
        return -1;
    }
    if(connect(sfd, (struct sockaddr*) &from, (socklen_t) fromlen) !=0){
        fprintf(stderr, "Error while connect() : %s\n", strerror(errno));
        return -1;
    }

    return 0;
}