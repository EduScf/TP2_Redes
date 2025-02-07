#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <arpa/inet.h>

void logexit(const char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage){
    if(addrstr == NULL || portstr == NULL){
        return -1;
    }
    uint16_t port = (uint16_t) atoi(portstr);
    if(port == 0){
        return -1;
    }
    port = htons(port); // hosto to network short
    struct in_addr inaddr4; // IPv4 32-bit IP address
    if(inet_pton(AF_INET, addrstr, &inaddr4)){
        struct sockaddr_in *addr4 = (struct sockaddr_in *) storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }
    struct in6_addr inaddr6; /// IPv6 128-bit IP address
    if(inet_pton(AF_INET6, addrstr, &inaddr6)){
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        // addr6->sin6_addr = inaddr6;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(struct in6_addr));
        return 0;
    }
    return -1;
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize){

    int version;
    char addrstr[INET6_ADDRSTRLEN+1] = "";
    uint16_t port;
    if(addr == NULL || str == NULL){
        return;
    }
    if(addr->sa_family == AF_INET){
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
        if(!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN)){
            logexit("ntop");
        }
        port = ntohs(addr4->sin_port); // network to host short
    }else if(addr->sa_family == AF_INET6){
        const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6 *) addr;
        if(inet_ntop(AF_INET6, &(addr6->sin6_addr), str, strsize) == NULL){
            logexit("ntop");
        }
        port = ntohs(addr6->sin6_port); //network to host short
    }else{
        logexit("Unknown protocol family");
    }
    if(str){
    snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage){
    uint16_t port = (uint16_t) atoi(portstr);
    if(port == 0){
        return -1;
    }
    port = htons(port);
    memset(storage, 0, sizeof(*storage));
    if(strcmp(proto, "v4") == 0){
        struct sockaddr_in *addr4 = (struct sockaddr_in *) storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr.s_addr = INADDR_ANY;
    }else if(strcmp(proto, "v6") == 0){
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        addr6->sin6_addr = in6addr_any;
    }else{
        return -1;
    }
    return 0;
}