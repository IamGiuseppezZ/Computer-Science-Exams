#ifndef SENSOR_PACKET_H
#define SENSOR_PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

#define LOCALHOST "127.0.0.1"
#define PORT_TCP_SERVER 8000
#define PORT_UDP_SERVER 8001
#define PORT_PYTHON_SERVER 11000

#pragma pack(push, 1)

typedef struct {
    int32_t id;
    int32_t udp_port;
    float value_sensor;
} Packet;

#pragma pack(pop)

#endif // SENSOR_PACKET_H