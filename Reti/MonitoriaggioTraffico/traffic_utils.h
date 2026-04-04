#ifndef TRAFFIC_UTILS_H
#define TRAFFIC_UTILS_H

#include "traffic_packet.h"

// Networking utilities
struct sockaddr_in createAddress(const char* ipAddress, int port);
int createSocketTcp(void);
int createSocketUdp(void);

// Packet processing and formatting
void printPacket(Packet p);
void convertPacket(Packet* p);
void printAlertPacket(AlertPacket* p);
void printPolicePacket(PolicePacket* p);

#endif // TRAFFIC_UTILS_H