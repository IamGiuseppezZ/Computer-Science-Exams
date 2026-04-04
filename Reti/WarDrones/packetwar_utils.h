#ifndef PACKETWAR_UTILS_H
#define PACKETWAR_UTILS_H

#include "packetwar.h"

// Funzioni di rete
int createSocketTcp(void);
int createSocketUdp(void);
struct sockaddr_in createAddress(const char* ip, int port);

// Funzioni di utilità pacchetti
Packet createPacket(int32_t type, int32_t drone_id, int32_t pos_x, int32_t pos_y, int32_t battery);
void printPacket(const Packet* p);

// Conversione Endianness (Host <-> Network)
void packetToNetworkByteOrder(Packet* p);
void packetToHostByteOrder(Packet* p);

#endif // PACKETWAR_UTILS_H