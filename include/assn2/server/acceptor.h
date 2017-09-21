//
// Created by shane on 2/13/17.
//

#ifndef COMP8005_ASSN2_ACCEPTOR_H
#define COMP8005_ASSN2_ACCEPTOR_H

#include "client.h"

typedef struct
{
    struct addrinfo* info;
    unsigned short port;
    int sock;
} acceptor_t;

/**
 * Attempts to accept a client using the given acceptor.
 *
 * @param acceptor The acceptor containing a socket on which to accept a client.
 * @param out      A client structure that will hold the new client's information on success.
 * @return 0 on success, -1 on failure (an error message will have been printed already).
 */
int accept_client(acceptor_t* acceptor, client_t* out);

/**
 * Cleans up the acceptor's addrinfo and socket.
 *
 * @param acceptor The acceptor to clean up.
 */
void cleanup_acceptor(acceptor_t* acceptor);

#endif //COMP8005_ASSN2_ACCEPT_H
