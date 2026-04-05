#include "battle_packet.h"
#include "battle_utils.h"

typedef struct Player {
    int sockfdPlayer;
    int32_t idPlayer;
    struct Player* next;
} Player;

typedef struct {
    Player* head;
    pthread_mutex_t lock;
    int tcpSock;
    int udpSock;
    struct sockaddr_in pythonDashboardAddr;
} ServerContext;

typedef struct {
    int clientSock;
    struct sockaddr_in clientAddr;
    ServerContext* ctx;
} TcpThreadArgs;

void addPlayer(ServerContext* ctx, int32_t idPlayer, int sockfd) {
    Player* temp = (Player*) malloc(sizeof(Player));
    if (!temp) return;
    temp->sockfdPlayer = sockfd;
    temp->idPlayer = idPlayer;
    
    pthread_mutex_lock(&ctx->lock);
    temp->next = ctx->head;
    ctx->head = temp;
    pthread_mutex_unlock(&ctx->lock);
}

void removePlayer(ServerContext* ctx, int sockfd) {
    pthread_mutex_lock(&ctx->lock);
    Player* current = ctx->head;
    Player* prev = NULL;

    while (current != NULL) {
        if (current->sockfdPlayer == sockfd) {
            if (prev == NULL) ctx->head = current->next;
            else prev->next = current->next;
            free(current);
            break;
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&ctx->lock);
}

int getSocketTcp(ServerContext* ctx, int32_t idPlayer) {
    int sock = -1;
    pthread_mutex_lock(&ctx->lock);
    Player *temp = ctx->head;
    while (temp != NULL) {
        if (temp->idPlayer == idPlayer) {
            sock = temp->sockfdPlayer;
            break;
        }
        temp = temp->next;
    }
    pthread_mutex_unlock(&ctx->lock);
    return sock;
}

void* handlePlayerConnectionTcp(void* args) {
    TcpThreadArgs* par = (TcpThreadArgs*) args;
    GamePacket game;
    int current_id = -1;

    while (1) {
        if (recv(par->clientSock, &game, sizeof(game), 0) <= 0) {
            printf("[SERVER] Giocatore %d si e' disconnesso.\n", current_id);
            break;
        }
        
        packetToHostByteOrder(&game);
        
        if (game.type == TYPE_LOGIN) {
            current_id = game.player_id;
            addPlayer(par->ctx, current_id, par->clientSock);
            printf("[SERVER] Login registrato per Player %d\n", current_id);
        }  
    }

    removePlayer(par->ctx, par->clientSock);
    close(par->clientSock);
    free(par);
    return NULL;
}

void* handleUdpTelemetry(void* args) {
    ServerContext* ctx = (ServerContext*) args;
    struct sockaddr_in clientAddress;
    socklen_t len = sizeof(clientAddress);

    printf("[SERVER] Ricezione telemetria UDP attiva...\n");

    while (1) {
        GamePacket game;
        if (recvfrom(ctx->udpSock, &game, sizeof(game), 0, (struct sockaddr*)&clientAddress, &len) > 0) {
            
            // Copia carbone a Python prima di decodificare l'ordine dei byte
            sendto(ctx->udpSock, &game, sizeof(game), 0, (struct sockaddr*)&ctx->pythonDashboardAddr, sizeof(ctx->pythonDashboardAddr));

            packetToHostByteOrder(&game);
            
            // Gestione morte
            if (game.health_left <= 0) {
                int sockPlayer = getSocketTcp(ctx, game.player_id);
                if (sockPlayer != -1) {
                    GamePacket deathPack = createPacket(TYPE_DEAD_PLAYER, game.player_id, 0, 0, 0);
                    packetToNetworkByteOrder(&deathPack);
                    send(sockPlayer, &deathPack, sizeof(deathPack), 0);
                    printf("[SERVER] Player %d eliminato!\n", game.player_id);
                }
            }
        }
    }
    return NULL;
}

int main(void) {
    ServerContext ctx;
    ctx.head = NULL;
    pthread_mutex_init(&ctx.lock, NULL);
    
    struct sockaddr_in serverAddressTcp = createAddress(LOCALHOST, SERVER_TCP_PORT);
    struct sockaddr_in serverAddressUdp = createAddress(LOCALHOST, SERVER_UDP_PORT);
    ctx.pythonDashboardAddr = createAddress(LOCALHOST, PYTHON_DASHBOARD_PORT);

    ctx.tcpSock = createSocketTcp();
    ctx.udpSock = createSocketUdp();

    if (bind(ctx.tcpSock, (struct sockaddr*)&serverAddressTcp, sizeof(serverAddressTcp)) < 0 ||
        bind(ctx.udpSock, (struct sockaddr*)&serverAddressUdp, sizeof(serverAddressUdp)) < 0) {
        perror("[-] Errore di bind");
        exit(EXIT_FAILURE);
    }

    if (listen(ctx.tcpSock, MAX_HOSTS) < 0) {
        perror("[-] Errore di listen TCP");
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] Server TCP in ascolto sulla porta %d...\n", SERVER_TCP_PORT);

    pthread_t threadUdp;
    pthread_create(&threadUdp, NULL, handleUdpTelemetry, &ctx);
    pthread_detach(threadUdp);

    while (1) {
        struct sockaddr_in clientAddress;
        socklen_t len = sizeof(clientAddress);

        int clientSocket = accept(ctx.tcpSock, (struct sockaddr*)&clientAddress, &len);
        if (clientSocket > 0) {
            TcpThreadArgs* playerPar = (TcpThreadArgs*) malloc(sizeof(TcpThreadArgs));
            if (playerPar) {
                playerPar->clientAddr = clientAddress;
                playerPar->clientSock = clientSocket;
                playerPar->ctx = &ctx;
                
                pthread_t threadId;
                pthread_create(&threadId, NULL, handlePlayerConnectionTcp, playerPar);
                pthread_detach(threadId);
            }
        }
    }
    
    pthread_mutex_destroy(&ctx.lock);
    return 0;
}