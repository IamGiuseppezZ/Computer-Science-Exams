#ifndef TERAPY_PACKET_H
#define TERAPY_PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>

#define LOCALHOST "127.0.0.1"
#define PORT_TCP_HOSPITAL 9000
#define PORT_UDP_HOSPITAL 9001

#define TYPE_PATIENT_ADMIT 1
#define TYPE_ADMIT_ACK 2
#define TYPE_VITALS_UPDATE 3
#define TYPE_CODE_RED 4

// Allinea a 1 byte per spedire i pacchetti in rete senza buchi
#pragma pack(push, 1)

typedef struct {
    int32_t type;           
    int32_t patient_id;     
    int32_t bpm;            
    int32_t oxygen;         
} MedPacket;

#pragma pack(pop)

#endif // TERAPY_PACKET_H