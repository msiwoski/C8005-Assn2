/*********************************************************************************************
Name:			main.c

    Required:	main.h	

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Description:
    This is the client application. This will create the clients that will connect to the
	servers and will act as a basic echo server. The client will send and receive data and
	will test the time between to record stats.	

    Revisions:
    (none)

*********************************************************************************************/

#include <arpa/inet.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <aio.h>
#include <errno.h>

#include "client.h"
#include "protocol.h"

#define DEFAULT_PORT "8005"
#define DEFAULT_IP "192.168.0.12"
#define DEFAULT_NUMBER_CLIENTS 5000
#define DEFAULT_MAXIMUM_REQUESTS 1
#define DEFAULT_MSG_SIZE 1024
#define NETWORK_BUFFER_SIZE 1024
#define STACK_SIZE 65536

static atomic_int thread_count = 0;


/*********************************************************************************************
FUNCTION

    Name:		print_usage

    Prototype:	void print_usage(char const* name) 

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    name - name

    Return Values:
	
    Description:
    Prints usage help when running the application.

    Revisions:
	(none)

*********************************************************************************************/
void print_usage(char const* name)
{
    printf("usage: %s [-h] [-i ip] [-p port] [-m max] [-t threads]\n", name);
    printf("\t-h, --help:               print this help message and exit.\n");
    printf("\t-i, --ip [ip]             the ip on which the server is on.\n");
    printf("\t-p, --port [port]:        the port on which to listen for connections;\n");
    printf("\t-m, --max [max]           the max numbers of requests.\n");
    printf("\t-n, --threads [threads]   the number of threads to create (each thread continuously creates clients).\n");
    printf("\t-s, --msg-size [size]     the size of the message that will be sent each request.\n");
    printf("\t                          default port is %s.\n", DEFAULT_PORT);
    printf("\t                          default IP is %s.\n", DEFAULT_IP);
    printf("\t                          default number of threads is %d.\n", DEFAULT_NUMBER_CLIENTS);
    printf("\t                          default number of max requests is %d.\n", DEFAULT_MAXIMUM_REQUESTS);
    printf("\t                          default message size is %d.\n", DEFAULT_MSG_SIZE);
}

/*********************************************************************************************
FUNCTION

    Name:	main

    Prototype:	int main(int argc, char** argv) 

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    argc - Number of arguments
	argv - Arguments

    Return Values:
	
    Description:
    Runs/creates the clients that will connect to the server. This function also accepts the
	user's inputs to customize the ip/port/max number of requests and the number of clients 
	to create.

    Revisions:
	(none)

*********************************************************************************************/
int main(int argc, char** argv)
{

    //system("ulimit -n 500000");

    client_info client_datas;
    char const* short_opts = "i:p:m:n:t:h";
    int file_descriptors[2];
    struct option long_opts[] =
    {
        {"ip",       1, NULL, 'i'},
        {"port",     1, NULL, 'p'},
        {"max",      1, NULL, 'm'},
        {"clients",  1, NULL, 'n'},
        {"msg-size", 1, NULL, 's'},
        {"help",     0, NULL, 'h'},
        {0, 0, 0, 0},
    };

    client_datas.port = DEFAULT_PORT;
    client_datas.ip = DEFAULT_IP;
    client_datas.max_requests = DEFAULT_MAXIMUM_REQUESTS;
    client_datas.num_of_clients = DEFAULT_NUMBER_CLIENTS;
    client_datas.msg_size = DEFAULT_MSG_SIZE;

    if (argc > 1)
    {
        int c;
        while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1)
        {
            switch(c)
            {
                case 'p':
                {
                    unsigned int port_int;
                    int num_read = sscanf(optarg, "%u", &port_int);
                    if (num_read != 1 || port_int > UINT16_MAX)
                    {
                        fprintf(stderr, "Invalid port number %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        size_t port_str_size = strlen(optarg);
                        char* port_copy = malloc(port_str_size + 1);
                        memcpy(port_copy, optarg, port_str_size);
                        port_copy[port_str_size] = 0;

                        client_datas.port = port_copy;
                    }
                }
                break;
                case 'i':
                {
                    size_t ip_str_size = strlen(optarg);
                    char* ip_copy = malloc(ip_str_size + 1);
                    memcpy(ip_copy, optarg, ip_str_size);
                    ip_copy[ip_str_size] = 0;

                    client_datas.ip = ip_copy;
                }
                break;
                case 'm':
                {
                    unsigned int max_requests_int;
                    int max_requests_read = sscanf(optarg, "%d", &max_requests_int);
                    if( max_requests_read != 1)
                    {
                        fprintf(stderr, "Invalid number of Max Requests %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        client_datas.max_requests = (unsigned int)max_requests_int;
                    }
                }                    
                break;
                case 'n':
                {
                    unsigned int num_of_clients_int;
                    int num_of_clients_read = sscanf(optarg, "%d", &num_of_clients_int);
                    if(num_of_clients_read != 1)
                    {
                        fprintf(stderr, "Invalid number of clients %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        client_datas.num_of_clients = (unsigned int)num_of_clients_int;
                    }
                }
                break;
		case 's':
                {
                    unsigned int msg_size;
                    int converted = sscanf(optarg, "%u", &msg_size);
                    if(converted != 1)
                    {
                        fprintf(stderr, "Invalid message size %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                    else if (msg_size == 0)
                    {
                        fprintf(stderr, "Message size cannot be 0.\n");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        client_datas.msg_size = msg_size;
                    }
                }
                break;
                case 'h':
                    print_usage(argv[0]);
                    exit(EXIT_SUCCESS);
                break;
                case '?':
                {
                    fprintf (stderr, "Incorrect argument or unknown option. See %s -h for help.\n", argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;
                default:
                    fprintf(stderr, "You broke getopt_long! Congrats.\n");
                    exit(EXIT_FAILURE);
                break;
            }
        }
    }
	
    if((client_datas.file_descriptor = open("result.txt", O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777)) == -1)
    {
        perror("open");
        return -1;
    }

    if (start_client(client_datas) == -1)
    {
        exit(EXIT_FAILURE);
    }

    if (client_datas.port != DEFAULT_PORT)
    {
        free(client_datas.port);
    }
    if (client_datas.ip != DEFAULT_IP)
    {
        free(client_datas.ip);
    }
}

/*********************************************************************************************
FUNCTION

    Name:		start_client

    Prototype:	int start_client(client_info client_datas)

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    client_datas - Server Data to connect to.

    Return Values:
	
    Description:
    Start the clients and connect to the server. Create the applicable number of threads dependent
	on the number of clients that the user wants to connect.

    Revisions:
	(none)

*********************************************************************************************/
int start_client(client_info client_datas)
{
    int thread_count = client_datas.num_of_clients;
    int count = 0;
    client_info* data = malloc(sizeof(client_info) * thread_count);
    pthread_t* threads = malloc(sizeof(pthread_t) * thread_count);
    pthread_attr_t attribute;

    atomic_store_explicit(&thread_count, client_datas.num_of_clients, memory_order_relaxed);

    if (data == NULL)
    {
        perror("malloc");
        return -1;
    }
        
    for (count = 0; count < thread_count; count++)
    {
        memcpy(&data[count], &client_datas, sizeof(client_datas));
    }
    
    pthread_attr_init(&attribute);
    pthread_attr_setstacksize(&attribute, STACK_SIZE);

    for (count = 0; count < thread_count; count++)
    {
        if (pthread_create(threads + count, &attribute, clients, &data[count]) != 0)
        {
            perror("pthread_create");
            return -1;
        }
    }

    for (count = 0; count < thread_count; count++)
    {
	    void* val;
        pthread_join(threads[count], &val);
    }
    return 1;
}


/*********************************************************************************************
FUNCTION

    Name:		connect_to_server

    Prototype:	void* clients(void* infos)

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    infos - Server Data to connect to.

    Return Values:
	
    Description:
    Create the clients that the will connect to the server and send/receive data.

    Revisions:
	(none)

*********************************************************************************************/
void* clients(void* infos)
{
    while (1)
    {
        int sock = 0;
        char* buffer = 0;
        int data_received = 0;
        int request_time = 0;
        client_info *data = (client_info *)infos;
        char* msg_send = make_random_string(data->msg_size);
        char* msg_recv = malloc(data->msg_size);
        char result_info[512];
        int client_count = 0;

        if (msg_recv == NULL)
        {
            perror("malloc");
            return NULL;
        }

        struct timeval start_time;
        struct timeval end_time;

        sock = connect_to_server(data->port, data->ip);
        if (sock == -1)
        {
            perror("connect");
            return NULL;
        }

        for (int i = 0; i < data->max_requests; i++)
        {
            gettimeofday(&start_time, NULL);

            uint32_t msg_send_size = (uint32_t)strlen(msg_send);
            if (send_data(sock, (char const*)&msg_send_size, sizeof(uint32_t)) == -1 ||
                send_data(sock, msg_send, strlen(msg_send)) == -1)
            {
                perror("send_data");
                break;
            }

            ssize_t bytes_read = read_data(sock, msg_recv, strlen(msg_send));
            if (bytes_read < 0)
            {
                struct sockaddr_in in;
                size_t len;
                getsockname(sock, (struct sockaddr*)&in, (socklen_t*)&len);

                char buf[128];
                snprintf(buf, 128, "read_data (port %hu)", ntohs(in.sin_port));
                perror(buf);
                break;
            }

            gettimeofday(&end_time, NULL);

            data_received += bytes_read;
            request_time += (end_time.tv_sec * 1000000 + end_time.tv_usec) -
                            (start_time.tv_sec * 1000000 + start_time.tv_usec);
            client_count++;
            usleep(250000);
        }
        
        uint32_t send_final_size = 0;
        ssize_t read_final_size = 0;

        if (errno == 0)
        {
            if (send_data(sock, (char const*)&send_final_size, sizeof(uint32_t)) == -1)
            {
                perror("send final size");
                return 0;
            }

            //Client Count, Request Time and Data Received
            sprintf(result_info, "%d, %d, %d\n", client_count, request_time,  data_received);
            size_t result_len = strlen(result_info);
            struct aiocb cb;
            memset(&cb, 0, sizeof(cb));

            cb.aio_buf = &result_info[0];
            cb.aio_nbytes = result_len;
            cb.aio_fildes = data->file_descriptor;
            if (aio_write(&cb) == -1)
            {
                perror("aio_write");
                close(data->file_descriptor);
            }

            while(aio_error(&cb) == EINPROGRESS);
        }
        
        close_socket(&sock);
        free(msg_recv);
        free(msg_send);
    }

    pthread_exit(NULL);
}

/*********************************************************************************************
FUNCTION

    Name:		connect_to_server

    Prototype:	int connect_to_server(const char *port, const char *ip)

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    port - Port to connect to server
	ip - IP to connect to server

    Return Values:
	
    Description:
    Connect to the server 

    Revisions:
	(none)

*********************************************************************************************/
int connect_to_server(const char *port, const char *ip)
{
    int done = 0;
    int sock = -1;
    do
    {
        struct addrinfo hints;
        struct addrinfo *result;
        struct addrinfo *rp = NULL;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(ip, port, &hints, &result) != 0)
        {
            perror("getaddrinfo");
            return -1;
        }
            
        for (rp = result; rp != NULL; rp = rp->ai_next)
        {
            sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            
            if (sock == -1)
            {
                continue;
            }
            else if (set_reuse(&sock) == -1)
            {
                close_socket(&sock);
                perror("set_reuse");
                continue;
            }

            if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1)
            {
                done = 1;
                break;
            }

            close(sock);
        }
        
        freeaddrinfo(result);
        if (rp == NULL && errno != EADDRNOTAVAIL)
        {
            perror("connect");
            done = 1;
        }
    } while (!done);

    return sock;
}

/*********************************************************************************************
FUNCTION

    Name:		set_reuse

    Prototype:	int set_reuse(int* socket)

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    socket - Socket

    Return Values:
	
    Description:
    Sets the socket for reuse.

    Revisions:
	(none)

*********************************************************************************************/
int set_reuse(int* socket)
{
    socklen_t optlen = 1;
    return setsockopt(*socket, SOL_SOCKET, SO_REUSEADDR, &optlen, sizeof(optlen));
}

/*********************************************************************************************
FUNCTION

    Name:		close_socket

    Prototype:	int close_socket(int* socket)

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    socket - Socket

    Return Values:
	
    Description:
    Closes the socket.

    Revisions:
	(none)

*********************************************************************************************/
int close_socket(int* socket)
{
    return close(*socket);
}


/*********************************************************************************************
FUNCTION

    Name:		make_random_string

    Prototype:	char* make_random_string(size_t length)

    Developer:	Mat Siwoski

    Created On: 2017-02-17

    Parameters:
    length - Length of the random string to generate

    Return Values:
	
    Description:
    Creates a random string of set length.

    Revisions:
	(none)

*********************************************************************************************/
char* make_random_string(size_t length) 
{
    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; 
    char *random_string;

    if (length) 
    {
        random_string = malloc(length +1); 

        if (random_string) 
        {
            int l = (int) (sizeof(charset)-1); 
            int key; 
            for (int n = 0;n < length;n++) 
            {        
                key = rand() % l;  
                random_string[n] = charset[key];
            }

            random_string[length] = '\0';
        }
    }

    return random_string;
}
