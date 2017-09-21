/*********************************************************************************************
Name:			thread_server.c

    Required:	vector.h
                ring_buffer.h
                done.h
                server.h
                protocol.h

    Developer:  Shane Spoor

    Created On: 2017-02-17

    Description:
    This is the threaded server application. This will create threads based off the number of 
    sockets it receives and receives the data from the client and then echos it back to the 
    client. This is a threaded echo server.

    Revisions:
    (none)

*********************************************************************************************/

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <client.h>
#include <sys/time.h>
#include <vector.h>
#include <arpa/inet.h>

#include "log.h"
#include "timing.h"
#include "vector.h"
#include "ring_buffer.h"
#include "done.h"
#include "server.h"
#include "protocol.h"

static const unsigned int WORKER_POOL_SIZE = 200;

#define CLIENT_BACKLOG_SIZE 100
static client_t client_backlog_buf[CLIENT_BACKLOG_SIZE];

typedef struct
{
    client_stats_t stats;
    uint32_t msg_size;
    char* msg;
} thread_server_request;

typedef struct
{
    atomic_int busy;
    client_t client;
} worker_params;

typedef struct
{
    vector_t worker_params_list;
    ring_buffer_t client_backlog;
    pthread_mutex_t stdout_guard;
} thread_server_private;

static int thread_server_start(server_t* server, acceptor_t* acceptor, int* handles_accept);
static int thread_server_add_client(server_t* server, client_t client);
static void thread_server_cleanup(server_t* server);

/*********************************************************************************************
FUNCTION

    Name:		worker_func

    Prototype:	static void* worker_func(void* void_params)

    Developer:	Shane Spoor/Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    void_params - Params from the threaded server

    Return Values:
	
    Description:
    Serves a single client request at a time, indicating to the main thread when it is no longer
    busy.

    Revisions:
	(none)

*********************************************************************************************/
static void* worker_func(void* void_params)
{
    worker_params* params = (worker_params*)void_params;

    // Busy wait for a socket or done ;)
    // If we cared about the threaded implementation, use of a conditional variable/eventfd would probably be a good
    // way to make it better
    while (1)
    {
        int done_local;
        while(!atomic_load(&params->busy) && !(done_local = atomic_load(&done)));
        if (done_local)
        {
            break;
        }

        // Handle the new client
        thread_server_request request;
        request.stats.transferred = 0;
        request.stats.transfer_time = 0;

        struct timeval start, end;
        gettimeofday(&start, NULL);

        ssize_t read_result = read_data(params->client.sock, &request.msg_size, sizeof(request.msg_size));
        if (read_result == -1)
        {
            atomic_store(&done, 1);
            break;
        }
        else if (read_result != 0) // We should actually always receive > 0 the first time, but who knows
        {
            request.msg = malloc(request.msg_size);
            if (request.msg == NULL)
            {
                atomic_store(&done, 1);
                close(params->client.sock);
                break;
            }
        }
        request.stats.transferred += sizeof(request.msg_size);

        // Continue reading from the client until we get size == 0
        while(1)
        {
            // Read all data, send it, then read the next message size
            read_data(params->client.sock, request.msg, request.msg_size);
            send_data(params->client.sock, request.msg, request.msg_size);
            read_data(params->client.sock, &request.msg_size, sizeof(request.msg_size));

            request.stats.transferred += sizeof(request.msg_size);
            request.stats.transferred += request.msg_size;

            if (request.msg_size == 0)
            {
                break;
            }
        }

        free(request.msg);
        close(params->client.sock);

        gettimeofday(&end, NULL);
        request.stats.transfer_time = TIME_DIFF(start, end);

        unsigned short src_port = ntohs(params->client.peer.sin_port);
        char addr_buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &params->client.peer.sin_addr, addr_buf, INET_ADDRSTRLEN);

        char csv[256];
        snprintf(csv, 256, "%ld,%ld,%s:%hu\n", request.stats.transfer_time, request.stats.transferred, addr_buf, src_port);
        log_msg(csv);

        char pretty[256];
        snprintf(pretty, 256, "Transfer time; %ldus; total bytes transferred: %ld; peer: %s:%hu\n",
                 request.stats.transfer_time, request.stats.transferred, addr_buf, src_port);

        pthread_mutex_t* stdout_guard = &((thread_server_private*)thread_server->private)->stdout_guard;
        pthread_mutex_lock(stdout_guard);
        printf("%s", pretty);
        pthread_mutex_unlock(stdout_guard);

        atomic_store(&params->busy, 0);
    }

    free(params);
    return NULL;
}

/*********************************************************************************************
FUNCTION

    Name:		worker_func

    Prototype:	static void* worker_func(void* void_params)

    Developer:	Shane Spoor/Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    void_params - Params from the threaded server

    Return Values:
	
    Description:
    Create the serrver sockets that the will connect to the client and send/receive data.

    Revisions:
	(none)

*********************************************************************************************/
static void accept_loop(server_t* server, acceptor_t* acceptor)
{
    thread_server_private* private = (thread_server_private*)server->private;

    // Get clients from the acceptor and send them to an available thread
    while (1)
    {
        client_t next_client;
        int result = accept_client(acceptor, &next_client);
        if (result == -1 || atomic_load(&done))
        {
            break;
        }

        if (server->add_client(server, next_client) == -1)
        {
            break;
        }

        ++server->total_served;
        if (private->worker_params_list.size > server->max_concurrent)
        {
            server->max_concurrent = private->worker_params_list.size;
        }
    }
}

/*********************************************************************************************
FUNCTION

    Name:		thread_server_start

    Prototype:	int thread_server_start(server_t *thread_server, acceptor_t *acceptor, int *handles_accept)

    Developer:	Shane Spoor/Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    thread_server - pointer to the struct with server read_data
    acceptor - Struct with acceptor information
    handles_accept - handles the accept

    Return Values:
	
    Description:
    Starts the threaded server. 

    Revisions:
	(none)

*********************************************************************************************/
int thread_server_start(server_t *thread_server, acceptor_t *acceptor, int *handles_accept)
{
    *handles_accept = 1;

    thread_server_private* priv = malloc(sizeof(thread_server_private));
    if (!priv)
    {
        perror("malloc");
        return -1;
    }

    if (pthread_mutex_init(&priv->stdout_guard, NULL) == -1)
    {
        perror("pthread_mutex_init");
        return -1;
    }

    //ring_buffer_init(&priv->client_backlog, &client_backlog_buf[0], CLIENT_BACKLOG_SIZE, sizeof(client_t));

    if (vector_init(&priv->worker_params_list, sizeof(worker_params*), WORKER_POOL_SIZE) == -1)
    {
        fprintf(stderr, "vector_init failed");
        return -1;
    }

    size_t i;
    for(i = 0; i < WORKER_POOL_SIZE; ++i)
    {
        worker_params* params = malloc(sizeof(worker_params));
        if (!params)
        {
            break;
        }
        params->busy = 0;
        vector_push_back(&priv->worker_params_list, &params);
    }
    if (i != WORKER_POOL_SIZE)
    {
        size_t const allocd = i;
        worker_params** worker_param_list = (worker_params**)priv->worker_params_list.items;
        perror("malloc");

        for (i = 0; i < allocd; ++i)
        {
            free(worker_param_list[i]);
        } 
        vector_free(&priv->worker_params_list);
        return -1;
    }

    for (i = 0; i < WORKER_POOL_SIZE; ++i)
    {
        worker_params** worker_param_list = (worker_params**)priv->worker_params_list.items;
        pthread_t thread;
        int result = pthread_create(&thread, NULL, worker_func, worker_param_list[i]);
        if (result == -1)
        {
            break;
        }
        pthread_detach(thread);
        
    }

    if (i != WORKER_POOL_SIZE)
    {
        perror("pthread_create");
        atomic_store(&done, 1);

        return -1;
    }

    thread_server->private = priv;
    accept_loop(thread_server, acceptor);
    return 0;
}

/*********************************************************************************************
FUNCTION

    Name:		thread_server_add_client

    Prototype:	static int thread_server_add_client(server_t* server, client_t client)

    Developer:	Shane Spoor/Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    server - struct with server information
    client - struct with client information

    Return Values:
	
    Description:
    Creates and adds the clients.

    Revisions:
	(none)

*********************************************************************************************/
static int thread_server_add_client(server_t* server, client_t client)
{
    thread_server_private* private = (thread_server_private*)server->private;
    worker_params** list = (worker_params**)private->worker_params_list.items;

    // Find the next non-busy thread, if any
    unsigned i;
    for(i = 0; i < private->worker_params_list.size; ++i)
    {
        worker_params* params = list[i];
        int busy = atomic_load(&params->busy);
        if (!busy)
        {
            list[i]->client = client;
            atomic_store(&params->busy, 1);
            break;
        }
    }

    if (i == private->worker_params_list.size)
    {
        // All threads are busy; add a new one to the list and give it the new connection
        worker_params* new_params = NULL;
        new_params = malloc(sizeof(worker_params));
        if (!new_params || (vector_push_back(&private->worker_params_list, &new_params) == -1))
        {
            atomic_store(&done, 1);
            free(new_params);
            return -1;
        }

        new_params->busy = 1;
        new_params->client = client;

        pthread_t new_thread;
        if(pthread_create(&new_thread, NULL, worker_func, new_params) == -1 ||
           pthread_detach(new_thread) == -1)
        {
            atomic_store(&done, 1);
            free(new_params);
            return -1;
        }
    }

    return 0;
}

/*********************************************************************************************
FUNCTION

    Name:		thread_server_cleanup

    Prototype:	static void thread_server_cleanup(server_t* thread_server)

    Developer:	Shane Spoor/Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    thread_server - stack with server data

    Return Values:
	
    Description:
    Cleans up and free any sockets/file descriptors the server created.

    Revisions:
	(none)

*********************************************************************************************/
static void thread_server_cleanup(server_t* thread_server)
{
    thread_server_private* private = (thread_server_private*)thread_server->private;
    pthread_mutex_destroy(&private->stdout_guard);
    vector_free(&private->worker_params_list); // The threads will free the individual elements... I hope
    atomic_store(&done, 1);
    free(private);
}

static server_t thread_server_impl =
{
    thread_server_start,
    thread_server_add_client,
    thread_server_cleanup,
    0,
    0,
    NULL
};

server_t* thread_server = &thread_server_impl;
