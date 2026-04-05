#ifndef CHAT_PACKET_H
#define CHAT_PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#define GATEWAY 6000
#define LOCALHOST "127.0.0.1"

#define TYPE_REGISTER 1
#define TYPE_REGISTER_ERROR 2
#define TYPE_REGISTER_ACK 3
#define TYPE_LOOKUP 4 

#pragma pack(push, 1)
typedef struct {
    int32_t type;             
    char sender_nick[32];     
    char target_nick[32];     
    int32_t peer_port;        
    char payload[255];        
} Packet;
#pragma pack(pop)

#endif // CHAT_PACKET_H