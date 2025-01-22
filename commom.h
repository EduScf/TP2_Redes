#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <arpa/inet.h>

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage);
void logexit(const char *msg);

// Definição da estrutura de mensagem do sensor
struct sensor_message {
    char type[12];        // Tipo do sensor (temperature, humidity, air_quality)
    int coords[2];        // Coordenadas do sensor no grid
    float measurement;    // Valor medido pelo sensor
};