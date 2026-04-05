#include "battle_packet.h"
#include "battle_utils.h"

typedef struct {
    int tcpSock;
    int udpSock;
    int32_t myId;
    struct sockaddr_in serverUdpAddress;
    int32_t life;
    pthread_mutex_t* life_mutex;
} ClientContext;

void* handleTcpConnection(void* args) {
    ClientContext* ctx = (ClientContext*) args;
    
    GamePacket pack = createPacket(TYPE_LOGIN, ctx->myId, 0, 0, 100);
    packetToNetworkByteOrder(&pack);
    send(ctx->tcpSock, &pack, sizeof(pack), 0);
    
    printf("[TCP] Login inviato al server. Partita iniziata!\n");

    while (1) {
        GamePacket rx_pack;
        if (recv(ctx->tcpSock, &rx_pack, sizeof(rx_pack), 0) <= 0) {
            printf("\n[-] Connessione con il server persa.\n");
            break;
        }
        
        packetToHostByteOrder(&rx_pack);
        
        if (rx_pack.type == TYPE_DEAD_PLAYER) {
            printf("\n[X_X] SEI MORTO! Fine partita.\n");
            break;
        }
        
        if (rx_pack.type == TYPE_SAFE_ZONE) {
            // Applica il danno ricevuto
            pthread_mutex_lock(ctx->life_mutex);
            ctx->life -= rx_pack.health_left;
            if (ctx->life < 0) ctx->life = 0;
            printf("[!] DANNO DA SAFE ZONE! Vita rimasta: %d\n", ctx->life);
            pthread_mutex_unlock(ctx->life_mutex);
        }
    }
    
    // Ferma l'UDP forzando la vita a 0
    pthread_mutex_lock(ctx->life_mutex);
    ctx->life = 0;
    pthread_mutex_unlock(ctx->life_mutex);
    
    close(ctx->tcpSock);
    return NULL;
}

void* handleUdpConnection(void* args) {
    ClientContext* ctx = (ClientContext*) args;
    
    while (1) {
        pthread_mutex_lock(ctx->life_mutex);
        int current_life = ctx->life;
        pthread_mutex_unlock(ctx->life_mutex);
        
        if (current_life <= 0) break;

        int posx = 10 + rand() % 41;
        int posy = 10 + rand() % 41;
        
        // Malus ambientale casuale
        if (rand() % 10 > 7) { 
            pthread_mutex_lock(ctx->life_mutex);
            ctx->life -= (rand() % 5);
            current_life = ctx->life;
            pthread_mutex_unlock(ctx->life_mutex);
        }

        GamePacket pack = createPacket(TYPE_UDP_LIFE, ctx->myId, posx, posy, current_life);
        packetToNetworkByteOrder(&pack);
        sendto(ctx->udpSock, &pack, sizeof(pack), 0, (struct sockaddr*)&ctx->serverUdpAddress, sizeof(ctx->serverUdpAddress));
        
        sleep(1);
    }
    
    close(ctx->udpSock);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <Player_ID>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int32_t myId = (int32_t)atoi(argv[1]);
    srand((unsigned int)time(NULL) ^ myId);
    
    pthread_mutex_t life_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct sockaddr_in serverTcp = createAddress(LOCALHOST, SERVER_TCP_PORT);
    struct sockaddr_in serverUdp = createAddress(LOCALHOST, SERVER_UDP_PORT);

    int sockTCP = createSocketTcp();
    if (connect(sockTCP, (struct sockaddr*)&serverTcp, sizeof(serverTcp)) < 0) {
        perror("[-] Errore connessione TCP");
        exit(EXIT_FAILURE);
    }

    int socketUDP = createSocketUdp();
    ClientContext ctx = {sockTCP, socketUDP, myId, serverUdp, 100, &life_mutex};
    pthread_t tIdTcp, tIdUdp;

    pthread_create(&tIdTcp, NULL, handleTcpConnection, &ctx);
    pthread_create(&tIdUdp, NULL, handleUdpConnection, &ctx);

    pthread_join(tIdTcp, NULL);
    pthread_join(tIdUdp, NULL);
    
    pthread_mutex_destroy(&life_mutex);
    return 0;
}