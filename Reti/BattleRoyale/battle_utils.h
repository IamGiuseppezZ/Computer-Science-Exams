#ifndef BATTLE_UTILS_H
#define BATTLE_UTILS_H

#include "battle_packet.h"

// Networking
struct sockaddr_in createAddress(const char* ipAddress, int port);
int createSocketTcp(void);
int createSocketUdp(void);

// Packets
GamePacket createPacket(int32_t type, int32_t player_id, int32_t pos_x, int32_t pos_y, int32_t health_left);
void printPacket(const GamePacket* packet);

// Endianness
void packetToNetworkByteOrder(GamePacket* p);
void packetToHostByteOrder(GamePacket* p);

#endif // BATTLE_UTILS_H