#include "terapy_packet.h"
#include "terapy_utils.h"

typedef struct ClientNode {
    int32_t patient_id;
    int client_socket;
    struct sockaddr_in client_address;
    struct ClientNode* next;
} ClientNode;

typedef struct {
    struct sockaddr_in server_address;
    int server_socket; 
    
    struct sockaddr_in client_address;
    int client_socket;
    
    pthread_mutex_t* shared_mutex;
    ClientNode** client_list_head;
} ThreadContext;

// Cerca il socket del paziente usando il suo ID
int getClientSocket(ThreadContext* par, int32_t idPatient){
    pthread_mutex_lock(par->shared_mutex);
    ClientNode *current = *(par->client_list_head);
    
    while(current != NULL){
        if (current->patient_id == idPatient){
            int sock = current->client_socket;
            pthread_mutex_unlock(par->shared_mutex);
            return sock;
        }
        current = current->next;
    }
    pthread_mutex_unlock(par->shared_mutex);
    return -1;
}

// Aggiunge in sicurezza un paziente alla lista
void addClient(ThreadContext* ctx, ClientNode new_client) {
    ClientNode* temp = (ClientNode*) malloc(sizeof(ClientNode));
    if (temp == NULL) return; 

    temp->client_address = new_client.client_address;
    temp->client_socket = new_client.client_socket; 
    temp->patient_id = new_client.patient_id;
    
    pthread_mutex_lock(ctx->shared_mutex);
    temp->next = *(ctx->client_list_head);
    *(ctx->client_list_head) = temp;
    pthread_mutex_unlock(ctx->shared_mutex);
}

void deletePatient(ThreadContext* ctx, int client_socket_to_remove) {
    pthread_mutex_lock(ctx->shared_mutex);
    
    ClientNode** current = ctx->client_list_head;
    while (*current != NULL) {
        if ((*current)->client_socket == client_socket_to_remove) {
            ClientNode* to_delete = *current;
            *current = (*current)->next;
            free(to_delete);             
            break; 
        }
        current = &((*current)->next);
    }
    
    pthread_mutex_unlock(ctx->shared_mutex);
}

void handlePatientAdmit(ThreadContext* ctx, MedPacket* packet) {
    ClientNode client = {packet->patient_id, ctx->client_socket, ctx->client_address, NULL};
    addClient(ctx, client);
    
    MedPacket pack = createPacket(TYPE_ADMIT_ACK, packet->patient_id, 0, 0);
    packetToNetworkByteOrder(&pack);
    send(ctx->client_socket, &pack, sizeof(pack), 0);
}

// Gestisce le richieste TCP di un singolo paziente
void* handleThreadTcpClient(void* args) {
    ThreadContext ctx = *(ThreadContext*) args;
    free(args); 
    
    MedPacket packet;
    int bytes_received;
    
    while(1) {
        bytes_received = recv(ctx.client_socket, &packet, sizeof(packet), 0);
        
        if (bytes_received > 0) {
            packetToHostByteOrder(&packet);
            printPacket(&packet);
            
            if (packet.type == TYPE_PATIENT_ADMIT) {
                handlePatientAdmit(&ctx, &packet);
            }
        } 
        else {
            printf("[-] Paziente disconnesso (Socket: %d).\n", ctx.client_socket);
            deletePatient(&ctx, ctx.client_socket); 
            close(ctx.client_socket);
            break; 
        }
    }
    return NULL;
}

void* handleTcpConnection(void* args) {
    ThreadContext ctx = *(ThreadContext*) args;
    socklen_t len = sizeof(ctx.client_address);

    if (bind(ctx.server_socket, (struct sockaddr*) &ctx.server_address, sizeof(ctx.server_address)) < 0) {
        perror("[-] Errore bind server TCP");
        exit(EXIT_FAILURE);
    }
    if (listen(ctx.server_socket, 10) < 0) {
        perror("[-] Errore listen server TCP");
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] Reparto ospedale TCP in ascolto...\n");

    while(1) {
        struct sockaddr_in clientAddress;
        int connected_socket = accept(ctx.server_socket, (struct sockaddr*)&clientAddress, &len);
        
        if (connected_socket < 0) {
            perror("[-] Errore accept");
            continue;
        }
        
        // Passiamo il contesto al nuovo thread in modo pulito
        ThreadContext* new_ctx = malloc(sizeof(ThreadContext));
        if (new_ctx) {
            *new_ctx = ctx; 
            new_ctx->client_socket = connected_socket;
            new_ctx->client_address = clientAddress;
            
            pthread_t threadId;
            pthread_create(&threadId, NULL, handleThreadTcpClient, new_ctx);
            pthread_detach(threadId);
        }
    }
    return NULL;
}

void* handleUdpConnection(void* args) {
    ThreadContext ctx = *(ThreadContext*) args;
    
    if (bind(ctx.server_socket, (struct sockaddr*) &ctx.server_address, sizeof(ctx.server_address)) < 0) {
        perror("[-] Errore bind server UDP");
        exit(EXIT_FAILURE);
    }
    
    printf("[SERVER] Monitoraggio vitale UDP attivo...\n");

    while(1) {
        MedPacket receivedPacket;
        struct sockaddr_in clientAddress;
        socklen_t len = sizeof(clientAddress);
        
        if (recvfrom(ctx.server_socket, &receivedPacket, sizeof(receivedPacket), 0, (struct sockaddr*)&clientAddress, &len) > 0) {
            packetToHostByteOrder(&receivedPacket);
            
            if (receivedPacket.type == TYPE_VITALS_UPDATE) {
                printPacket(&receivedPacket);

                // Se i parametri crollano, facciamo partire il Codice Rosso
                if (receivedPacket.bpm < 40 || receivedPacket.oxygen < 90) {
                    MedPacket urgPack = createPacket(TYPE_CODE_RED, receivedPacket.patient_id, 0, 0);
                    packetToNetworkByteOrder(&urgPack);
                    
                    int sockTcpClient = getClientSocket(&ctx, receivedPacket.patient_id);
                    if (sockTcpClient != -1) {
                        send(sockTcpClient, &urgPack, sizeof(urgPack), 0);
                        printf("[!] INVIATO CODICE ROSSO AL PAZIENTE %d\n", receivedPacket.patient_id);
                    }
                }
            }
        }
    }
    return NULL;
}

int main(void) {
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    
    struct sockaddr_in serverTcp = createAddress(LOCALHOST, PORT_TCP_HOSPITAL);
    struct sockaddr_in serverUdp = createAddress(LOCALHOST, PORT_UDP_HOSPITAL);

    int sockTcp = createSocketTcp();
    int sockUdp = createSocketUdp();

    ClientNode* head = NULL;

    pthread_t threadTcp, threadUdp;
    
    ThreadContext ctxTcp;
    memset(&ctxTcp, 0, sizeof(ThreadContext));
    ctxTcp.server_address = serverTcp;
    ctxTcp.server_socket = sockTcp;
    ctxTcp.shared_mutex = &mutex;
    ctxTcp.client_list_head = &head;
    
    ThreadContext ctxUdp;
    memset(&ctxUdp, 0, sizeof(ThreadContext));
    ctxUdp.server_address = serverUdp;
    ctxUdp.server_socket = sockUdp;
    ctxUdp.shared_mutex = &mutex;
    ctxUdp.client_list_head = &head;

    pthread_create(&threadTcp, NULL, handleTcpConnection, &ctxTcp);
    pthread_create(&threadUdp, NULL, handleUdpConnection, &ctxUdp);

    pthread_join(threadTcp, NULL);
    pthread_join(threadUdp, NULL);
    
    pthread_mutex_destroy(&mutex);
    return 0;
}