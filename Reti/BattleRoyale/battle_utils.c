#include "battle_utils.h"

struct sockaddr_in createAddress(const char* ipAddress, int port) {
    struct sockaddr_in tempAddress;
    memset(&tempAddress, 0, sizeof(tempAddress));
    tempAddress.sin_family = AF_INET;
    tempAddress.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress, &tempAddress.sin_addr.s_addr);
    return tempAddress;
}

int createSocketTcp(void) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("[-] Error creating TCP socket");
        exit(EXIT_FAILURE);
    }
    const int opt = 1; 
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return sockfd;
}

int createSocketUdp(void) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("[-] Error creating UDP socket");
        exit(EXIT_FAILURE);
    }
    const int opt = 1; 
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return sockfd;
}

GamePacket createPacket(int32_t type, int32_t player_id, int32_t pos_x, int32_t pos_y, int32_t health_left) {
    GamePacket pkt;
    pkt.type = type;
    pkt.player_id = player_id;
    pkt.pos_x = pos_x;
    pkt.pos_y = pos_y;
    pkt.health_left = health_left;
    return pkt;
}

void packetToNetworkByteOrder(GamePacket* p) {
    p->type = htonl(p->type);
    p->player_id = htonl(p->player_id);
    p->pos_x = htonl(p->pos_x);
    p->pos_y = htonl(p->pos_y);
    p->health_left = htonl(p->health_left);
}

void packetToHostByteOrder(GamePacket* p) {
    p->type = ntohl(p->type);
    p->player_id = ntohl(p->player_id);
    p->pos_x = ntohl(p->pos_x);
    p->pos_y = ntohl(p->pos_y);
    p->health_left = ntohl(p->health_left);
}

void printPacket(const GamePacket* packet) {
    printf("=== GamePacket ===\n");
    printf("Type        : %d\n", packet->type);
    printf("Player ID   : %d\n", packet->player_id);
    printf("Position    : (%d, %d)\n", packet->pos_x, packet->pos_y);
    printf("Health Left : %d\n", packet->health_left);
    printf("==================\n");
}