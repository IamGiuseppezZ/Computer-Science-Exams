#include "traffic_packet.h"
#include "traffic_utils.h"

typedef struct {
    struct sockaddr_in serverAddress;
    int sockfd;
} ThreadReceiverArgs;

void* handleTcpReceiver(void* args) {
    ThreadReceiverArgs par = *(ThreadReceiverArgs*)args;
    ssize_t receivedBytes;
    AlertPacket pack;
    
    while (1) {
        receivedBytes = recv(par.sockfd, &pack, sizeof(pack), 0);
        
        if (receivedBytes > 0) {
            printAlertPacket(&pack);
        } else if (receivedBytes == 0) {
            printf("[!] Server closed the connection.\n");
            break;
        } else {
            perror("[-] TCP connection error");
            break;
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <Client_Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int sockfd = createSocketTcp();
    struct sockaddr_in serverAddress = createAddress(LOCALHOST, PORT_SERVER_TCP);
    
    printf("[POLICE] Attempting connection to central server...\n");
    if (connect(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("[-] Failed to connect to central server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("[POLICE] Connection established. Listening for alerts...\n");

    pthread_t tid;
    ThreadReceiverArgs threadArgs = {serverAddress, sockfd};
    
    if (pthread_create(&tid, NULL, handleTcpReceiver, &threadArgs) != 0) {
        perror("[-] Failed to spawn receiver thread");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    pthread_join(tid, NULL);
    
    close(sockfd);
    return 0;
}
