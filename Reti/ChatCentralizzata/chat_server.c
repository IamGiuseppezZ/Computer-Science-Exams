#include "chat_packet.h"
#include "chat_utils.h"

#define FILENAME "users.txt"

typedef struct UserRecord {
    char nickname[32];
    int32_t port;
    struct UserRecord* next;
} UserRecord;

typedef struct {
    UserRecord* list;
    pthread_mutex_t lock;
    int sockfd;
} ServerContext;

typedef struct {
    Packet rx_packet;
    struct sockaddr_in sender_addr;
    ServerContext* ctx;
} RequestArgs;

// --- Gestione DB RAM ---
void addToList(UserRecord** list, const char* name, int32_t port){
    UserRecord* user = (UserRecord*) malloc(sizeof(UserRecord));
    if(!user) return;
    strncpy(user->nickname, name, sizeof(user->nickname) - 1);
    user->port = port;
    user->next = *list;
    *list = user;
}

int userExists(UserRecord* list, const char* name){
    while(list){
        if (strcmp(list->nickname, name) == 0) return 1;
        list = list->next;
    }
    return 0;
}

int32_t getPort(UserRecord* list, const char* name){
    while(list){
        if (strcmp(list->nickname, name) == 0) return list->port;
        list = list->next;
    }
    return -1;
}

void loadUsers(ServerContext* ctx){
    FILE *fd = fopen(FILENAME, "a+");
    if (!fd) return;
    rewind(fd);
    
    char name[32];
    int32_t port;
    
    pthread_mutex_lock(&ctx->lock);
    while (fscanf(fd, "%31s %d", name, &port) == 2){
        addToList(&ctx->list, name, port);
    }
    pthread_mutex_unlock(&ctx->lock);
    fclose(fd);
}

//  Gestori Richieste 
void processRegister(RequestArgs* args) {
    Packet rx = args->rx_packet;
    Packet tx;

    pthread_mutex_lock(&args->ctx->lock);
    
    if (userExists(args->ctx->list, rx.sender_nick)) {
        if (getPort(args->ctx->list, rx.sender_nick) == rx.peer_port) {
            tx = createPacket(TYPE_REGISTER_ACK, "Server", rx.sender_nick, GATEWAY, "AUTH_OK");
        } else {
            tx = createPacket(TYPE_REGISTER_ERROR, "Server", rx.sender_nick, GATEWAY, "USER_EXISTS_WRONG_PORT");
        }
    } else {
        FILE* fd = fopen(FILENAME, "a+");
        if (fd) {
            fprintf(fd, "%s %d\n", rx.sender_nick, rx.peer_port);
            fclose(fd);
            addToList(&args->ctx->list, rx.sender_nick, rx.peer_port);
            tx = createPacket(TYPE_REGISTER_ACK, "Server", rx.sender_nick, GATEWAY, "REG_OK");
            printf("[SERVER] Nuovo utente: %s (Port: %d)\n", rx.sender_nick, rx.peer_port);
        } else {
            tx = createPacket(TYPE_REGISTER_ERROR, "Server", rx.sender_nick, GATEWAY, "DB_ERROR");
        }
    }
    pthread_mutex_unlock(&args->ctx->lock);

    packetToNetworkByteOrder(&tx);
    sendto(args->ctx->sockfd, &tx, sizeof(tx), 0, (struct sockaddr*)&args->sender_addr, sizeof(args->sender_addr));
}

void processLookupProxy(RequestArgs* args) {
    Packet rx = args->rx_packet;
    Packet tx;

    pthread_mutex_lock(&args->ctx->lock);
    int32_t destPort = getPort(args->ctx->list, rx.target_nick);
    pthread_mutex_unlock(&args->ctx->lock);

    if (destPort != -1) {
        // PROXY: Inoltro esattamente il pacchetto ricevuto al destinatario
        struct sockaddr_in targetAddress = createAddress(LOCALHOST, destPort);
        Packet forwardPack = rx; // Copio il pacchetto decodificato
        packetToNetworkByteOrder(&forwardPack); // Lo ricodifico per la rete
        
        sendto(args->ctx->sockfd, &forwardPack, sizeof(forwardPack), 0, (struct sockaddr*)&targetAddress, sizeof(targetAddress));
        printf("[SERVER] Proxy inoltrato messaggio da %s a %s\n", rx.sender_nick, rx.target_nick);
    } else {
        // Errore al mittente
        tx = createPacket(TYPE_REGISTER_ERROR, "Server", rx.sender_nick, 0, "Utente offline o inesistente");
        packetToNetworkByteOrder(&tx);
        sendto(args->ctx->sockfd, &tx, sizeof(tx), 0, (struct sockaddr*)&args->sender_addr, sizeof(args->sender_addr));
    }
}

void* workerThread(void* arg) {
    RequestArgs* args = (RequestArgs*)arg;
    
    if (args->rx_packet.type == TYPE_REGISTER) {
        processRegister(args);
    } else if (args->rx_packet.type == TYPE_LOOKUP) {
        processLookupProxy(args);
    }
    
    free(args);
    return NULL;
}

int main(void){
    ServerContext ctx;
    ctx.list = NULL;
    pthread_mutex_init(&ctx.lock, NULL);
    loadUsers(&ctx);

    struct sockaddr_in addressServer = createAddress(LOCALHOST, GATEWAY);
    ctx.sockfd = createSocketIpv4();
    bind(ctx.sockfd, (struct sockaddr*)&addressServer, sizeof(addressServer));

    printf("[SERVER] Proxy in ascolto sulla porta %d...\n", GATEWAY);

    while(1){
        struct sockaddr_in senderAddr;
        socklen_t len = sizeof(senderAddr);
        Packet rx_pack;
        
        if (recvfrom(ctx.sockfd, &rx_pack, sizeof(rx_pack), 0, (struct sockaddr*)&senderAddr, &len) > 0) {
            packetToHostByteOrder(&rx_pack);

            RequestArgs* args = (RequestArgs*) malloc(sizeof(RequestArgs));
            if (args) {
                args->rx_packet = rx_pack;
                args->sender_addr = senderAddr;
                args->ctx = &ctx;

                pthread_t tid;
                pthread_create(&tid, NULL, workerThread, args);
                pthread_detach(tid);
            }
        }
    }
    
    pthread_mutex_destroy(&ctx.lock);
    close(ctx.sockfd);
    return 0;
}
