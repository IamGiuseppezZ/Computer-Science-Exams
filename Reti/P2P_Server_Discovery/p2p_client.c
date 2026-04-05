#include "p2p_packet.h"
#include "p2p_utils.h"

typedef struct {
    char nickname[32];
    int port;
} InputData;

typedef struct {
    int sockfd;
    InputData input;
    struct sockaddr_in serverAddress;
    
    // Variabili condivise tra il thread di invio e quello di ricezione
    char pending_target[32];  
    char pending_message[255]; 
    pthread_mutex_t lock; 
} ClientState;

void* waitForMessage(void* arg){
    ClientState* state = (ClientState*) arg;
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(senderAddr);

    while(1){
        Packet p;
        if (recvfrom(state->sockfd, &p, sizeof(p), 0, (struct sockaddr*)&senderAddr, &len) > 0) {
            packetToHostByteOrder(&p); 
            
            // CASO A: Il server ci ha risposto con la porta dell'utente (Lookup OK)
            // Ora possiamo mandare il messaggio direttamente a lui (P2P vero e proprio)
            if (p.type == TYPE_LOOKUP && strcmp(p.sender_nick, "Server") == 0){
                struct sockaddr_in destAddr = createAddress(LOCALHOST, p.peer_port);
                
                // Leggiamo il messaggio in sospeso in sicurezza
                pthread_mutex_lock(&state->lock);
                Packet pSend = createPacket(TYPE_MESSAGE, state->input.nickname, state->pending_target, 
                                            state->input.port, state->pending_message);
                pthread_mutex_unlock(&state->lock);
                
                packetToNetworkByteOrder(&pSend); 
                sendto(state->sockfd, &pSend, sizeof(pSend), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
                
                printf("\n[SISTEMA] Messaggio consegnato a %s!\n", pSend.target_nick);
                printf("\n[HOST %d] A chi vuoi scrivere?\n> ", state->input.port);
                fflush(stdout);
            }
            
            // CASO B: Qualcuno ci ha mandato un messaggio diretto (P2P)
            else if (p.type == TYPE_MESSAGE) {
                printf("\n\n\a[MESSAGGIO DA %s]: %s\n", p.sender_nick, p.payload);
                printf("\n[HOST %d] A chi vuoi scrivere?\n> ", state->input.port);
                fflush(stdout);
            }
            
            // CASO C: Errore dal server (es. utente offline)
            else if (p.type == TYPE_REGISTER_ERROR) {
                printf("\n[ERRORE SERVER]: %s\n", p.payload);
                printf("\n[HOST %d] A chi vuoi scrivere?\n> ", state->input.port);
                fflush(stdout);
            }
        }
    }
    return NULL;
}

void* senderMessage(void* arg){
    ClientState* state = (ClientState*) arg;
    usleep(100000); 

    while(1){
        char target_buffer[32];
        char message_buffer[255];

        printf("[HOST %d] A chi vuoi scrivere?\n> ", state->input.port);
        fgets(target_buffer, sizeof(target_buffer), stdin);
        target_buffer[strcspn(target_buffer, "\n")] = 0; 

        printf("Scrivi il messaggio:\n> ");
        fgets(message_buffer, sizeof(message_buffer), stdin);
        message_buffer[strcspn(message_buffer, "\n")] = 0; 

        // Salviamo destinatario e messaggio in attesa che il server ci dia la porta
        pthread_mutex_lock(&state->lock);
        strncpy(state->pending_target, target_buffer, sizeof(state->pending_target) - 1);
        strncpy(state->pending_message, message_buffer, sizeof(state->pending_message) - 1);
        pthread_mutex_unlock(&state->lock);

        // Chiediamo al server "Ehi, su che porta sta questo utente?"
        Packet pSend = createPacket(TYPE_LOOKUP, state->input.nickname, target_buffer, state->input.port, "Richiesta Porta");
        packetToNetworkByteOrder(&pSend);
        
        sendto(state->sockfd, &pSend, sizeof(pSend), 0, (struct sockaddr*)&state->serverAddress, sizeof(state->serverAddress));
        
        usleep(500000); // Pausetta per non far impazzire l'interfaccia
    }
    return NULL;
}

int main(int argc, char* argv[]){  
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <Nickname> <Porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    ClientState state;
    memset(&state, 0, sizeof(ClientState));
    strncpy(state.input.nickname, argv[1], sizeof(state.input.nickname) - 1);
    state.input.port = atoi(argv[2]);
    pthread_mutex_init(&state.lock, NULL);

    state.serverAddress = createAddress(LOCALHOST, GATEWAY);
    struct sockaddr_in myAddress = createAddress(LOCALHOST, state.input.port);

    state.sockfd = createSocketIpv4();
    if (bind(state.sockfd, (struct sockaddr*)&myAddress, sizeof(myAddress)) < 0) {
        perror("[-] Bind fallito");
        exit(EXIT_FAILURE);
    }

    printf("[SISTEMA] Registrazione al Tracker Server...\n");

    Packet p = createPacket(TYPE_REGISTER, state.input.nickname, "Server", state.input.port, "REGISTER_REQ");
    packetToNetworkByteOrder(&p);
    sendto(state.sockfd, &p, sizeof(p), 0, (struct sockaddr*)&state.serverAddress, sizeof(state.serverAddress));
    
    Packet rx_packet;
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(senderAddr);
    recvfrom(state.sockfd, &rx_packet, sizeof(rx_packet), 0, (struct sockaddr*)&senderAddr, &len);
    packetToHostByteOrder(&rx_packet);
    
    if (rx_packet.type == TYPE_REGISTER_ERROR){
        printf("\n[FATALE] Registrazione fallita: %s\n", rx_packet.payload);
        close(state.sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[SISTEMA] Autenticato come '%s' sulla porta %d!\n\n", state.input.nickname, state.input.port);

    pthread_t tid_receive, tid_send;
    pthread_create(&tid_receive, NULL, waitForMessage, &state);
    pthread_create(&tid_send, NULL, senderMessage, &state);
    
    pthread_join(tid_receive, NULL);
    pthread_join(tid_send, NULL);

    pthread_mutex_destroy(&state.lock);
    close(state.sockfd);
    return 0;
}