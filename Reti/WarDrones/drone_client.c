#include "packetwar.h"
#include "packetwar_utils.h"

typedef struct {
    int tcpSock;
    volatile int isFlying; 
} DroneContext;

// Thread sempre in ascolto sulla connessione sicura TCP.
// Se il server rileva un'anomalia, ci manda l'ordine di atterrare.
void* listenEmergencyCommands(void* arg) {
    DroneContext* ctx = (DroneContext*)arg;
    Packet p;
    
    while (recv(ctx->tcpSock, &p, sizeof(Packet), 0) > 0) {
        packetToHostByteOrder(&p);
        
        if (p.type == TYPE_EMERGENCY_LANDING) {
            printf("\n[CRITICAL] ATTERRAGGIO D'EMERGENZA AVVIATO DALLA BASE!\n");
            ctx->isFlying = 0; 
            break;
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <Drone_ID>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int32_t droneId = (int32_t)atoi(argv[1]);
    srand((unsigned int)time(NULL) ^ (droneId << 16)); 

    DroneContext ctx;
    ctx.tcpSock = createSocketTcp();
    ctx.isFlying = 1;
    
    int udpSock = createSocketUdp();
    struct sockaddr_in serverTcpAddr = createAddress(LOCALHOST, TCPLISTENSERVERPORT);
    struct sockaddr_in serverUdpAddr = createAddress(LOCALHOST, UDPLISTENSERVERPORT);

    if (connect(ctx.tcpSock, (struct sockaddr*)&serverTcpAddr, sizeof(serverTcpAddr)) < 0) {
        perror("[-] Connessione TCP fallita");
        exit(EXIT_FAILURE);
    }

    Packet loginReq = createPacket(TYPE_LOGIN, droneId, 0, 0, 0);
    packetToNetworkByteOrder(&loginReq);
    send(ctx.tcpSock, &loginReq, sizeof(Packet), 0);

    Packet loginRes;
    if (recv(ctx.tcpSock, &loginRes, sizeof(Packet), 0) <= 0) {
        perror("[-] Connessione persa durante il login");
        exit(EXIT_FAILURE);
    }
    
    packetToHostByteOrder(&loginRes);
    if (loginRes.type == TYPE_LOGIN_NACK) {
        printf("[-] Login rifiutato (ID %d forse gia' in uso).\n", droneId);
        close(ctx.tcpSock);
        close(udpSock);
        exit(EXIT_FAILURE);
    }

    printf("[+] Drone %d in volo. Avvio telemetria...\n", droneId);

    pthread_t commandThread;
    pthread_create(&commandThread, NULL, listenEmergencyCommands, &ctx);
    pthread_detach(commandThread); 

    int32_t battery = 100;
    int32_t posX = 0;
    int32_t posY = 0;

    // Finché siamo in volo e abbiamo batteria, spariamo i dati in UDP
    while (ctx.isFlying && battery > 0) {
        posX += (rand() % 5) - 2;
        posY += (rand() % 5) - 2;
        battery -= (rand() % 3) + 1;

        if (battery < 0) battery = 0;

        Packet telemetry = createPacket(TYPE_TELEMETRY, droneId, posX, posY, battery);
        packetToNetworkByteOrder(&telemetry);
        sendto(udpSock, &telemetry, sizeof(Packet), 0, (struct sockaddr*)&serverUdpAddr, sizeof(serverUdpAddr));

        usleep(500000); 
    }

    if (battery <= 0) {
        printf("[CRITICAL] Batteria esaurita. Drone perso.\n");
    } else {
        printf("[SYSTEM] Drone %d atterrato in sicurezza.\n", droneId);
    }

    close(ctx.tcpSock);
    close(udpSock);
    return 0;
}