/*********************************************************************************************
Name:			acceptor.c

    Required:	server.h
                done.h	

    Developer:  Shane Spoor

    Created On: 2017-02-17

    Description:
    The purpose of this file is to encapsulate the listening socket.

    Revisions:
    (none)

*********************************************************************************************/

#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "done.h"
#include "server.h"


/*********************************************************************************************
FUNCTION

    Name:		accept_client

    Prototype:	int accept_client(acceptor_t* acceptor, client_t* out)

    Developer:	Shane Spoor

    Created On: 2017-02-17

    Parameters:
    acceptor - Struct with socket info.
    out - Struct with client info.

    Return Values:
	
    Description:
    This accepts the client socket info.

    Revisions:
	(none)

*********************************************************************************************/
int accept_client(acceptor_t* acceptor, client_t* out)
{
    struct sockaddr_in peer;
    socklen_t accepted_len = sizeof(peer);
    int peer_sock = accept(acceptor->sock, (struct sockaddr*)&peer, &accepted_len);
    if (peer_sock < 0)
    {
        if (errno != EWOULDBLOCK && errno != EAGAIN)
        {
            atomic_store(&done, 1);

            if (errno != EINTR)
            {
                perror("accept");
            }
        }
        return -1;
    }

    out->peer = peer;
    out->sock = peer_sock;
    return 0;
}

/*********************************************************************************************
FUNCTION

    Name:		cleanup_acceptor

    Prototype:	void cleanup_acceptor(acceptor_t* acceptor)

    Developer:	Shane Spoor

    Created On: 2017-02-17

    Parameters:
    acceptor - Struct with socket info.

    Return Values:
	
    Description:
    This cleans up the acceptor.

    Revisions:
	(none)

*********************************************************************************************/
void cleanup_acceptor(acceptor_t* acceptor)
{
    shutdown(acceptor->sock, 0);
    close(acceptor->sock);
    freeaddrinfo(acceptor->info);
}
