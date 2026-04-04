#include "packetwar.h"
#include "packetwar_utils.h"

typedef struct ListDrones {
    int32_t idDrone;
    int sockfd;
    struct ListDrones* next;
} ListDrones;

// Stato Globale Protetto
ListDrones* head = NULL;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;

void addDrone(int32_t droneId, int sockfd) {
    ListDrones* temp = (ListDrones*)malloc(sizeof(ListDrones));
    if (!temp) return;
    
    temp->idDrone = droneId;
    temp->sockfd = sockfd;
    
    pthread_mutex_lock(&list_mutex);
    temp->next = head;
    head = temp;
    pthread_mutex_unlock(&list_mutex);
}

void removeDrone(int32_t droneId) {
    pthread_mutex_lock(&list_mutex);
    ListDrones* current = head;
    ListDrones* prev = NULL;

    while (current != NULL) {
        if (current->idDrone == droneId) {
            if (prev == NULL) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            break;
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&list_mutex);
}

int getDroneSocket(int32_t droneId) {
    int sock = -1;
    pthread_mutex_lock(&list_mutex);
    ListDrones* temp = head;
    while (temp != NULL) {
        if (temp->idDrone == droneId) {
            sock = temp->sockfd;
            break;
        }
        temp = temp->next;
    }
    pthread_mutex_unlock(&list_mutex);
    return sock;
}

void* handleTcpClient(void* arg) {
    int clientSock = *(int*)arg;
    free(arg); 

    Packet p;
    // Aspetto il login
    if (recv(clientSock, &p, sizeof(Packet), 0) <= 0) {
        close(clientSock);
        return NULL;
    }
    
    packetToHostByteOrder(&p);
    
    if (p.type != TYPE_LOGIN) {
        close(clientSock);
        return NULL;
    }

    int32_t currentDroneId = p.drone_id;

    // Se il drone esiste già, rifiuto
    if (getDroneSocket(currentDroneId) != -1) {
        Packet nack = createPacket(TYPE_LOGIN_NACK, currentDroneId, 0, 0, 0);
        packetToNetworkByteOrder(&nack);
        send(clientSock, &nack, sizeof(Packet), 0);
        
        close(clientSock);
        return NULL;
    }

    // Accetto il drone
    addDrone(currentDroneId, clientSock);
    
    Packet ack = createPacket(TYPE_LOGIN_ACK, currentDroneId, 0, 0, 0);
    packetToNetworkByteOrder(&ack);
    send(clientSock, &ack, sizeof(Packet), 0);
    
    printf("[TCP] Drone %d authenticated and connected.\n", currentDroneId);

    // Mantengo viva la connessione TCP. Se si rompe, il drone è caduto o disconnesso.
    while (recv(clientSock, &p, sizeof(Packet), 0) > 0) {
        // Nessun dato extra previsto via TCP per ora.
    }

    printf("[TCP] Drone %d disconnected.\n", currentDroneId);
    removeDrone(currentDroneId);
    close(clientSock);
    
    return NULL;
}

void* handleUdpTelemetry(void* arg) {
    int udpSock = *(int*)arg;
    struct sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);
    Packet p;

    while (1) {
        if (recvfrom(udpSock, &p, sizeof(Packet), 0, (struct sockaddr*)&clientAddr, &len) > 0) {
            packetToHostByteOrder(&p);
            
            if (p.type == TYPE_TELEMETRY) {
                printf("[UDP] Drone %d | Pos: (%d, %d) | Battery: %d%%\n", 
                       p.drone_id, p.pos_x, p.pos_y, p.battery);

                // Controllo emergenza
                if (p.battery < 15) {
                    int targetSock = getDroneSocket(p.drone_id);
                    if (targetSock != -1) {
                        Packet emergency = createPacket(TYPE_EMERGENCY_LANDING, p.drone_id, 0, 0, 0);
                        packetToNetworkByteOrder(&emergency);
                        
                        send(targetSock, &emergency, sizeof(Packet), 0);
                        printf("[ALERT] Emergency landing command sent to Drone %d.\n", p.drone_id);
                        
                        // Per non intasare la rete di comandi di emergenza continui, 
                        // rimuoviamo preventivamente il drone o lo gestiamo sul client. 
                        // Il client dovrebbe fermare la telemetria.
                    }
                }
            }
        }
    }
    return NULL;
}

int main(void) {
    int tcpSock = createSocketTcp();
    int udpSock = createSocketUdp();
    
    struct sockaddr_in tcpAddr = createAddress(LOCALHOST, TCPLISTENSERVERPORT);
    struct sockaddr_in udpAddr = createAddress(LOCALHOST, UDPLISTENSERVERPORT);

    if (bind(tcpSock, (struct sockaddr*)&tcpAddr, sizeof(tcpAddr)) < 0 ||
        bind(udpSock, (struct sockaddr*)&udpAddr, sizeof(udpAddr)) < 0) {
        perror("[-] Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(tcpSock, MAX_HOSTS) < 0) {
        perror("[-] Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[SYSTEM] Ground Station Active. TCP: %d | UDP: %d\n", TCPLISTENSERVERPORT, UDPLISTENSERVERPORT);

    pthread_t udpThread;
    pthread_create(&udpThread, NULL, handleUdpTelemetry, &udpSock);
    pthread_detach(udpThread); // Gira in background per sempre

    while (1) {
        struct sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);
        int clientSock = accept(tcpSock, (struct sockaddr*)&clientAddr, &len);
        
        if (clientSock >= 0) {
            int* arg = malloc(sizeof(int));
            if (arg) {
                *arg = clientSock;
                pthread_t tcpThread;
                pthread_create(&tcpThread, NULL, handleTcpClient, arg);
                pthread_detach(tcpThread);
            } else {
                close(clientSock);
            }
        }
    }

    close(tcpSock);
    close(udpSock);
    pthread_mutex_destroy(&list_mutex);
    return 0;
}