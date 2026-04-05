#ifndef SENSOR_UTILS_H
#define SENSOR_UTILS_H

#include "sensor_packet.h"

// Networking Utilities
struct sockaddr_in createAddress(const char* ipAddress, int port);
int createSocketTcp(void);
int createSocketUdp(void);

// Endianness Converters (Handles Floats implicitly)
void packetToNetworkByteOrder(Packet* p);
void packetToHostByteOrder(Packet* p);

#endif // SENSOR_UTILS_H