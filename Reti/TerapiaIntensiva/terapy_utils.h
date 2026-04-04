#ifndef TERAPY_UTILS_H
#define TERAPY_UTILS_H

#include "terapy_packet.h"

// Networking
struct sockaddr_in createAddress(const char* ipAddress, int port);
int createSocketTcp(void);
int createSocketUdp(void);

// Packet management
MedPacket createPacket(int32_t type, int32_t patientId, int32_t bpm, int32_t oxygen);
void printPacket(const MedPacket* packet);

// Endianness conversions
void packetToNetworkByteOrder(MedPacket* p);
void packetToHostByteOrder(MedPacket* p);

#endif // TERAPY_UTILS_H