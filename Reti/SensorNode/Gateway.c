#include "Packet.h"
#include "functionSensor.h"

void printPacket(Packet p){
    printf("------------ RECEIVED UPDATE ON GATEWAY ------------\n");
    printf("------------ From: [%d] ------------\n", p.source_id);
    printf("------------ To: [%d] ------------\n", p.destination_id);
    printf("------------ Data value: [%d] ------------\n", p.dataValue);
    puts("\n");
}

int main(){
    int myId = 0;
    struct sockaddr_in gatewayAddress = createAddress(LOCALHOST, GATEWAY);
    struct sockaddr_in hostAddress = createAddress(LOCALHOST, HOST1);
    struct sockaddr_in senderAddr;
    socklen_t len = sizeof(gatewayAddress);

    int nodi_attivi = 3;
    int sensoriVivi[3] = {1,1,1};
    int PORT = 6001;
    int nextHost = 1;

    int sockfd = createSocket();

    bind(sockfd, (struct sockaddr*)& gatewayAddress, sizeof(gatewayAddress));


    while(nodi_attivi != 0){
        if (sensoriVivi[nextHost - 1] == 1){
            Packet p = createPacket(myId, nextHost, 1, 0);

            //Mando pacchetto di tipo 1 (request)
            sendto(sockfd, &p, sizeof(p), 0, (struct sockaddr*)&hostAddress, sizeof(hostAddress));

            //Ricevo il pacchetto dall'host con pacchetto tipo 2
            Packet receivedPacket;
            recvfrom(sockfd, &receivedPacket, sizeof(receivedPacket), 0, (struct sockaddr*)& senderAddr, &len);
            printPacket(receivedPacket);
        
            //Se il valore del sensore è >= 45, termino forzatamente il nodo
            if (receivedPacket.dataValue >= 45){
                Packet pShutDown = createPacket(myId, nextHost, 3, 0);
                sendto(sockfd, &pShutDown, sizeof(pShutDown), 0, (struct sockaddr*)& hostAddress, sizeof(hostAddress));
                usleep(100000);
                nodi_attivi--;
                sensoriVivi[nextHost - 1] = 0;
            }
        }
        PORT = 6001 + ((PORT - 6001 + 1) % 3);
        hostAddress.sin_port = htons(PORT);
        nextHost = (nextHost % 3) + 1;
    }

    close(sockfd);
    
}