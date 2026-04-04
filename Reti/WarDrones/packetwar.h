#ifndef PACKETWAR_H
#define PACKETWAR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#define MAX_HOSTS 10
#define LOCALHOST "127.0.0.1"
#define TCPLISTENSERVERPORT 7000
#define UDPLISTENSERVERPORT 7001

#define TYPE_LOGIN 1
#define TYPE_LOGIN_ACK 2
#define TYPE_LOGIN_NACK 3
#define TYPE_TELEMETRY 4
#define TYPE_EMERGENCY_LANDING 5

// Previene il padding del compilatore
#pragma pack(push, 1)

typedef struct {
    int32_t type;       // 1=LOGIN, 2=LOGIN_ACK, 3=LOGIN_NACK, 4=TELEMETRY, 5=EMERGENCY
    int32_t drone_id;   // L'ID del drone (es. 0, 1, 2...)
    int32_t pos_x;
    int32_t pos_y;
    int32_t battery;    // Da 100 a 0
} Packet;

#pragma pack(pop)

#endif // PACKETWAR_H