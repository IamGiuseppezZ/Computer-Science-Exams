#include "sensor_packet.h"
#include "sensor_utils.h"

typedef struct {
    int sockTcp;
} ThreadParams;

void* receiveUpdatesThread(void* args) {
    ThreadParams* par = (ThreadParams*)args;
    Packet pack;

    printf("[TCP THREAD] Listening for broadcast updates...\n");

    while (recv(par->sockTcp, &pack, sizeof(Packet), 0) > 0) {
        packetToHostByteOrder(&pack); // Deserializzazione pulita
        printf("\n[BROADCAST] Sensor ID: %d | Value: %.2f\n", pack.id, pack.value_sensor);
    }

    printf("[TCP THREAD] Connection to central server lost.\n");
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        fprintf(stderr, "Usage: %s <Server_IP> <Server_Port_TCP> <Server_Port_UDP> <My_ID> <My_Port_UDP>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* serverIp = argv[1];
    int serverPortTcp = atoi(argv[2]);
    int serverPortUdp = atoi(argv[3]);
    int32_t myId = (int32_t)atoi(argv[4]);
    int32_t myPortUdp = (int32_t)atoi(argv[5]);

    srand((unsigned int)time(NULL) ^ myId); 

    int sockTcp = createSocketTcp();
    int sockUdp = createSocketUdp();

    struct sockaddr_in serverAddressTcp = createAddress(serverIp, serverPortTcp);
    struct sockaddr_in serverAddressUdp = createAddress(serverIp, serverPortUdp);

    // Registrazione TCP
    printf("[CLIENT] Connecting to TCP Server (%s:%d)...\n", serverIp, serverPortTcp);
    if (connect(sockTcp, (struct sockaddr*)&serverAddressTcp, sizeof(serverAddressTcp)) < 0) {
        perror("[-] TCP Connection Error");
        exit(EXIT_FAILURE);
    }

    Packet regPack = {myId, myPortUdp, 0.0f};
    packetToNetworkByteOrder(&regPack);
    send(sockTcp, &regPack, sizeof(Packet), 0);
    printf("[+] Registration successful!\n");

    pthread_t tid;
    ThreadParams tPar = {sockTcp};
    pthread_create(&tid, NULL, receiveUpdatesThread, &tPar);
    pthread_detach(tid);

    // Invio Dati UDP
    while (1) {
        float myValue = 10.0f + ((float)rand() / RAND_MAX) * 20.0f;

        Packet dataPack = {myId, myPortUdp, myValue};
        packetToNetworkByteOrder(&dataPack); 
        
        sendto(sockUdp, &dataPack, sizeof(dataPack), 0, (struct sockaddr*)&serverAddressUdp, sizeof(serverAddressUdp));
        
        printf("[UDP] Telemetry sent: %.2f\n", myValue);
        sleep(3);
    }

    close(sockTcp);
    close(sockUdp);
    return 0;
}