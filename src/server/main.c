/*********************************************************************************************
Name:		main.c

Required:	main.h	

Developer:	Shane Spoor

Created On: 2017-02-17

Description:
	
Revisions:
	(none)

*********************************************************************************************/

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "log.h"
#include "server.h"

#define DEFAULT_PORT 8005

/*********************************************************************************************
FUNCTION

Name:		print_usage

Prototype:	void print_usage(char const* name) 

Developer:	Shane Spoor

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
    printf("usage: %s [-h] [-p port]\n", name);
    printf("\t-h, --help:          print this help message and exit.\n");
    printf("\t-p, --port [port]:   the port on which to listen for connections;\n");
    printf("\t                     default is %u.\n", DEFAULT_PORT);
    printf("\t-s, --server [name]: the server used to handle connections.\n");
    printf("\t                     Valid values are thread, select, or epoll.");
    printf("\t                     Default is epoll.");
}

/*********************************************************************************************
FUNCTION

Name:		main

Prototype:	int main(int argc, char** argv) 

Developer:	Shane Spoor

Created On: 2017-02-17

Parameters:
    argc - Number of arguments
	argv - Arguments

Return Values:
	
Description:
    Runs the different servers based off the users choice.

Revisions:
	(none)

*********************************************************************************************/
int main(int argc, char** argv)
{
    unsigned short port = DEFAULT_PORT;
    server_t* server = epoll_server;

    char const* short_opts = "p:s:h";
    struct option long_opts[] =
    {
        {"port",   1, NULL, 'p'},
        {"server", 1, NULL, 's'},
        {"help",   0, NULL, 'h'},
        {0, 0, 0, 0},
    };

    struct rlimit open_file_limit;
    open_file_limit.rlim_cur = 131072;
    open_file_limit.rlim_max = 131072;

    if (setrlimit(RLIMIT_NOFILE, &open_file_limit) == -1)
    {
        perror("setrlimit");
        exit(EXIT_FAILURE);
    }

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
                        port = (unsigned short)port_int;
                    }
                }
                break;
                case 's':
                {
                    if (strcmp(optarg, "epoll") == 0)
                    {
                        server = epoll_server;
                        printf("epoll");
                    }
                    else if (strcmp(optarg, "select") == 0)
                    {
                        server = select_server;
                        printf("select");
                    }
                    else if (strcmp(optarg, "thread") == 0)
                    {
                        server = thread_server;
                    }
                    else
                    {
                        fprintf(stderr, "Invalid server %s.\n", optarg);
                        print_usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }
                }
                break;
                case 'h':
                    print_usage(argv[0]);
                    exit(EXIT_SUCCESS);
                break;
                case '?':
                {
                    fprintf (stderr, "Incorrect argument or unknown option. See %s -h for help.", argv[0]);
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

    if (log_open("transfers.txt") == -1)
    {
        exit(EXIT_FAILURE);
    }

    int ret = EXIT_SUCCESS;
    if (serve(server, port) == -1)
    {
        ret = EXIT_FAILURE;
    }

    int result = log_close();
    if (result < 0)
    {
        perror("close");
    }
    fprintf(stderr, "Total served: %lu; Max concurrent connections: %lu\n", server->total_served, server->max_concurrent);
    fflush(stderr);

    return ret;
}
