#include "p2p_packet.h"
#include "p2p_utils.h"

#define FILENAME "users.txt"

typedef struct UserRecord {
    char nickname[32];
    int32_t port;
    struct UserRecord* next;
} UserRecord;

typedef struct {
    UserRecord* list;
    pthread_mutex_t db_lock;
    int sockfd;
} ServerContext;

typedef struct {
    Packet rx_packet;
    struct sockaddr_in sender_addr;
    ServerContext* ctx;
} RequestHandlerArgs;

// --- Gestione DB (usare sempre con il mutex bloccato!) ---
void addToList(UserRecord** list, const char* nickname, int32_t port){
    UserRecord* user = (UserRecord*) malloc(sizeof(UserRecord));
    if (!user) return;
    strncpy(user->nickname, nickname, sizeof(user->nickname) - 1);
    user->port = port;
    user->next = *list;
    *list = user;
}

int userExists(UserRecord* list, const char* name){
    while(list != NULL){
        if (strcmp(list->nickname, name) == 0) return 1;
        list = list->next;
    }
    return 0;
}

int portIsFree(UserRecord* list, int32_t port){
    while(list != NULL){
        if (list->port == port) return 0;
        list = list->next;
    }
    return 1;
}

int32_t getPort(UserRecord* list, const char* name){
    while(list != NULL){
        if (strcmp(list->nickname, name) == 0) return list->port;
        list = list->next;
    }
    return -1;
}

void loadUsers(ServerContext* ctx){
    FILE *fd = fopen(FILENAME, "a+");
    if (!fd) return;
    rewind(fd);
    
    char nickname[32];
    int32_t port;
    
    pthread_mutex_lock(&ctx->db_lock);
    while (fscanf(fd, "%31s %d", nickname, &port) == 2){
        addToList(&ctx->list, nickname, port);
    }
    pthread_mutex_unlock(&ctx->db_lock);
    fclose(fd);
}
// ---------------------------------------------------------

void processRegister(RequestHandlerArgs* args) {
    Packet rx_pack = args->rx_packet;
    Packet pSend;
    socklen_t len = sizeof(args->sender_addr);

    pthread_mutex_lock(&args->ctx->db_lock);

    if (userExists(args->ctx->list, rx_pack.sender_nick) && getPort(args->ctx->list, rx_pack.sender_nick) == rx_pack.peer_port) {
        pSend = createPacket(TYPE_REGISTER_ACK, "Server", rx_pack.sender_nick, GATEWAY, "AUTH_SUCCESS");
    } 
    else if (!portIsFree(args->ctx->list, rx_pack.peer_port)) {
        pSend = createPacket(TYPE_REGISTER_ERROR, "Server", rx_pack.sender_nick, GATEWAY, "PORT_IN_USE");
    } 
    else {
        // Nuovo utente: lo salviamo sia su file che in memoria
        FILE* fd = fopen(FILENAME, "a+");
        if (fd) {
            fprintf(fd, "%s %d\n", rx_pack.sender_nick, rx_pack.peer_port);
            fclose(fd);
            addToList(&args->ctx->list, rx_pack.sender_nick, rx_pack.peer_port);
            pSend = createPacket(TYPE_REGISTER_ACK, "Server", rx_pack.sender_nick, GATEWAY, "REGISTER_SUCCESS");
            printf("[SERVER] Nuovo utente registrato: %s (Porta: %d)\n", rx_pack.sender_nick, rx_pack.peer_port);
        } else {
            pSend = createPacket(TYPE_REGISTER_ERROR, "Server", rx_pack.sender_nick, GATEWAY, "INTERNAL_DB_ERROR");
        }
    }

    pthread_mutex_unlock(&args->ctx->db_lock);

    packetToNetworkByteOrder(&pSend);
    sendto(args->ctx->sockfd, &pSend, sizeof(pSend), 0, (struct sockaddr*)&args->sender_addr, len);
}

void processLookup(RequestHandlerArgs* args) {
    Packet rx_pack = args->rx_packet;
    Packet pSend;

    pthread_mutex_lock(&args->ctx->db_lock);
    if (userExists(args->ctx->list, rx_pack.target_nick)){
        // Diamo al client la porta del destinatario così può scrivergli in P2P
        int32_t destPort = getPort(args->ctx->list, rx_pack.target_nick);
        pSend = createPacket(TYPE_LOOKUP, "Server", rx_pack.target_nick, destPort, "USER_FOUND");
    } else {
        pSend = createPacket(TYPE_REGISTER_ERROR, "Server", rx_pack.sender_nick, 0, "USER_OFFLINE_OR_NOT_FOUND");
    }
    pthread_mutex_unlock(&args->ctx->db_lock);

    packetToNetworkByteOrder(&pSend);
    sendto(args->ctx->sockfd, &pSend, sizeof(pSend), 0, (struct sockaddr*)&args->sender_addr, sizeof(args->sender_addr));
}

// Punto di ingresso per i thread generati ad ogni richiesta
void* requestHandler(void* arg) {
    RequestHandlerArgs* args = (RequestHandlerArgs*)arg;
    
    if (args->rx_packet.type == TYPE_REGISTER) {
        processRegister(args);
    } else if (args->rx_packet.type == TYPE_LOOKUP) {
        processLookup(args);
    }
    
    free(args); // Puliamo la memoria del thread "staccato"
    return NULL;
}

int main(void){
    ServerContext ctx;
    ctx.list = NULL;
    pthread_mutex_init(&ctx.db_lock, NULL);
    
    loadUsers(&ctx);

    struct sockaddr_in addressServer = createAddress(LOCALHOST, GATEWAY);
    ctx.sockfd = createSocketIpv4();
    
    if (bind(ctx.sockfd, (struct sockaddr*)&addressServer, sizeof(addressServer)) < 0) {
        perror("[-] Bind del server fallito");
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] Tracker P2P in ascolto sulla porta %d...\n", GATEWAY);

    while(1){
        struct sockaddr_in senderAddr;
        socklen_t len = sizeof(senderAddr);
        Packet rx_pack;
        
        if (recvfrom(ctx.sockfd, &rx_pack, sizeof(rx_pack), 0, (struct sockaddr*)&senderAddr, &len) > 0) {
            packetToHostByteOrder(&rx_pack);

            if (rx_pack.type != TYPE_LOOKUP) {
                printPacket(&rx_pack);
            }

            // Lanciamo un thread "detached" per non bloccare il server e rispondere in parallelo
            RequestHandlerArgs* args = (RequestHandlerArgs*) malloc(sizeof(RequestHandlerArgs));
            if (args) {
                args->rx_packet = rx_pack;
                args->sender_addr = senderAddr;
                args->ctx = &ctx;

                pthread_t tid;
                pthread_create(&tid, NULL, requestHandler, args);
                pthread_detach(tid); 
            }
        }
    }
    
    pthread_mutex_destroy(&ctx.db_lock);
    close(ctx.sockfd);
    return 0;
}