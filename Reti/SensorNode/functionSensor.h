#include "Packet.h"

struct sockaddr_in createAddress(char* ip, int port){
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    inet_pton(AF_INET, ip, &address.sin_addr.s_addr);
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    
    return address;
}

int createSocket(){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1){
        perror("Error creating socket.\n");
    }
    
    return sockfd;
}

Packet createPacket(int sourceId, int destId, int type, int dataValue){
    Packet p;
    p.source_id = sourceId;
    p.destination_id = destId;
    p.type = type;
    p.dataValue = dataValue;

    return p;
}