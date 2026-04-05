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
    
    // Shared state between UI thread and Network thread
    char pending_target[32];  
    char pending_message[255]; 
    pthread_mutex_t lock; // Protects pending_target and pending_message
} ClientState;

// RECEIVER THREAD: Handles Server responses and P2P messages

void* waitForMessage(void* arg){
    ClientState* state = (ClientState*) arg;
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(senderAddr);

    while(1){
        Packet p;
        if (recvfrom(state->sockfd, &p, sizeof(p), 0, (struct sockaddr*)&senderAddr, &len) > 0) {
            packetToHostByteOrder(&p); // Deserialize from network
            
            // CASE A: Server responded with peer's port (Lookup Success)
            if (p.type == TYPE_LOOKUP && strcmp(p.sender_nick, "Server") == 0){
                struct sockaddr_in destAddr = createAddress(LOCALHOST, p.peer_port);
                
                // Safely read the pending message using mutex
                pthread_mutex_lock(&state->lock);
                Packet pSend = createPacket(TYPE_MESSAGE, state->input.nickname, state->pending_target, 
                                            state->input.port, state->pending_message);
                pthread_mutex_unlock(&state->lock);
                
                packetToNetworkByteOrder(&pSend); // Serialize for network
                sendto(state->sockfd, &pSend, sizeof(pSend), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
                
                printf("\n[SYSTEM] Message delivered to %s!\n", pSend.target_nick);
                printf("\n[HOST %d] Who do you want to message?\n> ", state->input.port);
                fflush(stdout);
            }
            
            // CASE B: Received a direct P2P message
            else if (p.type == TYPE_MESSAGE) {
                printf("\n\n\a[MESSAGE FROM %s]: %s\n", p.sender_nick, p.payload);
                printf("\n[HOST %d] Who do you want to message?\n> ", state->input.port);
                fflush(stdout);
            }
            
            // CASE C: Server Error (e.g., User Offline)
            else if (p.type == TYPE_REGISTER_ERROR) {
                printf("\n[SERVER ERROR]: %s\n", p.payload);
                printf("\n[HOST %d] Who do you want to message?\n> ", state->input.port);
                fflush(stdout);
            }
        }
    }
    return NULL;
}

// SENDER THREAD: Handles User Input
void* senderMessage(void* arg){
    ClientState* state = (ClientState*) arg;
    usleep(100000); // Small delay to let the UI settle

    while(1){
        char target_buffer[32];
        char message_buffer[255];

        printf("[HOST %d] Who do you want to message?\n> ", state->input.port);
        fgets(target_buffer, sizeof(target_buffer), stdin);
        target_buffer[strcspn(target_buffer, "\n")] = 0; 

        printf("Type your message:\n> ");
        fgets(message_buffer, sizeof(message_buffer), stdin);
        message_buffer[strcspn(message_buffer, "\n")] = 0; 

        // Safely update shared state
        pthread_mutex_lock(&state->lock);
        strncpy(state->pending_target, target_buffer, sizeof(state->pending_target) - 1);
        strncpy(state->pending_message, message_buffer, sizeof(state->pending_message) - 1);
        pthread_mutex_unlock(&state->lock);

        // Ask server for peer port (LOOKUP)
        Packet pSend = createPacket(TYPE_LOOKUP, state->input.nickname, target_buffer, state->input.port, "Request Port");
        packetToNetworkByteOrder(&pSend);
        
        sendto(state->sockfd, &pSend, sizeof(pSend), 0, (struct sockaddr*)&state->serverAddress, sizeof(state->serverAddress));
        
        usleep(500000); // Prevent UI spamming
    }
    return NULL;
}

int main(int argc, char* argv[]){  
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Nickname> <Port>\n", argv[0]);
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
        perror("[-] Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("[SYSTEM] Registering to Tracker Server...\n");

    // Authenticate
    Packet p = createPacket(TYPE_REGISTER, state.input.nickname, "Server", state.input.port, "REGISTER_REQ");
    packetToNetworkByteOrder(&p);
    sendto(state.sockfd, &p, sizeof(p), 0, (struct sockaddr*)&state.serverAddress, sizeof(state.serverAddress));
    
    // Wait for auth ack
    Packet rx_packet;
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(senderAddr);
    recvfrom(state.sockfd, &rx_packet, sizeof(rx_packet), 0, (struct sockaddr*)&senderAddr, &len);
    packetToHostByteOrder(&rx_packet);
    
    if (rx_packet.type == TYPE_REGISTER_ERROR){
        printf("\n[FATAL] Registration failed: %s\n", rx_packet.payload);
        close(state.sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[SYSTEM] Authenticated as '%s' on port %d!\n\n", state.input.nickname, state.input.port);

    pthread_t tid_receive, tid_send;
    pthread_create(&tid_receive, NULL, waitForMessage, &state);
    pthread_create(&tid_send, NULL, senderMessage, &state);
    
    pthread_join(tid_receive, NULL);
    pthread_join(tid_send, NULL);

    pthread_mutex_destroy(&state.lock);
    close(state.sockfd);
    return 0;
}
