#ifndef TRAFFIC_PACKET_H
#define TRAFFIC_PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>

#define LOCALHOST "127.0.0.1"
#define PORT_SERVER_UDP 7000
#define PORT_SERVER_TCP 7001

#define MAX_HOSTS 10

#define TYPE_ALARM_CAR 1
#define TYPE_ALARM_VELOCITY 2

#define POLICE_TYPE_REQUEST_CONNECT 1

#pragma pack(push, 1)

// Sensor data packet
typedef struct {
    int32_t idSensor;
    int32_t portSensor;
    int32_t numeroVeicoli;
    float velocitaMedia;
} Packet;

// Alert packet to be broadcasted to police clients
typedef struct {
    int32_t typeAllarm;
    int32_t idSensorProblem;
    int32_t numCars;
    float velocity;
    char description[65];
} AlertPacket;

// Client authentication/request packet
typedef struct {
    int32_t typeRequest;
    int32_t idPolice;
} PolicePacket;

#pragma pack(pop)

#endif // TRAFFIC_PACKET_H