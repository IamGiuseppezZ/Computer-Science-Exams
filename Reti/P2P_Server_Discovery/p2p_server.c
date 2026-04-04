#include "p2p_packet.h"
#include "p2p_utils.h"

#define FILENAME "users.txt"

typedef struct UserRecord {
    char nickname[32];
    int32_t port;
    struct UserRecord* next;
} UserRecord;

// Global thread-safe state for the server
typedef struct {
    UserRecord* list;
    pthread_mutex_t db_lock;
    int sockfd;
} ServerContext;

// Thread arguments
typedef struct {
    Packet rx_packet;
    struct sockaddr_in sender_addr;
    ServerContext* ctx;
} RequestHandlerArgs;

// --- DB Operations (Must be called with mutex locked!) ---
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
        // New Registration
        FILE* fd = fopen(FILENAME, "a+");
        if (fd) {
            fprintf(fd, "%s %d\n", rx_pack.sender_nick, rx_pack.peer_port);
            fclose(fd);
            addToList(&args->ctx->list, rx_pack.sender_nick, rx_pack.peer_port);
            pSend = createPacket(TYPE_REGISTER_ACK, "Server", rx_pack.sender_nick, GATEWAY, "REGISTER_SUCCESS");
            printf("[SERVER] New user registered: %s (Port: %d)\n", rx_pack.sender_nick, rx_pack.peer_port);
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
        int32_t destPort = getPort(args->ctx->list, rx_pack.target_nick);
        pSend = createPacket(TYPE_LOOKUP, "Server", rx_pack.target_nick, destPort, "USER_FOUND");
    } else {
        pSend = createPacket(TYPE_REGISTER_ERROR, "Server", rx_pack.sender_nick, 0, "USER_OFFLINE_OR_NOT_FOUND");
    }
    pthread_mutex_unlock(&args->ctx->db_lock);

    packetToNetworkByteOrder(&pSend);
    sendto(args->ctx->sockfd, &pSend, sizeof(pSend), 0, (struct sockaddr*)&args->sender_addr, sizeof(args->sender_addr));
}

// Thread entry point for incoming requests
void* requestHandler(void* arg) {
    RequestHandlerArgs* args = (RequestHandlerArgs*)arg;
    
    if (args->rx_packet.type == TYPE_REGISTER) {
        processRegister(args);
    } else if (args->rx_packet.type == TYPE_LOOKUP) {
        processLookup(args);
    }
    
    free(args); // Clean up detached thread memory
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
        perror("[-] Server bind failed");
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] Tracker is running on port %d...\n", GATEWAY);

    while(1){
        struct sockaddr_in senderAddr;
        socklen_t len = sizeof(senderAddr);
        Packet rx_pack;
        
        if (recvfrom(ctx.sockfd, &rx_pack, sizeof(rx_pack), 0, (struct sockaddr*)&senderAddr, &len) > 0) {
            packetToHostByteOrder(&rx_pack);

            if (rx_pack.type != TYPE_LOOKUP) {
                printPacket(&rx_pack);
            }

            // Spawn a detached thread to handle the request
            RequestHandlerArgs* args = (RequestHandlerArgs*) malloc(sizeof(RequestHandlerArgs));
            if (args) {
                args->rx_packet = rx_pack;
                args->sender_addr = senderAddr;
                args->ctx = &ctx;

                pthread_t tid;
                pthread_create(&tid, NULL, requestHandler, args);
                pthread_detach(tid); // Detach so we don't have to join it
            }
        }
    }
    
    pthread_mutex_destroy(&ctx.db_lock);
    close(ctx.sockfd);
    return 0;
}