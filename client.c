#include "commom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define BUFSZ 1024

// Estrutura para passar informações para a thread
struct client_args {
    int socket;
    struct sensor_message msg;
    int interval;
};

// Gera um valor aleatório dentro de um intervalo
float generate_random(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

// Função que envia mensagens periodicamente
void *send_messages(void *arg) {
    struct client_args *args = (struct client_args *)arg;

    while (1) {
        // Gerar nova medição
        args->msg.measurement = generate_random(20.0, 40.0); // Alterar conforme o tipo do sensor
        send(args->socket, &args->msg, sizeof(args->msg), 0);

        printf("Sent: %s sensor in (%d,%d) measurement: %.4f\n",
               args->msg.type, args->msg.coords[0], args->msg.coords[1], args->msg.measurement);

        sleep(args->interval);
    }

    pthread_exit(NULL);
}

// Função para validar os argumentos passados
void validate_args(int argc, char **argv) {
    if (argc < 7) {
        fprintf(stderr, "Error: Invalid number of arguments\n");
        fprintf(stderr, "Usage: ./client <server_ip> <port> -type <type> -coords <x> <y>\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[3], "-type") != 0) {
        fprintf(stderr, "Error: Expected '-type' argument\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[5], "-coords") != 0) {
        fprintf(stderr, "Error: Expected '-coords' argument\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    validate_args(argc, argv);

    // Inicializar o endereço do servidor
    struct sockaddr_storage storage;
    if (addrparse(argv[1], argv[2], &storage) != 0) {
        fprintf(stderr, "Error: Invalid address or port\n");
        exit(EXIT_FAILURE);
    }

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr *addr = (struct sockaddr *)&storage;
    if (connect(s, addr, sizeof(storage)) != 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server.\n");

    // Configurar a mensagem do sensor
    struct sensor_message msg;
    strncpy(msg.type, argv[4], sizeof(msg.type) - 1);
    msg.type[sizeof(msg.type) - 1] = '\0';

    msg.coords[0] = atoi(argv[6]);
    msg.coords[1] = atoi(argv[7]);

    // Configurar o intervalo com base no tipo do sensor
    int interval;
    if (strcmp(msg.type, "temperature") == 0) {
        interval = 5;
    } else if (strcmp(msg.type, "humidity") == 0) {
        interval = 7;
    } else if (strcmp(msg.type, "air_quality") == 0) {
        interval = 10;
    } else {
        fprintf(stderr, "Error: Invalid sensor type\n");
        exit(EXIT_FAILURE);
    }

    // Criar thread para enviar mensagens
    struct client_args args = {s, msg, interval};
    pthread_t tid;
    pthread_create(&tid, NULL, send_messages, &args);

    // Receber mensagens do servidor
    struct sensor_message recv_msg;
    while (recv(s, &recv_msg, sizeof(recv_msg), 0) > 0) {
        printf("Received: %s sensor in (%d,%d) measurement: %.4f\n",
               recv_msg.type, recv_msg.coords[0], recv_msg.coords[1], recv_msg.measurement);
    }

    close(s);
    return 0;
}
