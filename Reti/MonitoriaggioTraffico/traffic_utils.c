#include "traffic_utils.h"

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

void printPacket(Packet p) {
    printf("----- SENSOR PACKET -----\n");
    printf("Sensor ID      : %d\n", p.idSensor);
    printf("Sensor Port    : %d\n", p.portSensor);
    printf("Vehicle Count  : %d\n", p.numeroVeicoli);
    printf("Avg Velocity   : %.2f\n", p.velocitaMedia);
    printf("-------------------------\n");
}

void convertPacket(Packet* p) {
    p->idSensor = ntohl(p->idSensor);
    p->numeroVeicoli = ntohl(p->numeroVeicoli);
    p->portSensor = ntohl(p->portSensor);
    
    // Handle float conversion (Network to Host byte order)
    uint32_t tmp_vel;
    memcpy(&tmp_vel, &p->velocitaMedia, sizeof(float)); 
    tmp_vel = ntohl(tmp_vel);                           
    memcpy(&p->velocitaMedia, &tmp_vel, sizeof(float)); 
}

void printAlertPacket(AlertPacket* p) {
    int32_t type = ntohl(p->typeAllarm);
    int32_t id   = ntohl(p->idSensorProblem);
    int32_t cars = ntohl(p->numCars);
    
    uint32_t tmp_vel;
    float vel;
    memcpy(&tmp_vel, &p->velocity, sizeof(float));
    tmp_vel = ntohl(tmp_vel);
    memcpy(&vel, &tmp_vel, sizeof(float));

    printf("\n===== ALERT PACKET RECEIVED =====\n");
    if (type == TYPE_ALARM_CAR) {
        printf(">>> HIGH TRAFFIC EMERGENCY <<<\n");
    } else if (type == TYPE_ALARM_VELOCITY) {
        printf(">>> LOW SPEED / ACCIDENT ALERT <<<\n");
    }

    printf("Alarm Type        : %d\n", type);
    printf("Problem Sensor ID : %d\n", id);
    printf("Detected Vehicles : %d\n", cars);
    printf("Average Speed     : %.2f\n", vel);
    printf("Description       : %s", p->description);
    printf("=================================\n\n");
}

void printPolicePacket(PolicePacket* p) {
    int32_t type = ntohl(p->typeRequest);
    int32_t id   = ntohl(p->idPolice);

    printf("===== POLICE PACKET =====\n");
    printf("Request Type : %d\n", type);
    printf("Police ID    : %d\n", id);
    printf("=========================\n");
}
