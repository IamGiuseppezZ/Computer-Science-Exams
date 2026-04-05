#include "p2p_packet.h"
#include "p2p_utils.h"

struct sockaddr_in createAddress(char* ipAddress, int port){
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress, &address.sin_addr.s_addr);
    address.sin_family = AF_INET;

    return address;
}

int createSocketIpv4(){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == - 1){
        perror("Error creating socket.\n");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

Packet createPacket(int type, char* senderNick, char* targetNick, int peer_port, char* payload){
    Packet p;
    p.type = type;
    strcpy(p.sender_nick, senderNick);
    strcpy(p.target_nick, targetNick);
    p.peer_port = peer_port;
    strcpy(p.payload, payload);

    return p;
}

void printPacket(Packet p){
    printf("\n========== PACKET ==========\n");
    printf("Type        : %d\n", p.type);
    printf("Sender Nick : %s\n", p.sender_nick);
    printf("Target Nick : %s\n", p.target_nick);
    printf("Peer Port   : %d\n", p.peer_port);
    printf("Payload     : %s\n", p.payload);
    printf("============================\n");
}