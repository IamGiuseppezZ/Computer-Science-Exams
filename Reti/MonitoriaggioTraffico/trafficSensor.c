#include "traffic_packet.h"
#include "traffic_utils.h"

typedef struct {
    int sockUdpSensor;  
    struct sockaddr_in clientAddressUdp;
    struct sockaddr_in serverAddressUdp;
    int myId;
    int myPort;
} SensorParams;

void startTelemetry(SensorParams par) {
    Packet packToSend;
    memset(&packToSend, 0, sizeof(Packet));

    while (1) {
        // Generate mock telemetry data
        int numVeicoli = rand() % 81; // 0 to 80
        float velocity = 5.0f + (float)(rand() % 41); // 5.0 to 45.0

        // Float serialization
        uint32_t tmp;
        memcpy(&tmp, &velocity, sizeof(float));
        tmp = htonl(tmp);
        memcpy(&packToSend.velocitaMedia, &tmp, sizeof(float));

        packToSend.idSensor = htonl(par.myId);
        packToSend.numeroVeicoli = htonl(numVeicoli);
        packToSend.portSensor = htonl(par.myPort);

        sendto(par.sockUdpSensor, &packToSend, sizeof(packToSend), 0, 
              (struct sockaddr*)&par.serverAddressUdp, sizeof(par.serverAddressUdp));
              
        printf("[SENSOR %d] Telemetry dispatched.\n", par.myId);
        sleep(2); // Wait before next transmission
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <SensorID> <UDP_Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    srand((unsigned int)time(NULL) ^ getpid()); 
    
    int myId = atoi(argv[1]);
    int myUdpPort = atoi(argv[2]);
    
    int sockfd = createSocketUdp();
    struct sockaddr_in clientAddressUDP = createAddress(LOCALHOST, myUdpPort);
    struct sockaddr_in serverAddressUdp = createAddress(LOCALHOST, PORT_SERVER_UDP);

    SensorParams par = {sockfd, clientAddressUDP, serverAddressUdp, myId, myUdpPort};
    
    printf("[SENSOR %d] Booting up on port %d...\n", myId, myUdpPort);
    startTelemetry(par);
    
    close(sockfd);
    return 0;
}
