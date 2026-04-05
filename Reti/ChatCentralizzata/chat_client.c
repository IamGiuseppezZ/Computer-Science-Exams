#include "chat_packet.h"
#include "chat_utils.h"

typedef struct {
    int sockfd;
    char nickname[32];
    int port;
    struct sockaddr_in serverAddress;
} ClientContext;

void* waitForMessage(void* arg){
    ClientContext* ctx = (ClientContext*) arg;
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(senderAddr);

    while(1){
        Packet p;
        if (recvfrom(ctx->sockfd, &p, sizeof(p), 0, (struct sockaddr*)&senderAddr, &len) > 0) {
            packetToHostByteOrder(&p);
            
            // Stampa a schermo il messaggio e ripristina la grafica per far scrivere l'utente
            if (p.type == TYPE_LOOKUP) {
                printf("\n\a[MESSAGE FROM %s]: %s\n", p.sender_nick, p.payload);
                printf("> Destinatario: "); 
                fflush(stdout);
            } else if (p.type == TYPE_REGISTER_ERROR) {
                printf("\n[SERVER ERROR]: %s\n", p.payload);
                printf("> Destinatario: ");
                fflush(stdout);
            }
        }
    }
    return NULL;
}

void* senderMessage(void* arg){
    ClientContext* ctx = (ClientContext*) arg;
    usleep(100000); 

    while(1){
        char target[32];
        char message[255];

        printf("\n> Destinatario: ");
        fgets(target, sizeof(target), stdin);
        target[strcspn(target, "\n")] = 0;

        printf("> Messaggio: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = 0;

        // Invece di contattare l'utente diretto, diamo il pacchetto al server proxy
        // che si occuperà di recapitarglielo
        Packet pSend = createPacket(TYPE_LOOKUP, ctx->nickname, target, ctx->port, message);
        packetToNetworkByteOrder(&pSend);

        sendto(ctx->sockfd, &pSend, sizeof(pSend), 0, (struct sockaddr*)&ctx->serverAddress, sizeof(ctx->serverAddress));
        
        usleep(100000); 
    }
    return NULL;
}

int main(int argc, char* argv[]){  
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <Nickname> <Porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    ClientContext ctx;
    strncpy(ctx.nickname, argv[1], sizeof(ctx.nickname) - 1);
    ctx.port = atoi(argv[2]);
    ctx.serverAddress = createAddress(LOCALHOST, GATEWAY);
    
    struct sockaddr_in myAddress = createAddress(LOCALHOST, ctx.port);
    ctx.sockfd = createSocketIpv4();

    if (bind(ctx.sockfd, (struct sockaddr*)&myAddress, sizeof(myAddress)) < 0) {
        perror("[-] Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("[SYSTEM] In attesa di registrazione al server...\n");

    Packet p = createPacket(TYPE_REGISTER, ctx.nickname, "Server", ctx.port, "REGISTER_REQ");
    packetToNetworkByteOrder(&p);
    sendto(ctx.sockfd, &p, sizeof(p), 0, (struct sockaddr*)&ctx.serverAddress, sizeof(ctx.serverAddress));
    
    Packet rx_packet;
    struct sockaddr_in senderAddress;
    socklen_t len = sizeof(senderAddress);
    recvfrom(ctx.sockfd, &rx_packet, sizeof(rx_packet), 0, (struct sockaddr*)&senderAddress, &len);
    packetToHostByteOrder(&rx_packet);
    
    if (rx_packet.type == TYPE_REGISTER_ERROR){
        printf("\n[FATALE] Registrazione fallita: %s\n", rx_packet.payload);
        close(ctx.sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[SYSTEM] Autenticato come '%s' sulla porta %d!\n", ctx.nickname, ctx.port);

    pthread_t tid_receive, tid_send;
    pthread_create(&tid_receive, NULL, waitForMessage, &ctx);
    pthread_create(&tid_send, NULL, senderMessage, &ctx);
    
    pthread_join(tid_receive, NULL);
    pthread_join(tid_send, NULL);

    close(ctx.sockfd);
    return 0;
}