//
// Created by shane on 2/13/17.
//

#ifndef COMP8005_ASSN2_SERVER_H
#define COMP8005_ASSN2_SERVER_H

#include <stdatomic.h>
#include <netinet/in.h>
#include "client.h"
#include "acceptor.h"

typedef struct server_t server_t;

extern server_t* thread_server;
extern server_t* select_server;
extern server_t* epoll_server;

struct server_t
{
    /**
     * Starts the server (pre-allocates threads, creates select/epoll fds, etc.). This may also
     * handle accepting connections, in which case it will not return until the server is finished
     * (the done variable will be set in this case).
     *
     * @param server   The server to initialise.
     * @param acceptor The acceptor, which should by this point have a bound socket with listen()
     *                 called on it.
     *
     * @return 0 on success, -1 on failure with errno set appropriately.
     */
    int (*start)(server_t *server, acceptor_t *acceptor, int *handles_accept);

    /**
     * Adds a client to the server. This function should return quickly so that the accept thread can resume accepting
     * connections as soon as possible.
     *
     * @param server The server to which the client will be added.
     * @param client The client to add.
     *
     * @return 0 on success, -1 on failure with errno set appropriately.
     */
    int (*add_client)(server_t* server, client_t client);

    /**
     * Cleans up the server's resources. This might not actually be called given that the point is to stress the server
     * to exhaustion. Could be used if the main thread receives a signal.
     *
     * @param server The server to clean up.
     */
    void (*cleanup)(server_t* server);

    // Summary stats
    size_t max_concurrent;
    size_t total_served;

    // Data private to the server implementation (reference to thread pool, queue for receiving new clients, etc.)
    void* private;
};

/**
 * Starts accepting connections and relaying them to the provided server.
 *
 * @param server The server used to handle each connection.
 * @param port   The port on which to listen for connections.
 * @return 0 on success, or -1 on failure with errno set appropriately.
 */
int serve(server_t *server, unsigned short port);

#endif //COMP8005_ASSN2_SERVER_H
