#include "traffic_packet.h"
#include "traffic_utils.h"

typedef struct PoliceNode {
    struct PoliceNode* next;
    int sockfdPolice;
    struct sockaddr_in policeAddress;
} PoliceNode;

typedef struct {
    int sockUdpServer;
    struct sockaddr_in serverAddressUdp;
    struct sockaddr_in serverAddressTcp;
    int sockTcpServer;
    PoliceNode** head;
    pthread_mutex_t* mutex;
} ServerParams;

typedef struct {
    struct sockaddr_in serverAddressTcp;
    struct sockaddr_in myAddressTcp;
    int sockfd;
} ClientHandlerArgs;

void addPoliceNode(PoliceNode** head, struct sockaddr_in senderAddress, int sockfd) {
    PoliceNode* temp = (PoliceNode*)malloc(sizeof(PoliceNode));
    if (!temp) return; // Guard against memory allocation failure
    temp->policeAddress = senderAddress;
    temp->sockfdPolice = sockfd;
    temp->next = *head;
    *head = temp;
}

void broadcastAlarmTcp(ServerParams* par, Packet pack, int typeAlarm) {
    AlertPacket alert;
    memset(&alert, 0, sizeof(AlertPacket));
    
    alert.typeAllarm = htonl(typeAlarm);
    alert.idSensorProblem = htonl(pack.idSensor);
    alert.numCars = htonl(pack.numeroVeicoli);
    
    // Float serialization
    uint32_t tmp_vel;
    memcpy(&tmp_vel, &pack.velocitaMedia, sizeof(float));
    tmp_vel = htonl(tmp_vel);
    memcpy(&alert.velocity, &tmp_vel, sizeof(float));
    
    // Set appropriate description
    if (typeAlarm == TYPE_ALARM_CAR) {
        strncpy(alert.description, "HEAVY TRAFFIC DETECTED! |!ALERT CAR\n", sizeof(alert.description) - 1);
    } else if (typeAlarm == TYPE_ALARM_VELOCITY) {
        strncpy(alert.description, "UNUSUAL LOW SPEED DETECTED! |!ALERT VELOCITY\n", sizeof(alert.description) - 1);
    }

    // Thread-safe iteration over the linked list
    pthread_mutex_lock(par->mutex);
    PoliceNode* current = *(par->head);
    while (current != NULL) {
        send(current->sockfdPolice, &alert, sizeof(alert), 0);
        current = current->next;
    }
    pthread_mutex_unlock(par->mutex);
    
    printf("\n[SERVER] Alert (Type %d) dispatched to all active Police units.\n\n", typeAlarm);
}

void* handleClientTcp(void* args) {
    ClientHandlerArgs par = *((ClientHandlerArgs*)args);
    free(args); // Prevent memory leak

    PolicePacket packPolice;
    int bytesReceived;
    
    while (1) {
        bytesReceived = recv(par.sockfd, &packPolice, sizeof(packPolice), 0);
        
        if (bytesReceived > 0) {
            printPolicePacket(&packPolice);
        } else if (bytesReceived == 0) {
            printf("[SERVER TCP] Police client disconnected cleanly.\n");
            break; 
        } else {
            perror("[-] Error receiving from TCP client");
            break; 
        }
    }
    
    close(par.sockfd);
    return NULL;
}

void* startTcpListener(void* args) {
    ServerParams par = *((ServerParams*)args);
    int sockfd = par.sockTcpServer;
    
    if (bind(sockfd, (struct sockaddr*)&par.serverAddressTcp, sizeof(par.serverAddressTcp)) < 0) {
        perror("[-] Error binding TCP server");
        exit(EXIT_FAILURE);
    }
    if (listen(sockfd, MAX_HOSTS) < 0) {
        perror("[-] Error listening on TCP server");
        exit(EXIT_FAILURE);
    }
    
    printf("[SERVER] TCP listener active and waiting for police units...\n");

    while (1) {
        struct sockaddr_in senderAddress;
        socklen_t len = sizeof(senderAddress);
        int sockClient = accept(sockfd, (struct sockaddr*)&senderAddress, &len);
        
        if (sockClient < 0) {
            perror("[-] Accept failed");
            continue;
        }

        // Safely add new client to the broadcast list
        pthread_mutex_lock(par.mutex);
        addPoliceNode(par.head, senderAddress, sockClient);
        pthread_mutex_unlock(par.mutex);
        
        pthread_t tid;
        ClientHandlerArgs* threadArgs = (ClientHandlerArgs*)malloc(sizeof(ClientHandlerArgs));
        threadArgs->serverAddressTcp = par.serverAddressTcp;
        threadArgs->sockfd = sockClient;
        threadArgs->myAddressTcp = senderAddress;
        
        pthread_create(&tid, NULL, handleClientTcp, threadArgs);
        pthread_detach(tid); // Detach to auto-reclaim resources upon completion
    }
    return NULL;
}

void startUdpListener(ServerParams* par) {
    if (bind(par->sockUdpServer, (struct sockaddr*)&par->serverAddressUdp, sizeof(par->serverAddressUdp)) < 0) {
        perror("[-] Error binding UDP server");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in senderAddress;
    socklen_t len = sizeof(senderAddress);

    printf("[SERVER] UDP listener active. Waiting for sensor telemetry...\n");

    while (1) {
        Packet pack;
        ssize_t bytes = recvfrom(par->sockUdpServer, &pack, sizeof(pack), 0, (struct sockaddr*)&senderAddress, &len);
        
        if (bytes > 0) {
            convertPacket(&pack);
            printPacket(pack);
            
            // Analyze telemetry and trigger alerts if necessary
            if (pack.numeroVeicoli > 50) {
                broadcastAlarmTcp(par, pack, TYPE_ALARM_CAR);
            } else if (pack.velocitaMedia < 15.0f) {
                broadcastAlarmTcp(par, pack, TYPE_ALARM_VELOCITY);
            }
        }
    }
}

int main(void) {
    int sockUDP = createSocketUdp();
    int sockTcp = createSocketTcp();
    
    struct sockaddr_in serverAddressUdp = createAddress(LOCALHOST, PORT_SERVER_UDP);
    struct sockaddr_in serverAddressTcp = createAddress(LOCALHOST, PORT_SERVER_TCP);
    
    PoliceNode* head = NULL;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    ServerParams params = {sockUDP, serverAddressUdp, serverAddressTcp, sockTcp, &head, &mutex};
    
    pthread_t tcpThread;
    pthread_create(&tcpThread, NULL, startTcpListener, &params);
    
    // Main thread handles UDP sensor telemetry
    startUdpListener(&params);
    
    pthread_mutex_destroy(&mutex);
    close(sockUDP);
    close(sockTcp);
    
    return 0;
}