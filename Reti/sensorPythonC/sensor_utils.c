#include "sensor_utils.h"

struct sockaddr_in createAddress(const char* ipAddress, int port) {
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress, &address.sin_addr.s_addr);
    return address;
}

int createSocketTcp(void) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[-] Error creating TCP socket");
        exit(EXIT_FAILURE);
    }
    const int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return sockfd;
}

int createSocketUdp(void) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[-] Error creating UDP socket");
        exit(EXIT_FAILURE);
    }
    const int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return sockfd;
}

void packetToNetworkByteOrder(Packet* p) {
    p->id = htonl(p->id);
    p->udp_port = htonl(p->udp_port);
    
    // Float serialization
    uint32_t tmp_float;
    memcpy(&tmp_float, &p->value_sensor, sizeof(float));
    tmp_float = htonl(tmp_float);
    memcpy(&p->value_sensor, &tmp_float, sizeof(float));
}

void packetToHostByteOrder(Packet* p) {
    p->id = ntohl(p->id);
    p->udp_port = ntohl(p->udp_port);
    
    // Float deserialization
    uint32_t tmp_float;
    memcpy(&tmp_float, &p->value_sensor, sizeof(float));
    tmp_float = ntohl(tmp_float);
    memcpy(&p->value_sensor, &tmp_float, sizeof(float));
}