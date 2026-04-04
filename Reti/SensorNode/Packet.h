#ifndef PACKET_H

#define PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define GATEWAY 6000
#define HOST1 6001
#define HOST2 6002
#define HOST3 6003
#define LOCALHOST "127.0.0.1"

typedef struct Packet {
    int source_id;
    int destination_id;
    int type;
    int dataValue;
}
Packet;

#endif