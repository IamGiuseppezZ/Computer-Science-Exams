#ifndef CHAT_UTILS_H
#define CHAT_UTILS_H

#include "chat_packet.h"

struct sockaddr_in createAddress(const char* ipAddress, int port);
int createSocketIpv4(void);

Packet createPacket(int32_t type, const char* senderNick, const char* targetNick, int32_t peer_port, const char* payload);
void printPacket(const Packet* p);

void packetToNetworkByteOrder(Packet* p);
void packetToHostByteOrder(Packet* p);

#endif // CHAT_UTILS_H