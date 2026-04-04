#include "terapy_utils.h"

struct sockaddr_in createAddress(const char* ipAddress, int port){
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress, &address.sin_addr.s_addr);

    return address;
}

int createSocketTcp(void){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("[-] Error creating TCP socket");
        exit(EXIT_FAILURE);
    }
    const int opt = 1; 
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("[-] setsockopt failed");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int createSocketUdp(void){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0){
        perror("[-] Error creating UDP socket");
        exit(EXIT_FAILURE);
    }
    const int opt = 1; 
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("[-] setsockopt failed");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

MedPacket createPacket(int32_t type, int32_t patientId, int32_t bpm, int32_t oxygen){
    MedPacket pack;
    pack.type = type;
    pack.patient_id = patientId;
    pack.bpm = bpm;
    pack.oxygen = oxygen;
    return pack;
}

void packetToNetworkByteOrder(MedPacket* p) {
    p->type = htonl(p->type);
    p->patient_id = htonl(p->patient_id);
    p->bpm = htonl(p->bpm);
    p->oxygen = htonl(p->oxygen);
}

void packetToHostByteOrder(MedPacket* p) {
    p->type = ntohl(p->type);
    p->patient_id = ntohl(p->patient_id);
    p->bpm = ntohl(p->bpm);
    p->oxygen = ntohl(p->oxygen);
}

void printPacket(const MedPacket* packet) {
    printf("\n----------------------------------------\n");
    
    switch (packet->type) {
        case TYPE_PATIENT_ADMIT:
            printf(" [▶] TYPE: PATIENT ADMIT\n");
            printf("----------------------------------------\n");
            printf(" > Patient ID : %d\n", packet->patient_id);
            break;

        case TYPE_ADMIT_ACK:
            printf(" [✓] TYPE: ADMIT ACK\n");
            printf("----------------------------------------\n");
            printf(" > Patient ID : %d\n", packet->patient_id);
            break;

        case TYPE_VITALS_UPDATE:
            printf(" [♥] TYPE: VITALS UPDATE\n");
            printf("----------------------------------------\n");
            printf(" > Patient ID : %d\n", packet->patient_id);
            printf(" > BPM        : %d\n", packet->bpm);
            printf(" > SpO2       : %d%%\n", packet->oxygen);
            break;

        case TYPE_CODE_RED:
            printf(" [🚨] TYPE: CODE RED (CRITICAL!)\n");
            printf("----------------------------------------\n");
            printf(" > Patient ID : %d\n", packet->patient_id);
            printf(" > ACTION     : IMMEDIATE RESUSCITATION\n");
            break;

        default:
            printf(" [?] TYPE: UNKNOWN PACKET\n");
            printf("----------------------------------------\n");
            printf(" > Raw Type   : %d\n", packet->type);
            break;
    }
    
    printf("----------------------------------------\n\n");
}