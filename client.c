#include "commom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <float.h>

#define BUFSZ 1024

// Estrutura para passar informações para a thread
struct client_args {
    int socket;
    struct sensor_message msg;
    float min_value;
    float max_value;
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
        // Gerar nova medição dentro do intervalo especificado
        args->msg.measurement = generate_random(args->min_value, args->max_value);

        // Enviar mensagem ao servidor
        send(args->socket, &args->msg, sizeof(args->msg), 0);

        sleep(args->interval);
    }

    pthread_exit(NULL);
}

// Função para validar os argumentos passados
void validate_args(int argc, char **argv) {
    if (argc < 7) {
        fprintf(stderr, "Error: Invalid number of arguments\n");
        fprintf(stderr, "Usage: ./client <server_ip> <port> -type <temperature|humidity|air_quality> -coords <x> <y>\n");
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

    if (argc != 8) {
        fprintf(stderr, "Error: Invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }

    int x = atoi(argv[6]);
    int y = atoi(argv[7]);

    if (x < 0 || x > 9 || y < 0 || y > 9) {
        fprintf(stderr, "Error: Coordinates must be in the range 0-9\n");
        exit(EXIT_FAILURE);
    }
}

// Função para calcular a distância euclidiana
float calculate_distance(int x1, int y1, int x2, int y2) {
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

// Função para atualizar a medição com base na fórmula
float update_measurement(float current, float remote, float distance) {
    return current + 0.1 * (1 / (distance + 1)) * (remote - current);
}

// Função para limitar a medição dentro do intervalo permitido
float clamp_measurement(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Função para encontrar as três menores distâncias
void update_closest_neighbors(float neighbors[3], float distance) {
    if (distance < neighbors[0] || neighbors[0] == FLT_MAX) {
        neighbors[2] = neighbors[1];
        neighbors[1] = neighbors[0];
        neighbors[0] = distance;
    } else if (distance < neighbors[1] || neighbors[1] == FLT_MAX) {
        neighbors[2] = neighbors[1];
        neighbors[1] = distance;
    } else if (distance < neighbors[2] || neighbors[2] == FLT_MAX) {
        neighbors[2] = distance;
    }
}

int main(int argc, char **argv) {
    validate_args(argc, argv);
    float closest_neighbors[3] = {FLT_MAX, FLT_MAX, FLT_MAX};

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

    // Configurar o intervalo de medição e intervalo entre mensagens
    float min_value, max_value;
    int interval;

    if (strcmp(msg.type, "temperature") == 0) {
        min_value = 20.0;
        max_value = 40.0;
        interval = 5;
    } else if (strcmp(msg.type, "humidity") == 0) {
        min_value = 10.0;
        max_value = 90.0;
        interval = 7;
    } else if (strcmp(msg.type, "air_quality") == 0) {
        min_value = 15.0;
        max_value = 30.0;
        interval = 10;
    } else {
        fprintf(stderr, "Error: Invalid sensor type\n");
        exit(EXIT_FAILURE);
    }

    // Criar thread para enviar mensagens
    struct client_args args = {s, msg, min_value, max_value, interval};
    pthread_t tid;
    pthread_create(&tid, NULL, send_messages, &args);

    // Receber mensagens do servidor
    struct sensor_message recv_msg;
    while (recv(s, &recv_msg, sizeof(recv_msg), 0) > 0) {
        char action[50];
        float distance = calculate_distance(msg.coords[0], msg.coords[1], recv_msg.coords[0], recv_msg.coords[1]);

        // Tratamento das mensagens recebidas
        if (recv_msg.measurement == -1.0) {
            strcpy(action, "removed");
        } else if (distance == 0.0) {
            strcpy(action, "same location");
        } else if (distance > closest_neighbors[2]) {
            strcpy(action, "not neighbor");
        } else {
            float correction = update_measurement(msg.measurement, recv_msg.measurement, distance) - msg.measurement;
            msg.measurement += correction;
            msg.measurement = clamp_measurement(msg.measurement, min_value, max_value);
            snprintf(action, sizeof(action), "correction of %.4f", correction);
        }

        // Log da mensagem recebida
        printf("log:\n%s sensor in (%d,%d)\nmeasurement: %.4f\naction: %s\n\n",
               recv_msg.type, recv_msg.coords[0], recv_msg.coords[1], recv_msg.measurement, action);

        update_closest_neighbors(closest_neighbors, distance);
    }

    close(s);
    return 0;
}
