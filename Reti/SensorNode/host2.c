#include "Packet.h"
#include "functionSensor.h"

void printPacket(Packet p){
    printf("------------ RECEIVED PACKET ON HOST ------------\n");
    printf("------------ From: [%d] ------------\n", p.source_id);
    printf("------------ To: [%d] ------------\n", p.destination_id);
    puts("\n");
}

int main(){
    srand(time(NULL));
    //Modifico qui
    struct sockaddr_in myAddr = createAddress(LOCALHOST, HOST2);
    struct sockaddr_in gatewayAddr = createAddress(LOCALHOST, GATEWAY);
    struct sockaddr_in receiverAddr;
    socklen_t len = sizeof(myAddr);

    //Dati Host
    int myId = 2;

    int sockfd = createSocket();

    bind(sockfd, (struct sockaddr*)& myAddr, len);

    while(1){
        int value = rand() % 31 + 20;
        Packet p;

        recvfrom(sockfd, &p, sizeof(p), 0, (struct sockaddr*)&receiverAddr, &len);
        if (p.type == 3) break;

        printPacket(p);

        Packet pCreated = createPacket(myId, 0, 2, value);
        sendto(sockfd, &pCreated, sizeof(pCreated), 0, (struct sockaddr*)& gatewayAddr, len);
    }
    printf("[Host %d] sto uscendo.\n", myId);
    close(sockfd);
}