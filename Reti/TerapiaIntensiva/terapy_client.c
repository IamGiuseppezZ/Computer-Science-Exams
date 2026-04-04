#include "terapy_packet.h"
#include "terapy_utils.h"

typedef struct {
    int32_t myId;
    struct sockaddr_in serverAddressTcp;
    struct sockaddr_in serverAddressUDP;
    int sockTcp;
    int sockUdp;
} ThreadParams;

void* handleTcpConnection(void* args){
    ThreadParams par = *(ThreadParams*) args;
    
    if (connect(par.sockTcp, (struct sockaddr*) &par.serverAddressTcp, sizeof(par.serverAddressTcp)) < 0){
        perror("[-] Error connecting to hospital server");
        exit(EXIT_FAILURE);
    }
    
    // Send admission request
    MedPacket pack = createPacket(TYPE_PATIENT_ADMIT, par.myId, 0, 0);
    packetToNetworkByteOrder(&pack);
    send(par.sockTcp, &pack, sizeof(pack), 0);
    
    // Wait for ACK
    MedPacket packReceived;
    if (recv(par.sockTcp, &packReceived, sizeof(packReceived), 0) > 0) {
        packetToHostByteOrder(&packReceived);
        printPacket(&packReceived);
    }

    // Listen for incoming alerts (Code Red)
    while(1){
        MedPacket packNew;
        if (recv(par.sockTcp, &packNew, sizeof(packNew), 0) > 0) {
            packetToHostByteOrder(&packNew);
            if (packNew.type == TYPE_CODE_RED){
                printf("\n[URG] CODE RED RECEIVED! Resuscitation unit incoming. Disconnecting.\n");
                close(par.sockTcp);
                close(par.sockUdp);
                exit(EXIT_SUCCESS);
            }
        } else {
            printf("[-] Connection to hospital lost.\n");
            break;
        }
    }
    return NULL;
}

void* handleUdpConnection(void* args){
    ThreadParams par = *(ThreadParams*) args;
    
    while(1){
        int32_t bpm = 35 + rand() % (200 - 35 + 1);
        int32_t oxg = 89 + rand() % (100 - 88 + 1);
        
        MedPacket pack = createPacket(TYPE_VITALS_UPDATE, par.myId, bpm, oxg);
        packetToNetworkByteOrder(&pack); // Serialize
        
        sendto(par.sockUdp, &pack, sizeof(pack), 0, (struct sockaddr*) &par.serverAddressUDP, sizeof(par.serverAddressUDP));
        sleep(1);
    }
    return NULL;
}

int main(int argc, char* argv[]){
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <patient_id> <client_port_TCP> <client_port_UDP>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    srand((unsigned int)time(NULL) ^ getpid()); 
    
    int32_t myId = (int32_t)atoi(argv[1]);
    int myPortTCP = atoi(argv[2]);
    int myPortUDP = atoi(argv[3]);
    
    struct sockaddr_in myAddressTCP = createAddress(LOCALHOST, myPortTCP);
    struct sockaddr_in myAddressUDP = createAddress(LOCALHOST, myPortUDP);
    
    struct sockaddr_in serverAddressTCP = createAddress(LOCALHOST, PORT_TCP_HOSPITAL);
    struct sockaddr_in serverAddressUDP = createAddress(LOCALHOST, PORT_UDP_HOSPITAL);
    
    int sockTcp = createSocketTcp();
    int sockUdp = createSocketUdp();
    
    // Bind UDP port (TCP is implicitly bound on connect, but UDP needs explicit bind if receiving)
    bind(sockUdp, (struct sockaddr*)&myAddressUDP, sizeof(myAddressUDP));

    ThreadParams par = {myId, serverAddressTCP, serverAddressUDP, sockTcp, sockUdp};
    
    pthread_t threadTcp, threadUdp;
    pthread_create(&threadTcp, NULL, handleTcpConnection, &par);
    pthread_create(&threadUdp, NULL, handleUdpConnection, &par); // Fixed bug: was creating threadTcp twice

    pthread_join(threadTcp, NULL);
    // pthread_join(threadUdp, NULL); // UDP thread will run indefinitely until TCP closes
    
    return 0;
}