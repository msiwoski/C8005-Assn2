//
// Created by shane on 2/16/17.
//

#ifndef COMP8005_ASSN2_CLIENT_H
#define COMP8005_ASSN2_CLIENT_H

#include <netinet/in.h>

typedef struct
{
    struct sockaddr_in peer;
    int sock;
} client_t;

typedef struct
{
    struct sockaddr_in peer;
    size_t transferred;
    time_t transfer_time;
} client_stats_t;

#endif //COMP8005_ASSN2_CLIENT_H
