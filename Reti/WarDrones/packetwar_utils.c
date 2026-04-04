#include "packetwar_utils.h"

int createSocketTcp(void){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("[-] Error creating TCP socket");
        exit(EXIT_FAILURE);
    }
    const int opt = 1; 
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("[-] setsockopt TCP failed");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

int createSocketUdp(void){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1){
        perror("[-] Error creating UDP socket");
        exit(EXIT_FAILURE);
    }
    const int opt = 1; 
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("[-] setsockopt UDP failed");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

struct sockaddr_in createAddress(const char* ip, int port){
    struct sockaddr_in tempAddress;
    memset(&tempAddress, 0, sizeof(tempAddress));
    tempAddress.sin_family = AF_INET;
    tempAddress.sin_port = htons(port);
    inet_pton(AF_INET, ip, &tempAddress.sin_addr.s_addr);
    return tempAddress;
}

Packet createPacket(int32_t type, int32_t drone_id, int32_t pos_x, int32_t pos_y, int32_t battery){
    Packet temp;
    temp.type = type;
    temp.drone_id = drone_id;
    temp.pos_x = pos_x;
    temp.pos_y = pos_y;
    temp.battery = battery;
    return temp;
} 

void packetToNetworkByteOrder(Packet* p) {
    p->type = htonl(p->type);
    p->drone_id = htonl(p->drone_id);
    p->pos_x = htonl(p->pos_x);
    p->pos_y = htonl(p->pos_y);
    p->battery = htonl(p->battery);
}

void packetToHostByteOrder(Packet* p) {
    p->type = ntohl(p->type);
    p->drone_id = ntohl(p->drone_id);
    p->pos_x = ntohl(p->pos_x);
    p->pos_y = ntohl(p->pos_y);
    p->battery = ntohl(p->battery);
}

void printPacket(const Packet* p){
    printf("----- PACKET -----\n");
    printf("Type: ");
    switch(p->type){
        case TYPE_LOGIN:              printf("LOGIN\n"); break;
        case TYPE_LOGIN_ACK:          printf("LOGIN ACK DRONE [%d]\n", p->drone_id); break;
        case TYPE_LOGIN_NACK:         printf("LOGIN NACK\n"); break;
        case TYPE_TELEMETRY:          printf("TELEMETRY\n"); break;
        case TYPE_EMERGENCY_LANDING:  printf("EMERGENCY LANDING\n"); break;
        default:                      printf("UNKNOWN (%d)\n", p->type); break;
    }
    printf("Drone ID : %d\n", p->drone_id);
    printf("Position : (%d, %d)\n", p->pos_x, p->pos_y);
    printf("Battery  : %d%%\n", p->battery);
    printf("------------------\n");
}