#ifndef BATTLE_PACKET_H
#define BATTLE_PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#define LOCALHOST "127.0.0.1"
#define SERVER_TCP_PORT 7000
#define SERVER_UDP_PORT 7001
#define PYTHON_DASHBOARD_PORT 12000
#define MAX_HOSTS 10

#define TYPE_LOGIN 1
#define TYPE_UDP_LIFE 2
#define TYPE_SAFE_ZONE 3
#define TYPE_DEAD_PLAYER 4

#pragma pack(push, 1)
typedef struct {
    int32_t type;
    int32_t player_id;
    int32_t pos_x;
    int32_t pos_y;
    int32_t health_left;
} GamePacket;
#pragma pack(pop)

#endif // BATTLE_PACKET_H