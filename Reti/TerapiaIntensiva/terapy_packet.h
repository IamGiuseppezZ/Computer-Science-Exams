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

// Allineamento della memoria a 1 byte per la trasmissione di rete
#pragma pack(push, 1)

typedef struct {
    int32_t type;           // Tipo di pacchetto
    int32_t patient_id;     // ID del paziente/letto
    int32_t bpm;            // Battiti per minuto
    int32_t oxygen;         // Saturazione ossigeno %
} MedPacket;

#pragma pack(pop)

#endif // TERAPY_PACKET_H