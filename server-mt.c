#include "commom.h"

//Coloque as bibliotecas padrões de C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFSZ 1024

void usage(int argc, char **argv){
    printf("usage: %s <v4|v6> <server ports>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

struct client_data{
    int csock;
    struct sockaddr_storage storage;
};

void *client_thread(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);

    char buf[BUFSZ];
    char response[BUFSZ]; // Buffer separado para a resposta
    while (1) {
        memset(buf, 0, BUFSZ);
        size_t count = recv(cdata->csock, buf, BUFSZ, 0);
        if (count == 0) {
            // Conexão encerrada pelo cliente
            printf("[log] connection closed by %s\n", caddrstr);
            break;
        }
        if (count == -1) {
            perror("recv");
            break;
        }

        printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

        // Construir a resposta no buffer separado, limitando o tamanho de buf
        snprintf(response, BUFSZ, "Echo from server: %.1000s", buf);
        count = send(cdata->csock, response, strlen(response) + 1, 0);
        if (count != strlen(response) + 1) {
            perror("send");
            break;
        }
    }

    close(cdata->csock);
    free(cdata);
    pthread_exit(EXIT_SUCCESS);
}


int main(int argc, char **argv){
    if(argc < 3){
        usage(argc, argv);
    }
    
    struct sockaddr_storage storage;
    if(0 != server_sockaddr_init(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0){
        logexit("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr *addr = (struct sockaddr *) &storage;
    if(0 != bind(s, addr, sizeof(storage))){
        logexit("bind");
        exit(EXIT_FAILURE);
    }

    if(0 != listen(s, 10)){
        logexit("listen");
        exit(EXIT_FAILURE);
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connection\n", addrstr);

    while(1){
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *) &cstorage;
        socklen_t caddrlen = sizeof(cstorage);
        int csock = accept(s, caddr, &caddrlen);
        if(csock == -1){
            logexit("accept");
        }    
        
        struct client_data *cdata = malloc(sizeof(*cdata));
        if(!cdata){
            logexit("malloc");
        }
        cdata->csock = csock;
        memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}