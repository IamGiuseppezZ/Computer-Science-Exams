#include "p2p_utils.h"

struct sockaddr_in createAddress(const char* ipAddress, int port){
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress, &address.sin_addr.s_addr);

    return address;
}

int createSocketIpv4(void){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0){
        perror("[-] Errore creazione socket");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

Packet createPacket(int32_t type, const char* senderNick, const char* targetNick, int32_t peer_port, const char* payload){
    Packet p;
    memset(&p, 0, sizeof(Packet));
    p.type = type;
    p.peer_port = peer_port;
    
    strncpy(p.sender_nick, senderNick, sizeof(p.sender_nick) - 1);
    strncpy(p.target_nick, targetNick, sizeof(p.target_nick) - 1);
    strncpy(p.payload, payload, sizeof(p.payload) - 1);

    return p;
}

void packetToNetworkByteOrder(Packet* p) {
    p->type = htonl(p->type);
    p->peer_port = htonl(p->peer_port);
}

void packetToHostByteOrder(Packet* p) {
    p->type = ntohl(p->type);
    p->peer_port = ntohl(p->peer_port);
}

void printPacket(const Packet* p){
    printf("\n========== PACCHETTO ==========\n");
    printf("Tipo        : %d\n", p->type);
    printf("Mittente    : %s\n", p->sender_nick);
    printf("Destinatario: %s\n", p->target_nick);
    printf("Porta Peer  : %d\n", p->peer_port);
    printf("Payload     : %s\n", p->payload);
    printf("===============================\n");
}