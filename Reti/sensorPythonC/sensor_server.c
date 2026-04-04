#include "sensor_packet.h"
#include "sensor_utils.h"

typedef struct SensorNode {
    int32_t id;
    int tcpSocket;
    struct SensorNode* next;
} SensorNode;

typedef struct {
    struct sockaddr_in senderAddress;
    int sockSender;
    pthread_mutex_t* mutex;
    SensorNode** head;
} TcpThreadParams;

typedef struct {
    struct sockaddr_in serverAddressTcp;
    struct sockaddr_in serverAddressUdp;
    int sockTcp;
    int sockUdp;
    pthread_mutex_t* mutex;
    SensorNode** head;
} ConnectionInfoParams;

void addSensor(SensorNode** head, int32_t id, int sockTcp) {
    SensorNode* temp = (SensorNode*)malloc(sizeof(SensorNode));
    if (!temp) return;
    temp->id = id;
    temp->tcpSocket = sockTcp;
    temp->next = *head;
    *head = temp;
}

void removeSensor(SensorNode** head, int sockTcp) {
    SensorNode* current = *head;
    SensorNode* prev = NULL;

    while (current != NULL) {
        if (current->tcpSocket == sockTcp) {
            if (prev == NULL) {
                *head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            break;
        }
        prev = current;
        current = current->next;
    }
}

void broadcastUpdate(SensorNode** head, Packet networkPack, int32_t senderId) {
    SensorNode* copy = *head;
    while (copy != NULL) {
        if (copy->id != senderId) {
            send(copy->tcpSocket, &networkPack, sizeof(Packet), 0);
        }
        copy = copy->next;
    }
}

void* handleClientConnection(void* args) {
    TcpThreadParams* par = (TcpThreadParams*)args;
    
    Packet pack;
    if (recv(par->sockSender, &pack, sizeof(Packet), 0) > 0) {
        packetToHostByteOrder(&pack);
        
        pthread_mutex_lock(par->mutex);
        addSensor(par->head, pack.id, par->sockSender);
        pthread_mutex_unlock(par->mutex);
        
        printf("[SERVER] Sensor ID %d registered.\n", pack.id);
    }

    char dummy;
    while (recv(par->sockSender, &dummy, 1, 0) > 0) {}

    // Pulizia dopo la disconnessione
    printf("[SERVER] Sensor disconnected. Cleaning up...\n");
    pthread_mutex_lock(par->mutex);
    removeSensor(par->head, par->sockSender);
    pthread_mutex_unlock(par->mutex);

    close(par->sockSender);
    free(par); // Libera la memoria allocata dalla malloc nel main TCP
    return NULL;
}

void* handleUdpConnection(void* args) {
    ConnectionInfoParams par = *(ConnectionInfoParams*)args;
    
    struct sockaddr_in pythonServer = createAddress(LOCALHOST, PORT_PYTHON_SERVER);
    int sockUdpPython = createSocketUdp();
    
    if (bind(par.sockUdp, (struct sockaddr*)&par.serverAddressUdp, sizeof(par.serverAddressUdp)) < 0) {
        perror("[-] UDP Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] UDP Monitoring active...\n");
    
    Packet pack;
    struct sockaddr_in senderAddress;
    socklen_t len = sizeof(senderAddress);
    
    while (1) {
        if (recvfrom(par.sockUdp, &pack, sizeof(Packet), 0, (struct sockaddr*)&senderAddress, &len) > 0) {
            
            Packet hostPack = pack; // Copia per uso locale
            packetToHostByteOrder(&hostPack);
            
            printf("[UDP RECV] ID: %d | Value: %.2f\n", hostPack.id, hostPack.value_sensor);
            
            // Inoltro ai client C connessi (escludendo il mittente)
            pthread_mutex_lock(par.mutex);
            broadcastUpdate(par.head, pack, hostPack.id);
            pthread_mutex_unlock(par.mutex);
            
            // Forward al Dashboard Python
            sendto(sockUdpPython, &pack, sizeof(Packet), 0, (struct sockaddr*)&pythonServer, sizeof(pythonServer));
        }
    }
    return NULL;
}

int main(void) {
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    SensorNode* head = NULL;

    struct sockaddr_in serverAddressTcp = createAddress(LOCALHOST, PORT_TCP_SERVER);
    struct sockaddr_in serverAddressUdp = createAddress(LOCALHOST, PORT_UDP_SERVER);
    int sockTcp = createSocketTcp();
    int sockUdp = createSocketUdp();

    if (bind(sockTcp, (struct sockaddr*)&serverAddressTcp, sizeof(serverAddressTcp)) < 0) {
        perror("[-] TCP Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(sockTcp, 10) < 0) {
        perror("[-] TCP Listen failed");
        exit(EXIT_FAILURE);
    }

    ConnectionInfoParams par = {serverAddressTcp, serverAddressUdp, sockTcp, sockUdp, &mutex, &head};
    
    pthread_t tid;
    pthread_create(&tid, NULL, handleUdpConnection, &par);
    pthread_detach(tid);

    printf("[SERVER] TCP Gateway listening...\n");

    while (1) {
        struct sockaddr_in senderAddress;
        socklen_t len = sizeof(senderAddress);
        int sockSender = accept(sockTcp, (struct sockaddr*)&senderAddress, &len);
        
        if (sockSender >= 0) {
            TcpThreadParams* threadParams = (TcpThreadParams*)malloc(sizeof(TcpThreadParams));
            if (threadParams) {
                threadParams->senderAddress = senderAddress;
                threadParams->sockSender = sockSender;
                threadParams->mutex = &mutex;
                threadParams->head = &head;

                pthread_t clientTid;
                pthread_create(&clientTid, NULL, handleClientConnection, threadParams);
                pthread_detach(clientTid);
            }
        }
    }

    pthread_mutex_destroy(&mutex);
    close(sockTcp);
    close(sockUdp);
    return 0;
}